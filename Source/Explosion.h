// Copyright (c) 2025 Nemisindo Ltd. All rights reserved.

/**
* Public API for the explosion model. Do not modify.
*/

#pragma once

#include <memory>

namespace nemisindo {

class Explosion {
public:
    Explosion();
    ~Explosion();

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

    void setRumble(float value);
    float getRumble() const;

    void setRumbleDecay(float value);
    float getRumbleDecay() const;

    void setAir(float value);
    float getAir() const;

    void setAirDecay(float value);
    float getAirDecay() const;

    void setDust(float value);
    float getDust() const;

    void setDustDecay(float value);
    float getDustDecay() const;

    void setTimeSeparation(float value);
    float getTimeSeparation() const;

    void setGrit(bool value);
    bool getGrit() const;

    void setGritAmount(float value);
    float getGritAmount() const;

    void setOverTheTop(bool value);
    bool getOverTheTop() const;


private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

}