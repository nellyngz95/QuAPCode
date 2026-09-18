// Copyright (c) 2025 Nemisindo Ltd. All rights reserved.

/**
* Actual implementation of the helicopter model.
* Write your DSP code in this file.
*
* The code generator will respect this file and NOT overwrite it if it exists.
*/

#include "HelicopterImpl.h"

using namespace nemisindo;

Helicopter::impl::impl() {

}

void Helicopter::impl::fillBuffer(float** buffer, const int numSamples) {
  for (int i = 0; i < numSamples; i++) {
        tick();
        for (int j = 0; j < getNumOutputChannels(); j++) {
            buffer[j][i] = outputSample(j);
        }
    }
}

void Helicopter::impl::initialize(float sampleRate) {
    _sampleRate = sampleRate;
    rotorPeriodSmoother.initialize(sampleRate);
    baseFreqSmoother.initialize(sampleRate);
    resonanceSmoother.initialize(sampleRate);
    engineSpeedSmoother.initialize(sampleRate);
    engineMixSmoother.initialize(sampleRate);
    tailMixSmoother.initialize(sampleRate);
    rotorMixSmoother.initialize(sampleRate);
    bladeNoiseSmoother.initialize(sampleRate);
    
    // Initialize envelope
    envelope.initialize(sampleRate);
    envelope.setParameters({
        .autoRelease = false,
        .attack = 0.01f,
        .release = 0.01f,
        .sustainLvl = 1.0f  // Full volume at sustain
    });

    // Initialize engine components
    enginePhasor.initialize(sampleRate);
    enginePhasor.setDutyCycle(1.0f);
    engineNoiseFilter.initialize(sampleRate);
    engineNoiseFilter.setFrequency(500.0f);
    engineNoiseFilter.setQ(5.0f);
    engineClipper.setBounds(-0.5f, 0.5f);
    
    // Initialize main rotor components
    rotorPhasor.initialize(sampleRate);
    rotorPhasor.setDutyCycle(bladeNoise); // Set initial duty cycle based on blade noise
    rotorNoiseFilter.initialize(sampleRate);
    
    // Initialize propeller filters
    for (auto& filter : propellerLPFilters) {
        filter.initialize(sampleRate);
        filter.setFrequency(400.0f);
        filter.setQ(1.f);
    }
    propellerLPFilters[3].setFrequency(3000.0f);
    propellerLPFilters[3].setQ(1.f);

    // Initialize propeller delays
    for (auto& delay : propellerDelays) {
        delay.initialize(sampleRate, 0.5f);
        nemlib::Delay::Parameters delayParams;
        delayParams.delayTime = 0.025f;
        delayParams.feedbackGain = 0.0f;
        delayParams.dryGain = 0.0f;
        delayParams.wetGain = 0.7f;
        delay.setParameters(delayParams);
    }
    
    // Initialize tail rotor components
    tailOsc.initialize(sampleRate);
    tailNoiseFilter.initialize(sampleRate);
    tailLowPassFilter.initialize(sampleRate);
    tailHighPassFilter.initialize(sampleRate);
    
    // Set default filter values
    tailLowPassFilter.setFrequency(2000.0f);
    tailHighPassFilter.setFrequency(300.0f);

    tailLowPassFilter.setQ(1.f);
    tailHighPassFilter.setQ(1.f);

    tailNoiseFilter.setQ(6.0f);
    // Initialize distance processing components
    for (auto& filter : distanceLPFilters) {
        filter.initialize(sampleRate);
        filter.setFrequency((distance * -1.0f + 1.0f) * 7000.0f + 50.0f);
    }
    
    reset();
}

