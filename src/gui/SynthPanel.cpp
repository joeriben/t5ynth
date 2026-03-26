#include "SynthPanel.h"
#include "../PluginProcessor.h"

// ── Helper: format ms (integer) ──
static juce::String fmtMs(double v)  { return juce::String(juce::roundToInt(v)) + "ms"; }
static juce::String fmtF2(double v)  { return juce::String(v, 2); }
static juce::String fmtPct(double v) { return juce::String(juce::roundToInt(v * 100.0)) + "%"; }
static juce::String fmtHz(double v)  { return juce::String(juce::roundToInt(v)) + " Hz"; }
static juce::String fmtHzF1(double v){ return juce::String(v, 1) + " Hz"; }
static juce::String fmtHzF2(double v){ return juce::String(v, 2) + " Hz"; }

// ──────────────────────────────────────────────────────────────────────────────
// Envelope init
// ──────────────────────────────────────────────────────────────────────────────
void SynthPanel::initEnv(EnvSection& env, const juce::String& name, int defaultTarget,
                          const juce::String& aId, const juce::String& dId,
                          const juce::String& sId, const juce::String& rId,
                          const juce::String& amtId, const juce::String& loopId,
                          juce::AudioProcessorValueTreeState& apvts)
{
    env.header.setText(name, juce::dontSendNotification);
    env.header.setColour(juce::Label::textColourId, kDim);
    addAndMakeVisible(env.header);

    env.targetBox.addItemList({"DCA", "Filter", "Scan", "---"}, 1);
    env.targetBox.setSelectedId(defaultTarget, juce::dontSendNotification);
    env.targetBox.onChange = [this] { updateVisibility(); resized(); };
    addAndMakeVisible(env.targetBox);

    env.loopToggle.setColour(juce::ToggleButton::textColourId, kDim);
    env.loopToggle.setColour(juce::ToggleButton::tickColourId, kAccent);
    addAndMakeVisible(env.loopToggle);

    env.aRow   = std::make_unique<SliderRow>("A",   fmtMs);
    env.dRow   = std::make_unique<SliderRow>("D",   fmtMs);
    env.sRow   = std::make_unique<SliderRow>("S",   fmtF2);
    env.rRow   = std::make_unique<SliderRow>("R",   fmtMs);
    env.amtRow = std::make_unique<SliderRow>("Amt", fmtF2);

    for (auto* row : { env.aRow.get(), env.dRow.get(), env.sRow.get(),
                       env.rRow.get(), env.amtRow.get() })
        addAndMakeVisible(*row);

    env.aA   = std::make_unique<SA>(apvts, aId,   env.aRow->getSlider());
    env.dA   = std::make_unique<SA>(apvts, dId,   env.dRow->getSlider());
    env.sA   = std::make_unique<SA>(apvts, sId,   env.sRow->getSlider());
    env.rA   = std::make_unique<SA>(apvts, rId,   env.rRow->getSlider());
    env.amtA = std::make_unique<SA>(apvts, amtId, env.amtRow->getSlider());
    env.loopA = std::make_unique<BA>(apvts, loopId, env.loopToggle);

    // Trigger initial value display
    env.aRow->updateValue();
    env.dRow->updateValue();
    env.sRow->updateValue();
    env.rRow->updateValue();
    env.amtRow->updateValue();
}

