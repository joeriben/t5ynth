#include "MainPanel.h"
#include "../PluginProcessor.h"
#include "GuiHelpers.h"
#include <thread>

MainPanel::MainPanel(T5ynthProcessor& processor)
    : processorRef(processor),
      promptPanel(processor),
      synthPanel(processor),
      fxPanel(processor.getValueTreeState()),
      presetPanel(processor),
      sequencerPanel(processor)
{
    addAndMakeVisible(promptPanel);
    addAndMakeVisible(axesPanel);
    addAndMakeVisible(synthPanel);
    addAndMakeVisible(fxPanel);
    addAndMakeVisible(sequencerPanel);
    addAndMakeVisible(statusBar);
    // PresetPanel is kept as logic handler but not shown — buttons are in StatusBar

    // Wire preset import callback
    presetPanel.onPresetLoaded = [this](const juce::String& pA, const juce::String& pB,
                                        int seed, const juce::String& device) {
        promptPanel.loadPresetData(pA, pB, seed, true, device);
    };

    // Master volume — vertical slider
    masterVolKnob.setSliderStyle(juce::Slider::LinearVertical);
    masterVolKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 14);
    masterVolKnob.setColour(juce::Slider::trackColourId, kAccent);
    masterVolKnob.setColour(juce::Slider::backgroundColourId, kSurface);
    masterVolKnob.setColour(juce::Slider::textBoxTextColourId, kDim);
    masterVolKnob.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    masterVolKnob.setTextValueSuffix(" dB");
    addAndMakeVisible(masterVolKnob);

    masterVolLabel.setText("Vol", juce::dontSendNotification);
    masterVolLabel.setColour(juce::Label::textColourId, kDim);
    masterVolLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(masterVolLabel);

    masterVolA = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getValueTreeState(), "master_vol", masterVolKnob);

    // Main Generate button at bottom of left column
    mainGenerateBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1b5e20));
    mainGenerateBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff4caf50));
    mainGenerateBtn.onClick = [this] {
        promptPanel.triggerGenerationWithOffsets({});
    };
    addAndMakeVisible(mainGenerateBtn);

    // Status callback — show in Generate button
    promptPanel.onStatusChanged = [this](const juce::String& text, bool isGenerating) {
        if (isGenerating)
        {
            mainGenerateBtn.setButtonText("generating...");
            mainGenerateBtn.setEnabled(false);
        }
        else
        {
            mainGenerateBtn.setButtonText("Re-Generate");
            mainGenerateBtn.setEnabled(true);
        }
    };

    // Scrim (click outside DimExplorer overlay to close)
    dimScrim.onClick = [this] { hideDimExplorer(); };
    dimScrim.setVisible(false);
    addChildComponent(dimScrim);

    // DimExplorer — always visible (mini-view in left column, overlay on click)
    addAndMakeVisible(dimensionExplorer);
    dimensionExplorer.onClicked = [this] {
        if (!dimExplorerVisible) showDimExplorer();
    };

    // Wire PromptPanel → DimensionExplorer (embedding stats after generation)
    promptPanel.onEmbeddingsReady = [this](const std::vector<float>& a, const std::vector<float>& b) {
        dimensionExplorer.setEmbeddings(a, b);
    };

    // "Anwenden + generieren" — green, triggers generation with offsets
    dimApplyBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1b5e20));
    dimApplyBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff4caf50));
    dimApplyBtn.onClick = [this] {
        auto offsets = dimensionExplorer.getDimensionOffsets();
        promptPanel.triggerGenerationWithOffsets(std::move(offsets));
    };
    dimApplyBtn.setVisible(false);
    addChildComponent(dimApplyBtn);

    dimUndoBtn.setColour(juce::TextButton::buttonColourId, kSurface);
    dimUndoBtn.setColour(juce::TextButton::textColourOffId, kDim);
    dimUndoBtn.onClick = [this] { dimensionExplorer.undo(); };
    dimUndoBtn.setVisible(false);
    addChildComponent(dimUndoBtn);

    dimRedoBtn.setColour(juce::TextButton::buttonColourId, kSurface);
    dimRedoBtn.setColour(juce::TextButton::textColourOffId, kDim);
    dimRedoBtn.onClick = [this] { dimensionExplorer.redo(); };
    dimRedoBtn.setVisible(false);
    addChildComponent(dimRedoBtn);

    // "Alle zurücksetzen" — orange
    dimResetBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4e2700));
    dimResetBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffff9800));
    dimResetBtn.onClick = [this] { dimensionExplorer.clear(); dimensionExplorer.repaint(); };
    dimResetBtn.setVisible(false);
    addChildComponent(dimResetBtn);

    // Load native inference models
    tryLoadInferenceModels();
}

