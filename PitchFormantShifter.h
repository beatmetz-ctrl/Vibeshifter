#pragma once
#include <JuceHeader.h>
#include <vector>
#include <cmath>
#include <complex>

class PitchFormantShifter
{
public:
    static constexpr int fftOrder = 11;              // 2^11 = 2048 Samples
    static constexpr int fftSize = 1 << fftOrder;    // 2048
    static constexpr int overlap = 4;                // 75% Overlap
    static constexpr int hopSize = fftSize / overlap;// 512 Samples

    PitchFormantShifter() : forwardFFT(fftOrder), inverseFFT(fftOrder)
    {
        reset();
    }

    void reset()
    {
        inputBuffer.assign(fftSize, 0.0f);
        outputBuffer.assign(fftSize * 2, 0.0f);
        window.resize(fftSize);
        
        lastPhase.assign(fftSize / 2 + 1, 0.0f);
        sumPhase.assign(fftSize / 2 + 1, 0.0f);
        
        fftData.assign(fftSize * 2, 0.0f);
        cepstrumData.assign(fftSize * 2, 0.0f);
        
        magnitudes.assign(fftSize / 2 + 1, 0.0f);
        frequencies.assign(fftSize / 2 + 1, 0.0f);
        envelope.assign(fftSize / 2 + 1, 1.0f);
        
        synthMagnitudes.assign(fftSize / 2 + 1, 0.0f);
        synthFrequencies.assign(fftSize / 2 + 1, 0.0f);

        for (int i = 0; i < fftSize; ++i)
            window[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / (float)fftSize));

        inputWritePos = 0;
        outputReadPos = 0;
        hopCounter = 0;
    }

    void processBlock(float* channelData, int numSamples, double sampleRate, float pitchSemitones, float formantSemitones, float dryWet)
    {
        float pitchRatio = std::pow(2.0f, pitchSemitones / 12.0f);
        float formantRatio = std::pow(2.0f, formantSemitones / 12.0f);

        for (int i = 0; i < numSamples; ++i)
        {
            float drySample = channelData[i];
            
            inputBuffer[inputWritePos] = drySample;
            inputWritePos = (inputWritePos + 1) % fftSize;

            float wetSample = outputBuffer[outputReadPos];
            outputBuffer[outputReadPos] = 0.0f;
            outputReadPos = (outputReadPos + 1) % outputBuffer.size();

            channelData[i] = drySample * (1.0f - dryWet) + wetSample * dryWet;

            if (++hopCounter >= hopSize)
            {
                hopCounter = 0;
                processFrame(sampleRate, pitchRatio, formantRatio);
            }
        }
    }