// ──────────────────────────────────────────────────────────────────────────────
// LFO init
// ──────────────────────────────────────────────────────────────────────────────
void SynthPanel::initLfo(LfoSection& lfo, const juce::String& name,
                          const juce::String& rateId, const juce::String& depthId,
                          const juce::String& waveId, const juce::String& modeId,
                          juce::AudioProcessorValueTreeState& apvts)
{
    lfo.header.setText(name, juce::dontSendNotification);
    lfo.header.setColour(juce::Label::textColourId, kDim);
    addAndMakeVisible(lfo.header);

    lfo.targetBox.addItemList({"Filter", "Scan", "Alpha", "---"}, 1);
    lfo.targetBox.setSelectedId(4, juce::dontSendNotification);
    lfo.targetBox.onChange = [this] { updateVisibility(); resized(); };
    addAndMakeVisible(lfo.targetBox);

    lfo.waveBox.addItemList({"Sin", "Tri", "Saw", "Sq", "S&H"}, 1);
    addAndMakeVisible(lfo.waveBox);

    lfo.modeBox.addItemList({"Free", "Trig"}, 1);
    addAndMakeVisible(lfo.modeBox);

    lfo.rateRow  = std::make_unique<SliderRow>("Rate",  fmtHzF1);
    lfo.depthRow = std::make_unique<SliderRow>("Depth", fmtF2);
    addAndMakeVisible(*lfo.rateRow);
    addAndMakeVisible(*lfo.depthRow);

    lfo.rateA  = std::make_unique<SA>(apvts, rateId,  lfo.rateRow->getSlider());
    lfo.depthA = std::make_unique<SA>(apvts, depthId, lfo.depthRow->getSlider());
    lfo.waveA  = std::make_unique<CA>(apvts, waveId,  lfo.waveBox);
    lfo.modeA  = std::make_unique<CA>(apvts, modeId,  lfo.modeBox);

    lfo.rateRow->updateValue();
    lfo.depthRow->updateValue();
}

// ──────────────────────────────────────────────────────────────────────────────
// Drift init
// ──────────────────────────────────────────────────────────────────────────────
void SynthPanel::initDrift(DriftSection& drift, const juce::String& name,
                            const juce::String& rateId, const juce::String& depthId,
                            const juce::String& targetId, const juce::String& waveId,
                            juce::AudioProcessorValueTreeState& apvts)
{
    drift.header.setText(name, juce::dontSendNotification);
    drift.header.setColour(juce::Label::textColourId, kDim);
    addAndMakeVisible(drift.header);

    drift.targetBox.addItemList({"None", "Alpha", "Axis 1", "Axis 2", "Axis 3", "WT Scan"}, 1);
    drift.targetBox.onChange = [this] { updateVisibility(); resized(); };
    addAndMakeVisible(drift.targetBox);

    drift.waveBox.addItemList({"Sine", "Tri", "Saw", "Sq"}, 1);
    addAndMakeVisible(drift.waveBox);

    drift.rateRow  = std::make_unique<SliderRow>("Rate",  fmtHzF2);
    drift.depthRow = std::make_unique<SliderRow>("Depth", fmtF2);
    addAndMakeVisible(*drift.rateRow);
    addAndMakeVisible(*drift.depthRow);

    drift.rateA  = std::make_unique<SA>(apvts, rateId,   drift.rateRow->getSlider());
    drift.depthA = std::make_unique<SA>(apvts, depthId,  drift.depthRow->getSlider());
    drift.targetA = std::make_unique<CA>(apvts, targetId, drift.targetBox);
    drift.waveA   = std::make_unique<CA>(apvts, waveId,   drift.waveBox);

    drift.rateRow->updateValue();
    drift.depthRow->updateValue();
}

