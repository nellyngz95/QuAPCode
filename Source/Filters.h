#pragma once

#include "Definitions.h"
#include <algorithm>

namespace nemisindo::nemlib {

enum class BiquadFilterType {
    LowPass,
    HighPass,
    BandPass,
    Notch,
    Peak,
    LowShelf,
    HighShelf,
    AllPass
};

template <BiquadFilterType Type>
class BiquadFilter {
public:
    BiquadFilter() = default;
    void initialize(float sampleRate) {
        assert(sampleRate > 0.f);
        this->sampleRate = sampleRate;
        computeCoefficients();
    }

    float nextSample(float sample) {
        float out = sample * b0 + x1 * b1 + x2 * b2 - y1 * a1 - y2 * a2;
        y2 = y1;
        y1 = out;
        x2 = x1;
        x1 = sample;
        return out;
    }

    void setFrequency(float frequency) {
        if (this->frequency != frequency) {
            this->frequency = frequency;
            computeCoefficients();
        }
    }

    void setQ(float q) {
        assert(q > 0.f);
        if (this->q != q) {
            this->q = q;
            computeCoefficients();
        }
    }

    void setPeakGainDb(float gainDb) {
        if (this->peakGainDb != gainDb) {
            this->peakGainDb = gainDb;
            computeCoefficients();
        }
    }

    void reset() {
        y1 = 0.f;
        y2 = 0.f;
        x1 = 0.f;
        x2 = 0.f;
    }

private:
    void computeCommonValues() {
        v = std::pow(10.f, peakGainDb / 40.f);
        omega = 2.f * M_PI * frequency / sampleRate;
        sinOmega = std::sinf(omega);
        cosOmega = std::cosf(omega);
        alpha = std::max(0.005f, sinOmega / (2.f * q));
    }

    void computeCoefficients() {
        _computeCoefficients();
        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;
    }

    void _computeCoefficients();

    float sampleRate = 48000.f;
    float frequency = 1.f;
    float peakGainDb = 0.f;
    float q = 1.f;

    // State variables
    float y1 = 0.f;
    float y2 = 0.f;
    float x1 = 0.f;
    float x2 = 0.f;

    // Filter coefficients
    float b0 = 0.f;
    float b1 = 0.f;
    float b2 = 0.f;
    float a0 = 0.f;
    float a1 = 0.f;
    float a2 = 0.f;