void Helicopter::impl::reset() {
    assert(_sampleRate > 0.f);

    rotorPeriodSmoother.setRampLengthSamples(0);
    baseFreqSmoother.setRampLengthSamples(0);
    resonanceSmoother.setRampLengthSamples(0);
    engineSpeedSmoother.setRampLengthSamples(0);
    engineMixSmoother.setRampLengthSamples(0);
    tailMixSmoother.setRampLengthSamples(0);
    rotorMixSmoother.setRampLengthSamples(0);
    bladeNoiseSmoother.setRampLengthSamples(0);
    
    envelope.reset();

    // Reset engine components
    enginePhasor.reset();
    engineNoiseFilter.reset();
    engineOutput = 0.0f;
    engineNode = 1.0f; // Reset the constant source value
    
    // Reset main rotor components
    rotorPhasor.reset();
    rotorNoiseFilter.reset();
    rotorOutput = 0.0f;
    asymCurveOut = 0.0f;
    cosOut = 0.0f;
    
    // Reset propeller filters
    for (auto& filter : propellerLPFilters) {
        filter.reset();
    }
    
    // Reset propeller delays
    for (auto& delay : propellerDelays) {
        delay.reset();
    }
    
    // Reset tail rotor components
    tailOsc.reset();
    tailNoiseFilter.reset();
    tailLowPassFilter.reset();
    tailHighPassFilter.reset();
    tailOutput = 0.0f;
    for (auto& filter : distanceLPFilters) {
        filter.reset();
    }
}