// ──────────────────────────────────────────────────────────────────────────────
// Constructor
// ──────────────────────────────────────────────────────────────────────────────
SynthPanel::SynthPanel(T5ynthProcessor& processor)
    : processorRef(processor)
{
    auto& apvts = processor.getValueTreeState();

    // ── Engine mode ──
    auto styleBtn = [](juce::TextButton& btn, bool on) {
        btn.setColour(juce::TextButton::buttonColourId, on ? juce::Colour(0xff1a2a3a) : kSurface);
        btn.setColour(juce::TextButton::buttonOnColourId, kAccent);
        btn.setColour(juce::TextButton::textColourOffId, kDim);
        btn.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        btn.setClickingTogglesState(true);
        btn.setRadioGroupId(1001);
        btn.setToggleState(on, juce::dontSendNotification);
    };
    styleBtn(looperBtn, true);
    styleBtn(wavetableBtn, false);
    addAndMakeVisible(looperBtn);
    addAndMakeVisible(wavetableBtn);

    engineModeHidden.addItemList({"Looper", "Wavetable"}, 1);
    engineModeHidden.onChange = [this] {
        bool isLooper = engineModeHidden.getSelectedId() == 1;
        looperBtn.setToggleState(isLooper, juce::dontSendNotification);
        wavetableBtn.setToggleState(!isLooper, juce::dontSendNotification);
    };
    looperBtn.onClick = [this] { engineModeHidden.setSelectedId(1); };
    wavetableBtn.onClick = [this] { engineModeHidden.setSelectedId(2); };
    engineModeA = std::make_unique<CA>(apvts, "engine_mode", engineModeHidden);

    addAndMakeVisible(waveformDisplay);

    // ── Loop mode ──
    auto styleLoopBtn = [](juce::TextButton& btn) {
        btn.setColour(juce::TextButton::buttonColourId, kSurface);
        btn.setColour(juce::TextButton::buttonOnColourId, kAccent);
        btn.setColour(juce::TextButton::textColourOffId, kDim);
        btn.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        btn.setClickingTogglesState(true);
        btn.setRadioGroupId(1002);
    };
    styleLoopBtn(oneshotBtn);
    styleLoopBtn(loopModeBtn);
    styleLoopBtn(pingpongBtn);
    loopModeBtn.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(oneshotBtn);
    addAndMakeVisible(loopModeBtn);
    addAndMakeVisible(pingpongBtn);

    loopModeHidden.addItemList({"One-shot", "Loop", "Ping-Pong"}, 1);
    loopModeHidden.onChange = [this] {
        int id = loopModeHidden.getSelectedId();
        oneshotBtn.setToggleState(id == 1, juce::dontSendNotification);
        loopModeBtn.setToggleState(id == 2, juce::dontSendNotification);
        pingpongBtn.setToggleState(id == 3, juce::dontSendNotification);
        updateVisibility();
        resized();
    };
    oneshotBtn.onClick  = [this] { loopModeHidden.setSelectedId(1); };
    loopModeBtn.onClick = [this] { loopModeHidden.setSelectedId(2); };
    pingpongBtn.onClick = [this] { loopModeHidden.setSelectedId(3); };
    loopModeA = std::make_unique<CA>(apvts, "loop_mode", loopModeHidden);

    // Crossfade
    crossfadeRow = std::make_unique<SliderRow>("Crossfade", fmtMs);
    addAndMakeVisible(*crossfadeRow);
    crossfadeA = std::make_unique<SA>(apvts, "crossfade_ms", crossfadeRow->getSlider());
    crossfadeRow->updateValue();

    // Normalize
    normalizeToggle.setColour(juce::ToggleButton::textColourId, kDim);
    normalizeToggle.setColour(juce::ToggleButton::tickColourId, kAccent);
    addAndMakeVisible(normalizeToggle);
    normalizeA = std::make_unique<BA>(apvts, "normalize", normalizeToggle);

    // ── Scan ──
    scanRow = std::make_unique<SliderRow>("Scan", fmtF2);
    addAndMakeVisible(*scanRow);
    scanHint.setText("Morph between frames (0 = start, 1 = end)", juce::dontSendNotification);
    scanHint.setColour(juce::Label::textColourId, kDimmer);
    addAndMakeVisible(scanHint);
    scanA = std::make_unique<SA>(apvts, "osc_scan", scanRow->getSlider());
    scanRow->updateValue();

    // ── Filter ──
    filterToggle.setColour(juce::ToggleButton::textColourId, kDim);
    filterToggle.setColour(juce::ToggleButton::tickColourId, kAccent);
    filterToggle.onClick = [this] { updateVisibility(); resized(); };
    addAndMakeVisible(filterToggle);
    filterEnableA = std::make_unique<BA>(apvts, "filter_enabled", filterToggle);

    filterTypeBox.addItemList({"LP", "HP", "BP"}, 1);
    addAndMakeVisible(filterTypeBox);
    filterSlopeBox.addItemList({"12dB", "24dB"}, 1);
    addAndMakeVisible(filterSlopeBox);

    cutoffRow    = std::make_unique<SliderRow>("Cutoff",    fmtHz);
    resoRow      = std::make_unique<SliderRow>("Resonance", fmtF2);
    filterMixRow = std::make_unique<SliderRow>("Mix",       fmtPct);
    kbdTrackRow  = std::make_unique<SliderRow>("Kbd Track", fmtPct);
    for (auto* r : { cutoffRow.get(), resoRow.get(), filterMixRow.get(), kbdTrackRow.get() })
        addAndMakeVisible(*r);

    cutoffA    = std::make_unique<SA>(apvts, "filter_cutoff",    cutoffRow->getSlider());
    resoA      = std::make_unique<SA>(apvts, "filter_resonance", resoRow->getSlider());
    filterMixA = std::make_unique<SA>(apvts, "filter_mix",       filterMixRow->getSlider());
    kbdTrackA  = std::make_unique<SA>(apvts, "filter_kbd_track", kbdTrackRow->getSlider());

    filterTypeA  = std::make_unique<CA>(apvts, "filter_type",  filterTypeBox);
    filterSlopeA = std::make_unique<CA>(apvts, "filter_slope", filterSlopeBox);

    cutoffRow->updateValue();
    resoRow->updateValue();
    filterMixRow->updateValue();
    kbdTrackRow->updateValue();

    // ── Envelopes ──
    initEnv(ampEnv,  "ENV 1", 1, "amp_attack",  "amp_decay",  "amp_sustain",  "amp_release",  "amp_amount",  "amp_loop",  apvts);
    initEnv(mod1Env, "ENV 2", 4, "mod1_attack", "mod1_decay", "mod1_sustain", "mod1_release", "mod1_amount", "mod1_loop", apvts);
    initEnv(mod2Env, "ENV 3", 4, "mod2_attack", "mod2_decay", "mod2_sustain", "mod2_release", "mod2_amount", "mod2_loop", apvts);

    // ── LFOs ──
    initLfo(lfo1, "LFO 1", "lfo1_rate", "lfo1_depth", "lfo1_wave", "lfo1_mode", apvts);
    initLfo(lfo2, "LFO 2", "lfo2_rate", "lfo2_depth", "lfo2_wave", "lfo2_mode", apvts);

    // ── Drift ──
    initDrift(drift1, "DRIFT 1", "drift1_rate", "drift1_depth", "drift1_target", "drift1_wave", apvts);
    initDrift(drift2, "DRIFT 2", "drift2_rate", "drift2_depth", "drift2_target", "drift2_wave", apvts);

    // ── Explore button ──
    exploreBtn.setColour(juce::TextButton::buttonColourId, kSurface);
    exploreBtn.setColour(juce::TextButton::textColourOffId, kAccent);
    exploreBtn.onClick = [this] { if (onExploreClicked) onExploreClicked(); };
    addAndMakeVisible(exploreBtn);

    updateVisibility();
}

