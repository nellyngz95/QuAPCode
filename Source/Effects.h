#pragma once

#include "Definitions.h"
#include "Utils.h"
#include "Filters.h"

#include <algorithm>

namespace nemisindo::nemlib {

class Clipper {
public:
    Clipper() = default;
    Clipper(float lo, float hi) {
        setBounds(lo, hi);
    }

    void setBounds(float lo, float hi) {
        assert(hi >= lo);

        this->lo = lo;
        this->hi = hi;
    }

    float nextSample(float in) const { return std::clamp(in, lo, hi); }

private:
    float lo = -1.0f, hi = 1.0f;
};

class Distortion {
public:
    Distortion() = default;

    void setAmount(float amount) { this->amount = amount; }
    void setOutGain(float outGain) { this->outGain = outGain; }

    float nextSample(float in) const {
        // Match the JS implementation's distortion formula
        return outGain * (3.f + amount) * in / (3.f + amount * std::abs(in));
    }

private:
    float amount = 1.f, outGain = 1.f / 3.f;
};

class Delay {
public:
    Delay() = default;

    struct Parameters {
        float delayTime = 0.5f;
        float feedbackGain = 0.5f;
        float dryGain = 0.0f;
        float wetGain = 1.0f;
    };

    void initialize(float sampleRate, float maxDelayTime = 5.0f) {
        this->sampleRate = sampleRate;

        maxSize = (int)(maxDelayTime * sampleRate);
        buffer.resize(maxSize);
        reset();
    }

    void reset() {
        std::fill_n(buffer.begin(), maxSize, 0.f);
        writeIdx = 0;
    }

    void setParameters(const Parameters& params) {
        delaySamples = (int)(params.delayTime * sampleRate);
        delaySamples = std::clamp(delaySamples, 0, maxSize - 1);
        feedbackGain = params.feedbackGain;
        dryGain = params.dryGain;
        wetGain = params.wetGain;
    }

    float nextSample(float in) {
        buffer[writeIdx] = in;
        int readIdx = writeIdx - delaySamples;
        if (readIdx < 0) readIdx += maxSize;

        float out = buffer[readIdx];
        buffer[writeIdx] += feedbackGain * out;

        if (++writeIdx == maxSize) writeIdx = 0;

        return dryGain * in + wetGain * out;
    }

private:
    std::vector<float> buffer;
    int delaySamples = 0;
    float sampleRate = 48000.0f;
    float dryGain = 0.0f, wetGain = 1.0f;
    float feedbackGain = 0.5f;

    int writeIdx = 0;
    int maxSize = 0;
};

class Overdrive {
public:
    struct Parameters {
        float drive = 0.0f;
        float volume = 6.0f;
        float bias = 0.0f;
        float knee = 0.0f;
        float tone = 1.f;
    };

    Overdrive() = default;

    void setParameters(const Parameters& params) {
        toneFilter.setFrequency(200 + (std::powf(10.f, params.tone) - 1.f) * 2200.f);

        inputGain = std::pow(10.0f, params.drive / 20.0f);
        outputGain = std::pow(10.0f, params.volume / 20.0f);
        bias = params.bias;
        knee = std::max(params.knee, 0.001f);

        computeCoefficients();
    }

    void initialize(float sampleRate) {
        toneFilter.initialize(sampleRate);
        dcBlocker.initialize(sampleRate);

        dcBlocker.setFrequency(20);
        toneFilter.setFrequency(20000);
    }

    void reset() {
        prev = 0.f;
        dcBlocker.reset();
        toneFilter.reset();
    }

    float nextSample(float in) {
        in = toneFilter.nextSample(in);

        float x = in * inputGain;
        float y = 0.0f;
        if (x > (1 - knee)) {
            if (x >= (1 + knee) || knee == 0.f) y = outputGain;
            else y = c2 * x * x + c1 * x + c0;
        }
        else if (x < -(1 - bias) * (1 - knee)) {
            if (x <= -(1 - bias) * (1 + knee) || knee == 0.f) y = -(1 - bias) * outputGain;
            else y = -c2 * x * x / (1 - bias) + c1 * x - c0 * (1 - bias);
        }
        else y = x * outputGain;

        float out = (1 - alpha) * prev + alpha * y;
        prev = out;

        return dcBlocker.nextSample(out);
    }

private:
    float inputGain = 1.0f;
    float outputGain = 1.0f;
    float bias = 0.0f;
    float knee = 0.0f;

    float c0 = 0.0f, c1 = 0.0f, c2 = 0.0f;
    float alpha = 0.99f, prev = 0.0f;

    BiquadFilter<BiquadFilterType::LowPass> toneFilter;
    BiquadFilter<BiquadFilterType::HighPass> dcBlocker;

    void computeCoefficients() {
        c2 = -outputGain / (4.f * knee);
        c1 = outputGain * (1.f + knee) / (2.f * knee);
        c0 = -outputGain * (1.f - knee) * (1.f - knee) / (4.f * knee);
    }
};

}
