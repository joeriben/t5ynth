#include "SequencerPanel.h"
#include "../PluginProcessor.h"

SequencerPanel::SequencerPanel(T5ynthProcessor& processor)
    : processorRef(processor)
{
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);

    modeBox.addItemList({"Seq", "Arp Up", "Arp Dn", "Arp UD", "Arp Rnd"}, 1);
    addAndMakeVisible(modeBox);

    // Linear slider rows instead of rotary knobs
    bpmRow = std::make_unique<SliderRow>("BPM", [](double v) {
        return juce::String(juce::roundToInt(v));
    });
    addAndMakeVisible(*bpmRow);

    octRow = std::make_unique<SliderRow>("Oct", [](double v) {
        return juce::String(juce::roundToInt(v));
    });
    addAndMakeVisible(*octRow);

    auto& apvts = processor.getValueTreeState();
    bpmAttach  = std::make_unique<SA>(apvts, "seq_bpm",     bpmRow->getSlider());
    octAttach  = std::make_unique<SA>(apvts, "arp_octaves", octRow->getSlider());
    modeAttach = std::make_unique<CA>(apvts, "arp_mode",    modeBox);

    bpmRow->updateValue();
    octRow->updateValue();

    for (int i = 0; i < 4; ++i)
        stepStates[static_cast<size_t>(i)] = true;
}

void SequencerPanel::paint(juce::Graphics& g)
{
    g.fillAll(kBg);

    for (int i = 0; i < numVisibleSteps; ++i)
    {
        auto r = getStepBounds(i);
        if (r.isEmpty()) continue;

        bool on = stepStates[static_cast<size_t>(i)];
        g.setColour(on ? juce::Colour(0xff2a2a2a) : juce::Colour(0xff131313));
        g.fillRect(r.reduced(1));

        if (i % 4 == 0)
        {
            g.setColour(juce::Colour(0xff333333));
            g.drawRect(r, 1);
        }
    }
}

void SequencerPanel::resized()
{
    float w = static_cast<float>(getWidth());
    float h = static_cast<float>(getHeight());
    int pad = juce::roundToInt(w * 0.01f);

    auto area = getLocalBounds().reduced(pad, juce::roundToInt(h * 0.05f));

    // Left controls strip (18% of width)
    int controlsW = juce::roundToInt(w * 0.18f);
    auto controls = area.removeFromLeft(controlsW);

    int rowH = juce::roundToInt(h * 0.28f);

    // Play / Stop
    auto btnRow = controls.removeFromTop(rowH);
    int btnW = btnRow.getWidth() / 2 - 2;
    playButton.setBounds(btnRow.removeFromLeft(btnW));
    btnRow.removeFromLeft(4);
    stopButton.setBounds(btnRow.removeFromLeft(btnW));

    controls.removeFromTop(2);
    modeBox.setBounds(controls.removeFromTop(rowH));

    // BPM + Oct as horizontal slider rows
    area.removeFromLeft(pad);
    int sliderStripW = juce::roundToInt(w * 0.15f);
    auto sliderStrip = area.removeFromLeft(sliderStripW);

    bpmRow->setBounds(sliderStrip.removeFromTop(juce::roundToInt(h * 0.45f)));
    octRow->setBounds(sliderStrip.removeFromTop(juce::roundToInt(h * 0.45f)));

    // Step grid: remaining space
    area.removeFromLeft(pad);
    gridArea = area;
}

void SequencerPanel::mouseDown(const juce::MouseEvent& e)
{
    for (int i = 0; i < numVisibleSteps; ++i)
    {
        if (getStepBounds(i).contains(e.getPosition()))
        {
            stepStates[static_cast<size_t>(i)] = !stepStates[static_cast<size_t>(i)];
            repaint();
            return;
        }
    }
}

juce::Rectangle<int> SequencerPanel::getStepBounds(int step) const
{
    if (gridArea.isEmpty() || gridArea.getWidth() < numVisibleSteps) return {};
    int stepW = gridArea.getWidth() / numVisibleSteps;
    return { gridArea.getX() + step * stepW, gridArea.getY(), stepW, gridArea.getHeight() };
}
