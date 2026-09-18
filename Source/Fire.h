// Copyright (c) 2025 Nemisindo Ltd. All rights reserved.

/**
* Public API for the fire model. Do not modify.
*/

#pragma once

#include <memory>

namespace nemisindo {

class Fire {
public:
    Fire();
    ~Fire();

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

    void setLapping(float value);
    float getLapping() const;

    void setHissing(float value);
    float getHissing() const;

    void setCrackling(float value);
    float getCrackling() const;

    void setIntensity(float value);
    float getIntensity() const;
    class impl;
    std::unique_ptr<impl> pimpl;

private:
    //class impl;
    //std::unique_ptr<impl> pimpl;
};

}
