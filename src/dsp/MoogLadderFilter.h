#pragma once
#include <JuceHeader.h>
#include <cmath>

/**
 * Huovilainen 4-pole Moog ladder filter.
 *
 * Reference: Huovilainen 2004, "Non-linear digital implementation of the
 * moog ladder filter" (DAFx-04).
 *
 * Per-sample at native rate. tanh-of-previous-state is cached between
 * iterations as a standard optimisation. Slope selector picks which ladder
 * tap is used as the output (1-pole = 6 dB, 2 = 12, 3 = 18, 4 = 24) — classic
 * Moog multi-mode. Self-oscillates at high resonance (k ≈ 4).
 *
 * Note: the tanh stages generate harmonics that can alias near Nyquist at
 * high cutoff. First iteration ships without internal oversampling; the
 * existing pre-filter drive-stage Oversampling covers the pre-filter signal.
 * If aliasing from the filter's own saturation becomes audible in practice,
 * wrap the per-sample body in a 2× JUCE Oversampling pass at construction
 * time (not done now to keep CPU low and the implementation readable).
 *
 * API mirrors T5ynthFilter so SynthVoice can treat all filter models
 * interchangeably.
 */
class MoogLadderFilter
{
public:
    void prepare(double sampleRate, int /*samplesPerBlock*/)
    {
        sr = sampleRate;
        reset();
        updateCoeffs();
    }

    void reset()
    {
        y1 = y2 = y3 = y4 = 0.0f;
        tanh_y1 = tanh_y2 = tanh_y3 = tanh_y4 = 0.0f;
    }

    void setCutoff(float hz)
    {
        if (std::abs(hz - lastCutoff) < 0.5f) return;
        lastCutoff = hz;
        updateCoeffs();
    }

    // Resonance 0..1 → k ∈ [0, 4]. At k ≈ 4 the ladder self-oscillates.
    void setResonance(float r)
    {
        if (std::abs(r - lastReso) < 0.001f) return;
        lastReso = juce::jlimit(0.0f, 1.0f, r);
        k = 4.0f * lastReso;
    }

    // currentType: 0 = LP, 1 = HP, 2 = BP (matches post-offset BlockParams::filterType)
    void setType(int type)    { currentType = juce::jlimit(0, 2, type); }
    void setSlope(int slope)  { currentSlope = juce::jlimit(0, 3, slope); }
    void setMix(float mix)    { currentMix = juce::jlimit(0.0f, 1.0f, mix); }

    float processSample(float input)
    {
        if (currentMix < 0.001f)
            return input;

        const float drivenIn = input - k * y4;
        const float tx = std::tanh(drivenIn);

        // 4 stacked one-pole smoothers with tanh-at-input. Cache tanh(y_n)
        // across iterations to save 4 tanh calls per sample.
        y1 += fc * (tx      - tanh_y1);  tanh_y1 = std::tanh(y1);
        y2 += fc * (tanh_y1 - tanh_y2);  tanh_y2 = std::tanh(y2);
        y3 += fc * (tanh_y2 - tanh_y3);  tanh_y3 = std::tanh(y3);
        y4 += fc * (tanh_y3 - tanh_y4);  tanh_y4 = std::tanh(y4);

        const float wet = tapOutput(input);

        if (currentMix > 0.999f)
            return wet;
        return input * (1.0f - currentMix) + wet * currentMix;
    }

private:
    void updateCoeffs()
    {
        // Pre-warped cutoff (Huovilainen sin warp).
        const double w = juce::MathConstants<double>::pi
                         * static_cast<double>(lastCutoff) / sr;
        fc = 2.0f * static_cast<float>(std::sin(w));
        fc = juce::jlimit(0.0f, 1.0f, fc);
    }

    // Selects the ladder tap based on currentType + currentSlope.
    // Tap N has roughly (N × 6) dB/oct lowpass character; HP is input − LP;
    // BP is a 2-pole band via y2 − y4 (classic Moog multi-mode convention).
    float tapOutput(float input) const
    {
        const float lp = (currentSlope == 0) ? y1
                       : (currentSlope == 1) ? y2
                       : (currentSlope == 2) ? y3
                       :                       y4;
        switch (currentType)
        {
            case 0: return lp;
            case 1: return input - lp;
            case 2: return (y2 - y4);
            default: return lp;
        }
    }

    double sr = 44100.0;

    float y1 = 0.0f, y2 = 0.0f, y3 = 0.0f, y4 = 0.0f;
    float tanh_y1 = 0.0f, tanh_y2 = 0.0f, tanh_y3 = 0.0f, tanh_y4 = 0.0f;

    float fc = 0.0f;
    float k  = 0.0f;
    float lastCutoff = 20000.0f;
    float lastReso   = 0.0f;
    float currentMix = 1.0f;
    int   currentType  = 0;
    int   currentSlope = 3;
};
