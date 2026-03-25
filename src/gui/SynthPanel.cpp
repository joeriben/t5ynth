#include "SynthPanel.h"

static void initKnob(juce::Slider& k, juce::Label& l, const juce::String& text, juce::Component* p)
{
    k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    k.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 1, 1);
    k.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff4a9eff));
    k.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff2a2a2a));
    p->addAndMakeVisible(k);
    l.setText(text, juce::dontSendNotification);
    l.setJustificationType(juce::Justification::centred);
    l.setColour(juce::Label::textColourId, juce::Colour(0xff888888));
    p->addAndMakeVisible(l);
}

SynthPanel::SynthPanel(juce::AudioProcessorValueTreeState& apvts)
{
    // Engine mode
    engineModeBox.addItemList({"Looper", "Wavetable"}, 1);
    addAndMakeVisible(engineModeBox);

    // Osc scan
    initKnob(scanKnob, scanLabel, "Scan", this);

    // Amp Env
    initKnob(ampA, ampAL, "A", this);
    initKnob(ampD, ampDL, "D", this);
    initKnob(ampS, ampSL, "S", this);
    initKnob(ampR, ampRL, "R", this);

    // Mod Env 1
    initKnob(mod1A, mod1AL, "A", this);
    initKnob(mod1D, mod1DL, "D", this);
    initKnob(mod1S, mod1SL, "S", this);
    initKnob(mod1R, mod1RL, "R", this);

    // Mod Env 2
    initKnob(mod2A, mod2AL, "A", this);
    initKnob(mod2D, mod2DL, "D", this);
    initKnob(mod2S, mod2SL, "S", this);
    initKnob(mod2R, mod2RL, "R", this);

    // LFO 1
    initKnob(lfo1Rate, lfo1RateL, "Rate", this);
    initKnob(lfo1Depth, lfo1DepthL, "Dep", this);
    lfo1Wave.addItemList({"Sin", "Tri", "Saw", "Sq", "S&H"}, 1);
    addAndMakeVisible(lfo1Wave);

    // LFO 2
    initKnob(lfo2Rate, lfo2RateL, "Rate", this);
    initKnob(lfo2Depth, lfo2DepthL, "Dep", this);
    lfo2Wave.addItemList({"Sin", "Tri", "Saw", "Sq", "S&H"}, 1);
    addAndMakeVisible(lfo2Wave);

    // Drift
    driftToggle.setColour(juce::ToggleButton::textColourId, juce::Colour(0xff888888));
    driftToggle.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xff4a9eff));
    addAndMakeVisible(driftToggle);
    for (auto* s : { &drift1Rate, &drift1Depth, &drift2Rate, &drift2Depth, &drift3Rate, &drift3Depth })
    {
        s->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s->setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff4a9eff));
        s->setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff2a2a2a));
        addAndMakeVisible(*s);
    }

    // Attachments
    engineModeAttach = std::make_unique<CA>(apvts, "engine_mode", engineModeBox);
    scanAttach = std::make_unique<SA>(apvts, "osc_scan", scanKnob);

    ampAA = std::make_unique<SA>(apvts, "amp_attack", ampA);
    ampDA = std::make_unique<SA>(apvts, "amp_decay", ampD);
    ampSA = std::make_unique<SA>(apvts, "amp_sustain", ampS);
    ampRA = std::make_unique<SA>(apvts, "amp_release", ampR);

    mod1AA = std::make_unique<SA>(apvts, "mod1_attack", mod1A);
    mod1DA = std::make_unique<SA>(apvts, "mod1_decay", mod1D);
    mod1SA = std::make_unique<SA>(apvts, "mod1_sustain", mod1S);
    mod1RA = std::make_unique<SA>(apvts, "mod1_release", mod1R);

    mod2AA = std::make_unique<SA>(apvts, "mod2_attack", mod2A);
    mod2DA = std::make_unique<SA>(apvts, "mod2_decay", mod2D);
    mod2SA = std::make_unique<SA>(apvts, "mod2_sustain", mod2S);
    mod2RA = std::make_unique<SA>(apvts, "mod2_release", mod2R);

    lfo1RateA  = std::make_unique<SA>(apvts, "lfo1_rate", lfo1Rate);
    lfo1DepthA = std::make_unique<SA>(apvts, "lfo1_depth", lfo1Depth);
    lfo1WaveA  = std::make_unique<CA>(apvts, "lfo1_wave", lfo1Wave);
    lfo2RateA  = std::make_unique<SA>(apvts, "lfo2_rate", lfo2Rate);
    lfo2DepthA = std::make_unique<SA>(apvts, "lfo2_depth", lfo2Depth);
    lfo2WaveA  = std::make_unique<CA>(apvts, "lfo2_wave", lfo2Wave);

    driftEnableA = std::make_unique<BA>(apvts, "drift_enabled", driftToggle);
    d1RA = std::make_unique<SA>(apvts, "drift1_rate", drift1Rate);
    d1DA = std::make_unique<SA>(apvts, "drift1_depth", drift1Depth);
    d2RA = std::make_unique<SA>(apvts, "drift2_rate", drift2Rate);
    d2DA = std::make_unique<SA>(apvts, "drift2_depth", drift2Depth);
    d3RA = std::make_unique<SA>(apvts, "drift3_rate", drift3Rate);
    d3DA = std::make_unique<SA>(apvts, "drift3_depth", drift3Depth);
}

