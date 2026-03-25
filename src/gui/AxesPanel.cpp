#include "AxesPanel.h"

AxesPanel::AxesPanel()
{
    addAxis("tonal_noisy",        "tonal",    "noisy");
    addAxis("rhythmic_sustained", "rhythmic", "sustained");
    addAxis("bright_dark",        "bright",   "dark");
    addAxis("loud_quiet",         "loud",     "quiet");
    addAxis("smooth_harsh",       "smooth",   "harsh");
    addAxis("fast_slow",          "fast",     "slow");
    addAxis("close_distant",      "close",    "distant");
    addAxis("dense_sparse",       "dense",    "sparse");
}

void AxesPanel::addAxis(const juce::String& name, const juce::String& poleA, const juce::String& poleB)
{
    auto& row = rows.emplace_back();
    row.name = name;
    row.poleA = poleA;
    row.poleB = poleB;

    row.slider = std::make_unique<juce::Slider>();
    row.slider->setSliderStyle(juce::Slider::LinearHorizontal);
    row.slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    row.slider->setRange(-1.0, 1.0, 0.01);
    row.slider->setValue(0.0, juce::dontSendNotification);
    row.slider->setColour(juce::Slider::trackColourId, juce::Colour(0xff4a9eff));
    row.slider->setColour(juce::Slider::backgroundColourId, juce::Colour(0xff1a1a1a));
    row.slider->setColour(juce::Slider::thumbColourId, juce::Colour(0xffe3e3e3));
    addAndMakeVisible(*row.slider);

    row.labelA = std::make_unique<juce::Label>("", poleA);
    row.labelA->setJustificationType(juce::Justification::centredRight);
    row.labelA->setColour(juce::Label::textColourId, juce::Colour(0xff666666));
    addAndMakeVisible(*row.labelA);

    row.labelB = std::make_unique<juce::Label>("", poleB);
    row.labelB->setJustificationType(juce::Justification::centredLeft);
    row.labelB->setColour(juce::Label::textColourId, juce::Colour(0xff666666));
    addAndMakeVisible(*row.labelB);
}

void AxesPanel::paint(juce::Graphics& g)
{
    float topH = (getTopLevelComponent() != nullptr) ? static_cast<float>(getTopLevelComponent()->getHeight()) : 800.0f;
    float fs = juce::jlimit(10.0f, 16.0f, topH * 0.016f);
    g.setFont(juce::FontOptions(fs));
    g.setColour(juce::Colour(0xff888888));
    float pad = getWidth() * 0.03f;
    g.drawText("AXES", juce::roundToInt(pad), 0, getWidth(),
               juce::roundToInt(static_cast<float>(getHeight()) * 0.08f),
               juce::Justification::centredLeft);
}

void AxesPanel::resized()
{
    if (rows.empty()) return;
    float w = static_cast<float>(getWidth());
    float h = static_cast<float>(getHeight());
    float pad = w * 0.03f;
    float headerH = h * 0.09f;
    float rowH = (h - headerH) / static_cast<float>(rows.size());
    float topH2 = (getTopLevelComponent() != nullptr) ? static_cast<float>(getTopLevelComponent()->getHeight()) : 800.0f;
    float fs = juce::jlimit(9.0f, 13.0f, topH2 * 0.013f);
    float labelW = w * 0.22f;

    for (size_t i = 0; i < rows.size(); ++i)
    {
        auto& row = rows[i];
        int y = juce::roundToInt(headerH + static_cast<float>(i) * rowH);
        int rh = juce::roundToInt(rowH);
        int lw = juce::roundToInt(labelW);
        int px = juce::roundToInt(pad);

        row.labelA->setFont(juce::FontOptions(fs));
        row.labelA->setBounds(px, y, lw, rh);
        row.labelB->setFont(juce::FontOptions(fs));
        row.labelB->setBounds(getWidth() - lw - px, y, lw, rh);

        int sliderX = px + lw + 2;
        int sliderW = getWidth() - 2 * (px + lw + 2);
        row.slider->setBounds(sliderX, y, juce::jmax(10, sliderW), rh);
    }
}

std::map<juce::String, float> AxesPanel::getAxisValues() const
{
    std::map<juce::String, float> vals;
    for (auto& row : rows)
        if (row.slider->getValue() != 0.0)
            vals[row.name] = static_cast<float>(row.slider->getValue());
    return vals;
}

void AxesPanel::setAxes(const juce::var& axesData)
{
    if (!axesData.isArray()) return;
    for (auto& row : rows)
    {
        removeChildComponent(row.slider.get());
        removeChildComponent(row.labelA.get());
        removeChildComponent(row.labelB.get());
    }
    rows.clear();
    for (int i = 0; i < axesData.size(); ++i)
    {
        auto axis = axesData[i];
        addAxis(axis.getProperty("name", "").toString(),
                axis.getProperty("pole_a", "").toString(),
                axis.getProperty("pole_b", "").toString());
    }
    resized();
}
