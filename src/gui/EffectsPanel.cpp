#include "EffectsPanel.h"

static void initRotary(juce::Slider& k, juce::Label& l, const juce::String& text, juce::Component* p)
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

EffectsPanel::EffectsPanel(juce::AudioProcessorValueTreeState& apvts)
{
    initRotary(cutoffKnob, cutoffLabel, "Cutoff", this);
    initRotary(resoKnob, resoLabel, "Reso", this);
    filterTypeBox.addItemList({"LP", "HP", "BP"}, 1);
    addAndMakeVisible(filterTypeBox);

    initRotary(delayTimeKnob, delayTimeLabel, "Time", this);
    initRotary(delayFbKnob, delayFbLabel, "FB", this);
    initRotary(delayMixKnob, delayMixLabel, "Mix", this);

    initRotary(reverbMixKnob, reverbMixLabel, "Mix", this);
    reverbIrBox.addItemList({"Bright", "Medium", "Dark"}, 1);
    addAndMakeVisible(reverbIrBox);

    initRotary(limThreshKnob, limThreshLabel, "Thresh", this);
    initRotary(limReleaseKnob, limReleaseLabel, "Rel", this);

    cutoffA    = std::make_unique<SA>(apvts, "filter_cutoff", cutoffKnob);
    resoA      = std::make_unique<SA>(apvts, "filter_resonance", resoKnob);
    filterTypeA= std::make_unique<CA>(apvts, "filter_type", filterTypeBox);
    delayTimeA = std::make_unique<SA>(apvts, "delay_time", delayTimeKnob);
    delayFbA   = std::make_unique<SA>(apvts, "delay_feedback", delayFbKnob);
    delayMixA  = std::make_unique<SA>(apvts, "delay_mix", delayMixKnob);
    reverbMixA = std::make_unique<SA>(apvts, "reverb_mix", reverbMixKnob);
    reverbIrA  = std::make_unique<CA>(apvts, "reverb_ir", reverbIrBox);
    limThreshA = std::make_unique<SA>(apvts, "limiter_thresh", limThreshKnob);
    limReleaseA= std::make_unique<SA>(apvts, "limiter_release", limReleaseKnob);
}

void EffectsPanel::paint(juce::Graphics& g)
{
    float h = static_cast<float>(getHeight());
    float w = static_cast<float>(getWidth());
    float fs = juce::jlimit(8.0f, 12.0f, h * 0.017f);
    float pad = w * 0.04f;

    g.setFont(juce::FontOptions(fs));
    g.setColour(juce::Colour(0xff888888));

    auto hdrY = [&](float frac) { return juce::roundToInt(h * frac); };
    g.drawText("FILTER", juce::roundToInt(pad), hdrY(0.01f), getWidth(), juce::roundToInt(fs + 4), juce::Justification::centredLeft);
    g.drawText("DELAY",  juce::roundToInt(pad), hdrY(0.30f), getWidth(), juce::roundToInt(fs + 4), juce::Justification::centredLeft);
    g.drawText("REVERB", juce::roundToInt(pad), hdrY(0.58f), getWidth(), juce::roundToInt(fs + 4), juce::Justification::centredLeft);
    g.drawText("LIMITER",juce::roundToInt(pad), hdrY(0.78f), getWidth(), juce::roundToInt(fs + 4), juce::Justification::centredLeft);

    g.setColour(juce::Colour(0xff1a1a1a));
    for (float frac : { 0.29f, 0.57f, 0.77f })
        g.drawHorizontalLine(hdrY(frac), pad, w - pad);
}

void EffectsPanel::resized()
{
    float w = static_cast<float>(getWidth());
    float h = static_cast<float>(getHeight());
    int pad = juce::roundToInt(w * 0.04f);
    float fs = juce::jlimit(7.0f, 11.0f, h * 0.015f);

    auto placeKnob = [&](juce::Slider& knob, juce::Label& label, int x, int y, int dia)
    {
        knob.setBounds(x, y, dia, dia);
        knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false,
                             juce::roundToInt(dia * 0.85f), juce::roundToInt(fs + 3.0f));
        label.setFont(juce::FontOptions(fs));
        label.setBounds(x, y + dia, dia, juce::roundToInt(h * 0.02f));
    };

    int usableW = getWidth() - pad * 2;

    // FILTER: cutoff (large), reso, type
    {
        int y = juce::roundToInt(h * 0.045f);
        int bigDia = juce::jmin(juce::roundToInt(h * 0.14f), usableW / 3);
        int smDia = juce::jmin(juce::roundToInt(h * 0.10f), usableW / 4);
        placeKnob(cutoffKnob, cutoffLabel, pad, y, bigDia);
        int x2 = pad + bigDia + 8;
        placeKnob(resoKnob, resoLabel, x2, y + (bigDia - smDia) / 2, smDia);
        filterTypeBox.setBounds(x2 + smDia + 8, y + (bigDia - juce::roundToInt(h * 0.035f)) / 2,
                                juce::roundToInt(w * 0.2f), juce::roundToInt(h * 0.035f));
    }

    // DELAY: time, fb, mix in a row
    {
        int y = juce::roundToInt(h * 0.34f);
        int colW = usableW / 3;
        int dia = juce::jmin(juce::roundToInt(h * 0.11f), colW - 4);
        placeKnob(delayTimeKnob, delayTimeLabel, pad + (colW - dia) / 2, y, dia);
        placeKnob(delayFbKnob, delayFbLabel, pad + colW + (colW - dia) / 2, y, dia);
        placeKnob(delayMixKnob, delayMixLabel, pad + colW * 2 + (colW - dia) / 2, y, dia);
    }

    // REVERB: mix + IR
    {
        int y = juce::roundToInt(h * 0.62f);
        int dia = juce::jmin(juce::roundToInt(h * 0.10f), usableW / 3);
        placeKnob(reverbMixKnob, reverbMixLabel, pad, y, dia);
        reverbIrBox.setBounds(pad + dia + 12, y + (dia - juce::roundToInt(h * 0.035f)) / 2,
                              juce::roundToInt(w * 0.3f), juce::roundToInt(h * 0.035f));
    }

    // LIMITER: thresh + release
    {
        int y = juce::roundToInt(h * 0.82f);
        int dia = juce::jmin(juce::roundToInt(h * 0.10f), usableW / 3);
        placeKnob(limThreshKnob, limThreshLabel, pad, y, dia);
        placeKnob(limReleaseKnob, limReleaseLabel, pad + dia + 12, y, dia);
    }
}
