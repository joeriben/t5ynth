#include "MainPanel.h"
#include "../PluginProcessor.h"

MainPanel::MainPanel(T5ynthProcessor& processor)
    : processorRef(processor),
      promptPanel(processor),
      synthPanel(processor.getValueTreeState()),
      effectsPanel(processor.getValueTreeState()),
      sequencerPanel(processor)
{
    addAndMakeVisible(promptPanel);
    addAndMakeVisible(axesPanel);
    addAndMakeVisible(dimensionExplorer);
    addAndMakeVisible(synthPanel);
    addAndMakeVisible(effectsPanel);
    addAndMakeVisible(sequencerPanel);
    addAndMakeVisible(statusBar);

    // Wire status bar
    processorRef.getBackendManager().setStatusCallback(
        [this](BackendManager::Status s)
        {
            juce::MessageManager::callAsync([this, s]()
            {
                statusBar.setConnected(s == BackendManager::Status::Running);
                switch (s)
                {
                    case BackendManager::Status::Stopped:  statusBar.setStatusText("Backend stopped"); break;
                    case BackendManager::Status::Starting:  statusBar.setStatusText("Starting..."); break;
                    case BackendManager::Status::Running:   statusBar.setStatusText("Ready"); break;
                    case BackendManager::Status::Failed:    statusBar.setStatusText("Backend failed"); break;
                }
            });
        });

    processorRef.getBackendConnection().setConnectionCallback(
        [this](bool connected) { statusBar.setConnected(connected); });
}

void MainPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0a0a0a));

    // Column separators
    float h = static_cast<float>(getHeight());
    float w = static_cast<float>(getWidth());
    float bottomH = h * 0.16f + h * 0.03f; // seq + status

    g.setColour(juce::Colour(0xff1a1a1a));

    // Vertical separators between columns
    float col1 = w * 0.22f;
    float col2 = col1 + w * 0.22f;
    float col3 = col2 + w * 0.30f;
    float topArea = h - bottomH;
    g.drawVerticalLine(juce::roundToInt(col1), 0.0f, topArea);
    g.drawVerticalLine(juce::roundToInt(col2), 0.0f, topArea);
    g.drawVerticalLine(juce::roundToInt(col3), 0.0f, topArea);

    // Horizontal separator above sequencer
    g.drawHorizontalLine(juce::roundToInt(topArea), 0.0f, w);

    // Horizontal separator above status bar
    g.drawHorizontalLine(juce::roundToInt(h - h * 0.03f), 0.0f, w);
}

void MainPanel::resized()
{
    auto b = getLocalBounds();
    float w = static_cast<float>(b.getWidth());
    float h = static_cast<float>(b.getHeight());

    // Bottom: status bar (3% height) + sequencer (13% height)
    int statusH = juce::roundToInt(h * 0.03f);
    int seqH = juce::roundToInt(h * 0.13f);

    statusBar.setBounds(b.removeFromBottom(statusH));
    sequencerPanel.setBounds(b.removeFromBottom(seqH));

    // 4 columns: SOURCE 22% | EXPLORE 22% | OSC/ENV/MOD 30% | FILTER/FX 26%
    int col1W = juce::roundToInt(w * 0.22f);
    int col2W = juce::roundToInt(w * 0.22f);
    int col3W = juce::roundToInt(w * 0.30f);
    int col4W = b.getWidth() - col1W - col2W - col3W;

    promptPanel.setBounds(b.removeFromLeft(col1W));

    // EXPLORE column: axes on top (55%), dimension explorer below (45%)
    auto exploreCol = b.removeFromLeft(col2W);
    int axesH = juce::roundToInt(static_cast<float>(exploreCol.getHeight()) * 0.55f);
    axesPanel.setBounds(exploreCol.removeFromTop(axesH));
    dimensionExplorer.setBounds(exploreCol);

    synthPanel.setBounds(b.removeFromLeft(col3W));
    effectsPanel.setBounds(b);
}
