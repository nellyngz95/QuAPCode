// Copyright (c) 2025 Nemisindo Ltd. All rights reserved.

/**
* Actual implementation of the gun model.
* Write your DSP code in this file.
*
* The code generator will respect this file and NOT overwrite it if it exists.
*/

#include "GunImpl.h"

using namespace nemisindo;

Gun::impl::impl()
    : shellOsc(nemlib::Waveshape::saw)
{}

void Gun::impl::fillBuffer(float** buffer, const int numSamples) {
    for (int i = 0; i < numSamples; i++) {
        tick();
        for (int j = 0; j < getNumOutputChannels(); j++) {
            buffer[j][i] = outputSample(j);
        }
    }
}

void Gun::impl::initialize(float sampleRate) {
    _sampleRate = sampleRate;
    
    // Initialize oscillators
    shellOsc.initialize(sampleRate);
    shellEnv.initialize(sampleRate);
    shellFreqEnv.initialize(sampleRate);

    overdrive.initialize(sampleRate);

    overdrive.setParameters({
        .drive = 15,
        .volume = 0,
        .bias = 0.22,
        .knee = 0,
        .tone = 1,
    });

    // Initialize clippers
    clipper1.setBounds(-0.3f, 0.3f);
    outputClipper.setBounds(-0.95f, 0.95f);

    // Initialize shell bandpass filters
    for (int i = 0; i < 6; i++) {
        shellBandpass[i].initialize(sampleRate);
    }

    shellBandpass[0].setQ(3.f);
    shellBandpass[1].setQ(3.f);
    shellBandpass[2].setQ(4.f);
    shellBandpass[3].setQ(5.f);
    shellBandpass[4].setQ(4.f);
    shellBandpass[5].setQ(2.f);
    
    // Initialize other filters
    mbp.initialize(sampleRate);
    mbp.setFrequency(500.0f);
    mbp.setQ(0.35f);

    bp11.initialize(sampleRate);
    bp11.setFrequency(300.0f);
    bp11.setQ(0.5f);

    reset();
}

void Gun::impl::reset() {
    assert(_sampleRate > 0.f);
    
    // Reset oscillators
    shellOsc.reset();
    shellEnv.reset();
    shellFreqEnv.reset();
    
    // Reset filters
    for (auto& bp : shellBandpass) {
        bp.reset();
    }
    
    mbp.reset();
    bp11.reset();

    overdrive.reset();

    _active = false;
}

void Gun::impl::tick() {
    assert(_sampleRate > 0.f);

    if (!isActive()) {
        for (float & i : out) i = 0.0f;
        return;
    }
    
    // Update oscillator frequencies with envelope modulation
    float shellOscFreq = shellFreqEnv.nextSample();
    shellOscFreq = std::clamp(shellOscFreq, 1.0f, _sampleRate / 2.0f - 1.0f);

    shellOsc.setFrequency(shellOscFreq);

    
    float shellOscSample = shellOsc.nextSample();
    shellOscSample *= shellOscSample;


    float shellNoise = whiteNoise.nextSample();
    float shellEnv1Value = shellEnv.nextSample();
    if (!shellEnv.isActive()) shellEnv1Value = 0.f;

    float shellClipped = clipper1.nextSample(shellNoise * 10 * shellOscSample);
    
    // Process shell through bandpass filters
    float bpOutputs[6];
    for (int i = 0; i < 6; i++) {
        bpOutputs[i] = shellBandpass[i].nextSample(shellClipped);
    }
    
    float bp11Output = bp11.nextSample(shellClipped);

    // Mix bandpass outputs
    float bp0_2Sum = (bpOutputs[0] + bpOutputs[1] + bpOutputs[2]) * gbp1Gain;
    float bp3Sum = bpOutputs[3] * gbp4Gain;
    float bp4Sum = bpOutputs[4] * gbp5Gain;
    float bp5Sum = bpOutputs[5] * gbp6Gain;
    
    // Final shell mix through mbp
    float mbpInput = bp0_2Sum + bp3Sum + bp4Sum + bp5Sum + bp11Output;
    float shellOutput = mbp.nextSample(mbpInput) * 8 * 0.4f * shellEnv1Value;

    out[0] = outputClipper.nextSample(overdrive.nextSample(shellOutput));
}

float Gun::impl::outputSample(int channel) const {
    return out[channel];
}

void Gun::impl::trigger() {
    assert(_sampleRate > 0.f);
    
    // Update filter frequencies based on shell frequency parameter
    shellBandpass[0].setFrequency(shellFreq * 3600.0f);
    shellBandpass[1].setFrequency(shellFreq * 4600.0f);
    shellBandpass[2].setFrequency(shellFreq * 7200.0f);
    shellBandpass[3].setFrequency(shellFreq * 480.0f);
    shellBandpass[4].setFrequency(shellFreq * 720.0f);
    shellBandpass[5].setFrequency(shellFreq * 80.0f);
    
    // Set up first shell component - immediate trigger
    shellFreqEnv.setParameters({
        .decay = 0.01f,
        .release = 5.0f,

        .holdLvl = 50.f,
        .sustainLvl = 500.0f,
        .endLvl = 0.8f
    });
    
    shellEnv.setParameters({
        .decay = 0.01f,
        .release = shellDecay * 1.66f - 0.01f,
        
        .holdLvl = 0.0001f,
        .sustainLvl = 0.8f,
        .endLvl = 0.0001f
    });

    reset();
    
    // Trigger all envelopes immediately
    shellFreqEnv.attack();
    shellEnv.attack();
    _active = true;
}

int Gun::impl::totalDurationSamples() const {
    assert(_sampleRate > 0.f);
    
    return shellEnv.getTotalDurationSamples();
}

bool Gun::impl::isActive() const {
    return shellEnv.isActive();
}

void Gun::impl::updateParameterInterval(int bufferSize) {
    assert(_sampleRate > 0.f);
    _audioBufferSize = bufferSize;
}

void Gun::impl::setShellDecay(float value) {
    assert(value >= shellDecayMin);
    assert(value <= shellDecayMax);

    this->shellDecay = value;
}

float Gun::impl::getShellDecay() const {
    return this->shellDecay;
}

void Gun::impl::setShellFreq(float value) {
    assert(value >= shellFreqMin);
    assert(value <= shellFreqMax);

    this->shellFreq = value;
}

float Gun::impl::getShellFreq() const {
    return this->shellFreq;
}
