// Copyright (c) 2025 Nemisindo Ltd. All rights reserved.

/**
* Glue code for the Rocket model
*
* NB! This file is auto-generated. Do not modify this file.
* The code generator will overwrite this file without warning.
* Any changes you make will be lost.
*
* Implement the actual DSP code in the RocketImpl files.
*/

#include "Rocket.h"
#include "RocketImpl.h"

using namespace nemisindo;

Rocket::Rocket() : pimpl(std::make_unique<impl>()) {}

Rocket::~Rocket() = default;

void Rocket::initialize(float sampleRate) {
    pimpl->initialize(sampleRate);
}

void Rocket::tick() {
    pimpl->tick();
}

void Rocket::reset() {
    pimpl->reset();
}

float Rocket::outputSample(int channel) const {
    return pimpl->outputSample(channel);
}

constexpr int Rocket::getNumOutputChannels() {
    return impl::getNumOutputChannels();
}

void Rocket::fillBuffer(float** buffer, int numSamples) {
    pimpl->fillBuffer(buffer, numSamples);
}

void Rocket::trigger() {
    pimpl->trigger();
}

int Rocket::totalDurationSamples() const {
    return pimpl->totalDurationSamples();
}

bool Rocket::isActive() const {
    return pimpl->isActive();
}

void Rocket::updateParameterInterval(int bufferSize) {
    pimpl->updateParameterInterval(bufferSize);
}

void Rocket::setDuration(float value) {
    pimpl->setDuration(value);
}

float Rocket::getDuration() const {
    return pimpl->getDuration();
}

void Rocket::setChamberResonance(float value) {
    pimpl->setChamberResonance(value);
}

float Rocket::getChamberResonance() const {
    return pimpl->getChamberResonance();
}

void Rocket::setFlutter(float value) {
    pimpl->setFlutter(value);
}

float Rocket::getFlutter() const {
    return pimpl->getFlutter();
}