void MainPanel::showDimExplorer()
{
    dimExplorerVisible = true;
    dimensionExplorer.setOverlayMode(true);
    dimScrim.setVisible(true);
    dimScrim.toFront(false);
    dimApplyBtn.setVisible(true);
    dimUndoBtn.setVisible(true);
    dimRedoBtn.setVisible(true);
    dimResetBtn.setVisible(true);
    dimensionExplorer.toFront(false);
    dimApplyBtn.toFront(false);
    dimUndoBtn.toFront(false);
    dimRedoBtn.toFront(false);
    dimResetBtn.toFront(false);
    resized();
    repaint();
}

void MainPanel::hideDimExplorer()
{
    dimExplorerVisible = false;
    dimensionExplorer.setOverlayMode(false);
    dimScrim.setVisible(false);
    dimApplyBtn.setVisible(false);
    dimUndoBtn.setVisible(false);
    dimRedoBtn.setVisible(false);
    dimResetBtn.setVisible(false);
    resized();  // repositions DimExplorer back to mini-view
    repaint();
}

void MainPanel::mouseDown(const juce::MouseEvent& e)
{
    // Close DimExplorer overlay on click outside
    if (dimExplorerVisible)
    {
        auto dimBounds = dimensionExplorer.getBounds();
        if (!dimBounds.contains(e.x, e.y))
            hideDimExplorer();
    }
}

void MainPanel::toggleSettings() {}

void MainPanel::tryLoadInferenceModels()
{
    // Try pipe inference first (Python subprocess — correct audio)
    statusBar.setStatusText("Loading inference...");

    auto* processor = &processorRef;

    // Find backend directory (contains pipe_inference.py)
    auto exe = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    juce::File backendDir;
    auto search = exe.getParentDirectory();
    for (int i = 0; i < 8; ++i)
    {
        if (search.getChildFile("backend/pipe_inference.py").existsAsFile())
        {
            backendDir = search.getChildFile("backend");
            break;
        }
        search = search.getParentDirectory();
    }

    if (backendDir.exists())
    {
        std::thread([this, processor, backendDir]()
        {
            bool ok = processor->launchPipeInference(backendDir);
            juce::MessageManager::callAsync([this, ok]()
            {
                if (ok)
                {
                    statusBar.setConnected(true);
                    statusBar.setStatusText("Ready (Python)");
                    settingsPage.setBackendConnected(true);
                }
                else
                {
                    statusBar.setStatusText("Python inference failed — trying native...");
                    tryLoadNativeInference();
                }
            });
        }).detach();
    }
    else
    {
        tryLoadNativeInference();
    }
}

void MainPanel::tryLoadNativeInference()
{
    // Fallback: try native LibTorch inference (deprecated, produces garbage)
    auto home = juce::File::getSpecialLocation(juce::File::userHomeDirectory);
    auto appData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);

    std::vector<juce::File> candidates = {
        appData.getChildFile("T5ynth/exported_models"),
        juce::File::getCurrentWorkingDirectory().getChildFile("exported_models"),
        home.getChildFile("t5ynth/exported_models"),
        home.getChildFile("ai/t5ynth/exported_models"),
    };

    for (auto& dir : candidates)
    {
        if (dir.getChildFile("dit.pt").existsAsFile())
        {
            statusBar.setStatusText("Loading native models...");
            auto* processor = &processorRef;
            auto modelDir = dir;
            std::thread([this, processor, modelDir]()
            {
                bool ok = processor->loadInferenceModels(modelDir);
                juce::MessageManager::callAsync([this, ok]()
                {
                    if (ok)
                    {
                        statusBar.setConnected(true);
                        statusBar.setStatusText("Ready (native)");
                        settingsPage.setBackendConnected(true);
                    }
                    else
                        statusBar.setStatusText("Model load failed");
                });
            }).detach();
            return;
        }
    }
    statusBar.setStatusText("No model — open Settings to download");
}

