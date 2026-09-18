// Copyright (c) 2025 Nemisindo Ltd. All rights reserved.

/**
* Actual implementation of the explosion model.
* Declare any private members and methods in this file.
*
* The code generator will respect this file and NOT overwrite it if it exists.
*/

#pragma once

#include "Explosion.h"
#include "NemisindoLibrary.h"

namespace nemisindo {

class Explosion::impl {
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

    void setRumble(float value);
    float getRumble() const;

    void setRumbleDecay(float value);
    float getRumbleDecay() const;

    void setAir(float value);
    float getAir() const;

    void setAirDecay(float value);
    float getAirDecay() const;

    void setDust(float value);
    float getDust() const;

    void setDustDecay(float value);
    float getDustDecay() const;

    void setTimeSeparation(float value);
    float getTimeSeparation() const;

    void setGrit(bool value);
    bool getGrit() const;

    void setGritAmount(float value);
    float getGritAmount() const;

    void setOverTheTop(bool value);
    bool getOverTheTop() const;


private:
    float _sampleRate = 0.f;
    bool _active = false;
    int _audioBufferSize = 0;

    static constexpr int OUTPUT_CHANNELS = 1;
    float out[OUTPUT_CHANNELS] = { 0.0f };

    float rumble = 0.6;
    const float rumbleMin = 0.0;
    const float rumbleMax = 1.0;

    float rumbleDecay = 4.0;
    const float rumbleDecayMin = 1.0;
    const float rumbleDecayMax = 10.0;

    float air = 0.8;
    const float airMin = 0.0;
    const float airMax = 1.0;

    float airDecay = 2.0;
    const float airDecayMin = 1.0;
    const float airDecayMax = 10.0;

    float dust = 0.8;
    const float dustMin = 0.0;
    const float dustMax = 1.0;

    float dustDecay = 2.0;
    const float dustDecayMin = 1.0;
    const float dustDecayMax = 10.0;

    float timeSeparation = 0.0;
    const float timeSeparationMin = 0.0;
    const float timeSeparationMax = 100.0;

    bool grit = true;

    float gritAmount = 25.0;
    const float gritAmountMin = 0.0;
    const float gritAmountMax = 100.0;

    bool overTheTop = true;

    // Add any further members required by the implementation here

    float _rumble, _dust, _air;

    // Noise generators
    nemlib::PinkNoiseSource pinkLow;   // For rumble (low freq)
    nemlib::PinkNoiseSource pinkHigh;  // For dust (high freq)
    nemlib::WhiteNoiseSource whiteHigh; // For air (mid freq)
    
    // Envelopes
    nemlib::ExponentialAHDSR lowEnv;  // For rumble
    nemlib::ExponentialAHDSR whiteEnv; // For air
    nemlib::ExponentialAHDSR pinkEnv;  // For dust
    
    // Filters
    nemlib::BiquadFilter<nemlib::BiquadFilterType::LowPass> lowPassFilter, finalLowPass;
    std::array<nemlib::BiquadFilter<nemlib::BiquadFilterType::BandPass>, 5> bandpass;
    
    // Distortion processors
    nemlib::Distortion distortion;
    nemlib::Overdrive overdrive;
    
    // Constants for bandpass filters
    std::array<float, 5> bpFreqs = {4700.0f, 2300.0f, 360.0f, 235.0f, 360.0f};
    std::array<float, 5> bpQs = {7.0f, 7.0f, 8.0f, 7.0f, 5.0f};
    std::array<float, 3> gainOut = {2.0f, 1.2f, 2.5f};
    float gainPreWS = 0.01f;

    // safety clipper
    nemlib::Clipper clipper;
};

}
