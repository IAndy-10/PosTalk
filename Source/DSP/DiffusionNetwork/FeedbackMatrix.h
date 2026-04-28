#pragma once
#include <array>
#include <cmath>

// 8x8 normalized Hadamard feedback matrix.
// H[i][j] = ±1/sqrt(8). Energy preserving.
// Constructed recursively: H8 = H4 ⊗ H2, H2 = [[1,1],[1,-1]].
class FeedbackMatrix {
public:
    static constexpr int N = 8;
    static constexpr float NORM = 0.35355339059f; // 1/sqrt(8)

    // Multiply vector x by the Hadamard matrix in-place.
    // Uses the fast Hadamard transform (Walsh-Hadamard) for O(N log N).
    static void apply(std::array<float, N>& x) {
        // Fast Walsh-Hadamard transform
        for (int len = 1; len < N; len <<= 1) {
            for (int i = 0; i < N; i += len << 1) {
                for (int j = 0; j < len; ++j) {
                    float u = x[static_cast<size_t>(i + j)];
                    float v = x[static_cast<size_t>(i + j + len)];
                    x[static_cast<size_t>(i + j)]       = u + v;
                    x[static_cast<size_t>(i + j + len)] = u - v;
                }
            }
        }
        // Normalize
        for (auto& val : x) val *= NORM;
    }
};

