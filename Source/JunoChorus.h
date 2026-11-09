#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

/**
 * Juno-60 Style Stereo BBD Chorus
 * - Authentic dual-voice triangle LFO (0.513 Hz + 0.863 Hz blend)
 * - Anti-phase Left/Right modulation for lush wide stereo imaging
 * - Warm BBD 7.5 kHz low-pass filtering + 4-point Catmull-Rom cubic delay read
 * - Smooth percentage Wet/Dry mix (0% - 100%)
 */
class JunoChorus
{
public:
    static constexpr int kBufSize = 4096;
    static constexpr int kBufMask = kBufSize - 1;

    void prepare(double sampleRate)
    {
        fs = sampleRate > 8000.0 ? sampleRate : 44100.0;
        bufL.assign(kBufSize, 0.0f);
        bufR.assign(kBufSize, 0.0f);
        writePos = 0;
        lfoPhase1 = 0.0;
        lfoPhase2 = 0.25;
        lpL = 0.0f;
        lpR = 0.0f;
    }

    void processStereo(float* left, float* right, int numSamples, float chorusMix)
    {
        if (chorusMix <= 0.001f)
            return;

        const float mix = std::clamp(chorusMix, 0.0f, 1.0f);
        const double lfoInc1 = 0.513 / fs;
        const double lfoInc2 = 0.863 / fs;

        // Warm BBD 7.5 kHz 1-pole low-pass
        const float lpAlpha = std::min(0.95f, static_cast<float>((2.0 * 3.14159265 * 7500.0) / (fs + 2.0 * 3.14159265 * 7500.0)));

        const double baseDelayMs = 3.2;
        const double modDepthMs = 1.9;
        const double baseDelaySamp = (baseDelayMs * 0.001) * fs;
        const double modDepthSamp = (modDepthMs * 0.001) * fs;

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = left[i];
            const float dryR = right ? right[i] : dryL;

            bufL[writePos] = dryL;
            bufR[writePos] = dryR;

            // Triangle LFO [-1, +1]
            const double tri1 = 1.0 - 4.0 * std::abs(std::fmod(lfoPhase1, 1.0) - 0.5);
            const double tri2 = 1.0 - 4.0 * std::abs(std::fmod(lfoPhase2, 1.0) - 0.5);
            const double lfo = 0.72 * tri1 + 0.28 * tri2;

            // Anti-phase stereo BBD modulation
            const double delayL = baseDelaySamp + lfo * modDepthSamp;
            const double delayR = baseDelaySamp - lfo * modDepthSamp;

            const float rawWetL = readCubic(bufL, static_cast<double>(writePos) - delayL);
            const float rawWetR = readCubic(bufR, static_cast<double>(writePos) - delayR);

            lpL += lpAlpha * (rawWetL - lpL);
            lpR += lpAlpha * (rawWetR - lpR);

            left[i] = dryL * (1.0f - mix * 0.5f) + lpL * (mix * 0.75f);
            if (right)
                right[i] = dryR * (1.0f - mix * 0.5f) + lpR * (mix * 0.75f);

            lfoPhase1 += lfoInc1;
            if (lfoPhase1 >= 1.0) lfoPhase1 -= 1.0;
            lfoPhase2 += lfoInc2;
            if (lfoPhase2 >= 1.0) lfoPhase2 -= 1.0;

            writePos = (writePos + 1) & kBufMask;
        }
    }

private:
    float readCubic(const std::vector<float>& buf, double pos) const
    {
        const int i1 = static_cast<int>(std::floor(pos)) & kBufMask;
        const float t = static_cast<float>(pos - std::floor(pos));

        const float y0 = buf[(i1 - 1) & kBufMask];
        const float y1 = buf[i1];
        const float y2 = buf[(i1 + 1) & kBufMask];
        const float y3 = buf[(i1 + 2) & kBufMask];

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
    std::vector<float> bufL;
    std::vector<float> bufR;
    int writePos = 0;
    double lfoPhase1 = 0.0;
    double lfoPhase2 = 0.25;
    float lpL = 0.0f;
    float lpR = 0.0f;
};
