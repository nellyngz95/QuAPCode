// Copyright (c) 2025 Nemisindo Ltd. All rights reserved.

/**
* Glue code for the Fire model
*
* NB! This file is auto-generated. Do not modify this file.
* The code generator will overwrite this file without warning.
* Any changes you make will be lost.
*
* Implement the actual DSP code in the FireImpl files.
*/

#include "Fire.h"
#include "FireImpl.h"

using namespace nemisindo;

Fire::Fire() : pimpl(std::make_unique<impl>()) {}

Fire::~Fire() = default;

void Fire::initialize(float sampleRate) {
    pimpl->initialize(sampleRate);
}

void Fire::tick() {
    pimpl->tick();
}

void Fire::reset() {
    pimpl->reset();
}

float Fire::outputSample(int channel) const {
    return pimpl->outputSample(channel);
}

constexpr int Fire::getNumOutputChannels() {
    return impl::getNumOutputChannels();
}

void Fire::fillBuffer(float** buffer, int numSamples) {
    pimpl->fillBuffer(buffer, numSamples);
}

void Fire::start() {
    pimpl->start();
}

void Fire::stop() {
    pimpl->stop();
}

int Fire::tailDurationSamples() const {
    return pimpl->tailDurationSamples();
}

bool Fire::isActive() const {
    return pimpl->isActive();
}

void Fire::updateParameterInterval(int bufferSize) {
    pimpl->updateParameterInterval(bufferSize);
}

void Fire::setLapping(float value) {
    pimpl->setLapping(value);
}

float Fire::getLapping() const {
    return pimpl->getLapping();
}

void Fire::setHissing(float value) {
    pimpl->setHissing(value);
}

float Fire::getHissing() const {
    return pimpl->getHissing();
}

void Fire::setCrackling(float value) {
    pimpl->setCrackling(value);
}

float Fire::getCrackling() const {
    return pimpl->getCrackling();
}

void Fire::setIntensity(float value) {
    pimpl->setIntensity(value);
}

float Fire::getIntensity() const {
    return pimpl->getIntensity();
}

