#include "NoiseGenerator.h"

void NoiseGenerator::prepare(double sampleRate)
{
    sr_ = sampleRate;
    dcState_ = 0.0f;
    b0_ = b1_ = b2_ = b3_ = b4_ = b5_ = b6_ = 0.0f;
    brownState_ = 0.0f;
    dcCoeff_ = 1.0f - static_cast<float>(std::exp(-2.0 * 3.14159265358979 * 5.0 / sr_));
}

void NoiseGenerator::reset()
{
    dcState_ = 0.0f;
    b0_ = b1_ = b2_ = b3_ = b4_ = b5_ = b6_ = 0.0f;
    brownState_ = 0.0f;
}

float NoiseGenerator::nextWhite()
{
    rngState_ ^= rngState_ << 13;
    rngState_ ^= rngState_ >> 7;
    rngState_ ^= rngState_ << 17;
    return static_cast<float>(static_cast<int64_t>(rngState_)) * (1.0f / 9223372036854775808.0f);
}

float NoiseGenerator::processSample()
{
    float white = nextWhite();
    float out;

    switch (type_)
    {
        case NoiseType::Pink:
        {
            // Paul Kellet approximation — pink noise (-3dB/octave)
            b0_ = 0.99886f * b0_ + white * 0.0555179f;
            b1_ = 0.99332f * b1_ + white * 0.0750759f;
            b2_ = 0.96900f * b2_ + white * 0.1538520f;
            b3_ = 0.86650f * b3_ + white * 0.3104856f;
            b4_ = 0.55000f * b4_ + white * 0.5329522f;
            b5_ = -0.7616f * b5_ - white * 0.0168980f;
            out = (b0_ + b1_ + b2_ + b3_ + b4_ + b5_ + b6_ + white * 0.5362f) * 0.11f;
            b6_ = white * 0.115926f;
            break;
        }
        case NoiseType::Brown:
        {
            // Leaky integrator — brown/red noise (-6dB/octave)
            brownState_ = brownState_ * 0.998f + white * 0.02f;
            out = brownState_ * 3.5f;
            break;
        }
        default: // White
            out = white;
            break;
    }

    // DC blocker
    float filtered = out - dcState_;
    dcState_ += dcCoeff_ * (out - dcState_);

    return filtered;
}
