// Copyright (c) 2025 Nemisindo Ltd. All rights reserved.

/**
* Actual implementation of the rocket model.
* Write your DSP code in this file.
*
* The code generator will respect this file and NOT overwrite it if it exists.
*/

#include "RocketImpl.h"

using namespace nemisindo;

Rocket::impl::impl() {

}

void Rocket::impl::fillBuffer(float** buffer, const int numSamples) {
  for (int i = 0; i < numSamples; i++) {
        tick();
        for (int j = 0; j < getNumOutputChannels(); j++) {
            buffer[j][i] = outputSample(j);
        }
    }
}

void Rocket::impl::initialize(float sampleRate) {
    _sampleRate = sampleRate;
    chamberResonanceSmoother.initialize(sampleRate);
    flutterSmoother.initialize(sampleRate);

    clickEnv.initialize(sampleRate);
    resBPEnv.initialize(sampleRate);
    chamberFreqEnv.initialize(sampleRate);
    exitEnv.initialize(sampleRate);
    rocketEnv.initialize(sampleRate);
    exConstant.initialize(sampleRate);

    clickEnv.setParameters({
        .release = 0.02f,
    });

    resBPEnv.setParameters({
        .attack = 0.08f,
        .decay = 0.1,
        .sustain = 0.12f,
        .release = 0.32f,

        .holdLvl = 0.f,
        .sustainLvl = 1.f,
        .endLvl = 0.f
    });

    chamberFreqEnv.setParameters({
        .attack = 0.15,
        .decay = 0.09f,
        .sustain = 0.02f,
        .release = 0.5f,

        .holdLvl = 0.f,
    });

    exitEnv.setParameters({
        .attack = 0.15,
        .decay = 0.05f,
        .sustain = 0.08f,
        .release = 0.86f,

        .holdLvl = 0.f
    });

    exConstant.setParameters({
        .attack = 0.15,
        .decay = 8.f,

        .holdLvl = 0.f,
        .sustainLvl = 0.8f,
        .endLvl = 0.8f,
    });

    for (int i = 0; i < 4; i++) {
        chamberBPs[i].initialize(sampleRate);
        chamberBPs[i + 4].initialize(sampleRate);

        chamberBPs[i].setFrequency(227.0f * (2 * i + 1));
        chamberBPs[i + 4].setFrequency(227.0f * (2 * i + 1));

        chamberBPs[i].setQ(20.1f - chamberResonance);
        chamberBPs[i + 4].setQ(20.1f - chamberResonance);

        staticBPs[i].initialize(sampleRate);
        staticBPs[i].setFrequency(320.0f * i + 160.0f);
        staticBPs[i].setQ(4.0f);
    }

    clickBP.initialize(sampleRate);
    clickBP.setFrequency(5000.0f);
    clickBP.setQ(6.0f);

    resBP1.initialize(sampleRate);
    resBP1.setFrequency(100.0f);
    resBP1.setQ(1.0f);

    resBP2.initialize(sampleRate);
    resBP2.setFrequency(100.0f);
    resBP2.setQ(1.0f);

    exitBP.initialize(sampleRate);
    exitBP.setFrequency(1000.f);
    exitBP.setQ(0.2f);

    exLP1.initialize(sampleRate);
    exLP1.setFrequency(9.0f);
    exLP1.setQ(1.0f);

    exLP2.initialize(sampleRate);
    exLP2.setFrequency(9.0f);
    exLP2.setQ(1.0f);

    for (int i = 0; i < 3; i++) {
        delays[i].initialize(sampleRate);
        delays[i].setParameters({
            .delayTime = 0.2f,
            .feedbackGain = 0.0f,
        });
    }

    exConstantDelay.initialize(sampleRate);
    exConstantDelay.setParameters({
        .delayTime = 150.0f / 1000.0f,
        .feedbackGain = 0.0f
    });

    reset();
}

void Rocket::impl::reset() {
    assert(_sampleRate > 0.f);

    chamberResonanceSmoother.setRampLengthSamples(0);
    flutterSmoother.setRampLengthSamples(0);

    clickEnv.reset();
    resBPEnv.reset();
    chamberFreqEnv.reset();
    exitEnv.reset();
    rocketEnv.reset();

    exConstant.reset();

    for (int i = 0; i < 8; i++) {
        chamberBPs[i].reset();
    }

    for (int i = 0; i < 4; i++) {
        staticBPs[i].reset();
    }

    clickBP.reset();
    resBP1.reset();
    resBP2.reset();
    exitBP.reset();
    exLP1.reset();
    exLP2.reset();

    for (int i = 0; i < 3; i++) {
        delays[i].reset();
    }

    exConstantDelay.reset();

    _active = false;
}

