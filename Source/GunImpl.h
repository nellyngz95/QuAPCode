// Copyright (c) 2025 Nemisindo Ltd. All rights reserved.

/**
* Actual implementation of the gun model.
* Declare any private members and methods in this file.
*
* The code generator will respect this file and NOT overwrite it if it exists.
*/

#pragma once

#include "Gun.h"
#include "NemisindoLibrary.h"

namespace nemisindo {

class Gun::impl {
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

    void setShellGain(float value);
    float getShellGain() const;

    void setShellDecay(float value);
    float getShellDecay() const;

    void setShellFreq(float value);
    float getShellFreq() const;

private:
    float _sampleRate = 0.f;
    bool _active = false;
    int _audioBufferSize = 0;

    static constexpr int OUTPUT_CHANNELS = 1;
    float out[OUTPUT_CHANNELS] = { 0.0f };

    float shellDecay = 0.5;
    const float shellDecayMin = 0.01;
    const float shellDecayMax = 1.0;

    float shellFreq = 0.6;
    const float shellFreqMin = 0.05;
    const float shellFreqMax = 1.0;

    // Add any further members required by the implementation here

    // Noise generators
    nemlib::WhiteNoiseSource whiteNoise;
    
    nemlib::OscillatorSource shellOsc;
    nemlib::ExponentialAHDSR shellEnv;
    
    // Envelopes for frequency modulation
    nemlib::ExponentialAHDSR shellFreqEnv;

    // Filters
    std::array<nemlib::BiquadFilter<nemlib::BiquadFilterType::BandPass>, 6> shellBandpass;
    nemlib::BiquadFilter<nemlib::BiquadFilterType::BandPass> mbp;
    nemlib::BiquadFilter<nemlib::BiquadFilterType::LowPass> bp11;
    
    nemlib::Clipper clipper1;
    
    // Mixer gains
    float gbp1Gain = 2.0f;
    float gbp4Gain = 1.2f;
    float gbp5Gain = 2.5f;
    float gbp6Gain = 10.0f;

    nemlib::Overdrive overdrive;

    // Safety clipper
    nemlib::Clipper outputClipper;
};

}