void MainPanel::paint(juce::Graphics& g)
{
    g.fillAll(kBg);

    float w = static_cast<float>(getWidth());
    float footerTop = static_cast<float>(sequencerPanel.getY());

    g.setColour(kBorder);
    float x1 = static_cast<float>(promptPanel.getRight() + 4);
    g.drawVerticalLine(juce::roundToInt(x1), 0.0f, footerTop);
    g.drawHorizontalLine(juce::roundToInt(footerTop), 0.0f, w);

    // ── Left column: ONE card "OSCILLATOR" covering everything ──
    int col1Left = promptPanel.getX();
    int col1Right = promptPanel.getRight();
    int col1Top = promptPanel.getY() - 22;
    int col1Bot = mainGenerateBtn.getBottom() + 4;
    int cardW = col1Right - col1Left;

    // Card background
    paintCard(g, juce::Rectangle<int>(col1Left, col1Top, cardW, col1Bot - col1Top));

    // Header bar — green like Generate button
    int headerH = 18;
    g.setColour(juce::Colour(0xff4caf50).withAlpha(0.7f));
    g.fillRect(col1Left + 1, col1Top + 1, cardW - 2, headerH);
    g.setColour(juce::Colour(0xff0e1018));
    g.setFont(juce::FontOptions(11.0f));
    g.drawText(" OSCILLATOR", col1Left + 4, col1Top, cardW, headerH, juce::Justification::centredLeft);
}

void MainPanel::resized()
{
    auto b = getLocalBounds();
    float w = static_cast<float>(b.getWidth());
    float h = static_cast<float>(b.getHeight());

    int statusH = 14;
    int footerH = juce::jlimit(160, 280, juce::roundToInt(h * 0.24f));
    statusBar.setBounds(b.removeFromBottom(statusH));

    // Gap between footer and main content
    b.removeFromBottom(6);

    // Footer
    auto footer = b.removeFromBottom(footerH);
    int volW = juce::jlimit(40, 60, juce::roundToInt(w * 0.05f));
    int fxW = juce::jlimit(180, 400, juce::roundToInt(w * 0.28f));
    auto volArea = footer.removeFromRight(volW);
    masterVolLabel.setFont(juce::FontOptions(10.0f));
    masterVolLabel.setBounds(volArea.removeFromTop(14));
    masterVolKnob.setBounds(volArea);
    footer.removeFromRight(6);  // gap Vol–FX
    fxPanel.setBounds(footer.removeFromRight(fxW));
    footer.removeFromRight(6);  // gap FX–Seq
    sequencerPanel.setBounds(footer);

    // ═══ Col 1: OSCILLATOR — one card, header at top ═══
    int col1W = juce::jlimit(240, 420, juce::roundToInt(w * 0.25f));
    auto genCol = b.removeFromLeft(col1W).reduced(6, 2);
    genCol.removeFromTop(20); // header bar space

    // Fixed heights — Prompt and Axes get exactly what they need, no more
    constexpr int kPromptH   = 310;
    constexpr int kAxesH     = 100;
    constexpr int kGenBtnH   = 34;
    constexpr int kGap       = 3;

    promptPanel.setBounds(genCol.removeFromTop(kPromptH));
    genCol.removeFromTop(kGap);

    axesPanel.setBounds(genCol.removeFromTop(kAxesH));
    genCol.removeFromTop(kGap);

    // Generate button at bottom (same width as other content)
    mainGenerateBtn.setBounds(genCol.removeFromBottom(kGenBtnH));
    genCol.removeFromBottom(kGap);

    // DimExplorer gets remaining
    if (!dimExplorerVisible)
        dimensionExplorer.setBounds(genCol);

    // Col 2: ENGINE
    synthPanel.setBounds(b);



    // Scrim covers everything
    dimScrim.setBounds(getLocalBounds());

    // DimExplorer overlay
    if (dimExplorerVisible)
    {
        auto overlayBounds = getLocalBounds().reduced(40);
        int btnH = 30;
        int applyW = 180;
        int smallW = 70;
        int resetW = 140;
        int btnGap = 10;

        auto btnArea = overlayBounds.removeFromBottom(btnH + 10);
        int totalBtnW = applyW + smallW * 2 + resetW + btnGap * 3;
        int startX = btnArea.getCentreX() - totalBtnW / 2;
        int y = btnArea.getY();

        dimApplyBtn.setBounds(startX, y, applyW, btnH);
        dimUndoBtn.setBounds(startX + applyW + btnGap, y, smallW, btnH);
        dimRedoBtn.setBounds(startX + applyW + smallW + btnGap * 2, y, smallW, btnH);
        dimResetBtn.setBounds(startX + applyW + smallW * 2 + btnGap * 3, y, resetW, btnH);

        dimensionExplorer.setBounds(overlayBounds.reduced(20, 10));
    }
}
