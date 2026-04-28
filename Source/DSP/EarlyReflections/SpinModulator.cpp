#include "SpinModulator.h"
#include <cmath>

void SpinModulator::prepare(double sampleRate) {
    lfoSin.prepare(sampleRate);
    lfoCos.prepare(sampleRate);
    lfoSin.setFrequency(1.0f);
    lfoCos.setFrequency(1.0f);
    // Advance only lfoCos by 90 degrees (quarter period at 1 Hz)
    for (int i = 0; i < static_cast<int>(sampleRate / 4.0); ++i)
        lfoCos.getNext();
}

void SpinModulator::setRate(float hz) {
    lfoSin.setFrequency(hz);
    lfoCos.setFrequency(hz);
}

void SpinModulator::setAmount(float a) {
    amount = a;
}

void SpinModulator::getNextPair(float& outL, float& outR) {
    float s = lfoSin.getNext();
    float c = lfoCos.getNext();
    outL = amount * s;
    outR = amount * c;
}

void SpinModulator::reset() {
    lfoSin.reset();
    lfoCos.reset();
}