// ──────────────────────────────────────────────────────────────────────────────
// Visibility
// ──────────────────────────────────────────────────────────────────────────────
void SynthPanel::updateVisibility()
{
    bool filterOn = filterToggle.getToggleState();
    filterTypeBox.setVisible(filterOn);
    filterSlopeBox.setVisible(filterOn);
    for (auto* r : { cutoffRow.get(), resoRow.get(), filterMixRow.get(), kbdTrackRow.get() })
        r->setVisible(filterOn);

    bool isOneshot = loopModeHidden.getSelectedId() == 1;
    crossfadeRow->setVisible(!isOneshot);

    auto setEnvVisible = [](EnvSection& env) {
        bool active = env.targetBox.getSelectedId() != 4;
        env.loopToggle.setVisible(active);
        for (auto* r : { env.aRow.get(), env.dRow.get(), env.sRow.get(),
                         env.rRow.get(), env.amtRow.get() })
            if (r) r->setVisible(active);
    };
    setEnvVisible(ampEnv);
    setEnvVisible(mod1Env);
    setEnvVisible(mod2Env);

    auto setLfoVisible = [](LfoSection& lfo) {
        bool active = lfo.targetBox.getSelectedId() != 4;
        lfo.waveBox.setVisible(active);
        lfo.modeBox.setVisible(active);
        if (lfo.rateRow)  lfo.rateRow->setVisible(active);
        if (lfo.depthRow) lfo.depthRow->setVisible(active);
    };
    setLfoVisible(lfo1);
    setLfoVisible(lfo2);

    auto setDriftVisible = [](DriftSection& drift) {
        bool active = drift.targetBox.getSelectedId() != 1; // 1 = "None"
        drift.waveBox.setVisible(active);
        if (drift.rateRow)  drift.rateRow->setVisible(active);
        if (drift.depthRow) drift.depthRow->setVisible(active);
    };
    setDriftVisible(drift1);
    setDriftVisible(drift2);
}

