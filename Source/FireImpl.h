// Copyright (c) 2025 Nemisindo Ltd. All rights reserved.

/**
* Actual implementation of the fire model.
* Declare any private members and methods in this file.
*
* The code generator will respect this file and NOT overwrite it if it exists.
*/

#pragma once

#include "Fire.h"
#include "NemisindoLibrary.h"
#include <array>
#include <random>

namespace nemisindo {

class Fire::impl {
public:
    impl();

    void initialize(float sampleRate);
    void reset();
    void tick();
    float outputSample(int channel = 0) const;
    void fillBuffer(float** buffer, const int numSamples);
    static constexpr int getNumOutputChannels() {
        return 2;
    }

    void start();
    void stop();
    int tailDurationSamples() const;
    bool isActive() const;

    void updateParameterInterval(int bufferSize);

    void setLapping(float value);
    float getLapping() const;

    void setHissing(float value);
    float getHissing() const;

    void setCrackling(float value);
    float getCrackling() const;

    void setIntensity(float value);
    float getIntensity() const;

private:
    float _sampleRate = 0.f;
    bool _active = false;
    int _audioBufferSize = 0;

    static constexpr int OUTPUT_CHANNELS = 2;
    float out[OUTPUT_CHANNELS] = { 0.0f };

    float lapping = 0.5;
    nemlib::LinearSmoothedValue lappingSmoother{ lapping };
    const float lappingMin = 0.0;
    const float lappingMax = 1.0;

    float hissing = 0.25;
    nemlib::LinearSmoothedValue hissingSmoother{ hissing };
    const float hissingMin = 0.0;
    const float hissingMax = 1.0;

    float crackling = 0.4;
    nemlib::LinearSmoothedValue cracklingSmoother{ crackling };
    const float cracklingMin = 0.0;
    const float cracklingMax = 1.0;

    float intensity = 1.0;
    nemlib::LinearSmoothedValue intensitySmoother{ intensity };
    const float intensityMin = 0.0;
    const float intensityMax = 1.0;

    // Envelope for smooth start/stop
    nemlib::LinearAHDSR envelope;
    
    // Noise generators
    nemlib::WhiteNoiseSource noiseSource;
    
    // Filters for lapping flames
    nemlib::BiquadFilter<nemlib::BiquadFilterType::BandPass> bpFilter;     // Bandpass filter at 30Hz, Q=5
    nemlib::BiquadFilter<nemlib::BiquadFilterType::BandPass> bpFilter2;    // Bandpass filter at 80Hz, Q=1
    nemlib::BiquadFilter<nemlib::BiquadFilterType::HighPass> hipFilter;    // Highpass filter at 25Hz
    nemlib::BiquadFilter<nemlib::BiquadFilterType::HighPass> hipFilter2;   // Highpass filter at 25Hz
    nemlib::Clipper clipper;           // Clipper at +/- 0.9
    
    // Filters for hissing
    nemlib::OnePoleLPF lpFilter1;   // Lowpass filter for hissing
    nemlib::BiquadFilter<nemlib::BiquadFilterType::HighPass> hipFilter3;   // Highpass filter at 1200Hz
    
    // Filters for crackling
    nemlib::BiquadFilter<nemlib::BiquadFilterType::BandPass> bpFilter3;    // Bandpass filter at 4000Hz, Q=1
    std::array<nemlib::OnePoleLPF, 4> lpFilters2;  // Lowpass filters for crackling
    std::array<nemlib::MovingRMS, 4> rmsDetectors;  // RMS detectors
    
    // Master filters - match JavaScript implementation exactly
    std::array<nemlib::BiquadFilter<nemlib::BiquadFilterType::BandPass>, 3> bpMasterFilters;
    nemlib::BiquadFilter<nemlib::BiquadFilterType::HighPass> hpMasterFilter;
    nemlib::BiquadFilter<nemlib::BiquadFilterType::HighPass> hpOutFilterLeft, hpOutFilterRight;
    nemlib::BiquadFilter<nemlib::BiquadFilterType::LowPass> lpOutFilterLeft, lpOutFilterRight;
    
    // Crackling pulse generation
    struct PulseState {
        int decaySamples = 0;
        int sampleCounter = 0;
        float output = 0.0f;
    };
    std::array<PulseState, 4> pulseStates;
    
    // Random number generation
    nemlib::Random random;

    // Panning values for stereo
    std::array<float, 4> panValues = {-1.0f, 1.0f, 1.0f, -1.0f};

    nemlib::Clipper safetyClipper;

    // Helper methods
    float processPulse(PulseState& state, float rmsValue);
    float pan(float input, float position); // Used for mono->stereo panning
};

}

