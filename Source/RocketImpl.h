// Copyright (c) 2025 Nemisindo Ltd. All rights reserved.

/**
* Actual implementation of the rocket model.
* Declare any private members and methods in this file.
*
* The code generator will respect this file and NOT overwrite it if it exists.
*/

#pragma once

#include "Rocket.h"
#include "NemisindoLibrary.h"

namespace nemisindo {

class Rocket::impl {
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

    void trigger();
    int totalDurationSamples() const;
    bool isActive() const;

    void updateParameterInterval(int bufferSize);

    void setDuration(float value);
    float getDuration() const;

    void setChamberResonance(float value);
    float getChamberResonance() const;

    void setFlutter(float value);
    float getFlutter() const;


private:
    float _sampleRate = 44.1f;
    bool _active = false;
    int _audioBufferSize = 0;

    static constexpr int OUTPUT_CHANNELS = 1;
    float out[OUTPUT_CHANNELS] = { 0.0f };

    float duration = 5.0;
    const float durationMin = 1.0;
    const float durationMax = 10.0;

    float chamberResonance = 19.0;
    nemlib::LinearSmoothedValue chamberResonanceSmoother{ chamberResonance };
    const float chamberResonanceMin = 16.0;
    const float chamberResonanceMax = 20.0;

    float flutter = 30.0;
    nemlib::LinearSmoothedValue flutterSmoother{ flutter };
    const float flutterMin = 2.0;
    const float flutterMax = 40.0;

    // Add any further members required by the implementation here

    nemlib::WhiteNoiseSource whiteNoise;

    std::array<nemlib::BiquadFilter<nemlib::BiquadFilterType::BandPass>, 8> chamberBPs;
    std::array<nemlib::TwoPoleBPF, 4> staticBPs;

    nemlib::BiquadFilter<nemlib::BiquadFilterType::BandPass> clickBP, resBP1, resBP2, exitBP;
    nemlib::BiquadFilter<nemlib::BiquadFilterType::LowPass> exLP1, exLP2;

    nemlib::Clipper resBPClipper, combClipper, safetyClipper;

    nemlib::LinearAHDSR clickEnv, resBPEnv, chamberFreqEnv, exitEnv, rocketEnv, exConstant;

    std::array<nemlib::Delay, 3> delays;
    nemlib::Delay exConstantDelay;
};

}
