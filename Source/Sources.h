#pragma once

#include "Definitions.h"
#include "Utils.h"
#include <array>

namespace nemisindo::nemlib {

template<unsigned int WtSize=NEM_OSC_DEFAULT_WT_SIZE>
using Wavetable = std::array<float, WtSize>;

enum class Waveshape {
    sine,
    square,
    triangle,
    saw,
};

template<unsigned int WtSize=NEM_OSC_DEFAULT_WT_SIZE>
class WavetableIndex {
public:
    void initialize(float sampleRate) {
        assert(sampleRate > 0.f);
        phaseFactor = WtSize / sampleRate;
        index = 0.f;
    }

    void tick() {
        index += phaseInc;
        if (index >= WtSize) {
            index -= WtSize;
        }
    }

    float getSample(Wavetable<WtSize> const& wavetable) const {
        if (phaseInc == 0.f) {
            return wavetable[0];
        }

        int i1 = static_cast<int>(index);
        int i2 = i1 + 1;
        if (i2 == wavetable.size()) {
            i2 = 0;
        }
        float frac = index - i1;
        return wavetable[i1] + frac * (wavetable[i2] - wavetable[i1]);
    }

    void setFrequency(float frequency) {
        assert(frequency >= 0.f);
        phaseInc = frequency * phaseFactor;
    }

    // initialPhase should be in [0, 1)
    void reset(float initialPhase = 0.f) {
        assert(initialPhase >= 0.f && initialPhase < 1.f);
        index = initialPhase * WtSize;
    }

private:
    float index = 0.f;
    float phaseFactor = 0.f;
    float phaseInc = 0.f;
};

template<unsigned int WtSize=NEM_OSC_DEFAULT_WT_SIZE>
class WavetableOscillatorSource {
public:
    WavetableOscillatorSource(Waveshape oscType = Waveshape::sine, int numOscillators = 1)
    : wtIdx(numOscillators), gains(numOscillators), oscType(oscType), numOscillators(numOscillators) {
        std::fill(gains.begin(), gains.end(), 1.f);
    }

    ~WavetableOscillatorSource() = default;

    void initialize(float sampleRate) {
        assert(sampleRate > 0.f);

        for (int i = 0; i < numOscillators; ++i) {
            wtIdx[i].initialize(sampleRate);
        }

        int prevNumPartials = 0;

        for (int j = wavetables.size() - 1; j >= 0 ; j--) {

            float frequency = NEM_OSC_WT_LOWEST_FREQ * std::powf(2.f, j / static_cast<float>(NEM_OSC_WT_PER_OCTAVE));
            int numPartials = std::min(static_cast<int>((sampleRate / 2.f) / frequency), NEM_OSC_MAX_NUM_PARTIALS);

            if (j < wavetables.size() - 1)
                std::copy_n(wavetables[j + 1].begin(), WtSize, wavetables[j].begin());
            else
                std::fill_n(wavetables[j].begin(), WtSize, 0.f);

            for (int i = prevNumPartials + 1; i <= numPartials; i++) {
                float amp = 0.f;
                switch (oscType) {
                    case Waveshape::sine:
                        if (i == 1) amp = 1.f;
                    break;
                    case Waveshape::square:
                        // https://mathworld.wolfram.com/FourierSeriesSquareWave.html
                            if (i % 2 == 1) amp = 4.f / (i * M_PI);
                    break;
                    case Waveshape::triangle:
                        if (i % 2 == 1) amp = 8.f / (i * i * M_PI * M_PI);
                    if (i / 2 % 2 == 1) amp = -amp;
                    break;
                    case Waveshape::saw:
                        amp = 2.f / (i * M_PI);
                    if (i % 2 == 0) amp = -amp;
                    break;
                    default:
                        assert(false);
                }

                for (int k = 0; k < WtSize; k++) {
                    wavetables[j][k] += amp * std::sin(2 * M_PI * i * k / WtSize);
                }
            }
            assert(wavetables[j][0] == 0.f);

            prevNumPartials = numPartials;
        }
    }

    float nextSample()  {
        float out = 0.f;
        for (int i = 0; i < numOscillators; ++i) {
            float sample;
            unsigned int idx = tableLowerIdx[i];

            if (idx == wavetables.size() - 1) {
                sample = wtIdx[i].getSample(wavetables[idx]);
            }
            else {
                float frac = tableFractionalIdx[i];
                sample = (1.f - frac) * wtIdx[i].getSample(wavetables[idx]) + frac * wtIdx[i].getSample(wavetables[idx + 1]);
            }
            out += sample * gains[i];

            wtIdx[i].tick();
        }

        return out;
    }

