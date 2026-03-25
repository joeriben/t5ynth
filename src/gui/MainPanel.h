#pragma once
#include <JuceHeader.h>
#include "PromptPanel.h"
#include "AxesPanel.h"
#include "DimensionExplorer.h"
#include "SynthPanel.h"
#include "EffectsPanel.h"
#include "SequencerPanel.h"
#include "StatusBar.h"

class T5ynthProcessor;

class MainPanel : public juce::Component
{
public:
    explicit MainPanel(T5ynthProcessor& processor);
    ~MainPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    T5ynthProcessor& processorRef;

    // 4 columns: SOURCE | EXPLORE | OSC/ENV/MOD | FILTER/FX
    PromptPanel promptPanel;
    AxesPanel axesPanel;
    DimensionExplorer dimensionExplorer;
    SynthPanel synthPanel;
    EffectsPanel effectsPanel;

    // Bottom
    SequencerPanel sequencerPanel;
    StatusBar statusBar;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainPanel)
};
