// Copyright (c) 2025 Nemisindo Ltd. All rights reserved.

/**
* Actual implementation of the fire model.
* Write your DSP code in this file.
*
* The code generator will respect this file and NOT overwrite it if it exists.
*/

#include "FireImpl.h"
#include <cmath>
#include <algorithm>

using namespace nemisindo;

Fire::impl::impl() = default;

void Fire::impl::fillBuffer(float** buffer, const int numSamples) {
    for (int i = 0; i < numSamples; i++) {
        tick();
        for (int j = 0; j < getNumOutputChannels(); j++) {
            buffer[j][i] = outputSample(j);
        }
    }
}

void Fire::impl::initialize(float sampleRate) {
    _sampleRate = sampleRate;
    lappingSmoother.initialize(sampleRate);
    hissingSmoother.initialize(sampleRate);
    cracklingSmoother.initialize(sampleRate);
    intensitySmoother.initialize(sampleRate);
    
    // Initialize envelope
    envelope.initialize(sampleRate);
    envelope.setParameters({
        .autoRelease = false,
        .attack = 0.005f,
        .release = 0.005f,

        .sustainLvl = 0.8f
    });
    
    // Initialize lapping filters
    bpFilter.initialize(sampleRate);
    bpFilter.setFrequency(30.0f);
    bpFilter.setQ(5.0f);
    
    bpFilter2.initialize(sampleRate);
    bpFilter2.setFrequency(80.0f);
    bpFilter2.setQ(1.0f);
    
    hipFilter.initialize(sampleRate);
    hipFilter.setFrequency(25.0f);

    hipFilter2.initialize(sampleRate);
    hipFilter2.setFrequency(25.0f);
    
    clipper.setBounds(-0.9f, 0.9f);
    
    // Initialize hissing filters
    lpFilter1.initialize(sampleRate);
    lpFilter1.setFrequency(1.0f);
    
    hipFilter3.initialize(sampleRate);
    hipFilter3.setFrequency(1200.0f);
    
    // Initialize crackling filters
    bpFilter3.initialize(sampleRate);
    bpFilter3.setFrequency(4000.0f);
    bpFilter3.setQ(1.0f);
    
    for (auto& lpf : lpFilters2) {
        lpf.initialize(sampleRate);
        lpf.setFrequency(1.0f);
    }
    
    for (auto& rms : rmsDetectors) {
        rms.setWindowLength(256);
    }
    
    // Initialize master filters - match JavaScript implementation exactly
    bpMasterFilters[0].initialize(sampleRate);
    bpMasterFilters[0].setFrequency(800.0f);
    bpMasterFilters[0].setQ(0.2f);
    
    bpMasterFilters[1].initialize(sampleRate);
    bpMasterFilters[1].setFrequency(1400.0f);
    bpMasterFilters[1].setQ(0.6f);
    
    bpMasterFilters[2].initialize(sampleRate);
    bpMasterFilters[2].setFrequency(3000.0f);
    bpMasterFilters[2].setQ(0.4f);
    
    hpMasterFilter.initialize(sampleRate);
    hpMasterFilter.setFrequency(1000.0f);
    
    // Initialize output filters
    hpOutFilterLeft.initialize(sampleRate);
    hpOutFilterLeft.setFrequency(1.0f);
    hpOutFilterRight.initialize(sampleRate);
    hpOutFilterRight.setFrequency(1.0f);

    lpOutFilterLeft.initialize(sampleRate);
    lpOutFilterLeft.setFrequency(15000.0f);
    lpOutFilterRight.initialize(sampleRate);
    lpOutFilterRight.setFrequency(15000.0f);

    reset();
}

