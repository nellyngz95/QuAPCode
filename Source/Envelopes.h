#pragma once

#include "Definitions.h"

namespace nemisindo::nemlib {

enum class ValueSmoothingType {
    Linear,
    Exponential
};

template<ValueSmoothingType SmoothingType=ValueSmoothingType::Linear>
class SmoothedValue {
public:
    SmoothedValue();
    SmoothedValue(float initialValue);

    void initialize(float sampleRate) { inc = 0.f; this->sampleRate = sampleRate; }

    void setRampLengthSamples(int rampLengthSamples) {
        this->rampSamples = rampLengthSamples;
        _updateIncSafe();
    }

    void setRampLengthSeconds(float rampLengthSeconds) {
       setRampLengthSamples(static_cast<int>(rampLengthSeconds * sampleRate));
    }

    void reset() {
        reset(currentValue, targetValue);
    }

    void reset(float target) {
        reset(target, target);
    }

    void reset(float start, float target) {
        currentValue = start;
        targetValue = target;
        elapsedSamples = 0;
        _updateIncSafe();
    }

    void setTargetValue(float targetValue) {
        this->targetValue = targetValue;
        _updateIncSafe();
    }

    float nextSample() {
        float out = currentValue;
        if (!isRamping()) {
            out = currentValue = targetValue;
        } else {
            _tick();
        }
        elapsedSamples++;
        return out;
    }

    float getCurrentValue() const { return currentValue; }
    bool isRamping() const {
        return elapsedSamples < rampSamples && currentValue != targetValue;
    }

private:
    void _tick();
    void _updateInc();
    void _updateIncSafe() {
        if (!isRamping()) return;
        _updateInc();
    }

    float currentValue;
    float targetValue;

    float sampleRate = 48000.f;
    float inc = 0.f;
    int elapsedSamples = 0;
    int rampSamples = 0;
};
template<>
inline SmoothedValue<ValueSmoothingType::Linear>::SmoothedValue()
    : currentValue(0.f), targetValue(0.f) {}

template<>
inline SmoothedValue<ValueSmoothingType::Linear>::SmoothedValue(float initialValue)
    : currentValue(initialValue), targetValue(initialValue) {}

template<>
inline void SmoothedValue<ValueSmoothingType::Linear>::_tick() { currentValue += inc; }

template<>
inline void SmoothedValue<ValueSmoothingType::Linear>::_updateInc() {
    inc = (targetValue - currentValue) / static_cast<float>(rampSamples - elapsedSamples);
}

template<>
inline SmoothedValue<ValueSmoothingType::Exponential>::SmoothedValue()
    : currentValue(1.f), targetValue(1.f) {}

template<>
inline SmoothedValue<ValueSmoothingType::Exponential>::SmoothedValue(float initialValue)
    : currentValue(initialValue), targetValue(initialValue) { assert(initialValue > 0.f); }

template<>
inline void SmoothedValue<ValueSmoothingType::Exponential>::_tick() { currentValue *= inc; }

template<>
inline void SmoothedValue<ValueSmoothingType::Exponential>::_updateInc() {
    assert(currentValue != 0.f);
    assert(targetValue != 0.f);

    inc = std::powf(targetValue / currentValue, 1.f / static_cast<float>(rampSamples - elapsedSamples));
}

// redundant namespace specification is a workaround for a cppyy bug
typedef SmoothedValue<nemisindo::nemlib::ValueSmoothingType::Linear> LinearSmoothedValue;
typedef SmoothedValue<nemisindo::nemlib::ValueSmoothingType::Exponential> ExponentialSmoothedValue;


template<ValueSmoothingType SmoothingType=ValueSmoothingType::Linear>
class AHDSR {

public:
    struct Parameters {
        /*
         * If autoDecay is set to false, then the hold time has no effect; the envelope
         * will hold until decay() is called manually.
         */
        bool autoDecay = true;

        /*
         * If autoRelease is set to false, then the sustain time has no effect;
         * the envelope will sustain until release() is called manually.
         */
        bool autoRelease = true;

        /*
         * Envelope times in seconds
         */
        float attack = 0.f;
        float hold = 0.f;
        float decay = 0.f;
        float sustain = 0.f;
        float release = 0.f;

        float startLvl = 0.f;
        float holdLvl = 1.f;
        float sustainLvl = 1.f;
        float endLvl = 0.f;
    };

    enum class State {
        Attack,
        Hold,
        Decay,
        Sustain,
        Release,
        Idle
    };

    AHDSR() = default;

    void initialize(float sampleRate) {
        this->sampleRate = sampleRate;
        value.initialize(sampleRate);
    }

    void setParameters(Parameters const& params) {
        this->params = params;

        attackUntil = static_cast<int>(params.attack * sampleRate);
        holdUntil = static_cast<int>((params.attack + params.hold) * sampleRate);
        decayUntil = static_cast<int>((params.attack + params.hold + params.decay) * sampleRate);
        sustainUntil = static_cast<int>((params.attack + params.hold + params.decay + params.sustain) * sampleRate);
        releaseUntil = static_cast<int>((params.attack + params.hold + params.decay + params.sustain + params.release) * sampleRate);
    }

