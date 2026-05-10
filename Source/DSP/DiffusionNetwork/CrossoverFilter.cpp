#include "CrossoverFilter.h"
#include <cmath>

static constexpr double kPi = 3.14159265358979323846;

void CrossoverFilter::prepare(double sampleRate) {
    sr = sampleRate;
    updateCoeff();
    smoothedCoeff = coeff; // start at target — no initial ramp artifact
    smoothK = 1.0 - std::exp(-1.0 / (sr * 0.02)); // ~20 ms smoothing time constant
    reset();
}

void CrossoverFilter::setCutoffFreq(float hz) {
    cutoff = hz;
    updateCoeff();
}

void CrossoverFilter::setDamping(float d) {
    damping = d;
    updateCoeff();
}

void CrossoverFilter::setFilterType(bool shelving) {
    useShelving = shelving;
    // coeff is shared; no coefficient recomputation needed.
}

void CrossoverFilter::reset() { z1 = 0.0; smoothedCoeff = coeff; }

void CrossoverFilter::updateCoeff() {
    // Blend between no-filter (coeff=0) and full LP at cutoff (coeff=1-e^(-2pi*f/sr))
    double omega = 2.0 * kPi * static_cast<double>(cutoff) / sr;
    double baseCoeff = 1.0 - std::exp(-omega);
    // Scale by damping: 0 damping → coeff≈1 (no LP), 1 damping → coeff near full LP
    coeff = 1.0 - static_cast<double>(damping) * (1.0 - baseCoeff);
    // Clamp for stability
    if (coeff < 0.001) coeff = 0.001;
    if (coeff > 1.0)   coeff = 1.0;
}

float CrossoverFilter::processSample(float in) {
    // Ramp smoothedCoeff toward coeff at ~20 ms rate to avoid abrupt filter shifts
    // when setCutoffFreq() or setDamping() are called between blocks.
    smoothedCoeff += smoothK * (coeff - smoothedCoeff);

    // Update one-pole LP state using the smoothed coefficient.
    z1 = smoothedCoeff * static_cast<double>(in) + (1.0 - smoothedCoeff) * z1;

    if (useShelving) {
        // High-shelf mode: reduce HF above corner, pass LF fully.
        // y[n] = SHELF_GAIN * x[n] + (1 - SHELF_GAIN) * z1_lp
        // At DC:      z1 ≈ in  → y ≈ in           (full LF pass)
        // At Nyquist: z1 ≈ 0   → y ≈ SHELF_GAIN*in (HF reduced, ≈ -6 dB)
        // Damping parameter has no effect in this mode.
        return static_cast<float>(SHELF_GAIN * static_cast<double>(in) + (1.0 - SHELF_GAIN) * z1);
    }

    // LP mode (default): y[n] = coeff * x[n] + (1-coeff) * y[n-1]
    return static_cast<float>(z1);
}