void Fire::impl::reset() {
    assert(_sampleRate > 0.f);

    lappingSmoother.setRampLengthSamples(0);
    hissingSmoother.setRampLengthSamples(0);
    cracklingSmoother.setRampLengthSamples(0);
    intensitySmoother.setRampLengthSamples(0);
    
    envelope.reset();
    
    bpFilter.reset();
    bpFilter2.reset();
    hipFilter.reset();
    hipFilter2.reset();

    lpFilter1.reset();
    hipFilter3.reset();
    
    bpFilter3.reset();
    for (auto& lpf : lpFilters2) {
        lpf.reset();
    }
    
    for (auto& filter : bpMasterFilters) {
        filter.reset();
    }
    
    hpMasterFilter.reset();
    
    hpOutFilterLeft.reset();
    lpOutFilterLeft.reset();

    hpOutFilterRight.reset();
    lpOutFilterRight.reset();
    
    for (auto& pulse : pulseStates) {
        pulse.decaySamples = 0;
        pulse.sampleCounter = 0;
        pulse.output = 0.0f;
    }

    _active = false;
}

float Fire::impl::pan(float input, float position) {
    // Simple linear panning - corrected implementation
    // For left channel (output 0), we want full volume when position is -1, zero when position is 1
    // For right channel (output 1), we want full volume when position is 1, zero when position is -1
    return input * (1.0f - std::abs(position));
}

float Fire::impl::processPulse(PulseState& state, float rmsValue) {
    // Check if we should trigger a new pulse
    if (rmsValue > 0.49f && rmsValue < 0.52f) {
        // Random decay time between 0-30ms
        state.decaySamples = static_cast<int>(random.nextFloat() * 30.0f * _sampleRate / 1000.0f);
        state.sampleCounter = 0;
        
        // Update bandpass filter frequency for crackling
        bpFilter3.setFrequency(1500.0f + (500.0f * state.decaySamples * 1000.0f / _sampleRate));
    }
    
    // Process the pulse
    if (state.decaySamples > 0) {
        if (state.sampleCounter <= state.decaySamples) {
            float progress = static_cast<float>(state.sampleCounter) / static_cast<float>(state.decaySamples);
            state.output = (1.0f - progress) * (1.0f - progress);
            state.sampleCounter++;
        } else {
            state.output = 0.0f;
            state.decaySamples = 0;
        }
    } else {
        state.output = 0.0f;
    }
    
    return state.output;
}

