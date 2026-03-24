#pragma once
#include <JuceHeader.h>
#include <vector>

/**
 * Semantic axes management for latent space exploration.
 *
 * Stores axis definitions (pairs of opposing concepts) and
 * their current interpolation values.
 */
class SemanticAxes
{
public:
    SemanticAxes() = default;

    /** Add a semantic axis defined by two opposing prompts. */
    void addAxis(const juce::String& negative, const juce::String& positive);

    /** Set the interpolation value for an axis (-1 to +1). */
    void setAxisValue(int axisIndex, float value);

    /** Get the current value for an axis. */
    float getAxisValue(int axisIndex) const;

    /** Get the number of defined axes. */
    int getNumAxes() const { return static_cast<int>(axes.size()); }

    /** Clear all axes. */
    void clear() { axes.clear(); }

private:
    struct Axis
    {
        juce::String negative;
        juce::String positive;
        float value = 0.0f;
    };

    std::vector<Axis> axes;
};