float SynthPanel::fs() const
{
    float topH = (getTopLevelComponent() != nullptr)
                     ? static_cast<float>(getTopLevelComponent()->getHeight()) : 800.0f;
    return juce::jlimit(12.0f, 22.0f, topH * 0.022f);
}

// ──────────────────────────────────────────────────────────────────────────────
// Layout helpers
// ──────────────────────────────────────────────────────────────────────────────
void SynthPanel::layoutEnv(EnvSection& env, juce::Rectangle<int>& area, float f, int rowH, int gap)
{
    env.header.setFont(juce::FontOptions(f));
    auto hdr = area.removeFromTop(rowH);
    int headerW = juce::roundToInt(hdr.getWidth() * 0.18f);
    int targetW = juce::roundToInt(hdr.getWidth() * 0.28f);
    env.header.setBounds(hdr.removeFromLeft(headerW));
    env.targetBox.setBounds(hdr.removeFromLeft(targetW));
    hdr.removeFromLeft(4);
    env.loopToggle.setBounds(hdr.removeFromLeft(juce::roundToInt(hdr.getWidth() * 0.4f)));

    bool active = env.targetBox.getSelectedId() != 4;
    if (active)
    {
        // A | D as pair
        auto adRow = area.removeFromTop(rowH);
        int colW = (adRow.getWidth() - 4) / 2;
        env.aRow->setBounds(adRow.removeFromLeft(colW));
        adRow.removeFromLeft(4);
        env.dRow->setBounds(adRow);

        // S | R as pair
        auto srRow = area.removeFromTop(rowH);
        env.sRow->setBounds(srRow.removeFromLeft(colW));
        srRow.removeFromLeft(4);
        env.rRow->setBounds(srRow);

        // Amt full width
        env.amtRow->setBounds(area.removeFromTop(rowH));
    }
    area.removeFromTop(gap);
}

void SynthPanel::layoutLfo(LfoSection& lfo, juce::Rectangle<int>& area, float f, int rowH, int gap)
{
    lfo.header.setFont(juce::FontOptions(f));
    auto hdr = area.removeFromTop(rowH);
    int headerW = juce::roundToInt(hdr.getWidth() * 0.18f);
    int targetW = juce::roundToInt(hdr.getWidth() * 0.22f);
    int waveW = juce::roundToInt(hdr.getWidth() * 0.18f);
    int modeW = juce::roundToInt(hdr.getWidth() * 0.15f);

    lfo.header.setBounds(hdr.removeFromLeft(headerW));
    lfo.targetBox.setBounds(hdr.removeFromLeft(targetW));
    hdr.removeFromLeft(4);
    lfo.waveBox.setBounds(hdr.removeFromLeft(waveW));
    hdr.removeFromLeft(4);
    lfo.modeBox.setBounds(hdr.removeFromLeft(modeW));

    bool active = lfo.targetBox.getSelectedId() != 4;
    if (active)
    {
        int colW = (area.getWidth() - 4) / 2;
        auto slRow = area.removeFromTop(rowH);
        lfo.rateRow->setBounds(slRow.removeFromLeft(colW));
        slRow.removeFromLeft(4);
        lfo.depthRow->setBounds(slRow);
    }
    area.removeFromTop(gap);
}

void SynthPanel::layoutDrift(DriftSection& drift, juce::Rectangle<int>& area, float f, int rowH, int gap)
{
    drift.header.setFont(juce::FontOptions(f));
    auto hdr = area.removeFromTop(rowH);
    int headerW = juce::roundToInt(hdr.getWidth() * 0.22f);
    int targetW = juce::roundToInt(hdr.getWidth() * 0.28f);
    int waveW = juce::roundToInt(hdr.getWidth() * 0.18f);

    drift.header.setBounds(hdr.removeFromLeft(headerW));
    drift.targetBox.setBounds(hdr.removeFromLeft(targetW));
    hdr.removeFromLeft(4);
    drift.waveBox.setBounds(hdr.removeFromLeft(waveW));

    bool active = drift.targetBox.getSelectedId() != 1; // 1 = "None"
    if (active)
    {
        int colW = (area.getWidth() - 4) / 2;
        auto slRow = area.removeFromTop(rowH);
        drift.rateRow->setBounds(slRow.removeFromLeft(colW));
        slRow.removeFromLeft(4);
        drift.depthRow->setBounds(slRow);
    }
    area.removeFromTop(gap);
}