void Helicopter::impl::tick() {
    assert(_sampleRate > 0.f);

    if (!envelope.isActive()) {
        for (float & i : out) i = 0.0f;
        return;
    }

    rotorPeriod = rotorPeriodSmoother.nextSample();
    baseFreq = baseFreqSmoother.nextSample();
    resonance = resonanceSmoother.nextSample();
    engineSpeed = engineSpeedSmoother.nextSample();
    engineMix = engineMixSmoother.nextSample();
    tailMix = tailMixSmoother.nextSample();
    rotorMix = rotorMixSmoother.nextSample();
    bladeNoise = bladeNoiseSmoother.nextSample();

    // Process engine sound
    float engineFrequency = engineSpeed * 90.0f + 1.0f + 40.f;
    enginePhasor.setFrequency(engineFrequency);

    // Get phasor output (0 to 1 ramp)
    float phasorOut = enginePhasor.nextSample();
    
    // Square the phasor output (phasorPow in JS)
    // var phasorPow = new power(EnginePhasor,2,this.context);
    float phasorPow = phasorOut * phasorOut;
    
    // Create the engine waveform (exactly as in JS)
    // var newEngine = new multiplySignals(phasorPow,EnginePhasor, this.context);
    float newEngine = phasorPow * phasorOut;
    
    // var newEngine2 = new addSignals(new multiplySigVal(newEngine,-14,this.context),new multiplySigVal(phasorPow,14,this.context),this.context);
    float newEngine2 = (newEngine * -14.0f) + (phasorPow * 14.0f);
    
    // var newEngine3 = new addSigVal(new power(new multiplySigVal(newEngine2,2,this.context),2,this.context),1,this.context);
    float newEngine2Squared = newEngine2 * 2.0f;
    float newEngine3 = (newEngine2Squared * newEngine2Squared) + 1.0f;
    
    // Process noise for the engine
    float noiseOut = engineNoise.nextSample();
    
    // Filter the noise (using the exact JS filter implementation)
    engineNoiseFilter.setFrequency(500.0f);
    engineNoiseFilter.setQ(5.0f);
    float filteredNoise = engineNoiseFilter.nextSample(noiseOut);
    
    // Amplify and clip the noise (exactly as in JS)
    // var engineNoise2 = new multiplySigVal(engineBPFilter1,12,this.context)
    float engineNoise2 = filteredNoise * 12.0f;
    
    // this.Clipper = new AudioWorkletNode(this.context, 'clip-processor',{parameterData:{LowThreshold: -0.5,HighThreshold:0.5}});
    float clippedNoise = engineClipper.nextSample(engineNoise2) * 0.05f;
    
    // Combine the engine components (exactly as in JS)
    // var newEngine4 = new divideSignals(engineNode,newEngine3,this.context);
    float newEngine4 = engineNode / newEngine3;
    
    // engine5 = new multiplySignals(newEngine4,new multiplySigVal(this.Clipper,0.05,this.context),this.context);
    float engine5 = newEngine4 * clippedNoise;
    
    // engine6 = new subtractSigVal(new addSignals(newEngine4,engine5,this.context),0.5,this.context);
    float engine6 = (newEngine4 + engine5) - 0.5f;
    
    // engine7 = new multiplySigVal(engine6,0.2,this.context);
    engineOutput = engine6 * 0.2f * engineMix;
    
    // Process tail rotor sound
    // Set the phasor frequency based on engine speed (similar to JS code)
    float tailFrequency = engineSpeed * 2.0f;
    tailOsc.setFrequency(std::max(engineFrequency * tailFrequency,1.f));

    // Get phasor output
    float tailOscOut = tailOsc.nextSample();
    
    // Process the oscillator output (similar to oscOut in JS)
    float oscPower = tailOscOut * tailOscOut;
    float oscOut = (1.0f / (oscPower + 1.0f)) * 0.04f;
    
    // Process noise for the tail rotor
    float tailNoiseOut = tailNoise.nextSample();
    
    // Filter the noise (similar to TailFilter in JS)
    tailNoiseFilter.setFrequency(std::max(350 + 100.0f + tailOscOut * 500.0f, 1.f));
    float filteredTailNoise = tailNoiseFilter.nextSample(tailNoiseOut);
    
    // Process through low pass filter
    float tailLowPassOut = tailLowPassFilter.nextSample(filteredTailNoise);
    
    // Combine oscillator and noise
    float tailCombined = + 0.04f*(1.0f+ (1.5f*oscOut) * (1.5f*oscOut)) + tailLowPassOut * 0.1f;
    
    // Process through high pass filter
    tailOutput = tailHighPassFilter.nextSample(tailCombined) * tailMix * 5.0f;
    
    // Process main rotor sound
    // Set the phasor frequency based on rotor period (similar to JS code)
    float rotorFrequency = 1000.0f / rotorPeriod;
    rotorPhasor.setFrequency(rotorFrequency);
    rotorPhasor.setDutyCycle(bladeNoise); // Set duty cycle based on blade noise parameter
    
    // Get phasor output
    float rotorOut = rotorPhasor.nextSample();
    
    // Process asymmetric curve (similar to JS code)
    // var leftNode1 = new subtractSignals(constantNode[0], constantNode[1], this.context);
    // var leftNode2 = new multiplySignals(leftNode1, Phasor, this.context);
    float symValue = symmetry * 0.6f + 0.2f;
    float leftNode1 = 1.0f - symValue;
    float leftNode2 = leftNode1 * rotorOut;
    
    // var symMid1 = new multiplySigVal(new subtractSigVal(constantNode[1], 0.5, this.context),3,this.context);
    // var symMid2 = new addSigVal(new multiplySigVal(new multiplySignals(symMid1,symMid1,this.context),3,this.context), 3,this.context);
    float symMid1 = (symValue - 0.5f) * 3.0f;
    float symMid2 = (symMid1 * symMid1) * 3.0f + 3.0f;
    
    // var bottomMainNode = new multiplySignals(leftNode2, symMid2, this.context);
    // var asymCurveOut = new subtractSigVal(new multiplySigVal(bottomMainNode, 0.25, this.context), 0.25, this.context);
    float bottomMainNode = leftNode2 * symMid2;
    asymCurveOut = bottomMainNode * 0.25f - 0.25f;
    
    // Apply cosine shaper
    cosOut = std::cosf(2.f * asymCurveOut * M_PI);

    // Process noise for the rotor
    float rotorNoiseOut = rotorNoise.nextSample();
    
    // Calculate frequency modulation
    // var devNode = new multiplySignals(cos, constantNode[2], this.context);
    // var baseNode = new addSignals(devNode, constantNode[3], this.context);
    float devNode = cosOut * (baseFreq * 600.0f);
    float baseNode = devNode + (baseFreq * 400.0f);
    
    // Filter the noise based on parameters
    rotorNoiseFilter.setFrequency(baseNode);
    rotorNoiseFilter.setQ(std::max(resonance * 30.0f, 0.1f));
    float filteredRotorNoise = rotorNoiseFilter.nextSample(rotorNoiseOut);
    
    // Apply resonance
    float resonanceValue = std::sqrt(resonance * 30.0f) * 0.6f + 0.1f;
    filteredRotorNoise *= resonanceValue;
    
    // Combine with the cosine output (similar to mainTopNode in JS)
    float mainTopNode = cosOut * filteredRotorNoise;
    
    // Process through propeller filter chain
    // First delay section
    float propFilter0Out = propellerLPFilters[0].nextSample(mainTopNode);
    
    // Update delay time based on shift parameter
    nemlib::Delay::Parameters delayParams1;
    delayParams1.delayTime = (cosOut * (shift * 270.0f) + 25.0f) * 0.001f;
    delayParams1.feedbackGain = 0.0f;
    delayParams1.dryGain = 0.0f;
    delayParams1.wetGain = 0.7f;
    propellerDelays[0].setParameters(delayParams1);
    
    float delay1Out = propellerDelays[0].nextSample(propFilter0Out);
    float mainProp1Out = delay1Out + mainTopNode;
    
    // Second delay section
    float mainMidNode = mainProp1Out * cosOut;
    float propFilter1Out = propellerLPFilters[1].nextSample(mainMidNode);
    
    nemlib::Delay::Parameters delayParams2;
    delayParams2.delayTime = (cosOut * (shift * 270.0f * 1.22f) + 25.0f) * 0.001f;
    delayParams2.feedbackGain = 0.0f;
    delayParams2.dryGain = 0.0f;
    delayParams2.wetGain = 0.7f;
    propellerDelays[1].setParameters(delayParams2);
    
    float delay2Out = propellerDelays[1].nextSample(propFilter1Out);
    float mainProp2Out = delay2Out + mainMidNode;
    
    // Third delay section
    float propFilter2Out = propellerLPFilters[2].nextSample(mainProp2Out);
    
    nemlib::Delay::Parameters delayParams3;
    delayParams3.delayTime = (cosOut * (shift * 270.0f * 1.33f) + 25.0f) * 0.001f;
    delayParams3.feedbackGain = 0.0f;
    delayParams3.dryGain = 0.0f;
    delayParams3.wetGain = 0.5f;
    propellerDelays[2].setParameters(delayParams3);
    
    float delay3Out = propellerDelays[2].nextSample(propFilter2Out);
    float mainProp3Out = delay3Out + mainProp2Out;
    
    // Combine all propeller outputs
    rotorOutput = (mainProp1Out + mainProp2Out + mainProp3Out) * rotorMix;
    
    // Combine all sound components
    float combinedOutput = engineOutput + rotorOutput + tailOutput;
    
    // Process distance effects
    // Update filter frequencies based on distance
    float distanceFilterFreq = (distance * -1.0f + 1.0f) * 7000.0f + 50.0f;
    for (auto& filter : distanceLPFilters) {
        filter.setFrequency(distanceFilterFreq);
    }
    
    // Apply distance processing
    float distanceProcessed = combinedOutput;
    for (auto& filter : distanceLPFilters) {
        distanceProcessed = filter.nextSample(distanceProcessed);
    }

    // Apply envelope to the output
    float envSample = envelope.nextSample();
    out[0] = safetyClipper.nextSample(distanceProcessed * envSample);
}

