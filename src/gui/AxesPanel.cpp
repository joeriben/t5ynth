#include "AxesPanel.h"
#include "GuiHelpers.h"

static const juce::Colour kAxisColors[] = { kAxis1, kAxis2, kAxis3 };

static const juce::StringArray kSemanticAxes {
    "---",
    "tonal / noisy (d=4.81)",
    "rhythmic / sustained (d=2.60)",
    "bright / dark (d=1.28)",
    "loud / quiet (d=1.02)",
    "smooth / harsh (d=0.81)",
    "fast / slow (d=0.80)",
    "close / distant (d=0.76)",
    "dense / sparse (d=0.74)"
};

static const juce::StringArray kPcaAxes {
    "---",
    "PC1: natural / synthetic",
    "PC2: sonic / physical",
    "PC3: tonal / atonal",
    "PC4: continuous / impulsive",
    "PC5: harmonic / inharmonic",
    "PC6: wet / dry",
    "PC7: melodic / percussive",
    "PC8: soft / aggressive",
    "PC9: clean / distorted",
    "PC10: ambient / direct"
};

AxesPanel::AxesPanel()
{
    semHeader.setText("SEMANTIC AXES", juce::dontSendNotification);
    semHeader.setColour(juce::Label::textColourId, kDim);
    addAndMakeVisible(semHeader);

    pcaHeader.setText("PCA AXES", juce::dontSendNotification);
    pcaHeader.setColour(juce::Label::textColourId, kDim);
    addAndMakeVisible(pcaHeader);

    semSlots.resize(3);
    for (size_t i = 0; i < semSlots.size(); ++i)
        initSlot(semSlots[i], kSemanticAxes, static_cast<int>(i));

    pcaSlots.resize(6);
    for (auto& slot : pcaSlots)
        initSlot(slot, kPcaAxes, -1);
}

void AxesPanel::initSlot(AxisSlot& slot, const juce::StringArray& options, int axisIndex)
{
    slot.axisIndex = axisIndex;

    slot.dropdown = std::make_unique<juce::ComboBox>();
    slot.dropdown->addItemList(options, 1);
    slot.dropdown->setSelectedId(1, juce::dontSendNotification); // "---"
    slot.dropdown->onChange = [this] { resized(); };
    addAndMakeVisible(*slot.dropdown);

    juce::Colour sliderColor = (axisIndex >= 0 && axisIndex < 3) ? kAxisColors[axisIndex] : kAccent;

    slot.slider = std::make_unique<juce::Slider>();
    slot.slider->setSliderStyle(juce::Slider::LinearHorizontal);
    slot.slider->setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    slot.slider->setRange(-1.0, 1.0, 0.002);
    slot.slider->setValue(0.0, juce::dontSendNotification);
    slot.slider->setColour(juce::Slider::trackColourId, sliderColor);
    slot.slider->setColour(juce::Slider::backgroundColourId, kSurface);
    addAndMakeVisible(*slot.slider);

    slot.valueLabel = std::make_unique<juce::Label>("", "0.00");
    slot.valueLabel->setColour(juce::Label::textColourId, sliderColor);
    slot.valueLabel->setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(*slot.valueLabel);

    slot.slider->onValueChange = [&slot] {
        slot.valueLabel->setText(juce::String(slot.slider->getValue(), 2), juce::dontSendNotification);
    };
}

float AxesPanel::fs() const
{
    float topH = (getTopLevelComponent() != nullptr)
                     ? static_cast<float>(getTopLevelComponent()->getHeight()) : 800.0f;
    return juce::jlimit(12.0f, 22.0f, topH * 0.022f);
}

void AxesPanel::paint(juce::Graphics& g)
{
    // Draw color dots for semantic axis slots
    float f = fs();
    int dotSize = juce::roundToInt(f * 0.55f);
    for (size_t i = 0; i < semSlots.size(); ++i)
    {
        auto& slot = semSlots[i];
        auto dropBounds = slot.dropdown->getBounds();
        int dotX = dropBounds.getX() - dotSize - 4;
        int dotY = dropBounds.getCentreY() - dotSize / 2;
        g.setColour(kAxisColors[i]);
        g.fillEllipse(static_cast<float>(dotX), static_cast<float>(dotY),
                       static_cast<float>(dotSize), static_cast<float>(dotSize));
    }
}

void AxesPanel::layoutSlots(std::vector<AxisSlot>& slots, juce::Rectangle<int>& area, float f, int dotOffset)
{
    int rowH = juce::roundToInt(f * 1.4f);
    int sliderH = juce::roundToInt(f * 1.2f);
    int gap = juce::roundToInt(f * 0.2f);
    int valW = juce::roundToInt(f * 3.0f);

    for (auto& slot : slots)
    {
        bool active = slot.dropdown->getSelectedId() != 1; // 1 = "---"

        auto dropRow = area.removeFromTop(rowH);
        if (dotOffset > 0)
            dropRow.removeFromLeft(dotOffset); // space for color dot
        slot.dropdown->setBounds(dropRow);

        slot.slider->setVisible(active);
        slot.valueLabel->setVisible(active);

        if (active)
        {
            auto sliderRow = area.removeFromTop(sliderH);
            if (dotOffset > 0)
                sliderRow.removeFromLeft(dotOffset);
            slot.valueLabel->setFont(juce::FontOptions(f * 0.8f));
            slot.valueLabel->setBounds(sliderRow.removeFromRight(valW));
            slot.slider->setBounds(sliderRow);
        }
        area.removeFromTop(gap);
    }
}

void AxesPanel::resized()
{
    float w = static_cast<float>(getWidth());
    float h = static_cast<float>(getHeight());
    int pad = juce::roundToInt(w * 0.04f);
    auto area = getLocalBounds().reduced(pad, juce::roundToInt(h * 0.01f));
    float f = fs();
    int headerH = juce::roundToInt(f * 1.3f);
    int dotOffset = juce::roundToInt(f * 0.8f); // space for color dot

    semHeader.setFont(juce::FontOptions(f * 0.85f));
    semHeader.setBounds(area.removeFromTop(headerH));
    layoutSlots(semSlots, area, f * 0.75f, dotOffset);

    area.removeFromTop(juce::roundToInt(f * 0.5f));

    pcaHeader.setFont(juce::FontOptions(f * 0.85f));
    pcaHeader.setBounds(area.removeFromTop(headerH));
    layoutSlots(pcaSlots, area, f * 0.65f, 0); // no dots for PCA
}

std::map<juce::String, float> AxesPanel::getAxisValues() const
{
    std::map<juce::String, float> vals;
    for (auto& slot : semSlots)
    {
        if (slot.dropdown->getSelectedId() > 1)
            vals[slot.dropdown->getText()] = static_cast<float>(slot.slider->getValue());
    }
    for (auto& slot : pcaSlots)
    {
        if (slot.dropdown->getSelectedId() > 1)
            vals[slot.dropdown->getText()] = static_cast<float>(slot.slider->getValue());
    }
    return vals;
}