private:
    void processFrame(double sampleRate, float pitchRatio, float formantRatio)
    {
        const float pi = juce::MathConstants<float>::pi;
        const float twoPi = 2.0f * pi;
        const float expectedPhaseDiff = twoPi * (float)hopSize / (float)fftSize;
        const float freqPerBin = (float)sampleRate / (float)fftSize;

        std::fill(fftData.begin(), fftData.end(), 0.0f);
        for (int i = 0; i < fftSize; ++i)
        {
            int idx = (inputWritePos + i) % fftSize;
            fftData[i] = inputBuffer[idx] * window[i];
        }

        forwardFFT.performRealOnlyForwardTransform(fftData.data(), true);

        int numBins = fftSize / 2 + 1;
        for (int k = 0; k < numBins; ++k)
        {
            float real = fftData[k * 2];
            float imag = fftData[k * 2 + 1];

            float mag = std::sqrt(real * real + imag * imag);
            float phase = std::atan2(imag, real);

            float phaseDiff = phase - lastPhase[k];
            lastPhase[k] = phase;

            phaseDiff -= (float)k * expectedPhaseDiff;
            phaseDiff = std::remainder(phaseDiff, twoPi);

            float binDeviation = phaseDiff / expectedPhaseDiff;
            frequencies[k] = ((float)k + binDeviation) * freqPerBin;
            magnitudes[k] = mag;
        }

        extractSpectralEnvelope(numBins);

        std::fill(synthMagnitudes.begin(), synthMagnitudes.end(), 0.0f);
        std::fill(synthFrequencies.begin(), synthFrequencies.end(), 0.0f);

        for (int k = 0; k < numBins; ++k)
        {
            int targetBin = static_cast<int>(std::round(k * pitchRatio));
            if (targetBin >= 0 && targetBin < numBins)
            {
                float flattenedMag = magnitudes[k] / (envelope[k] + 1e-6f);
                synthMagnitudes[targetBin] += flattenedMag;
                synthFrequencies[targetBin] = frequencies[k] * pitchRatio;
            }
        }

        for (int k = 0; k < numBins; ++k)
        {
            float sourceEnvBinFloat = (float)k / formantRatio;
            int idx0 = static_cast<int>(std::floor(sourceEnvBinFloat));
            int idx1 = idx0 + 1;
            float frac = sourceEnvBinFloat - idx0;

            float shiftedEnv = 1.0f;
            if (idx0 >= 0 && idx1 < numBins)
                shiftedEnv = envelope[idx0] * (1.0f - frac) + envelope[idx1] * frac;
            else if (idx0 >= 0 && idx0 < numBins)
                shiftedEnv = envelope[idx0];

            synthMagnitudes[k] *= shiftedEnv;
        }

        std::fill(fftData.begin(), fftData.end(), 0.0f);
        for (int k = 0; k < numBins; ++k)
        {
            float mag = synthMagnitudes[k];
            float freq = synthFrequencies[k];

            float binDeviation = (freq - (float)k * freqPerBin) / freqPerBin;
            float phaseDiff = ((float)k + binDeviation) * expectedPhaseDiff;

            sumPhase[k] = std::remainder(sumPhase[k] + phaseDiff, twoPi);

            fftData[k * 2] = mag * std::cos(sumPhase[k]);
            fftData[k * 2 + 1] = mag * std::sin(sumPhase[k]);
        }

        inverseFFT.performRealOnlyInverseTransform(fftData.data());

        const float normFactor = 1.0f / 1.5f; 
        for (int i = 0; i < fftSize; ++i)
        {
            int outIdx = (outputReadPos + i) % outputBuffer.size();
            outputBuffer[outIdx] += fftData[i] * window[i] * normFactor;
        }
    }

    void extractSpectralEnvelope(int numBins)
    {
        std::fill(cepstrumData.begin(), cepstrumData.end(), 0.0f);
        for (int k = 0; k < numBins; ++k)
        {
            float logMag = std::log(magnitudes[k] + 1e-6f);
            cepstrumData[k * 2] = logMag;
            cepstrumData[k * 2 + 1] = 0.0f;
        }

        inverseFFT.performRealOnlyInverseTransform(cepstrumData.data());

        const int cepstrumCutoff = 35;
        for (int i = cepstrumCutoff; i < fftSize - cepstrumCutoff; ++i)
            cepstrumData[i] = 0.0f;

        forwardFFT.performRealOnlyForwardTransform(cepstrumData.data(), true);

        for (int k = 0; k < numBins; ++k)
        {
            float envVal = std::exp(cepstrumData[k * 2]);
            envelope[k] = juce::jlimit(0.01f, 100.0f, envVal);
        }
    }

    juce::dsp::FFT forwardFFT;
    juce::dsp::FFT inverseFFT;

    std::vector<float> inputBuffer;
    std::vector<float> outputBuffer;
    std::vector<float> window;

    std::vector<float> lastPhase;
    std::vector<float> sumPhase;
    std::vector<float> fftData;
    std::vector<float> cepstrumData;

    std::vector<float> magnitudes;
    std::vector<float> frequencies;
    std::vector<float> envelope;

    std::vector<float> synthMagnitudes;
    std::vector<float> synthFrequencies;

    int inputWritePos = 0;
    int outputReadPos = 0;
    int hopCounter = 0;
};
