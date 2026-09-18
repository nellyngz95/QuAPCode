// Copyright (c) 2025 Nemisindo Ltd. All rights reserved.

/**
* Public API for the jet model. Do not modify.
*/

#pragma once

#include <memory>

namespace nemisindo {

class Jet {
public:
    Jet();
    ~Jet();

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

    void setSpeed(float value);
    float getSpeed() const;

    void setTurbine(float value);
    float getTurbine() const;

    void setBurn(float value);
    float getBurn() const;
    class impl;
    std::unique_ptr<impl> pimpl;

private:
 
};

}
