// Copyright (c) 2025 Nemisindo Ltd. All rights reserved.

/**
* Glue code for the Jet model
*
* NB! This file is auto-generated. Do not modify this file.
* The code generator will overwrite this file without warning.
* Any changes you make will be lost.
*
* Implement the actual DSP code in the JetImpl files.
*/

#include "Jet.h"
#include "JetImpl.h"

using namespace nemisindo;

Jet::Jet() : pimpl(std::make_unique<impl>()) {}

Jet::~Jet() = default;

void Jet::initialize(float sampleRate) {
    pimpl->initialize(sampleRate);
}

void Jet::tick() {
    pimpl->tick();
}

void Jet::reset() {
    pimpl->reset();
}

float Jet::outputSample(int channel) const {
    return pimpl->outputSample(channel);
}

constexpr int Jet::getNumOutputChannels() {
    return impl::getNumOutputChannels();
}

void Jet::fillBuffer(float** buffer, int numSamples) {
    pimpl->fillBuffer(buffer, numSamples);
}

void Jet::start() {
    pimpl->start();
}

void Jet::stop() {
    pimpl->stop();
}

int Jet::tailDurationSamples() const {
    return pimpl->tailDurationSamples();
}

bool Jet::isActive() const {
    return pimpl->isActive();
}

void Jet::updateParameterInterval(int bufferSize) {
    pimpl->updateParameterInterval(bufferSize);
}

void Jet::setSpeed(float value) {
    pimpl->setSpeed(value);
}

float Jet::getSpeed() const {
    return pimpl->getSpeed();
}

void Jet::setTurbine(float value) {
    pimpl->setTurbine(value);
}

float Jet::getTurbine() const {
    return pimpl->getTurbine();
}

void Jet::setBurn(float value) {
    pimpl->setBurn(value);
}

float Jet::getBurn() const {
    return pimpl->getBurn();
}

