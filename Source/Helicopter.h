// Copyright (c) 2025 Nemisindo Ltd. All rights reserved.

/**
* Public API for the helicopter model. Do not modify.
*/

#pragma once

#include <memory>

namespace nemisindo {

class Helicopter {
public:
    Helicopter();
    ~Helicopter();

    void initialize(float sampleRate);
    void tick();
    void reset();
    float outputSample(int channel = 0) const;
    void fillBuffer(float** buffer, int numSamples);

    static constexpr int getNumOutputChannels();

    void start();
    void stop();
    int tailDurationSamples() const;
    bool isActive() const;

    void updateParameterInterval(int bufferSize);

    void setRotorPeriod(float value);
    float getRotorPeriod() const;

    void setBaseFreq(float value);
    float getBaseFreq() const;

    void setResonance(float value);
    float getResonance() const;

    void setEngineSpeed(float value);
    float getEngineSpeed() const;

    void setEngineMix(float value);
    float getEngineMix() const;

    void setTailMix(float value);
    float getTailMix() const;

    void setRotorMix(float value);
    float getRotorMix() const;

    void setBladeNoise(float value);
    float getBladeNoise() const;
    class impl;
    std::unique_ptr<impl> pimpl;


private:

};

}
