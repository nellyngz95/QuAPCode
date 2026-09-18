// Copyright (c) 2025 Nemisindo Ltd. All rights reserved.

/**
* Glue code for the Helicopter model
*
* NB! This file is auto-generated. Do not modify this file.
* The code generator will overwrite this file without warning.
* Any changes you make will be lost.
*
* Implement the actual DSP code in the HelicopterImpl files.
*/

#include "Helicopter.h"
#include "HelicopterImpl.h"

using namespace nemisindo;

Helicopter::Helicopter() : pimpl(std::make_unique<impl>()) {}

Helicopter::~Helicopter() = default;

void Helicopter::initialize(float sampleRate) {
    pimpl->initialize(sampleRate);
}

void Helicopter::tick() {
    pimpl->tick();
}

void Helicopter::reset() {
    pimpl->reset();
}

float Helicopter::outputSample(int channel) const {
    return pimpl->outputSample(channel);
}

constexpr int Helicopter::getNumOutputChannels() {
    return impl::getNumOutputChannels();
}

void Helicopter::fillBuffer(float** buffer, int numSamples) {
    pimpl->fillBuffer(buffer, numSamples);
}

void Helicopter::start() {
    pimpl->start();
}

void Helicopter::stop() {
    pimpl->stop();
}

int Helicopter::tailDurationSamples() const {
    return pimpl->tailDurationSamples();
}

bool Helicopter::isActive() const {
    return pimpl->isActive();
}

void Helicopter::updateParameterInterval(int bufferSize) {
    pimpl->updateParameterInterval(bufferSize);
}

void Helicopter::setRotorPeriod(float value) {
    pimpl->setRotorPeriod(value);
}

float Helicopter::getRotorPeriod() const {
    return pimpl->getRotorPeriod();
}

void Helicopter::setBaseFreq(float value) {
    pimpl->setBaseFreq(value);
}

float Helicopter::getBaseFreq() const {
    return pimpl->getBaseFreq();
}

void Helicopter::setResonance(float value) {
    pimpl->setResonance(value);
}

float Helicopter::getResonance() const {
    return pimpl->getResonance();
}

void Helicopter::setEngineSpeed(float value) {
    pimpl->setEngineSpeed(value);
}

float Helicopter::getEngineSpeed() const {
    return pimpl->getEngineSpeed();
}

void Helicopter::setEngineMix(float value) {
    pimpl->setEngineMix(value);
}

float Helicopter::getEngineMix() const {
    return pimpl->getEngineMix();
}

void Helicopter::setTailMix(float value) {
    pimpl->setTailMix(value);
}

float Helicopter::getTailMix() const {
    return pimpl->getTailMix();
}

void Helicopter::setRotorMix(float value) {
    pimpl->setRotorMix(value);
}

float Helicopter::getRotorMix() const {
    return pimpl->getRotorMix();
}

void Helicopter::setBladeNoise(float value) {
    pimpl->setBladeNoise(value);
}

float Helicopter::getBladeNoise() const {
    return pimpl->getBladeNoise();
}

