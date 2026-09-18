// Copyright (c) 2025 Nemisindo Ltd. All rights reserved.

/**
* Glue code for the Explosion model
*
* NB! This file is auto-generated. Do not modify this file.
* The code generator will overwrite this file without warning.
* Any changes you make will be lost.
*
* Implement the actual DSP code in the ExplosionImpl files.
*/

#include "Explosion.h"
#include "ExplosionImpl.h"

using namespace nemisindo;

Explosion::Explosion() : pimpl(std::make_unique<impl>()) {}

Explosion::~Explosion() = default;

void Explosion::initialize(float sampleRate) {
    pimpl->initialize(sampleRate);
}

void Explosion::tick() {
    pimpl->tick();
}

void Explosion::reset() {
    pimpl->reset();
}

float Explosion::outputSample(int channel) const {
    return pimpl->outputSample(channel);
}

constexpr int Explosion::getNumOutputChannels() {
    return impl::getNumOutputChannels();
}

void Explosion::fillBuffer(float** buffer, int numSamples) {
    pimpl->fillBuffer(buffer, numSamples);
}

void Explosion::trigger() {
    pimpl->trigger();
}

int Explosion::totalDurationSamples() const {
    return pimpl->totalDurationSamples();
}

bool Explosion::isActive() const {
    return pimpl->isActive();
}

void Explosion::updateParameterInterval(int bufferSize) {
    pimpl->updateParameterInterval(bufferSize);
}

void Explosion::setRumble(float value) {
    pimpl->setRumble(value);
}

float Explosion::getRumble() const {
    return pimpl->getRumble();
}

void Explosion::setRumbleDecay(float value) {
    pimpl->setRumbleDecay(value);
}

float Explosion::getRumbleDecay() const {
    return pimpl->getRumbleDecay();
}

void Explosion::setAir(float value) {
    pimpl->setAir(value);
}

float Explosion::getAir() const {
    return pimpl->getAir();
}

void Explosion::setAirDecay(float value) {
    pimpl->setAirDecay(value);
}

float Explosion::getAirDecay() const {
    return pimpl->getAirDecay();
}

void Explosion::setDust(float value) {
    pimpl->setDust(value);
}

float Explosion::getDust() const {
    return pimpl->getDust();
}

void Explosion::setDustDecay(float value) {
    pimpl->setDustDecay(value);
}

float Explosion::getDustDecay() const {
    return pimpl->getDustDecay();
}

void Explosion::setTimeSeparation(float value) {
    pimpl->setTimeSeparation(value);
}

float Explosion::getTimeSeparation() const {
    return pimpl->getTimeSeparation();
}

void Explosion::setGrit(bool value) {
    pimpl->setGrit(value);
}

bool Explosion::getGrit() const {
    return pimpl->getGrit();
}

void Explosion::setGritAmount(float value) {
    pimpl->setGritAmount(value);
}

float Explosion::getGritAmount() const {
    return pimpl->getGritAmount();
}

void Explosion::setOverTheTop(bool value) {
    pimpl->setOverTheTop(value);
}

bool Explosion::getOverTheTop() const {
    return pimpl->getOverTheTop();
}

