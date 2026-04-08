#pragma once
#include <cmath>
#include <cstdint>

enum class NoiseType { White = 0, Pink = 1, Brown = 2 };

/**
 * Multi-color noise generator: White, Pink (-3dB/oct), Brown (-6dB/oct).
 *
 * Per-voice instance: each voice gets independent noise that follows the
 * voice's amplitude envelope and filter chain.
 */
class NoiseGenerator
{
public:
    NoiseGenerator() = default;

    void prepare(double sampleRate);
    void reset();

    void setType(NoiseType t) { type_ = t; }

    /** Generate one sample of noise (type selected via setType). */
    float processSample();

private:
    double sr_ = 44100.0;
    NoiseType type_ = NoiseType::White;

    // xorshift64 PRNG
    uint64_t rngState_ = 0x12345678ABCDEF01ULL;
    float nextWhite();

    // Pink noise state (Paul Kellet approximation)
    float b0_ = 0.0f, b1_ = 0.0f, b2_ = 0.0f;
    float b3_ = 0.0f, b4_ = 0.0f, b5_ = 0.0f, b6_ = 0.0f;

    // Brown noise state (leaky integrator)
    float brownState_ = 0.0f;

    // DC blocker (one-pole HP at ~5 Hz)
    float dcState_ = 0.0f;
    float dcCoeff_ = 0.0f;
};
