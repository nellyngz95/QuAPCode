// Copyright (c) 2025 Nemisindo Ltd. All rights reserved.

/**
* Actual implementation of the helicopter model.
* Declare any private members and methods in this file.
*
* The code generator will respect this file and NOT overwrite it if it exists.
*/

#pragma once

#include "Helicopter.h"
#include "NemisindoLibrary.h"

namespace nemisindo {

class Helicopter::impl {
public:
    impl();

    void initialize(float sampleRate);
    void reset();
    void tick();
    float outputSample(int channel = 0) const;
    void fillBuffer(float** buffer, const int numSamples);
    static constexpr int getNumOutputChannels() {
        return 1;
    }

    void start();
    void stop();
    int tailDurationSamples() const;
    bool isActive() const;

    void updateParameterInterval(int bufferSize);

    void setRotorPeriod(float value);
    float getRotorPeriod() const;

    void setBaseFreq(float value);
    float getBaseFreq() const;

    void setResonance(float value);
    float getResonance() const;

    void setEngineSpeed(float value);
    float getEngineSpeed() const;

    void setEngineMix(float value);
    float getEngineMix() const;

    void setTailMix(float value);
    float getTailMix() const;

    void setRotorMix(float value);
    float getRotorMix() const;

    void setBladeNoise(float value);
    float getBladeNoise() const;


private:
    float _sampleRate = 0.f;
    bool _active = false; // This will be managed by the envelope
    int _audioBufferSize = 0;

    static constexpr int OUTPUT_CHANNELS = 1;
    float out[OUTPUT_CHANNELS] = { 0.0f };

    float rotorPeriod = 77.0;
    nemlib::LinearSmoothedValue rotorPeriodSmoother{ rotorPeriod };
    const float rotorPeriodMin = 10.0;
    const float rotorPeriodMax = 210.0;

    float baseFreq = 0.25;
    nemlib::LinearSmoothedValue baseFreqSmoother{ baseFreq };
    const float baseFreqMin = 0.0;
    const float baseFreqMax = 1.0;

    float resonance = 0.02;
    nemlib::LinearSmoothedValue resonanceSmoother{ resonance };
    const float resonanceMin = 0.0;
    const float resonanceMax = 1.0;

    float shift = 0.3;
    float symmetry = 0.2;
    float distance = 0.0;

    float engineSpeed = 0.5;
    nemlib::LinearSmoothedValue engineSpeedSmoother{ engineSpeed };
    const float engineSpeedMin = 0.0;
    const float engineSpeedMax = 1.0;

    float engineMix = 0.2;
    nemlib::LinearSmoothedValue engineMixSmoother{ engineMix };
    const float engineMixMin = 0.0;
    const float engineMixMax = 1.0;

    float tailMix = 0.2;
    nemlib::LinearSmoothedValue tailMixSmoother{ tailMix };
    const float tailMixMin = 0.0;
    const float tailMixMax = 1.0;

    float gearMix = 0.2;

    float rotorMix = 0.65;
    nemlib::LinearSmoothedValue rotorMixSmoother{ rotorMix };
    const float rotorMixMin = 0.0;
    const float rotorMixMax = 1.0;

    float bladeNoise = 0.3;
    nemlib::LinearSmoothedValue bladeNoiseSmoother{ bladeNoise };
    const float bladeNoiseMin = 0.0;
    const float bladeNoiseMax = 1.0;

    // Engine components
    nemlib::Phasor enginePhasor;
    nemlib::WhiteNoiseSource engineNoise;
    nemlib::JSOnePoleProcessor engineNoiseFilter;
    nemlib::Clipper engineClipper, safetyClipper;
    
    // Engine state variables
    float engineOutput = 0.0f;
    float engineNode = 1.0f; // Constant source in JS
    
    // Main rotor components
    nemlib::Phasor rotorPhasor;
    nemlib::WhiteNoiseSource rotorNoise;
    nemlib::BiquadFilter<nemlib::BiquadFilterType::BandPass> rotorNoiseFilter;
    
    // Propeller processing components
    std::array<nemlib::BiquadFilter<nemlib::BiquadFilterType::LowPass>, 4> propellerLPFilters;
    std::array<nemlib::Delay, 3> propellerDelays;
    
    // Main rotor state variables
    float rotorOutput = 0.0f;
    float asymCurveOut = 0.0f;
    float cosOut = 0.0f;
    
    // Tail rotor components
    nemlib::OscillatorSource tailOsc;
    nemlib::WhiteNoiseSource tailNoise;
    nemlib::BiquadFilter<nemlib::BiquadFilterType::BandPass> tailNoiseFilter;
    nemlib::BiquadFilter<nemlib::BiquadFilterType::LowPass> tailLowPassFilter;
    nemlib::BiquadFilter<nemlib::BiquadFilterType::HighPass> tailHighPassFilter;
    
    // Tail state variables
    float tailOutput = 0.0f;
    
    // Envelope for smooth start/stop
    nemlib::LinearAHDSR envelope;
    
    // Distance processing components
    std::array<nemlib::BiquadFilter<nemlib::BiquadFilterType::LowPass>, 3> distanceLPFilters;
};

}
