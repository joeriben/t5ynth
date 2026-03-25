#include "PromptPanel.h"
#include "../PluginProcessor.h"
#include "../backend/GenerationRequest.h"

PromptPanel::PromptPanel(T5ynthProcessor& processor)
    : processorRef(processor)
{
    headerLabel.setText("SOURCE", juce::dontSendNotification);
    headerLabel.setColour(juce::Label::textColourId, juce::Colour(0xff888888));
    addAndMakeVisible(headerLabel);

    // Prompt editors
    promptAEditor.setMultiLine(false);
    promptAEditor.setTextToShowWhenEmpty("prompt A", juce::Colour(0xff555555));
    promptAEditor.onReturnKey = [this] { triggerGeneration(); };
    addAndMakeVisible(promptAEditor);

    promptBEditor.setMultiLine(false);
    promptBEditor.setTextToShowWhenEmpty("prompt B", juce::Colour(0xff555555));
    addAndMakeVisible(promptBEditor);

    // Alpha: horizontal slider between prompts — the A↔B interpolation
    alphaSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    alphaSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    alphaSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xff4a9eff));
    alphaSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff1a1a1a));
    addAndMakeVisible(alphaSlider);

    // Magnitude + Noise: rotary knobs
    auto setupKnob = [this](juce::Slider& knob, juce::Label& label, const juce::String& text)
    {
        knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 1, 1);
        knob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff4a9eff));
        knob.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff2a2a2a));
        addAndMakeVisible(knob);
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colour(0xff888888));
        addAndMakeVisible(label);
    };
    setupKnob(magnitudeKnob, magnitudeLabel, "Mag");
    setupKnob(noiseKnob, noiseLabel, "Noise");

    // Tertiary: compact horizontal sliders
    auto setupCompact = [this](juce::Slider& slider, juce::Label& label, const juce::String& text)
    {
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 1, 1);
        slider.setColour(juce::Slider::trackColourId, juce::Colour(0xff3a3a3a));
        slider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff151515));
        addAndMakeVisible(slider);
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centredLeft);
        label.setColour(juce::Label::textColourId, juce::Colour(0xff666666));
        addAndMakeVisible(label);
    };
    setupCompact(durationSlider, durationLabel, "Dur");
    setupCompact(stepsSlider, stepsLabel, "Steps");
    setupCompact(cfgSlider, cfgLabel, "CFG");

    generateButton.onClick = [this] { triggerGeneration(); };
    addAndMakeVisible(generateButton);
    addAndMakeVisible(waveformDisplay);

    infoLabel.setColour(juce::Label::textColourId, juce::Colour(0xff888888));
    addAndMakeVisible(infoLabel);

    // APVTS attachments
    auto& apvts = processor.getValueTreeState();
    alphaAttach     = std::make_unique<Attachment>(apvts, "gen_alpha", alphaSlider);
    magnitudeAttach = std::make_unique<Attachment>(apvts, "gen_magnitude", magnitudeKnob);
    noiseAttach     = std::make_unique<Attachment>(apvts, "gen_noise", noiseKnob);
    durationAttach  = std::make_unique<Attachment>(apvts, "gen_duration", durationSlider);
    stepsAttach     = std::make_unique<Attachment>(apvts, "gen_steps", stepsSlider);
    cfgAttach       = std::make_unique<Attachment>(apvts, "gen_cfg", cfgSlider);
}

void PromptPanel::paint(juce::Graphics&)
{
}

