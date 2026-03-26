#pragma once
#include <JuceHeader.h>
#include <vector>
#include <map>

/**
 * Semantic + PCA axes with color-coded slots.
 * Semantic: 3 slots with pink/blue/green dots and colored sliders.
 * PCA: 6 slots, neutral accent color.
 */
class AxesPanel : public juce::Component
{
public:
    AxesPanel();
    ~AxesPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    std::map<juce::String, float> getAxisValues() const;

private:
    float fs() const;

    struct AxisSlot
    {
        std::unique_ptr<juce::ComboBox> dropdown;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::Label> valueLabel;
        int axisIndex = -1; // 0/1/2 for semantic (color), -1 for PCA
    };

    juce::Label semHeader, pcaHeader;
    std::vector<AxisSlot> semSlots;  // 3
    std::vector<AxisSlot> pcaSlots;  // 6

    void initSlot(AxisSlot& slot, const juce::StringArray& options, int axisIndex);
    void layoutSlots(std::vector<AxisSlot>& slots, juce::Rectangle<int>& area, float f, int dotOffset);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AxesPanel)
};
