#include "MainPanel.h"
#include "../PluginProcessor.h"
#include "GuiHelpers.h"

MainPanel::MainPanel(T5ynthProcessor& processor)
    : processorRef(processor),
      promptPanel(processor),
      synthPanel(processor),
      fxPanel(processor.getValueTreeState()),
      sequencerPanel(processor)
{
    addAndMakeVisible(promptPanel);
    addAndMakeVisible(axesPanel);
    addAndMakeVisible(synthPanel);
    addAndMakeVisible(fxPanel);
    addAndMakeVisible(sequencerPanel);
    addAndMakeVisible(statusBar);

    // DimExplorer overlay — initially hidden
    dimensionExplorer.setVisible(false);
    addChildComponent(dimensionExplorer);

    dimExplorerClose.setColour(juce::TextButton::buttonColourId, kSurface);
    dimExplorerClose.setColour(juce::TextButton::textColourOffId, kAccent);
    dimExplorerClose.onClick = [this] { hideDimExplorer(); };
    dimExplorerClose.setVisible(false);
    addChildComponent(dimExplorerClose);

    dimExplorerReset.setColour(juce::TextButton::buttonColourId, kSurface);
    dimExplorerReset.setColour(juce::TextButton::textColourOffId, kDim);
    dimExplorerReset.onClick = [this] { dimensionExplorer.clear(); dimensionExplorer.repaint(); };
    dimExplorerReset.setVisible(false);
    addChildComponent(dimExplorerReset);

    // Wire up Explore button in SynthPanel
    synthPanel.onExploreClicked = [this] { showDimExplorer(); };

    // Backend status
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

void MainPanel::showDimExplorer()
{
    dimExplorerVisible = true;
    dimensionExplorer.setVisible(true);
    dimExplorerClose.setVisible(true);
    dimExplorerReset.setVisible(true);
    dimensionExplorer.toFront(false);
    dimExplorerClose.toFront(false);
    dimExplorerReset.toFront(false);
    resized();
    repaint();
}

void MainPanel::hideDimExplorer()
{
    dimExplorerVisible = false;
    dimensionExplorer.setVisible(false);
    dimExplorerClose.setVisible(false);
    dimExplorerReset.setVisible(false);
    repaint();
}

void MainPanel::paint(juce::Graphics& g)
{
    g.fillAll(kBg);

    float w = static_cast<float>(getWidth());
    float h = static_cast<float>(getHeight());
    float bottomH = h * 0.16f;
    float topH = h - bottomH;

    g.setColour(kSurface);

    // 3 column separators
    float x1 = w * 0.25f;
    float x2 = x1 + w * 0.55f;
    g.drawVerticalLine(juce::roundToInt(x1), 0.0f, topH);
    g.drawVerticalLine(juce::roundToInt(x2), 0.0f, topH);

    g.drawHorizontalLine(juce::roundToInt(topH), 0.0f, w);
    g.drawHorizontalLine(juce::roundToInt(h - h * 0.03f), 0.0f, w);

    // DimExplorer overlay background
    if (dimExplorerVisible)
    {
        g.setColour(juce::Colour(0xdd0a0a0a)); // semi-transparent dark
        g.fillRect(getLocalBounds());
    }
}

void MainPanel::resized()
{
    auto b = getLocalBounds();
    float w = static_cast<float>(b.getWidth());
    float h = static_cast<float>(b.getHeight());

    int statusH = juce::roundToInt(h * 0.03f);
    int seqH = juce::roundToInt(h * 0.13f);
    statusBar.setBounds(b.removeFromBottom(statusH));
    sequencerPanel.setBounds(b.removeFromBottom(seqH));

    // 3 columns: 25% | 55% | 20%
    int col1W = juce::roundToInt(w * 0.25f);
    int col2W = juce::roundToInt(w * 0.55f);
    int col3W = b.getWidth() - col1W - col2W;

    // Col 1: GENERATION
    auto genCol = b.removeFromLeft(col1W);
    int promptH = juce::roundToInt(static_cast<float>(genCol.getHeight()) * 0.60f);
    promptPanel.setBounds(genCol.removeFromTop(promptH));
    axesPanel.setBounds(genCol);

    // Col 2: ENGINE + FILTER + MODULATION
    synthPanel.setBounds(b.removeFromLeft(col2W));

    // Col 3: FX
    fxPanel.setBounds(b);

    // DimExplorer overlay
    if (dimExplorerVisible)
    {
        auto overlayBounds = getLocalBounds().reduced(40);
        int btnH = 30;
        int btnW = 80;
        int btnGap = 10;

        auto btnArea = overlayBounds.removeFromBottom(btnH + 10);
        int totalBtnW = btnW * 2 + btnGap;
        int startX = btnArea.getCentreX() - totalBtnW / 2;

        dimExplorerReset.setBounds(startX, btnArea.getY(), btnW, btnH);
        dimExplorerClose.setBounds(startX + btnW + btnGap, btnArea.getY(), btnW, btnH);

        dimensionExplorer.setBounds(overlayBounds.reduced(20, 10));
    }
}