    void reset() {
        elapsedSamples = 0;
        state = State::Idle;
        value.reset(params.startLvl);
    }

    void attack() {
        elapsedSamples = 0;
        state = State::Attack;
        value.setRampLengthSamples(attackUntil);
        value.setTargetValue(params.holdLvl);
        value.reset();
    }

    void decay() {
        elapsedSamples = holdUntil;
        state = State::Decay;
        value.setRampLengthSamples(decayUntil - holdUntil);
        value.setTargetValue(params.sustainLvl);
        value.reset();
    }

    void release() {
        elapsedSamples = sustainUntil;
        state = State::Release;
        value.setRampLengthSamples(releaseUntil - sustainUntil);
        value.setTargetValue(params.endLvl);
        value.reset();
    }

    float nextSample() {
        if (state == State::Idle) return value.nextSample();

        // successive checks instead of a switch statement allows for the envelope
        // to go through multiple states in one call if their times are zero
        if (state == State::Attack) {
            if (elapsedSamples >= attackUntil) {
                state = State::Hold;
                value.reset(params.holdLvl);
            }
        }
        if (state == State::Hold) {
            if (params.autoDecay && elapsedSamples >= holdUntil) decay();
        }
        if (state == State::Decay) {
            if (elapsedSamples >= decayUntil) {
                elapsedSamples = decayUntil;
                state = State::Sustain;
                value.reset(params.sustainLvl);
            }
        }
        if (state == State::Sustain) {
            if (params.autoRelease && elapsedSamples >= sustainUntil) release();
        }
        if (state == State::Release) {
            if (elapsedSamples >= releaseUntil) {
                elapsedSamples = releaseUntil;
                state = State::Idle;
                value.reset(params.endLvl);
            }
        }

        elapsedSamples++;
        return value.nextSample();
    }

    State getState() const { return state; }
    bool isActive() const { return state != State::Idle; }

    int getTotalDurationSamples() const { return releaseUntil; }

    int getReleaseDurationSamples() const {
        return static_cast<int>(params.release * sampleRate) + 1;
    }

    float getTotalDurationSeconds() const {
        return params.attack + params.hold + params.decay + params.sustain + params.release;
    }

private:
    Parameters params;
    State state = State::Idle;

    float sampleRate = 48000.f;
    int attackUntil = 0;
    int holdUntil = 0;
    int decayUntil = 0;
    int sustainUntil = 0;
    int releaseUntil = 0;

    int elapsedSamples = 0;

    SmoothedValue<SmoothingType> value;
};

typedef AHDSR<nemisindo::nemlib::ValueSmoothingType::Linear> LinearAHDSR;
typedef AHDSR<nemisindo::nemlib::ValueSmoothingType::Exponential> ExponentialAHDSR;


class Pulse {
public:
    Pulse() = default;
    void initialize(float sampleRate) { envelope.initialize(sampleRate); }

    void setFrequency(float frequency) { this->frequency = frequency; }
    void setDutyCycle(float dutyCycle) { this->dutyCycle = dutyCycle; }
    void setAttack(float attack) { this->attack = attack; }
    void setRelease(float release) { this->release = release; }

    void reset() { envelope.reset(); }

    float nextSample() {
        if (!envelope.isActive()) {
            float pulseWidth = dutyCycle / frequency;
            pulseWidth = std::max(0.f, pulseWidth - attack - release);

            envelope.setParameters({
                .attack = attack,
                .sustain = pulseWidth,
                .release = release,
            });
            envelope.attack();
        }
        return envelope.nextSample();
    }

private:
    float frequency = 440.f;
    float dutyCycle = 0.5f;
    float attack = 0.0f;
    float release = 0.0f;

    LinearAHDSR envelope;
};

class Phasor {
public:
    Phasor() = default;
    
    void initialize(float sampleRate) {
        this->sampleRate = sampleRate;
        phase = 0.0f;
    }
    
    void setFrequency(float frequency) {
        this->frequency = frequency;
    }
    
    void setDutyCycle(float dutyCycle) {
        this->dutyCycle = dutyCycle;
    }
    
    void reset(float initialPhase = 0.0f) {
        phase = initialPhase;
    }
    
    float nextSample() {
        float output;
        
        if (dutyCycle > 0.0f && phase < dutyCycle) {
            output = phase * (1.0f / dutyCycle);
        } else {
            output = 0.0f;
        }
        
        // Update phase
        phase += frequency / sampleRate;
        phase = phase - std::floor(phase);
        
        return output;
    }
    
private:
    float sampleRate = 48000.0f;
    float frequency = 1000.0f;
    float dutyCycle = 1.0f;
    float phase = 0.0f;
};

}
