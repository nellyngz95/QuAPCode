#pragma once

#include <random>
#include <ctime>

#include "Definitions.h"

namespace nemisindo::nemlib {

inline float pureDataFreq(float sampleRate, float frequency) {
    assert(sampleRate > 0.f);
    return sampleRate / (2.0f * M_PI) * std::acosf(2.0f * (1.0f - 2.0f * M_PI * frequency / sampleRate)
        / (1.0f + pow(1.0f - 2.0f * M_PI * frequency / sampleRate, 2.0f)));
}

class Random {
public:
    Random() : generator((unsigned int)time(nullptr)) {}
    Random(unsigned int seed) : generator(seed) {}

    float nextFloat() { return distribution(generator); }

private:
    std::minstd_rand generator;
    std::uniform_real_distribution<float> distribution;
};

// updates the estimate every time a window is filled;
// beware the sudden jumps in the output.
class MovingRMS {
public:
    MovingRMS() = default;

    void setWindowLength(int length) {
        assert(length > 0);
        windowLength = length;
    }

    float nextSample(float sample) {
        sqSum += sample * sample;
        if (++count == windowLength) {
            rms = std::sqrt(sqSum / windowLength);
            sqSum = 0.f;
            count = 0;
        }
        return rms;
    }

private:
    int windowLength = 256;
    int count = 0;
    float sqSum = 0.f;
    float rms = 0.f;
};

class Timer {
public:
    Timer() = default;

    void initialize(float sampleRate) {
        this->sampleRate = sampleRate;
        reset();
    }

    void reset() {
        active = false;
        counter = 0;
    }

    void setTimeSeconds(float time) {
        assert(time > 0.f);
        countUntil = std::ceilf(time * sampleRate);
    }

    void setTimeSamples(int samples) {
        assert(samples > 0);
        countUntil = samples;
    }

    void start() { active = true; }
    void stop() { active = false; }

    bool tick() {
        assert(sampleRate > 0.f);
        bool completed = false;
        if (active) {
            if (counter++ == countUntil) {
                reset();
                completed = true;
            }
        }
        return completed;
    }

private:
    int countUntil = 0;
    int counter = 0;
    bool active = false;
    float sampleRate = 0.f;
};

}
