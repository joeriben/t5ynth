#pragma once
#include <JuceHeader.h>
#include <vector>
#include <map>

/**
 * Semantic axes as compact bipolar sliders (pole_a ←→ pole_b).
 * All visible, no scrollbar.
 */
class AxesPanel : public juce::Component
{
public:
    AxesPanel();
    ~AxesPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    std::map<juce::String, float> getAxisValues() const;
    void setAxes(const juce::var& axesData);

private:
    struct AxisRow
    {
        juce::String name, poleA, poleB;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::Label> labelA, labelB;
    };

    std::vector<AxisRow> rows;
    void addAxis(const juce::String& name, const juce::String& poleA, const juce::String& poleB);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AxesPanel)
};