void Fire::impl::tick() {
    assert(_sampleRate > 0.f);

    if (!isActive()) {
        for (float & i : out) i = 0.0f;
        return;
    }

    // Get parameter values with smoothing
    lapping = lappingSmoother.nextSample();
    hissing = hissingSmoother.nextSample();
    crackling = cracklingSmoother.nextSample();
    intensity = intensitySmoother.nextSample();
    
    // Update output filters based on intensity
    hpOutFilterLeft.setFrequency(-600.0f * intensity + 601.0f);
    hpOutFilterRight.setFrequency(-600.0f * intensity + 601.0f);

    lpOutFilterLeft.setFrequency(9000.0f * intensity + 6000.0f);
    lpOutFilterRight.setFrequency(9000.0f * intensity + 6000.0f);

    // Generate white noise
    float noiseSample = noiseSource.nextSample();
    
    // Process lapping flames
    float lappingSample = bpFilter.nextSample(noiseSample);
    lappingSample = bpFilter2.nextSample(lappingSample);
    lappingSample *= 100.0f; // Gain boost
    lappingSample = hipFilter.nextSample(lappingSample);
    lappingSample = clipper.nextSample(lappingSample);
    lappingSample = hipFilter2.nextSample(lappingSample);
    lappingSample *= lapping; // Apply lapping parameter

    // Process hissing - correctly implement the chain from JavaScript
    float lpSample = lpFilter1.nextSample(noiseSample);
    float hipSample = hipFilter3.nextSample(noiseSample);

    float hissSample = 10 * lpSample;
    hissSample = hissSample * hissSample;
    hissSample = hissSample * hissSample;

    hissSample = 600.f * hissSample * hipSample * hissing;

    // Process crackling for each of the 4 channels
    std::array<float, 4> crackleSamples{};
    for (size_t i = 0; i < 4; ++i) {
        float lpSample2 = lpFilters2[i].nextSample(noiseSample);
        float rmsValue = rmsDetectors[i].nextSample(lpSample2) * 100.0f; // RMS with gain
        float pulseOutput = processPulse(pulseStates[i], rmsValue);
        
        float bpNoiseSample = bpFilter3.nextSample(noiseSample);
        crackleSamples[i] = bpNoiseSample * pulseOutput * crackling; // Apply crackling parameter
    }
    
    // Mix all components and apply master filtering - match JavaScript implementation exactly
    std::array<float, 4> masterSamples{};
    
    // Mix all components for each channel
    for (size_t i = 0; i < 4; ++i) {
        masterSamples[i] = lappingSample + hissSample + crackleSamples[i];
    }
    
    // Apply the appropriate filter to each channel
    masterSamples[0] = bpMasterFilters[0].nextSample(masterSamples[0]); // BP 800Hz to pan left
    masterSamples[1] = bpMasterFilters[1].nextSample(masterSamples[1]); // BP 1400Hz to pan right
    masterSamples[2] = bpMasterFilters[2].nextSample(masterSamples[2]); // BP 3000Hz to pan right
    masterSamples[3] = hpMasterFilter.nextSample(masterSamples[3]);     // HP 1000Hz to pan left
    
    // Apply stereo panning and final output processing
    float leftChannel = 0.0f;
    float rightChannel = 0.0f;

    leftChannel = 0.5f * (masterSamples[0] + masterSamples[3]);
    rightChannel = 0.5f * (masterSamples[1] + masterSamples[2]);

    leftChannel = hpOutFilterLeft.nextSample(leftChannel);
    leftChannel = lpOutFilterLeft.nextSample(leftChannel);

    rightChannel = hpOutFilterRight.nextSample(rightChannel);
    rightChannel = lpOutFilterRight.nextSample(rightChannel);

    float envSample = envelope.nextSample();

    out[0] = safetyClipper.nextSample(leftChannel * envSample);
    out[1] = safetyClipper.nextSample(rightChannel * envSample);
}

float Fire::impl::outputSample(int channel) const {
    return out[channel];
}

void Fire::impl::start() {
    assert(_sampleRate > 0.f);
    envelope.attack();
    _active = true;
}

void Fire::impl::stop() {
    assert(_sampleRate > 0.f);
    envelope.release();
}

int Fire::impl::tailDurationSamples() const {
    assert(_sampleRate > 0.f);
    return envelope.getReleaseDurationSamples();
}

bool Fire::impl::isActive() const {
    return envelope.isActive();
}

void Fire::impl::updateParameterInterval(int bufferSize) {
    assert(_sampleRate > 0.f);
    _audioBufferSize = bufferSize;

    lappingSmoother.setRampLengthSamples(bufferSize);
    hissingSmoother.setRampLengthSamples(bufferSize);
    cracklingSmoother.setRampLengthSamples(bufferSize);
    intensitySmoother.setRampLengthSamples(bufferSize);
}

void Fire::impl::setLapping(float value) {
    assert(value >= lappingMin);
    assert(value <= lappingMax);

    lappingSmoother.setTargetValue(value);
}

float Fire::impl::getLapping() const {
    return this->lapping;
}

void Fire::impl::setHissing(float value) {
    assert(value >= hissingMin);
    assert(value <= hissingMax);

    hissingSmoother.setTargetValue(value);
}

float Fire::impl::getHissing() const {
    return this->hissing;
}

void Fire::impl::setCrackling(float value) {
    assert(value >= cracklingMin);
    assert(value <= cracklingMax);

    cracklingSmoother.setTargetValue(value);
}

float Fire::impl::getCrackling() const {
    return this->crackling;
}

void Fire::impl::setIntensity(float value) {
    assert(value >= intensityMin);
    assert(value <= intensityMax);

    intensitySmoother.setTargetValue(value);
}

float Fire::impl::getIntensity() const {
    return this->intensity;
}