float Helicopter::impl::outputSample(int channel) const {
    return out[channel];
}

void Helicopter::impl::start() {
    assert(_sampleRate > 0.f);

    // Ramp up engine speed from 0 to target value (similar to JS code)
    engineSpeedSmoother.setTargetValue(0.0f);
    engineSpeedSmoother.setRampLengthSeconds(0.0f);
    engineSpeedSmoother.setTargetValue(engineSpeed);
    engineSpeedSmoother.setRampLengthSeconds(1.5f);
    
    // Trigger the envelope attack
    envelope.attack();
}

void Helicopter::impl::stop() {
    assert(_sampleRate > 0.f);

    // Ramp down engine speed to 0 (similar to JS code)
    engineSpeedSmoother.setTargetValue(0.0f);
    engineSpeedSmoother.setRampLengthSeconds(1.5f);
    
    // Trigger the envelope release
    envelope.release();
}

int Helicopter::impl::tailDurationSamples() const {
    assert(_sampleRate > 0.f);

    // Use the envelope's release duration
    return envelope.getReleaseDurationSamples();
}

bool Helicopter::impl::isActive() const {
    return envelope.isActive();
}

void Helicopter::impl::updateParameterInterval(int bufferSize) {
    assert(_sampleRate > 0.f);
    _audioBufferSize = bufferSize;

    rotorPeriodSmoother.setRampLengthSamples(bufferSize);
    baseFreqSmoother.setRampLengthSamples(bufferSize);
    resonanceSmoother.setRampLengthSamples(bufferSize);
    engineSpeedSmoother.setRampLengthSamples(bufferSize);
    engineMixSmoother.setRampLengthSamples(bufferSize);
    tailMixSmoother.setRampLengthSamples(bufferSize);
    rotorMixSmoother.setRampLengthSamples(bufferSize);
    bladeNoiseSmoother.setRampLengthSamples(bufferSize);
}

