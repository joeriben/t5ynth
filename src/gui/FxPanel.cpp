#include "FxPanel.h"

static juce::String fmtMs(double v)  { return juce::String(juce::roundToInt(v)) + "ms"; }
static juce::String fmtF2(double v)  { return juce::String(v, 2); }

// Map normalized 0-1 to 200-20000 Hz for display
static juce::String fmtDampHz(double v)
{
    double hz = 200.0 * std::pow(100.0, v); // exponential: 200 Hz at 0, 20000 Hz at 1
    return juce::String(juce::roundToInt(hz)) + " Hz";
}

FxPanel::FxPanel(juce::AudioProcessorValueTreeState& apvts)
{
    // ── Delay ──
    delayToggle.setColour(juce::ToggleButton::textColourId, kDim);
    delayToggle.setColour(juce::ToggleButton::tickColourId, kAccent);
    delayToggle.onClick = [this] { resized(); };
    addAndMakeVisible(delayToggle);
    delayEnableA = std::make_unique<BA>(apvts, "delay_enabled", delayToggle);

    delayTimeRow = std::make_unique<SliderRow>("Time",     fmtMs);
    delayFbRow   = std::make_unique<SliderRow>("Feedback", fmtF2);
    delayDampRow = std::make_unique<SliderRow>("Damp",     fmtDampHz);
    delayMixRow  = std::make_unique<SliderRow>("Mix",      fmtF2);

    for (auto* r : { delayTimeRow.get(), delayFbRow.get(), delayDampRow.get(), delayMixRow.get() })
        addAndMakeVisible(*r);

    delayTimeA = std::make_unique<SA>(apvts, "delay_time",     delayTimeRow->getSlider());
    delayFbA   = std::make_unique<SA>(apvts, "delay_feedback", delayFbRow->getSlider());
    delayDampA = std::make_unique<SA>(apvts, "delay_damp",     delayDampRow->getSlider());
    delayMixA  = std::make_unique<SA>(apvts, "delay_mix",      delayMixRow->getSlider());

    delayTimeRow->updateValue();
    delayFbRow->updateValue();
    delayDampRow->updateValue();
    delayMixRow->updateValue();

    // ── Reverb ──
    reverbToggle.setColour(juce::ToggleButton::textColourId, kDim);
    reverbToggle.setColour(juce::ToggleButton::tickColourId, kAccent);
    reverbToggle.onClick = [this] { resized(); };
    addAndMakeVisible(reverbToggle);
    reverbEnableA = std::make_unique<BA>(apvts, "reverb_enabled", reverbToggle);

    reverbIrBox.addItemList({"Bright", "Medium", "Dark"}, 1);
    addAndMakeVisible(reverbIrBox);

    reverbMixRow = std::make_unique<SliderRow>("Mix", fmtF2);
    addAndMakeVisible(*reverbMixRow);

    reverbMixA = std::make_unique<SA>(apvts, "reverb_mix", reverbMixRow->getSlider());
    reverbIrA  = std::make_unique<CA>(apvts, "reverb_ir",  reverbIrBox);
    reverbMixRow->updateValue();
}

float FxPanel::fs() const
{
    float topH = (getTopLevelComponent() != nullptr)
                     ? static_cast<float>(getTopLevelComponent()->getHeight()) : 800.0f;
    return juce::jlimit(12.0f, 22.0f, topH * 0.022f);
}

void FxPanel::paint(juce::Graphics& g)
{
    g.fillAll(kBg);
}

void FxPanel::resized()
{
    float w = static_cast<float>(getWidth());
    float h = static_cast<float>(getHeight());
    int pad = juce::roundToInt(w * 0.06f);
    auto area = getLocalBounds().reduced(pad, juce::roundToInt(h * 0.01f));
    float f = fs();
    int rowH = juce::roundToInt(f * 1.4f);
    int gap = juce::roundToInt(f * 0.3f);

    // ── Delay ──
    delayToggle.setBounds(area.removeFromTop(rowH));
    bool delayOn = delayToggle.getToggleState();

    for (auto* r : { delayTimeRow.get(), delayFbRow.get(), delayDampRow.get(), delayMixRow.get() })
        r->setVisible(delayOn);

    if (delayOn)
    {
        delayTimeRow->setBounds(area.removeFromTop(rowH));
        delayFbRow->setBounds(area.removeFromTop(rowH));
        delayDampRow->setBounds(area.removeFromTop(rowH));
        delayMixRow->setBounds(area.removeFromTop(rowH));
    }
    area.removeFromTop(gap);

    // ── Reverb ──
    reverbToggle.setBounds(area.removeFromTop(rowH));
    bool reverbOn = reverbToggle.getToggleState();

    reverbIrBox.setVisible(reverbOn);
    reverbMixRow->setVisible(reverbOn);

    if (reverbOn)
    {
        reverbIrBox.setBounds(area.removeFromTop(rowH));
        area.removeFromTop(gap / 2);
        reverbMixRow->setBounds(area.removeFromTop(rowH));
    }
}
