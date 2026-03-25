#pragma once
#include <JuceHeader.h>
#include <map>

/**
 * Encapsulates a generation request to the Python backend.
 *
 * Field names match the /api/cross_aesthetic/synth contract:
 *   prompt_a, prompt_b, alpha, magnitude, noise_sigma,
 *   duration_seconds, start_position, steps, cfg_scale, seed,
 *   axes, dimension_offsets
 */
class GenerationRequest
{
public:
    GenerationRequest() = default;

    // --- Prompts ---
    void setPromptA(const juce::String& p) { promptA = p; }
    void setPromptB(const juce::String& p) { promptB = p; }
    juce::String getPromptA() const { return promptA; }
    juce::String getPromptB() const { return promptB; }

    // --- Embedding control ---
    void setAlpha(float a) { alpha = a; }
    void setMagnitude(float m) { magnitude = m; }
    void setNoiseSigma(float n) { noiseSigma = n; }
    float getAlpha() const { return alpha; }
    float getMagnitude() const { return magnitude; }
    float getNoiseSigma() const { return noiseSigma; }

    // --- Generation parameters ---
    void setDurationSeconds(float d) { durationSeconds = d; }
    void setStartPosition(float p) { startPosition = p; }
    void setSteps(int s) { steps = s; }
    void setCfgScale(float c) { cfgScale = c; }
    void setSeed(int s) { seed = s; }
    float getDurationSeconds() const { return durationSeconds; }
    float getStartPosition() const { return startPosition; }
    int getSteps() const { return steps; }
    float getCfgScale() const { return cfgScale; }
    int getSeed() const { return seed; }

    // --- Semantic axes ---
    void setAxis(const juce::String& name, float value) { axes[name] = value; }
    void clearAxes() { axes.clear(); }
    const std::map<juce::String, float>& getAxes() const { return axes; }

    // --- Dimension offsets ---
    void setDimensionOffset(int dim, float offset) { dimensionOffsets[dim] = offset; }
    void clearDimensionOffsets() { dimensionOffsets.clear(); }
    const std::map<int, float>& getDimensionOffsets() const { return dimensionOffsets; }

    /** Serialize to JSON matching the Python API contract. */
    juce::String toJson() const;

private:
    juce::String promptA;
    juce::String promptB;          // empty = not sent
    float alpha = 0.5f;
    float magnitude = 1.0f;
    float noiseSigma = 0.0f;
    float durationSeconds = 1.0f;
    float startPosition = 0.0f;
    int steps = 20;
    float cfgScale = 7.0f;
    int seed = -1;                 // -1 = random
    std::map<juce::String, float> axes;
    std::map<int, float> dimensionOffsets;
};