    void setFrequency(float frequency, int index = 0) {
        assert(frequency >= 0.f);
        assert(index >= 0 && index < numOscillators);

        wtIdx[index].setFrequency(frequency);

        float exactTable = 1 + std::log2(frequency / NEM_OSC_WT_LOWEST_FREQ) * NEM_OSC_WT_PER_OCTAVE;

        exactTable = std::max(0.f, exactTable);
        exactTable = std::min(wavetables.size() - 1.f, exactTable);

        unsigned int lowerTable = static_cast<unsigned int>(exactTable);

        float frac = exactTable - lowerTable;

        tableLowerIdx[index] = lowerTable;
        tableFractionalIdx[index] = frac;
    }

    void setGain(float gain, int index = 0) {
        assert(index >= 0 && index < numOscillators);

        gains[index] = gain;
    }

    void reset(float initialPhase = 0.f, int index = 0) {
        assert(index >= 0 && index < numOscillators);

        wtIdx[index].reset(initialPhase);
    }

private:
    std::vector<WavetableIndex<WtSize>> wtIdx;
    std::vector<float> gains;

    Waveshape oscType = Waveshape::sine;
    int numOscillators;

    std::array<Wavetable<WtSize>, NEM_OSC_WT_NUM_OCTAVES * NEM_OSC_WT_PER_OCTAVE> wavetables;

    std::array<unsigned int, NEM_OSC_WT_NUM_OCTAVES * NEM_OSC_WT_PER_OCTAVE> tableLowerIdx = {};
    std::array<float, NEM_OSC_WT_NUM_OCTAVES * NEM_OSC_WT_PER_OCTAVE> tableFractionalIdx = { 0.f };
};

typedef WavetableOscillatorSource<> OscillatorSource;
typedef WavetableOscillatorSource<1024> LFOSource;

class WhiteNoiseSource {
public:
    WhiteNoiseSource() = default;
    WhiteNoiseSource(unsigned int seed) : random(seed) {}

    float nextSample() {
        return 2.f * random.nextFloat() - 1.f;
    }

private:
    Random random;
};

class PinkNoiseSource {
public:
    PinkNoiseSource() = default;
    PinkNoiseSource(unsigned int seed) : whiteNoise(seed) {};

    float nextSample() {
        float white = whiteNoise.nextSample();

        B[0] = 0.99886f * B[0] + white * 0.0555179f;
        B[1] = 0.99332f * B[1] + white * 0.0750759f;
        B[2] = 0.96900f * B[2] + white * 0.1538520f;
        B[3] = 0.86650f * B[3] + white * 0.3104856f;
        B[4] = 0.55000f * B[4] + white * 0.5329522f;
        B[5] = -0.7616f * B[5] - white * 0.0168980f;

        float out = B[0] + B[1] + B[2] + B[3] + B[4] + B[5] + B[6] + white * 0.5362f;
        B[6] = white * 0.115926f;

        return 0.11f * out;
    }

private:
    WhiteNoiseSource whiteNoise;
    float B[7] = { 0.f };
};

class PulseSource {
public:
    PulseSource() = default;
    PulseSource(unsigned int seed) : random(seed) {}

    void initialize(float sampleRate) {
        this->sampleRate = sampleRate;
    }

    float nextSample(float in, bool& pulseTriggered, float& outFreq) {
        pulseTriggered = false;
        float out = 0.f;

        if (sampleCounter == 255) {
            if (in > 0.49f && in < 0.52f) {
                decayS = (random.nextFloat() * 30.f) * sampleRate / 1000.f;
                outFreq = 1500.f + (500.f * decayS * 1000.f / sampleRate);
                pulseTriggered = true;
            }
            else {
                decayS = 0.f;
            }
            sampleCounter = 0;
        }
        if (decayS > 0.f) {
            if (sampleNum < decayS) {
                out = (1.f - sampleNum / decayS) * (1.f - sampleNum / decayS);
            }
            else if (sampleNum > 256) {
                sampleNum = -1;
            }
            sampleNum++;
        }
        sampleCounter++;
        return out;
    }

private:
    float sampleRate = 0.f;
    float decayS = 0.f;
    int sampleNum = 0;
    int sampleCounter = 0;

    Random random;
};

}
