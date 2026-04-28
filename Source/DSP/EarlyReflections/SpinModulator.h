#pragma once
#include "DiffusionNetwork/LFO.h"

// Spin modulator for early reflections: two quadrature LFOs that create
// a slow rotation effect, adding movement to the early reflection field.
class SpinModulator {
public:
    void prepare(double sampleRate);
    void setRate(float hz);
    void setAmount(float amount); // 0-1

    // Returns modulation values for L and R channels
    void getNextPair(float& outL, float& outR);

    void reset();

private:
    LFO   lfoSin, lfoCos;
    float amount = 0.3f;
};

