// Copyright (c) 2025 Nemisindo Ltd. All rights reserved.

/**
* Public API for the rocket model. Do not modify.
*/

#pragma once

#include <memory>

namespace nemisindo {

class Rocket {
public:
    Rocket();
    ~Rocket();

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

    void setDuration(float value);
    float getDuration() const;

    void setChamberResonance(float value);
    float getChamberResonance() const;

    void setFlutter(float value);
    float getFlutter() const;


private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

}