#pragma once
#include <JuceHeader.h>
#include <functional>

// ── Color constants (shared across all GUI files) ──────────────────────────
static const auto kAccent  = juce::Colour(0xff4a9eff);  // Blue accent
static const auto kDim     = juce::Colour(0xff888888);
static const auto kDimmer  = juce::Colour(0xff606060);
static const auto kSurface = juce::Colour(0xff1a1a1a);
static const auto kBg      = juce::Colour(0xff0a0a0a);

// Semantic axis colors
static const auto kAxis1   = juce::Colour(0xffe91e63);  // Pink
static const auto kAxis2   = juce::Colour(0xff2196f3);  // Blue
static const auto kAxis3   = juce::Colour(0xff4caf50);  // Green

/**
 * Compact horizontal slider row:  Label [===slider===] Value+Unit
 * ~22px tall, used everywhere instead of rotary knobs.
 */
class SliderRow : public juce::Component
{
public:
    SliderRow(const juce::String& name,
              std::function<juce::String(double)> formatter,
              juce::Colour trackColor = kAccent)
        : valueFormatter(std::move(formatter)),
          trackCol(trackColor)
    {
        label.setText(name, juce::dontSendNotification);
        label.setColour(juce::Label::textColourId, kDim);
        label.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(label);

        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        slider.setColour(juce::Slider::trackColourId, trackCol);
        slider.setColour(juce::Slider::backgroundColourId, kSurface);
        slider.onValueChange = [this] { updateValue(); };
        addAndMakeVisible(slider);

        value.setColour(juce::Label::textColourId, trackCol);
        value.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(value);
    }

    juce::Slider& getSlider() { return slider; }
    juce::Label& getLabel() { return label; }
    juce::Label& getValueLabel() { return value; }

    void setTrackColor(juce::Colour c)
    {
        trackCol = c;
        slider.setColour(juce::Slider::trackColourId, c);
        value.setColour(juce::Label::textColourId, c);
    }

    void updateValue()
    {
        if (valueFormatter)
            value.setText(valueFormatter(slider.getValue()), juce::dontSendNotification);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        float f = static_cast<float>(b.getHeight());

        int labelW = juce::jmax(40, juce::roundToInt(b.getWidth() * 0.22f));
        int valueW = juce::jmax(50, juce::roundToInt(b.getWidth() * 0.20f));

        label.setFont(juce::FontOptions(f * 0.75f));
        value.setFont(juce::FontOptions(f * 0.75f));

        label.setBounds(b.removeFromLeft(labelW));
        value.setBounds(b.removeFromRight(valueW));
        slider.setBounds(b);
    }

private:
    juce::Label label, value;
    juce::Slider slider;
    std::function<juce::String(double)> valueFormatter;
    juce::Colour trackCol;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SliderRow)
};

/**
 * Two SliderRows side by side in a 2-column grid.
 * Height = one row height. Each SliderRow gets 50% width minus gap.
 */
class SliderPair : public juce::Component
{
public:
    SliderPair(std::unique_ptr<SliderRow> l, std::unique_ptr<SliderRow> r)
        : left(std::move(l)), right(std::move(r))
    {
        addAndMakeVisible(*left);
        addAndMakeVisible(*right);
    }

    SliderRow& getLeft() { return *left; }
    SliderRow& getRight() { return *right; }

    void resized() override
    {
        auto b = getLocalBounds();
        int gap = juce::jmax(4, juce::roundToInt(b.getWidth() * 0.02f));
        int colW = (b.getWidth() - gap) / 2;
        left->setBounds(b.removeFromLeft(colW));
        b.removeFromLeft(gap);
        right->setBounds(b);
    }

private:
    std::unique_ptr<SliderRow> left, right;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SliderPair)
};
