#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

/**
 * Studio-Grade Real-Time Pitch-Marked TD-PSOLA for VST3
 * - YIN CMNDF pitch tracking with parabolic refinement & continuity lock
 * - 600 Hz LPF pitch-mark peak alignment
 * - 4-point Catmull-Rom cubic interpolation (zero aliasing grit)
 * - Safe overlap-add normalization + 40 Hz DC blocker + RMS envelope match
 * - Zero heap allocations in processBlock()
 */
class PitchFormantShifter
{
public:
    static constexpr int kRingSize = 16384;
    static constexpr int kRingMask = kRingSize - 1;
    static constexpr int kLatencySamples = 1024;

    void prepare(double sampleRate)
    {
        fs = sampleRate > 8000.0 ? sampleRate : 44100.0;
        inRing.assign(kRingSize, 0.0f);
        smoothRing.assign(kRingSize, 0.0f);
        wetRing.assign(kRingSize, 0.0f);
        normRing.assign(kRingSize, 0.0f);
        cmndf.assign(1024, 1.0);

        writePos = 0;
        readPos = (kRingSize - kLatencySamples) & kRingMask;
        nextSynthPos = 0.0;
        lastPeriod = 256.0;
        lastVoiced = false;
        lpState = 0.0f;
        dcPrevX = 0.0f;
        dcPrevY = 0.0f;
        inSq = 0.0f;
        wetSq = 0.0f;
        smoothGain = 1.0f;
    }

    void process(float* buffer, int numSamples, float pitchSemitones, float formantSemitones, float dryWet)
    {
        if (dryWet <= 0.001f || (std::abs(pitchSemitones) < 0.01f && std::abs(formantSemitones) < 0.01f))
            return;

        const double pitchRatio = std::pow(2.0, static_cast<double>(pitchSemitones) / 12.0);
        const double formantRatio = std::pow(2.0, static_cast<double>(formantSemitones) / 12.0);
        const float mix = std::clamp(dryWet, 0.0f, 1.0f);

        const float lpAlpha = std::min(0.95f, static_cast<float>((2.0 * 3.14159265 * 600.0) / (fs + 2.0 * 3.14159265 * 600.0)));
        const float dcAlpha = static_cast<float>(1.0 - (2.0 * 3.14159265 * 40.0) / fs);
        const float envAlpha = static_cast<float>(1.0 - std::exp(-1.0 / (fs * 0.025)));

        for (int i = 0; i < numSamples; ++i)
        {
            const float x = buffer[i];
            inRing[writePos] = x;

            lpState += lpAlpha * (x - lpState);
            smoothRing[writePos] = lpState;

            // Trigger synthesis grain when writePos has enough lookahead past nextSynthPos
            while (ringDiff(writePos, static_cast<int>(std::floor(nextSynthPos)) & kRingMask) >= kLatencySamples)
            {
                const int center = static_cast<int>(std::floor(nextSynthPos)) & kRingMask;
                const auto est = estimatePitchYIN(center);
                const double T0 = est.voiced ? est.period : 256.0;
                if (est.voiced) lastPeriod = T0;
                lastVoiced = est.voiced;

                int inMark = center;
                if (est.voiced)
                {
                    const int rad = std::min(static_cast<int>(T0 * 0.35), 140);
                    float maxVal = -1e9f;
                    for (int d = -rad; d <= rad; ++d)
                    {
                        const int idx = (center + d) & kRingMask;
                        if (smoothRing[idx] > maxVal)
                        {
                            maxVal = smoothRing[idx];
                            inMark = idx;
                        }
                    }
                }

                const double hopOut = est.voiced ? std::clamp(T0 / pitchRatio, 48.0, 900.0) : 128.0;
                const int halfWinOut = est.voiced
                    ? std::max(32, static_cast<int>(std::llround(T0 / formantRatio)))
                    : std::max(64, static_cast<int>(std::llround(256.0 / formantRatio)));

                const int outCenter = (static_cast<int>(std::llround(nextSynthPos)) + kLatencySamples / 2) & kRingMask;
                const int cappedHalfWin = std::min(halfWinOut, kLatencySamples / 2 - 4);

                for (int d = -cappedHalfWin; d <= cappedHalfWin; ++d)
                {
                    const int outIdx = (outCenter + d) & kRingMask;
                    const float w = 0.5f * (1.0f + std::cos(3.14159265f * static_cast<float>(d) / static_cast<float>(cappedHalfWin)));
                    const double inPos = static_cast<double>(inMark) + static_cast<double>(d) * formantRatio;
                    const float sample = readSampleCubicRing(inPos);

                    wetRing[outIdx] += sample * w;
                    normRing[outIdx] += w;
                }

                nextSynthPos = std::fmod(nextSynthPos + hopOut, static_cast<double>(kRingSize));
                if (nextSynthPos < 0.0) nextSynthPos += kRingSize;
            }

            // Read synthesized output & clear ring slot
            const float rawWet = normRing[readPos] > 1e-4f
                ? wetRing[readPos] / std::max(0.75f, normRing[readPos])
                : 0.0f;
            wetRing[readPos] = 0.0f;
            normRing[readPos] = 0.0f;

            // 40 Hz DC blocker
            const float dcOut = dcAlpha * (dcPrevY + rawWet - dcPrevX);
            dcPrevX = rawWet;
            dcPrevY = dcOut;

            // Delayed dry sample for phase-aligned Wet/Dry mix & RMS match
            const int dryIdx = (writePos - (kLatencySamples / 2)) & kRingMask;
            const float dryDelayed = inRing[dryIdx];

            inSq += envAlpha * (dryDelayed * dryDelayed - inSq);
            wetSq += envAlpha * (dcOut * dcOut - wetSq);
            const float targetGain = (wetSq > 1e-7f && inSq > 1e-7f)
                ? std::clamp(std::sqrt(inSq / wetSq), 0.7f, 1.45f)
                : 1.0f;
            smoothGain += 0.005f * (targetGain - smoothGain);

            const float finalWet = std::clamp(dcOut * smoothGain, -0.98f, 0.98f);
            buffer[i] = dryDelayed * (1.0f - mix) + finalWet * mix;

            writePos = (writePos + 1) & kRingMask;
            readPos = (readPos + 1) & kRingMask;
        }
    }

private:
    struct PitchEstimate { double period; bool voiced; };

