#pragma once
#include <JuceHeader.h>
#include <vector>

/**
 * 2D visualization of the embedding space.
 * Shows generated points as dots; click to navigate.
 */
class DimensionExplorer : public juce::Component
{
public:
    DimensionExplorer();
    ~DimensionExplorer() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    /** Add a point from a generation result (embedding stats). */
    void addPoint(float x, float y, int seed);

    /** Clear all points. */
    void clear();

    /** Called when the mini-view is clicked (to open overlay). */
    std::function<void()> onClicked;

    void mouseUp(const juce::MouseEvent& e) override;

private:
    struct Point { float x, y; int seed; };
    std::vector<Point> points;
    int selectedIndex = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DimensionExplorer)
};
