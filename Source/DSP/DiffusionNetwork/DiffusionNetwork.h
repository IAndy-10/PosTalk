#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include "DiffusionNetwork/DiffusionStage.h"

// Chain of 4 stereo allpass diffusion stages applied before the FDN input.
// Each stage has different delay lengths (prime) to spread the input density.
class DiffusionNetwork {
public:
    void prepare(double sampleRate);
    void setDiffusion(float d);   // 0-1: allpass coefficient
    void setNumStages(int n);     // 0–4: how many allpass stages are active
    void processStereo(float& sampleL, float& sampleR);
    void reset();

private:
    static constexpr int NUM_STAGES = 4;

    // Stereo prime delay lengths for each stage (at 44100 Hz)
    static constexpr int DELAYS_L[NUM_STAGES] = { 151, 211, 379, 479 };
    static constexpr int DELAYS_R[NUM_STAGES] = { 167, 239, 397, 509 };

    std::array<DiffusionStage, NUM_STAGES> stages;
    float diffusion  = 0.6f;
    int   numActive  = NUM_STAGES; // controlled by density parameter
};

