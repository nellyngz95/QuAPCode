// Copyright (c) 2025 Nemisindo Ltd. All rights reserved.

/**
* Actual implementation of the jet model.
* Write your DSP code in this file.
*
* The code generator will respect this file and NOT overwrite it if it exists.
*/

#include "JetImpl.h"

using namespace nemisindo;

Jet::impl::impl() {

}

void Jet::impl::fillBuffer(float** buffer, const int numSamples) {
  for (int i = 0; i < numSamples; i++) {
        tick();
        for (int j = 0; j < getNumOutputChannels(); j++) {
            buffer[j][i] = outputSample(j);
        }
    }
}

void Jet::impl::initialize(float sampleRate) {
    _sampleRate = sampleRate;
    speedSmoother.initialize(sampleRate);
    turbineSmoother.initialize(sampleRate);
    burnSmoother.initialize(sampleRate);

    envelope.initialize(sampleRate);
    envelope.setParameters({
        .autoRelease = false,

        .attack = 0.01f,
        .release = 0.01f
    });

    jetOsc.initialize(sampleRate);
    jetClipper.setBounds(-0.9f, 0.9f);

    bLPF.initialize(sampleRate);
    bLPF.setFrequency(12000);
    bLPF.setQ(0.5f);

    bVCF1.initialize(sampleRate);
    bVCF1.setFrequency(1.0f);
    bVCF1.setQ(1.0f);

    bHPF.initialize(sampleRate);
    bHPF.setFrequency(120.0f);
    bHPF.setQ(1.0f);

    bVCF2.initialize(sampleRate);
    bVCF2.setFrequency(1.0f);
    bVCF2.setQ(1.0f);

    mixLPF.initialize(sampleRate);
    mixLPF.setFrequency(11000.0f);
    mixLPF.setQ(1.0f);

    jetOsc.setGain(0.25f, 0);
    jetOsc.setGain(0.25f, 1);
    jetOsc.setGain(1.f, 2);
    jetOsc.setGain(0.4f, 3);
    jetOsc.setGain(0.4f, 4);

    reset();
}

void Jet::impl::reset() {
    assert(_sampleRate > 0.f);

    speedSmoother.setRampLengthSamples(0);
    turbineSmoother.setRampLengthSamples(0);
    burnSmoother.setRampLengthSamples(0);

    bLPF.reset();
    bVCF1.reset();
    bHPF.reset();
    bVCF2.reset();
    mixLPF.reset();

    jetOsc.reset();

    envelope.reset();
}

void Jet::impl::tick() {
    assert(_sampleRate > 0.f);

    if (!isActive()) {
        for (float & i : out) i = 0.0f;
        return;
    }

    speed = speedSmoother.nextSample();
    turbine = turbineSmoother.nextSample();
    burn = burnSmoother.nextSample();

    // most of your DSP code will go here

    for (int i = 0; i < 5; i++) {
        jetOsc.setFrequency(speed * speedMult[i], i);
    }

    float jetOut = 0.7f * turbine * jetClipper.nextSample(jetOsc.nextSample());

    bVCF1.setFrequency(speed * speed * 150.0f);
    bVCF2.setFrequency(speed * 12000.0f);

    float burnClipperIn = 80.0f * bHPF.nextSample(bVCF1.nextSample(bLPF.nextSample(burnNoise.nextSample())));
    float burnOut = burn * bVCF2.nextSample(burnClipper.nextSample(burnClipperIn));

    float combinedOut = burnClipper.nextSample(mixLPF.nextSample(jetOut + burnOut));

    out[0] = combinedOut * envelope.nextSample() * 0.5f;
}

float Jet::impl::outputSample(int channel) const {
    return out[channel];
}

void Jet::impl::start() {
    assert(_sampleRate > 0.f);

    // startup logic
    envelope.attack();
}

void Jet::impl::stop() {
    assert(_sampleRate > 0.f);

    // shutdown logic...
    // if the model has a tail, you might want to set active = false later

    envelope.release();
}

int Jet::impl::tailDurationSamples() const {
    assert(_sampleRate > 0.f);

    return envelope.getReleaseDurationSamples();
}

bool Jet::impl::isActive() const {
    return envelope.isActive();
}

void Jet::impl::updateParameterInterval(int bufferSize) {
    assert(_sampleRate > 0.f);
    _audioBufferSize = bufferSize;

    speedSmoother.setRampLengthSamples(bufferSize);
    turbineSmoother.setRampLengthSamples(bufferSize);
    burnSmoother.setRampLengthSamples(bufferSize);
}

void Jet::impl::setSpeed(float value) {
    assert(value >= speedMin);
    assert(value <= speedMax);

    speedSmoother.setTargetValue(value);
}

float Jet::impl::getSpeed() const {
    return this->speed;
}

void Jet::impl::setTurbine(float value) {
    assert(value >= turbineMin);
    assert(value <= turbineMax);

    turbineSmoother.setTargetValue(value);
}

float Jet::impl::getTurbine() const {
    return this->turbine;
}

void Jet::impl::setBurn(float value) {
    assert(value >= burnMin);
    assert(value <= burnMax);

    burnSmoother.setTargetValue(value);
}

float Jet::impl::getBurn() const {
    return this->burn;
}