void SynthPanel::paint(juce::Graphics& g)
{
    float h = static_cast<float>(getHeight());
    float w = static_cast<float>(getWidth());
    float fs = juce::jlimit(8.0f, 12.0f, h * 0.017f);

    // Section headers drawn directly
    g.setFont(juce::FontOptions(fs));
    g.setColour(juce::Colour(0xff888888));

    float pad = w * 0.04f;
    float y = h * 0.001f;
    auto hdrY = [&](float frac) { return juce::roundToInt(h * frac); };

    g.drawText("OSC", juce::roundToInt(pad), hdrY(0.01f), getWidth(), juce::roundToInt(fs + 4), juce::Justification::centredLeft);
    g.drawText("AMP ENV", juce::roundToInt(pad), hdrY(0.17f), getWidth(), juce::roundToInt(fs + 4), juce::Justification::centredLeft);
    g.drawText("MOD ENV 1", juce::roundToInt(pad), hdrY(0.33f), getWidth(), juce::roundToInt(fs + 4), juce::Justification::centredLeft);
    g.drawText("MOD ENV 2", juce::roundToInt(pad), hdrY(0.47f), getWidth(), juce::roundToInt(fs + 4), juce::Justification::centredLeft);
    g.drawText("LFO 1        LFO 2", juce::roundToInt(pad), hdrY(0.61f), getWidth(), juce::roundToInt(fs + 4), juce::Justification::centredLeft);
    g.drawText("DRIFT", juce::roundToInt(pad), hdrY(0.80f), getWidth(), juce::roundToInt(fs + 4), juce::Justification::centredLeft);

    // Thin separator lines
    g.setColour(juce::Colour(0xff1a1a1a));
    for (float frac : { 0.165f, 0.325f, 0.465f, 0.605f, 0.795f })
        g.drawHorizontalLine(hdrY(frac), pad, w - pad);
}

