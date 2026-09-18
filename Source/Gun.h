// Copyright (c) 2025 Nemisindo Ltd. All rights reserved.

/**
* Public API for the gun model. Do not modify.
*/

#pragma once

#include <memory>

namespace nemisindo {

class Gun {
public:
    Gun();
    ~Gun();

    void initialize(float sampleRate);
    void tick();
    void reset();
    float outputSample(int channel = 0) const;
    void fillBuffer(float** buffer, int numSamples);

    static constexpr int getNumOutputChannels();

    void trigger();
    int totalDurationSamples() const;
    bool isActive() const;

    void updateParameterInterval(int bufferSize);

    void setShellDecay(float value);
    float getShellDecay() const;

    void setShellFreq(float value);
    float getShellFreq() const;
    


private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

}
