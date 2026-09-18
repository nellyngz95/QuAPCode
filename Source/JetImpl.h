// Copyright (c) 2025 Nemisindo Ltd. All rights reserved.

/**
* Actual implementation of the jet model.
* Declare any private members and methods in this file.
*
* The code generator will respect this file and NOT overwrite it if it exists.
*/

#pragma once

#include "Jet.h"
#include "NemisindoLibrary.h"

namespace nemisindo {

class Jet::impl {
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

    void setSpeed(float value);
    float getSpeed() const;

    void setTurbine(float value);
    float getTurbine() const;

    void setBurn(float value);
    float getBurn() const;


private:
    float _sampleRate = 0.f;
    int _audioBufferSize = 0;

    static constexpr int OUTPUT_CHANNELS = 1;
    float out[OUTPUT_CHANNELS] = { 0.0f };

    float speed = 0.5;
    nemlib::LinearSmoothedValue speedSmoother{ speed };
    const float speedMin = 0.0;
    const float speedMax = 1.0;

    float turbine = 1.0;
    nemlib::LinearSmoothedValue turbineSmoother{ turbine };
    const float turbineMin = 0.0;
    const float turbineMax = 1.0;

    float burn = 1.0;
    nemlib::LinearSmoothedValue burnSmoother{ burn };
    const float burnMin = 0.0;
    const float burnMax = 1.0;

    // Add any further members required by the implementation here
    nemlib::OscillatorSource jetOsc{nemlib::Waveshape::sine, 5};
    nemlib::WhiteNoiseSource burnNoise;

    nemlib::Clipper jetClipper, burnClipper;

    nemlib::BiquadFilter<nemlib::BiquadFilterType::LowPass> bLPF;
    nemlib::BiquadFilter<nemlib::BiquadFilterType::BandPass> bVCF1;
    nemlib::BiquadFilter<nemlib::BiquadFilterType::HighPass> bHPF;
    nemlib::BiquadFilter<nemlib::BiquadFilterType::LowPass> bVCF2;
    nemlib::BiquadFilter<nemlib::BiquadFilterType::LowPass> mixLPF;

    std::array<float, 5> speedMult{ 3097.0f, 4495.0f, 5588.0f, 7471.0f, 11000.0f };

    // Envelope for smooth start/stop
    nemlib::LinearAHDSR envelope;
};

}