void SynthPanel::resized()
{
    float w = static_cast<float>(getWidth());
    float h = static_cast<float>(getHeight());
    int pad = juce::roundToInt(w * 0.04f);
    float fs = juce::jlimit(7.0f, 11.0f, h * 0.015f);

    auto placeKnob = [&](juce::Slider& knob, juce::Label& label, int x, int y, int dia)
    {
        int tbW = juce::roundToInt(dia * 0.85f);
        int tbH = juce::roundToInt(fs + 3.0f);
        knob.setBounds(x, y, dia, dia);
        knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, tbW, tbH);
        label.setFont(juce::FontOptions(fs));
        label.setBounds(x, y + dia, dia, juce::roundToInt(h * 0.02f));
    };

    // 4 knobs in a row helper
    auto placeEnvRow = [&](juce::Slider& a, juce::Label& al,
                           juce::Slider& d, juce::Label& dl,
                           juce::Slider& s, juce::Label& sl,
                           juce::Slider& r, juce::Label& rl,
                           float yFrac)
    {
        int rowY = juce::roundToInt(h * yFrac);
        int colW = (getWidth() - pad * 2) / 4;
        int dia = juce::jmin(juce::roundToInt(h * 0.10f), colW - 4);
        for (int i = 0; i < 4; ++i)
        {
            auto& knob = (i == 0) ? a : (i == 1) ? d : (i == 2) ? s : r;
            auto& lab  = (i == 0) ? al : (i == 1) ? dl : (i == 2) ? sl : rl;
            int cx = pad + i * colW + (colW - dia) / 2;
            placeKnob(knob, lab, cx, rowY, dia);
        }
    };

    // OSC section: engine mode + scan
    int oscY = juce::roundToInt(h * 0.04f);
    int modeW = juce::roundToInt(w * 0.45f);
    int modeH = juce::roundToInt(h * 0.035f);
    engineModeBox.setBounds(pad, oscY, modeW, modeH);

    int scanDia = juce::jmin(juce::roundToInt(h * 0.09f), juce::roundToInt(w * 0.25f));
    placeKnob(scanKnob, scanLabel, pad + modeW + juce::roundToInt(w * 0.05f), oscY, scanDia);

    // Amp Env
    placeEnvRow(ampA, ampAL, ampD, ampDL, ampS, ampSL, ampR, ampRL, 0.20f);

    // Mod Env 1
    placeEnvRow(mod1A, mod1AL, mod1D, mod1DL, mod1S, mod1SL, mod1R, mod1RL, 0.355f);

    // Mod Env 2
    placeEnvRow(mod2A, mod2AL, mod2D, mod2DL, mod2S, mod2SL, mod2R, mod2RL, 0.495f);

    // LFO 1 + LFO 2 side by side
    int lfoY = juce::roundToInt(h * 0.645f);
    int halfW = (getWidth() - pad * 2) / 2;
    int lfoDia = juce::jmin(juce::roundToInt(h * 0.08f), halfW / 3);
    int comboH = juce::roundToInt(h * 0.03f);

    auto placeLfo = [&](juce::Slider& rate, juce::Label& rateL,
                        juce::Slider& depth, juce::Label& depthL,
                        juce::ComboBox& wave, int xOff)
    {
        int x = pad + xOff;
        placeKnob(rate, rateL, x, lfoY, lfoDia);
        placeKnob(depth, depthL, x + lfoDia + 4, lfoY, lfoDia);
        wave.setBounds(x, lfoY + lfoDia + juce::roundToInt(h * 0.025f),
                       lfoDia * 2 + 4, comboH);
    };
    placeLfo(lfo1Rate, lfo1RateL, lfo1Depth, lfo1DepthL, lfo1Wave, 0);
    placeLfo(lfo2Rate, lfo2RateL, lfo2Depth, lfo2DepthL, lfo2Wave, halfW);

    // Drift
    int driftY = juce::roundToInt(h * 0.835f);
    int driftDia = juce::jmin(juce::roundToInt(h * 0.055f), (getWidth() - pad * 2) / 7);
    driftToggle.setBounds(pad, driftY, juce::roundToInt(w * 0.25f), juce::roundToInt(h * 0.03f));

    int dkY = driftY + juce::roundToInt(h * 0.035f);
    int dkX = pad;
    int dkStep = driftDia + 2;
    drift1Rate.setBounds(dkX, dkY, driftDia, driftDia);
    drift1Depth.setBounds(dkX + dkStep, dkY, driftDia, driftDia);
    dkX += dkStep * 2 + juce::roundToInt(w * 0.03f);
    drift2Rate.setBounds(dkX, dkY, driftDia, driftDia);
    drift2Depth.setBounds(dkX + dkStep, dkY, driftDia, driftDia);
    dkX += dkStep * 2 + juce::roundToInt(w * 0.03f);
    drift3Rate.setBounds(dkX, dkY, driftDia, driftDia);
    drift3Depth.setBounds(dkX + dkStep, dkY, driftDia, driftDia);
}
