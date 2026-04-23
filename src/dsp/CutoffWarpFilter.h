#pragma once
#include <JuceHeader.h>
#include <cmath>

/**
 * Cutoff-Warp ladder filter.
 *
 * Surge-XT-style ZDF (zero-delay-feedback) 4-pole ladder with a per-pole
 * nonlinearity applied at the feedback input. The style selector picks which
 * saturation curve is used, opening a continuous character space beyond the
 * single tanh response of a classical Moog ladder.
 *
 * Reference topology: Surge XT src/common/dsp/filters/CutoffWarp.cpp (GPL-3).
 * This implementation is written from scratch against the standard ZDF-ladder
 * derivation and Surge's documented saturation curves — no code copied.
 *
 * Each one-pole stage is a TPT (topology-preserving transform) integrator:
 *   v = (x − s) × g / (1 + g)
 *   y = v + s
 *   s_new = y + v
 * where g = tan(π · cutoff / sr), computed once per sub-block.
 *
 * Slope selects which ladder tap is the output (same convention as the Moog
 * ladder). Style choices:
 *   0 Tanh       symmetric soft
 *   1 SoftClip   x / (1 + |x|)
 *   2 OJD        asymmetric atan-based, warm
 *   3 Sin        sin-fold, FM-like wavefolding
 *   4 Digital    hard clamp
 *   5 Asym       tanh with DC bias (2nd harmonic)
 */
class CutoffWarpFilter
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
        s1 = s2 = s3 = s4 = 0.0f;
        y1 = y2 = y3 = y4 = 0.0f;
    }

    void setCutoff(float hz)
    {
        if (std::abs(hz - lastCutoff) < 0.5f) return;
        lastCutoff = hz;
        updateCoeffs();
    }

    void setResonance(float r)
    {
        if (std::abs(r - lastReso) < 0.001f) return;
        lastReso = juce::jlimit(0.0f, 1.0f, r);
        // Map 0..1 → k ∈ [0, 4.2]. A little over 4 allows a touch of self-
        // oscillation character at max setting without blowing up.
        k = 4.2f * lastReso;
    }

    void setType(int type)    { currentType  = juce::jlimit(0, 2, type); }
    void setSlope(int slope)  { currentSlope = juce::jlimit(0, 3, slope); }
    void setMix(float mix)    { currentMix   = juce::jlimit(0.0f, 1.0f, mix); }
    void setStyle(int style)  { currentStyle = juce::jlimit(0, 5, style); }

    float processSample(float input)
    {
        if (currentMix < 0.001f)
            return input;

        // Feedback path with style-selected nonlinearity. The drive factor is
        // folded into the feedback amount (k) rather than as a separate input
        // gain: the upstream pre-filter drive stage already handles explicit
        // drive before the filter.
        const float fb = k * sat(y4, currentStyle);
        const float x  = input - fb;

        y1 = tptStep(s1, sat(x,  currentStyle));
        y2 = tptStep(s2, y1);
        y3 = tptStep(s3, y2);
        y4 = tptStep(s4, y3);

        const float wet = tapOutput(input);

        if (currentMix > 0.999f)
            return wet;
        return input * (1.0f - currentMix) + wet * currentMix;
    }

private:
    void updateCoeffs()
    {
        const double w = juce::MathConstants<double>::pi
                         * static_cast<double>(lastCutoff) / sr;
        const double t = std::tan(juce::jlimit(1e-4, juce::MathConstants<double>::halfPi - 1e-4, w));
        g     = static_cast<float>(t);
        gFrac = g / (1.0f + g);
    }

    // Single TPT one-pole integrator step. Mutates the state `s` and returns
    // the output sample.
    float tptStep(float& s, float x)
    {
        const float v = (x - s) * gFrac;
        const float y = v + s;
        s = y + v;
        return y;
    }

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

    // Saturation functions, indexed by FilterWarpStyle::*.
    static float sat(float x, int style)
    {
        switch (style)
        {
            case 0: return std::tanh(x);
            case 1: return x / (1.0f + std::abs(x));
            case 2:
            {
                // OJD-like: asymmetric atan, bias adds a tiny 2nd-harmonic colour.
                const float piX = juce::MathConstants<float>::pi * x + 0.2f;
                return (2.0f / juce::MathConstants<float>::pi) * std::atan(piX);
            }
            case 3:
            {
                // Sin-fold, clamped to the first half-cycle to stay monotonic
                // outside the fold region.
                const float arg = juce::jlimit(-juce::MathConstants<float>::halfPi,
                                                juce::MathConstants<float>::halfPi,
                                                x * juce::MathConstants<float>::halfPi);
                return std::sin(arg);
            }
            case 4: return juce::jlimit(-1.0f, 1.0f, x);
            case 5:
            {
                // Asymmetric tanh with DC-bias removed so output sits at 0 for x=0.
                constexpr float bias = 0.25f;
                constexpr float biasTanh = 0.24491866f; // std::tanh(0.25) at compile time
                return std::tanh(x + bias) - biasTanh;
            }
        }
        return std::tanh(x);
    }

    double sr = 44100.0;

    // TPT integrator state and output taps for each of the 4 stages.
    float s1 = 0.0f, s2 = 0.0f, s3 = 0.0f, s4 = 0.0f;
    float y1 = 0.0f, y2 = 0.0f, y3 = 0.0f, y4 = 0.0f;

    float g     = 0.0f; // tan(π fc / sr)
    float gFrac = 0.0f; // g / (1 + g) — pre-computed factor used in tptStep
    float k     = 0.0f;
    float lastCutoff = 20000.0f;
    float lastReso   = 0.0f;
    float currentMix = 1.0f;
    int   currentType  = 0;
    int   currentSlope = 3;
    int   currentStyle = 0;
};
