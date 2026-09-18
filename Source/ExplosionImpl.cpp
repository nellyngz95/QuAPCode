// Copyright (c) 2025 Nemisindo Ltd. All rights reserved.

/**
* Actual implementation of the explosion model.
* Write your DSP code in this file.
*
* The code generator will respect this file and NOT overwrite it if it exists.
*/

#include "ExplosionImpl.h"

using namespace nemisindo;

Explosion::impl::impl() {
}

void Explosion::impl::fillBuffer(float** buffer, const int numSamples) {
    for (int i = 0; i < numSamples; i++) {
        tick();
        for (int j = 0; j < getNumOutputChannels(); j++) {
            buffer[j][i] = outputSample(j);
        }
    }
}

void Explosion::impl::initialize(float sampleRate) {
    _sampleRate = sampleRate;

    overdrive.initialize(sampleRate);

    lowEnv.initialize(sampleRate);
    whiteEnv.initialize(sampleRate);
    pinkEnv.initialize(sampleRate);
    
    lowPassFilter.initialize(sampleRate);
    lowPassFilter.setFrequency(100.0f);
    lowPassFilter.setQ(1.0f);
    lowPassFilter.setPeakGainDb(0.0f);
    
    finalLowPass.initialize(sampleRate);
    finalLowPass.setFrequency(10000.0f);
    finalLowPass.setQ(1.0f);
    finalLowPass.setPeakGainDb(0.0f);
    
    for (int i = 0; i < 5; i++) {
        bandpass[i].initialize(sampleRate);
        bandpass[i].setFrequency(bpFreqs[i]);
        bandpass[i].setQ(bpQs[i]);
        bandpass[i].setPeakGainDb(0.0f);
    }

    overdrive.setParameters({
        .drive = 21,
        .volume = -8,
        .bias = 1.0f,
        .knee = 0.0f
    });
    
    reset();
}

void Explosion::impl::reset() {
    assert(_sampleRate > 0.f);

    // Reset envelopes
    lowEnv.reset();
    whiteEnv.reset();
    pinkEnv.reset();
    
    // Reset filters
    lowPassFilter.reset();
    finalLowPass.reset();
    
    for (auto& bp : bandpass) {
        bp.reset();
    }
}

void Explosion::impl::tick() {
    assert(_sampleRate > 0.f);

    if (!isActive()) {
        for (float & i : out) i = 0.0f;
        return;
    }

    // --- Rumble ---
    float pinkLowSample = lowPassFilter.nextSample(pinkLow.nextSample()) * lowEnv.nextSample() * _rumble;

    std::array<float, 5> bpSample{};
    for (int i = 0; i < 5; i++) {
        bpSample[i] = bandpass[i].nextSample(pinkLowSample);
    }
    
    float bp0_2Sum = (bpSample[0] + bpSample[1] + bpSample[2]) * gainOut[0];
    float bp3Sum = bpSample[3] * gainOut[1];
    float bp4Sum = bpSample[4] * gainOut[2];
    
    float rumbleOut = gainPreWS * (bp0_2Sum + bp3Sum + bp4Sum);

    if (grit) {
        rumbleOut = distortion.nextSample(rumbleOut);
    }

    // --- Dust & Air ---
    float dustOut = pinkHigh.nextSample() * pinkEnv.nextSample() * _dust;
    float airOut = whiteHigh.nextSample() * whiteEnv.nextSample() * _air;

    float outputSample = finalLowPass.nextSample(dustOut + airOut + rumbleOut);
    
    if (overTheTop) {
        outputSample = overdrive.nextSample(outputSample);
    }
    
    out[0] = clipper.nextSample(outputSample);
}

float Explosion::impl::outputSample(int channel) const {
    return out[channel];
}

void Explosion::impl::trigger() {
    assert(_sampleRate > 0.f);

    distortion.setAmount(gritAmount);

    _dust = dust;
    _air = air;
    _rumble = rumble;

    lowEnv.setParameters({
        .decay = 0.001f,
        .release = rumbleDecay,

        .holdLvl = 1,
        .sustainLvl = 3000,
        .endLvl = 0.13619970f,
    });

    whiteEnv.setParameters({
        .attack = timeSeparation / 2000.0f,
        .decay = 0.001f,
        .release = airDecay,

        .startLvl = 0.0001f,
        .holdLvl = 0.0001f,
        .sustainLvl = 0.32,
        .endLvl = 0.000014528f
    });
    
    pinkEnv.setParameters({
        .attack = timeSeparation / 2000.0f,
        .decay = 0.001f,
        .release = dustDecay,
        
        .startLvl = 0.0001f,
        .holdLvl = 0.0001f,
        .sustainLvl = 0.64f,
        .endLvl = 0.000029056f
    });
    
    lowEnv.attack();
    whiteEnv.attack();
    pinkEnv.attack();
    
    distortion.setAmount(gritAmount);
}

int Explosion::impl::totalDurationSamples() const {
    assert(_sampleRate > 0.f);

    int highDur = std::max(whiteEnv.getTotalDurationSamples(), pinkEnv.getTotalDurationSamples());
    return std::max(highDur, lowEnv.getTotalDurationSamples());
}

bool Explosion::impl::isActive() const {
    return lowEnv.isActive() || whiteEnv.isActive() || pinkEnv.isActive();
}

void Explosion::impl::updateParameterInterval(int bufferSize) {
    assert(_sampleRate > 0.f);
    _audioBufferSize = bufferSize;
}

void Explosion::impl::setRumble(float value) {
    assert(value >= rumbleMin);
    assert(value <= rumbleMax);

    this->rumble = value;
}

float Explosion::impl::getRumble() const {
    return this->rumble;
}

void Explosion::impl::setRumbleDecay(float value) {
    assert(value >= rumbleDecayMin);
    assert(value <= rumbleDecayMax);

    this->rumbleDecay = value;
}

float Explosion::impl::getRumbleDecay() const {
    return this->rumbleDecay;
}

void Explosion::impl::setAir(float value) {
    assert(value >= airMin);
    assert(value <= airMax);

    this->air = value;
}

float Explosion::impl::getAir() const {
    return this->air;
}

void Explosion::impl::setAirDecay(float value) {
    assert(value >= airDecayMin);
    assert(value <= airDecayMax);

    this->airDecay = value;
}

float Explosion::impl::getAirDecay() const {
    return this->airDecay;
}

void Explosion::impl::setDust(float value) {
    assert(value >= dustMin);
    assert(value <= dustMax);

    this->dust = value;
}

float Explosion::impl::getDust() const {
    return this->dust;
}

void Explosion::impl::setDustDecay(float value) {
    assert(value >= dustDecayMin);
    assert(value <= dustDecayMax);

    this->dustDecay = value;
}

float Explosion::impl::getDustDecay() const {
    return this->dustDecay;
}

void Explosion::impl::setTimeSeparation(float value) {
    assert(value >= timeSeparationMin);
    assert(value <= timeSeparationMax);

    this->timeSeparation = value;
}

float Explosion::impl::getTimeSeparation() const {
    return this->timeSeparation;
}

void Explosion::impl::setGrit(bool value) {
    this->grit = value;
}

bool Explosion::impl::getGrit() const {
    return this->grit;
}

void Explosion::impl::setGritAmount(float value) {
    assert(value >= gritAmountMin);
    assert(value <= gritAmountMax);

    this->gritAmount = value;
}

float Explosion::impl::getGritAmount() const {
    return this->gritAmount;
}

void Explosion::impl::setOverTheTop(bool value) {
    this->overTheTop = value;
}

bool Explosion::impl::getOverTheTop() const {
    return this->overTheTop;
}
