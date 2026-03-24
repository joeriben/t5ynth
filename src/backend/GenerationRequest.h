#pragma once
#include <JuceHeader.h>

/**
 * Encapsulates a generation request to the backend.
 *
 * Holds prompt, axis values, alpha, magnitude, noise, and seed.
 */
class GenerationRequest
{
public:
    GenerationRequest() = default;

    void setPrompt(const juce::String& p) { prompt = p; }
    void setAlpha(float a) { alpha = a; }
    void setMagnitude(float m) { magnitude = m; }
    void setNoise(float n) { noise = n; }
    void setSeed(int s) { seed = s; }

    juce::String getPrompt() const { return prompt; }
    float getAlpha() const { return alpha; }
    float getMagnitude() const { return magnitude; }
    float getNoise() const { return noise; }
    int getSeed() const { return seed; }

    /** Serialize to JSON for the backend API. */
    juce::String toJson() const;

private:
    juce::String prompt;
    float alpha = 0.0f;
    float magnitude = 1.0f;
    float noise = 0.0f;
    int seed = -1; // -1 = random
};