    float v = 1.f, omega = 0.f, alpha = 0.f;
    float sinOmega = 0.f, cosOmega = 0.f;
};

template<>
inline void BiquadFilter<BiquadFilterType::LowPass>::_computeCoefficients() {
    frequency = std::clamp(frequency, 1.f, sampleRate / 2.f - 1.f);
    computeCommonValues();

    b0 = (1.f - cosOmega) / 2.f;
    b1 = 1.f - cosOmega;
    b2 = (1.f - cosOmega) / 2.f;
    a0 = 1.f + alpha;
    a1 = -2.f * cosOmega;
    a2 = 1.f - alpha;
}

template<>
inline void BiquadFilter<BiquadFilterType::HighPass>::_computeCoefficients() {
    frequency = std::clamp(frequency, 1.f, sampleRate / 2.f - 1.f);
    computeCommonValues();

    b0 = (1.f + cosOmega) / 2.f;
    b1 = -(1.f + cosOmega);
    b2 = (1.f + cosOmega) / 2.f;
    a0 = 1.f + alpha;
    a1 = -2.f * cosOmega;
    a2 = 1.f - alpha;
}

template<>
inline void BiquadFilter<BiquadFilterType::BandPass>::_computeCoefficients() {
    frequency = std::clamp(frequency, 1.f, sampleRate / 2.f - 1.f);
    computeCommonValues();

    b0 = alpha;
    b1 = 0.f;
    b2 = -alpha;
    a0 = 1.f + alpha;
    a1 = -2.f * cosOmega;
    a2 = 1.f - alpha;
}

template<>
inline void BiquadFilter<BiquadFilterType::Notch>::_computeCoefficients() {
    frequency = std::clamp(frequency, 1.f, sampleRate / 2.f - 1.f);
    computeCommonValues();

    b0 = 1.f;
    b1 = -2.f * cosOmega;
    b2 = 1.f;
    a0 = 1.f + alpha;
    a1 = -2.f * cosOmega;
    a2 = 1.f - alpha;
}

template<>
inline void BiquadFilter<BiquadFilterType::Peak>::_computeCoefficients() {
    frequency = std::clamp(frequency, 1.f, sampleRate / 2.f - 1.f);
    computeCommonValues();

    b0 = 1.f + alpha * v;
    b1 = -2.f * cosOmega;
    b2 = 1.f - alpha * v;
    a0 = 1.f + alpha / v;
    a1 = -2.f * cosOmega;
    a2 = 1.f - alpha / v;
}

template<>
inline void BiquadFilter<BiquadFilterType::LowShelf>::_computeCoefficients() {
    frequency = std::clamp(frequency, 1.f, sampleRate / 2.f - 1.f);
    computeCommonValues();

    b0 = v * ((v + 1.f) - (v - 1.f) * cosOmega + 2.f * std::sqrt(v) * alpha);
    b1 = 2.f * v * ((v - 1.f) - (v + 1.f) * cosOmega);
    b2 = v * ((v + 1.f) - (v - 1.f) * cosOmega - 2.f * std::sqrt(v) * alpha);
    a0 = (v + 1.f) + (v - 1.f) * cosOmega + 2.f * std::sqrt(v) * alpha;
    a1 = -2.f * ((v - 1.f) + (v + 1.f) * cosOmega);
    a2 = (v + 1.f) + (v - 1.f) * cosOmega - 2.f * std::sqrt(v) * alpha;
}

template<>
inline void BiquadFilter<BiquadFilterType::HighShelf>::_computeCoefficients() {
    frequency = std::clamp(frequency, 1.f, sampleRate / 2.f - 1.f);
    computeCommonValues();

    b0 = v * ((v + 1.f) + (v - 1.f) * cosOmega + 2.f * std::sqrt(v) * alpha);
    b1 = -2.f * v * ((v - 1.f) + (v + 1.f) * cosOmega);
    b2 = v * ((v + 1.f) + (v - 1.f) * cosOmega - 2.f * std::sqrt(v) * alpha);
    a0 = (v + 1.f) - (v - 1.f) * cosOmega + 2.f * std::sqrt(v) * alpha;
    a1 = 2.f * ((v - 1.f) - (v + 1.f) * cosOmega);
    a2 = (v + 1.f) - (v - 1.f) * cosOmega - 2.f * std::sqrt(v) * alpha;
}

template<>
inline void BiquadFilter<BiquadFilterType::AllPass>::_computeCoefficients() {
    frequency = std::clamp(frequency, 1.f, sampleRate / 2.f - 1.f);
    computeCommonValues();

    b0 = 1.f - alpha;
    b1 = -2.f * cosOmega;
    b2 = 1.f + alpha;
    a0 = 1.f + alpha;
    a1 = -2.f * cosOmega;
    a2 = 1.f - alpha;
}

template class BiquadFilter<nemisindo::nemlib::BiquadFilterType::LowPass>;
template class BiquadFilter<nemisindo::nemlib::BiquadFilterType::HighPass>;
template class BiquadFilter<nemisindo::nemlib::BiquadFilterType::BandPass>;
template class BiquadFilter<nemisindo::nemlib::BiquadFilterType::Notch>;
template class BiquadFilter<nemisindo::nemlib::BiquadFilterType::Peak>;
template class BiquadFilter<nemisindo::nemlib::BiquadFilterType::LowShelf>;
template class BiquadFilter<nemisindo::nemlib::BiquadFilterType::HighShelf>;
template class BiquadFilter<nemisindo::nemlib::BiquadFilterType::AllPass>;


enum class OnePoleFilterType {
    LowPass,
    HighPass
};

template<OnePoleFilterType Type>
class OnePoleFilter {
public:
    OnePoleFilter() = default;

    void initialize(float sampleRate) {
        assert(sampleRate > 0.f);
        this->sampleRate = sampleRate;
        computeCoefficient();
    }

    // frankly i have no clue what this is
    void setNorm(float norm) {
        this->norm = norm;
        computeCoefficient();
    }

    float nextSample(float sample);
    void setFrequency(float frequency) {
        assert(frequency > 0.f);
        this->frequency = frequency;
        computeCoefficient();
    }

    void reset() {
        lastIn = 0.f;
        lastOut = 0.f;
    }

protected:
    void computeCoefficient();

