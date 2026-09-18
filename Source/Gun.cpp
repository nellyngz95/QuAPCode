// Copyright (c) 2025 Nemisindo Ltd. All rights reserved.

/**
* Glue code for the Gun model
*
* NB! This file is auto-generated. Do not modify this file.
* The code generator will overwrite this file without warning.
* Any changes you make will be lost.
*
* Implement the actual DSP code in the GunImpl files.
*/

#include "Gun.h"
#include "GunImpl.h"

using namespace nemisindo;

Gun::Gun() : pimpl(std::make_unique<impl>()) {}

Gun::~Gun() = default;

void Gun::initialize(float sampleRate) {
    pimpl->initialize(sampleRate);
}

void Gun::tick() {
    pimpl->tick();
}

void Gun::reset() {
    pimpl->reset();
}

float Gun::outputSample(int channel) const {
    return pimpl->outputSample(channel);
}

constexpr int Gun::getNumOutputChannels() {
    return impl::getNumOutputChannels();
}

void Gun::fillBuffer(float** buffer, int numSamples) {
    pimpl->fillBuffer(buffer, numSamples);
}

void Gun::trigger() {
    pimpl->trigger();
}

int Gun::totalDurationSamples() const {
    return pimpl->totalDurationSamples();
}

bool Gun::isActive() const {
    return pimpl->isActive();
}

void Gun::updateParameterInterval(int bufferSize) {
    pimpl->updateParameterInterval(bufferSize);
}

void Gun::setShellDecay(float value) {
    pimpl->setShellDecay(value);
}

float Gun::getShellDecay() const {
    return pimpl->getShellDecay();
}

void Gun::setShellFreq(float value) {
    pimpl->setShellFreq(value);
}

float Gun::getShellFreq() const {
    return pimpl->getShellFreq();
}