void PromptPanel::resized()
{
    auto b = getLocalBounds();
    float w = static_cast<float>(b.getWidth());
    float h = static_cast<float>(b.getHeight());
    float pad = w * 0.04f;
    auto area = b.reduced(juce::roundToInt(pad));

    float topH = (getTopLevelComponent() != nullptr) ? static_cast<float>(getTopLevelComponent()->getHeight()) : 800.0f;
    float fs = juce::jlimit(10.0f, 16.0f, topH * 0.016f);
    float fsSmall = fs * 0.85f;
    headerLabel.setFont(juce::FontOptions(fs));
    infoLabel.setFont(juce::FontOptions(fsSmall));
    magnitudeLabel.setFont(juce::FontOptions(fsSmall));
    noiseLabel.setFont(juce::FontOptions(fsSmall));
    durationLabel.setFont(juce::FontOptions(fsSmall));
    stepsLabel.setFont(juce::FontOptions(fsSmall));
    cfgLabel.setFont(juce::FontOptions(fsSmall));

    int g = juce::roundToInt(h * 0.01f);  // gap
    int inputH = juce::roundToInt(h * 0.042f);

    // Header
    headerLabel.setBounds(area.removeFromTop(juce::roundToInt(h * 0.03f)));
    area.removeFromTop(g);

    // Prompt A
    promptAEditor.setBounds(area.removeFromTop(inputH));
    area.removeFromTop(g / 2);

    // Alpha slider (A↔B) — thin, full width
    alphaSlider.setBounds(area.removeFromTop(juce::roundToInt(h * 0.03f)));
    area.removeFromTop(g / 2);

    // Prompt B
    promptBEditor.setBounds(area.removeFromTop(inputH));
    area.removeFromTop(g);

    // Mag + Noise knobs side by side
    int knobH = juce::roundToInt(h * 0.14f);
    int labelH = juce::roundToInt(h * 0.025f);
    int knobDia = juce::jmin(knobH - labelH, juce::roundToInt(w * 0.38f));
    int tbW = juce::roundToInt(knobDia * 0.85f);
    int tbH = juce::roundToInt(fsSmall + 4.0f);
    {
        auto knobRow = area.removeFromTop(knobH);
        int colW = knobRow.getWidth() / 2;
        for (int i = 0; i < 2; ++i)
        {
            auto& knob = (i == 0) ? magnitudeKnob : noiseKnob;
            auto& label = (i == 0) ? magnitudeLabel : noiseLabel;
            auto cell = knobRow.removeFromLeft(colW);
            auto kr = cell.removeFromTop(knobDia).withSizeKeepingCentre(knobDia, knobDia);
            knob.setBounds(kr);
            knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, tbW, tbH);
            label.setBounds(cell.withHeight(labelH));
        }
    }
    area.removeFromTop(g);

    // Tertiary: Duration / Steps / CFG — compact rows
    int compactH = juce::roundToInt(h * 0.03f);
    int labelW = juce::roundToInt(w * 0.22f);
    int textBoxW = juce::roundToInt(w * 0.2f);
    auto layoutCompact = [&](juce::Label& label, juce::Slider& slider)
    {
        auto row = area.removeFromTop(compactH);
        label.setBounds(row.removeFromLeft(labelW));
        slider.setBounds(row);
        slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, textBoxW, compactH);
        area.removeFromTop(g / 3);
    };
    layoutCompact(durationLabel, durationSlider);
    layoutCompact(stepsLabel, stepsSlider);
    layoutCompact(cfgLabel, cfgSlider);
    area.removeFromTop(g);

    // Generate button
    generateButton.setBounds(area.removeFromTop(juce::roundToInt(h * 0.05f)));
    area.removeFromTop(g);

    // Info at bottom
    infoLabel.setBounds(area.removeFromBottom(juce::roundToInt(h * 0.025f)));
    area.removeFromBottom(g / 2);

    // Waveform fills remaining space
    waveformDisplay.setBounds(area);
}

void PromptPanel::triggerGeneration()
{
    if (generating) return;
    auto promptA = promptAEditor.getText().trim();
    if (promptA.isEmpty()) return;

    if (!processorRef.getBackendConnection().isConnected())
    {
        infoLabel.setText("not connected", juce::dontSendNotification);
        return;
    }

    generating = true;
    generateButton.setEnabled(false);
    infoLabel.setText("generating...", juce::dontSendNotification);

    GenerationRequest request;
    request.setPromptA(promptA);
    auto promptB = promptBEditor.getText().trim();
    if (promptB.isNotEmpty()) request.setPromptB(promptB);

    auto& apvts = processorRef.getValueTreeState();
    request.setAlpha(apvts.getRawParameterValue("gen_alpha")->load());
    request.setMagnitude(apvts.getRawParameterValue("gen_magnitude")->load());
    request.setNoiseSigma(apvts.getRawParameterValue("gen_noise")->load());
    request.setDurationSeconds(apvts.getRawParameterValue("gen_duration")->load());
    request.setSteps(static_cast<int>(apvts.getRawParameterValue("gen_steps")->load()));
    request.setCfgScale(apvts.getRawParameterValue("gen_cfg")->load());

    processorRef.getBackendConnection().requestGeneration(request,
        [this](BackendConnection::GenerationResult result)
        {
            generating = false;
            generateButton.setEnabled(true);
            if (result.success)
            {
                processorRef.loadGeneratedAudio(result.audioBuffer, result.sampleRate);
                if (result.audioBuffer.getNumSamples() > 0)
                    waveformDisplay.setWaveform(result.audioBuffer.getReadPointer(0),
                                                result.audioBuffer.getNumSamples());
                infoLabel.setText(juce::String(result.generationTimeMs / 1000.0f, 1) + "s | seed "
                                  + juce::String(result.seed), juce::dontSendNotification);
            }
            else
                infoLabel.setText(result.errorMessage, juce::dontSendNotification);
        });
}