void Helicopter::impl::setRotorPeriod(float value) {
    assert(value >= rotorPeriodMin);
    assert(value <= rotorPeriodMax);

    rotorPeriodSmoother.setTargetValue(value);
}

float Helicopter::impl::getRotorPeriod() const {
    return this->rotorPeriod;
}

void Helicopter::impl::setBaseFreq(float value) {
    assert(value >= baseFreqMin);
    assert(value <= baseFreqMax);

    baseFreqSmoother.setTargetValue(value);
    
    // Update the base frequency and deviation frequency (similar to JS code)
    // constantNode[3].offset.setValueAtTime(Basefreq_parameter.value * 400, plugin.context.currentTime);
    // constantNode[2].offset.setValueAtTime(Basefreq_parameter.value * 600, plugin.context.currentTime);
}

float Helicopter::impl::getBaseFreq() const {
    return this->baseFreq;
}

void Helicopter::impl::setResonance(float value) {
    assert(value >= resonanceMin);
    assert(value <= resonanceMax);

    resonanceSmoother.setTargetValue(value);
    
    // Update the resonance value for filters (similar to JS code)
    // FlatNoiseBPFilter.Q.setValueAtTime(Resonance_parameter.value * 30, plugin.context.currentTime);
    // csResonNode.offset.setValueAtTime(Math.sqrt(Resonance_parameter.value * 30)*0.6+0.1, plugin.context.currentTime);
}

float Helicopter::impl::getResonance() const {
    return this->resonance;
}

void Helicopter::impl::setEngineSpeed(float value) {
    assert(value >= engineSpeedMin);
    assert(value <= engineSpeedMax);

    engineSpeedSmoother.setTargetValue(value);
}

float Helicopter::impl::getEngineSpeed() const {
    return this->engineSpeed;
}

void Helicopter::impl::setEngineMix(float value) {
    assert(value >= engineMixMin);
    assert(value <= engineMixMax);

    engineMixSmoother.setTargetValue(value);
}

float Helicopter::impl::getEngineMix() const {
    return this->engineMix;
}

void Helicopter::impl::setTailMix(float value) {
    assert(value >= tailMixMin);
    assert(value <= tailMixMax);

    tailMixSmoother.setTargetValue(value);
}

float Helicopter::impl::getTailMix() const {
    return this->tailMix;
}

void Helicopter::impl::setRotorMix(float value) {
    assert(value >= rotorMixMin);
    assert(value <= rotorMixMax);

    rotorMixSmoother.setTargetValue(value);
}

float Helicopter::impl::getRotorMix() const {
    return this->rotorMix;
}

void Helicopter::impl::setBladeNoise(float value) {
    assert(value >= bladeNoiseMin);
    assert(value <= bladeNoiseMax);

    bladeNoiseSmoother.setTargetValue(value);
}

float Helicopter::impl::getBladeNoise() const {
    return this->bladeNoise;
}