    int ringDiff(int ahead, int behind) const
    {
        return (ahead - behind) & kRingMask;
    }

    PitchEstimate estimatePitchYIN(int center)
    {
        const int minLag = std::max(16, static_cast<int>(fs / 480.0));
        const int maxLag = std::min(800, static_cast<int>(fs / 65.0));
        const int winLen = 512;
        const int start = (center - winLen / 2) & kRingMask;

        double energy = 0.0;
        for (int i = 0; i < winLen; ++i)
        {
            const float v = inRing[(start + i) & kRingMask];
            energy += static_cast<double>(v) * v;
        }
        if (energy / winLen < 1e-6)
            return { 256.0, false };

        cmndf[0] = 1.0;
        double runningSum = 0.0;
        for (int tau = 1; tau <= maxLag + 1; ++tau)
        {
            double sum = 0.0;
            for (int i = 0; i < winLen; i += 2)
            {
                const double d = inRing[(start + i) & kRingMask] - inRing[(start + i + tau) & kRingMask];
                sum += d * d;
            }
            runningSum += sum;
            cmndf[tau] = runningSum > 1e-12 ? (sum * tau) / runningSum : 1.0;
        }

        int bestTau = -1;
        if (lastVoiced && lastPeriod >= minLag && lastPeriod <= maxLag)
        {
            const int cMin = std::max(minLag, static_cast<int>(std::floor(lastPeriod * 0.75)));
            const int cMax = std::min(maxLag, static_cast<int>(std::ceil(lastPeriod * 1.25)));
            int localBest = cMin;
            for (int tau = cMin + 1; tau <= cMax; ++tau)
                if (cmndf[tau] < cmndf[localBest]) localBest = tau;
            if (cmndf[localBest] < 0.25) bestTau = localBest;
        }

        if (bestTau == -1)
        {
            for (int tau = minLag; tau <= maxLag; ++tau)
            {
                if (cmndf[tau] < 0.16)
                {
                    while (tau + 1 <= maxLag && cmndf[tau + 1] < cmndf[tau]) ++tau;
                    bestTau = tau;
                    break;
                }
            }
        }

        if (bestTau == -1)
        {
            int globalMin = minLag;
            for (int tau = minLag + 1; tau <= maxLag; ++tau)
                if (cmndf[tau] < cmndf[globalMin]) globalMin = tau;
            if (cmndf[globalMin] < 0.35) bestTau = globalMin;
            else return { 256.0, false };
        }

        if (bestTau > minLag && bestTau < maxLag)
        {
            const double s0 = cmndf[bestTau - 1];
            const double s1 = cmndf[bestTau];
            const double s2 = cmndf[bestTau + 1];
            const double denom = 2.0 * (2.0 * s1 - s2 - s0);
            if (std::abs(denom) > 1e-9)
            {
                const double shift = (s2 - s0) / denom;
                return { std::clamp(bestTau + shift, static_cast<double>(minLag), static_cast<double>(maxLag)), true };
            }
        }

        return { static_cast<double>(bestTau), true };
    }

    float readSampleCubicRing(double pos) const
    {
        const int i1 = static_cast<int>(std::floor(pos)) & kRingMask;
        const float t = static_cast<float>(pos - std::floor(pos));

        const float y0 = inRing[(i1 - 1) & kRingMask];
        const float y1 = inRing[i1];
        const float y2 = inRing[(i1 + 1) & kRingMask];
        const float y3 = inRing[(i1 + 2) & kRingMask];

        const float t2 = t * t;
        const float t3 = t2 * t;

        return 0.5f * (
            2.0f * y1 +
            (-y0 + y2) * t +
            (2.0f * y0 - 5.0f * y1 + 4.0f * y2 - y3) * t2 +
            (-y0 + 3.0f * y1 - 3.0f * y2 + y3) * t3
        );
    }

    double fs = 44100.0;
    std::vector<float> inRing;
    std::vector<float> smoothRing;
    std::vector<float> wetRing;
    std::vector<float> normRing;
    std::vector<double> cmndf;

    int writePos = 0;
    int readPos = 0;
    double nextSynthPos = 0.0;
    double lastPeriod = 256.0;
    bool lastVoiced = false;
    float lpState = 0.0f;
    float dcPrevX = 0.0f;
    float dcPrevY = 0.0f;
    float inSq = 0.0f;
    float wetSq = 0.0f;
    float smoothGain = 1.0f;
};