void Rocket::impl::tick() {
    assert(_sampleRate > 0.f);

    if (!isActive()) {
        for (float & i : out) i = 0.0f;
        return;
    }

    chamberResonance = chamberResonanceSmoother.nextSample();
    flutter = flutterSmoother.nextSample();

    // most of your DSP code will go here

    for (auto& filter : chamberBPs) {
        filter.setQ(20.1f - chamberResonance);
    }

    float noiseOut = whiteNoise.nextSample();

    float clickEnvOut = clickEnv.nextSample();
    float clickOut = 4 * clickEnvOut * clickEnvOut * clickBP.nextSample(noiseOut);

    float resBPClipperIn = 12.0f * std::powf(resBPEnv.nextSample(), 8.0f) * resBP2.nextSample(resBP1.nextSample(noiseOut));
    float resBPClipperOut = resBPClipper.nextSample(resBPClipperIn);

    float exitNoiseSample = std::powf(exitEnv.nextSample(), 8.0f) * exitBP.nextSample(noiseOut);
    float chamberEnvPos = chamberFreqEnv.nextSample();
    float chamberBPOut = 0.0f;
    for (int i = 0; i < 4; i++) {
        chamberBPs[i].setFrequency((227.0f + 300.0f * chamberEnvPos) * (2.0f * i + 1.0f));
        chamberBPs[i + 4].setFrequency((227.0f + 300.0f * chamberEnvPos) * (2.0f * i + 1.0f));
        chamberBPOut += chamberBPs[i + 4].nextSample(chamberBPs[i].nextSample(resBPClipperOut + exitNoiseSample));
    }
    float resOut = chamberBPOut + 2.0f * resBPClipperOut + exitNoiseSample;

    float rocketEnvOut = rocketEnv.nextSample();
    float cg = 4.0f * (1.0f - rocketEnvOut);
    float combClipperIn = 0.0f;
    for (int i = 0; i < 3; i++) {
        delays[i].setParameters({
            .delayTime = 0.001f * (10.0f + (1.0f + i * 0.2f) * cg),
            .feedbackGain = 0.f,
        });
        combClipperIn += 3.0f * delays[i].nextSample(noiseOut);
    }

    float combClipperOut = combClipper.nextSample(combClipperIn);
    float pipeOut = 0.0f;
    for (int i = 0; i < 4; i++) {
        pipeOut += staticBPs[i].nextSample(combClipperOut);
    }

    float exLPOut = flutter * exLP2.nextSample(exLP1.nextSample(noiseOut));
    float powerQ = std::sqrt(exConstantDelay.nextSample(exConstant.nextSample()));
    float exGainOut = powerQ * (exLPOut - 1.0f) + 1.0f;

    pipeOut = std::sqrt(rocketEnvOut) * pipeOut * exGainOut / 2.0f;

    out[0] = safetyClipper.nextSample((clickOut + resOut + pipeOut) * 0.5f);
}

float Rocket::impl::outputSample(int channel) const {
    return out[channel];
}

void Rocket::impl::trigger() {
    assert(_sampleRate > 0.f);

    // startup logic
    rocketEnv.setParameters({
        .attack = 0.15f,
        .decay = 0.09f,
        .sustain = 0.09f,
        .release = duration,

        .holdLvl = 0.f,
        .sustainLvl = 1.f,
        .endLvl = 0.f
    });

    reset();

    clickEnv.attack();
    resBPEnv.attack();
    chamberFreqEnv.attack();
    exitEnv.attack();
    rocketEnv.attack();
    exConstant.attack();
}

int Rocket::impl::totalDurationSamples() const {
    assert(_sampleRate > 0.f);

    return rocketEnv.getTotalDurationSamples();
}

bool Rocket::impl::isActive() const {
    return rocketEnv.isActive();
}

void Rocket::impl::updateParameterInterval(int bufferSize) {
    assert(_sampleRate > 0.f);
    _audioBufferSize = bufferSize;

    chamberResonanceSmoother.setRampLengthSamples(bufferSize);
    flutterSmoother.setRampLengthSamples(bufferSize);
}

void Rocket::impl::setDuration(float value) {
    assert(value >= durationMin);
    assert(value <= durationMax);

    this->duration = value;}

float Rocket::impl::getDuration() const {
    return this->duration;
}

void Rocket::impl::setChamberResonance(float value) {
    assert(value >= chamberResonanceMin);
    assert(value <= chamberResonanceMax);

    chamberResonanceSmoother.setTargetValue(value);
}

float Rocket::impl::getChamberResonance() const {
    return this->chamberResonance;
}

void Rocket::impl::setFlutter(float value) {
    assert(value >= flutterMin);
    assert(value <= flutterMax);

    flutterSmoother.setTargetValue(value);
}

float Rocket::impl::getFlutter() const {
    return this->flutter;
}