// ──────────────────────────────────────────────────────────────────────────────
// Paint
// ──────────────────────────────────────────────────────────────────────────────
void SynthPanel::paint(juce::Graphics& g)
{
    g.fillAll(kBg);
}

// ──────────────────────────────────────────────────────────────────────────────
// Resized
// ──────────────────────────────────────────────────────────────────────────────
void SynthPanel::resized()
{
    updateVisibility();

    float w = static_cast<float>(getWidth());
    float h = static_cast<float>(getHeight());
    int padX = juce::roundToInt(w * 0.02f);
    int padY = juce::roundToInt(h * 0.005f);
    auto area = getLocalBounds().reduced(padX, padY);
    float f = fs();
    int rowH = juce::roundToInt(f * 1.4f);
    int gap = juce::roundToInt(f * 0.25f);

    // ── Engine mode: horizontal switch ──
    auto modeRow = area.removeFromTop(juce::roundToInt(f * 1.8f));
    int third = modeRow.getWidth() / 2;
    looperBtn.setBounds(modeRow.removeFromLeft(third));
    wavetableBtn.setBounds(modeRow);
    area.removeFromTop(gap);

    // ── Waveform ──
    int waveH = juce::roundToInt(h * 0.12f);
    waveformDisplay.setBounds(area.removeFromTop(waveH));
    area.removeFromTop(gap);

    // ── Loop mode ──
    auto loopRow = area.removeFromTop(rowH);
    int btnW = loopRow.getWidth() / 3;
    oneshotBtn.setBounds(loopRow.removeFromLeft(btnW));
    loopModeBtn.setBounds(loopRow.removeFromLeft(btnW));
    pingpongBtn.setBounds(loopRow);
    area.removeFromTop(gap);

    // Crossfade (hidden if oneshot)
    if (crossfadeRow->isVisible())
    {
        crossfadeRow->setBounds(area.removeFromTop(rowH));
        area.removeFromTop(gap);
    }

    // Normalize
    normalizeToggle.setBounds(area.removeFromTop(rowH));
    area.removeFromTop(gap);

    // ── Scan ──
    scanRow->setBounds(area.removeFromTop(rowH));
    scanHint.setFont(juce::FontOptions(f * 0.7f));
    scanHint.setBounds(area.removeFromTop(juce::roundToInt(f * 0.9f)));
    area.removeFromTop(gap);

    // ── Filter ──
    auto filterHdr = area.removeFromTop(rowH);
    filterToggle.setBounds(filterHdr.removeFromLeft(juce::roundToInt(w * 0.18f)));
    if (filterToggle.getToggleState())
    {
        filterTypeBox.setBounds(filterHdr.removeFromLeft(juce::roundToInt(w * 0.10f)));
        filterHdr.removeFromLeft(4);
        filterSlopeBox.setBounds(filterHdr.removeFromLeft(juce::roundToInt(w * 0.10f)));
    }
    area.removeFromTop(gap);

    if (filterToggle.getToggleState())
    {
        // Cutoff | Resonance
        int colW = (area.getWidth() - 4) / 2;
        auto row1 = area.removeFromTop(rowH);
        cutoffRow->setBounds(row1.removeFromLeft(colW));
        row1.removeFromLeft(4);
        resoRow->setBounds(row1);

        // Mix | Kbd Track
        auto row2 = area.removeFromTop(rowH);
        filterMixRow->setBounds(row2.removeFromLeft(colW));
        row2.removeFromLeft(4);
        kbdTrackRow->setBounds(row2);
        area.removeFromTop(gap);
    }

    // ── Envelopes ──
    layoutEnv(ampEnv,  area, f, rowH, gap);
    layoutEnv(mod1Env, area, f, rowH, gap);
    layoutEnv(mod2Env, area, f, rowH, gap);

    // ── LFOs ──
    layoutLfo(lfo1, area, f, rowH, gap);
    layoutLfo(lfo2, area, f, rowH, gap);

    // ── Drift ──
    layoutDrift(drift1, area, f, rowH, gap);
    layoutDrift(drift2, area, f, rowH, gap);

    // ── Explore button ──
    exploreBtn.setBounds(area.removeFromTop(juce::roundToInt(f * 1.6f)));
}