    float norm = 1.f;
    float sampleRate = 48000.f;
    float lastOut = 0.f;
    float lastIn = 0.f;
    float coeff = 1.f;
    float frequency = 1000.f;
};

template<>
inline void OnePoleFilter<OnePoleFilterType::LowPass>::computeCoefficient() {
    coeff = 2.f * M_PI * frequency / sampleRate;
    coeff = std::clamp(coeff, 0.f, 1.f);
}

template<>
inline float OnePoleFilter<OnePoleFilterType::LowPass>::nextSample(float sample) {
    lastOut = sample * coeff + (1.f - coeff) * lastOut;
    return lastOut;
}

template<>
inline void OnePoleFilter<OnePoleFilterType::HighPass>::computeCoefficient() {
    coeff = 1 - 2.f * M_PI * frequency / sampleRate;
    coeff = std::clamp(coeff, 0.f, 1.f);
    if (norm != 1) norm = (1 + coeff) / 2;
}

template<>
inline float OnePoleFilter<OnePoleFilterType::HighPass>::nextSample(float sample) {
    lastOut = norm * (sample - lastIn) + coeff * lastOut;
    lastIn = sample;
    return lastOut;
}

using OnePoleLPF = OnePoleFilter<OnePoleFilterType::LowPass>;
using OnePoleHPF = OnePoleFilter<OnePoleFilterType::HighPass>;

// redundant declaration to work around a bug in cppyy
template class OnePoleFilter<nemisindo::nemlib::OnePoleFilterType::LowPass>;
template class OnePoleFilter<nemisindo::nemlib::OnePoleFilterType::HighPass>;

class TwoPoleBPF {
public:
    TwoPoleBPF() = default;
    void initialize(float sampleRate)  {
        assert(sampleRate > 0.f);
        this->sampleRate = sampleRate;
        computeCoefficients();
    }

    float nextSample(float sample) {
        float output = sample + (coeff1 * z1) + (coeff2 * z2);
        z2 = z1;
        z1 = output;
        return compGain * output;
    }

    void setFrequency(float frequency) {
        assert(frequency > 0.f);
        this->frequency = frequency;
        computeCoefficients();
    }

    void setQ(float q) {
        assert(q > 0.f);
        this->q = q;
        computeCoefficients();
    }

    void reset() {
        z1 = 0;
        z2 = 0;
    }

private:
    void computeCoefficients() {
        float k = 2.f * M_PI * frequency / sampleRate;
        float oneMinusR = std::min(1.f, k/q);
        float r = 1.f - oneMinusR;
        coeff1 = 2.f * std::cos(k) * r;
        coeff2 = -r * r;
        compGain = 2.f * oneMinusR * (oneMinusR + r * k);
    }

    float sampleRate = 48000.f;
    float frequency = 2000.f;
    float q = 0.707f;

    float z1 = 0.f;
    float z2 = 0.f;
    float coeff1 = 0.f;
    float coeff2 = 0.f;
    float compGain = 0.f;
};

// Exact implementation of the JavaScript 'one-pole-BP-processor' (which is actually a two-pole filter)
class JSOnePoleProcessor {
public:
    JSOnePoleProcessor() = default;
    
    void initialize(float sampleRate) {
        assert(sampleRate > 0.f);
        this->sampleRate = sampleRate;
        reset();
    }
    
    void reset() {
        prevOut[0] = 0.0f;
        prevOut[1] = 0.0f;
        prevOut2[0] = 0.0f;
        prevOut2[1] = 0.0f;
    }
    
    void setFrequency(float frequency) {
        this->frequency = frequency;
        computeCoefficients();
    }
    
    void setQ(float q) {
        this->q = q;
        computeCoefficients();
    }
    
    float nextSample(float sample, int channel = 0) {
        float output1 = sample + (coeff1 * prevOut[channel]) + (coeff2 * prevOut2[channel]);
        float outputSample = bpGain * output1;
        
        prevOut2[channel] = prevOut[channel];
        prevOut[channel] = output1;
        
        return outputSample;
    }
    
private:
    void computeCoefficients() {
        float omega = 2.0f * M_PI * frequency / sampleRate;
        float r;
        
        if (q < 0.001f) {
            r = 0.0f;
        } else {
            r = 1.0f - omega / q;
        }
        
        if (r < 0.0f) {
            r = 0.0f;
        }
        
        float c;
        if (omega >= -M_PI / 2.0f && omega <= M_PI / 2.0f) {
            float g = omega * omega;
            // This strange calculation is an approximation of cos(g)
            c = (((g * g * g * (-1.0f / 720.0f) + g * g * (1.0f / 24.0f)) - g * 0.5f) + 1.0f);
        } else {
            c = 0.0f;
        }
        
        coeff1 = 2.0f * c * r;
        coeff2 = -r * r;
        bpGain = 2.0f * (1.0f - r) * (1.0f - r + r * omega);
    }
    
    float sampleRate = 48000.0f;
    float frequency = 250.0f;
    float q = 10.0f;
    
    float prevOut[2] = {0.0f, 0.0f};
    float prevOut2[2] = {0.0f, 0.0f};
    
    float coeff1 = 0.0f;
    float coeff2 = 0.0f;
    float bpGain = 0.0f;
};

}
