#pragma once
#include <JuceHeader.h>

/**
 * 2D/3D visualization of the latent space.
 *
 * Allows clicking to navigate dimensions and see how generated
 * sounds relate to each other in the semantic space.
 */
class DimensionExplorer : public juce::Component
{
public:
    DimensionExplorer();
    ~DimensionExplorer() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DimensionExplorer)
};
