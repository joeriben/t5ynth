#include "FxPanel.h"
#include "../dsp/BlockParams.h"
#include "../PluginProcessor.h"

static juce::String fmtMs(double v)
{
    int ms = juce::roundToInt(v);
    if (ms >= 1000) return juce::String(v / 1000.0, 2) + "s";
    return juce::String(ms) + "ms";
}
static juce::String fmtF2(double v)  { return juce::String(v, 2); }
static juce::String fmtF3(double v)  { return juce::String(v, 3); }

static juce::String fmtDampHz(double v)
{
    // Must match DSP mapping in DelayLine::setDamp: freq = 20000 * pow(500/20000, d)
    // 0 = bright (20kHz cutoff), 1 = dark (500Hz cutoff)
    double hz = 20000.0 * std::pow(500.0 / 20000.0, v);
    if (hz >= 1000.0) return juce::String(hz / 1000.0, 1) + "k";
    return juce::String(juce::roundToInt(hz)) + "Hz";
}

FxPanel::FxPanel(juce::AudioProcessorValueTreeState& apvts, T5ynthProcessor& processor)
    : processorRef(processor)
{
    // ══════════ DELAY section ══════════
    paintSectionHeader(delayHeader, "DELAY", kFxCol);
    addAndMakeVisible(delayHeader);

    // Delay type switchbox: OFF / Stereo
    juce::StringArray delayTypeItems;
    for (const auto& e : DelayType::kEntries) delayTypeItems.add(e.label);
    delayTypeHidden.addItemList(delayTypeItems, 1);
    delayTypeHidden.onChange = [this] {
        int id = delayTypeHidden.getSelectedId();
        for (int i = 0; i < kNumDelayBtns; ++i)
            delayTypeBtns[i].setToggleState(i + 1 == id, juce::dontSendNotification);
        updateVisibility();
    };

    static const char* delayLabels[] = {"OFF", "Stereo"};
    for (int i = 0; i < kNumDelayBtns; ++i)
    {
        delayTypeBtns[i].setButtonText(delayLabels[i]);
        delayTypeBtns[i].setColour(juce::TextButton::buttonColourId, kSurface);
        delayTypeBtns[i].setColour(juce::TextButton::buttonOnColourId, kFxCol);
        delayTypeBtns[i].setColour(juce::TextButton::textColourOffId, kDim);
        delayTypeBtns[i].setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        delayTypeBtns[i].setClickingTogglesState(true);
        delayTypeBtns[i].setRadioGroupId(4001);
        delayTypeBtns[i].onClick = [this, i] { delayTypeHidden.setSelectedId(i + 1); };
        addAndMakeVisible(delayTypeBtns[i]);
    }

    delayTimeRow = std::make_unique<SliderRow>("Time", fmtMs, kFxCol);
    delayFbRow   = std::make_unique<SliderRow>("FB",   fmtF2, kFxCol);
    delayDampRow = std::make_unique<SliderRow>("Damp", fmtDampHz, kFxCol);
    delayMixRow  = std::make_unique<SliderRow>("Mix",  fmtF3, kFxCol);

    for (auto* r : { delayTimeRow.get(), delayFbRow.get(), delayDampRow.get(), delayMixRow.get() })
        addAndMakeVisible(*r);

    delayTimeA = std::make_unique<SA>(apvts, PID::delayTime,     delayTimeRow->getSlider());
    delayFbA   = std::make_unique<SA>(apvts, PID::delayFeedback, delayFbRow->getSlider());
    delayDampA = std::make_unique<SA>(apvts, PID::delayDamp,     delayDampRow->getSlider());
    delayMixA  = std::make_unique<SA>(apvts, PID::delayMix,      delayMixRow->getSlider());

    delayTimeRow->updateValue();
    delayFbRow->updateValue();
    delayDampRow->updateValue();
    delayMixRow->updateValue();

    // Attach APVTS AFTER buttons are set up (triggers onChange → updateVisibility)
    delayTypeA = std::make_unique<CA>(apvts, PID::delayType, delayTypeHidden);

    // ══════════ REVERB section ══════════
    paintSectionHeader(reverbHeader, "REVERB", kFxCol);
    addAndMakeVisible(reverbHeader);

    // Reverb type switchbox: OFF / Dark / Med / Brt / Algo
    juce::StringArray reverbTypeItems;
    for (const auto& e : ReverbType::kEntries) reverbTypeItems.add(e.label);
    reverbTypeHidden.addItemList(reverbTypeItems, 1);
    reverbTypeHidden.onChange = [this] {
        int id = reverbTypeHidden.getSelectedId();
        for (int i = 0; i < kNumReverbBtns; ++i)
            reverbTypeBtns[i].setToggleState(i + 1 == id, juce::dontSendNotification);
        updateVisibility();
    };

    static const char* reverbLabels[] = {"OFF", "Drk", "Med", "Brt", "Algo"};
    for (int i = 0; i < kNumReverbBtns; ++i)
    {
        reverbTypeBtns[i].setButtonText(reverbLabels[i]);
        reverbTypeBtns[i].setColour(juce::TextButton::buttonColourId, kSurface);
        reverbTypeBtns[i].setColour(juce::TextButton::buttonOnColourId, kFxCol);
        reverbTypeBtns[i].setColour(juce::TextButton::textColourOffId, kDim);
        reverbTypeBtns[i].setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        reverbTypeBtns[i].setClickingTogglesState(true);
        reverbTypeBtns[i].setRadioGroupId(4002);
        reverbTypeBtns[i].onClick = [this, i] { reverbTypeHidden.setSelectedId(i + 1); };
        addAndMakeVisible(reverbTypeBtns[i]);
    }

    reverbMixRow  = std::make_unique<SliderRow>("Mix",   fmtF3, kFxCol);
    algoRoomRow   = std::make_unique<SliderRow>("Room",  fmtF2, kFxCol);
    algoDampRow   = std::make_unique<SliderRow>("Damp",  fmtF2, kFxCol);
    algoWidthRow  = std::make_unique<SliderRow>("Width", fmtF2, kFxCol);

    for (auto* r : { reverbMixRow.get(), algoRoomRow.get(), algoDampRow.get(), algoWidthRow.get() })
        addAndMakeVisible(*r);

    reverbMixA = std::make_unique<SA>(apvts, PID::reverbMix,   reverbMixRow->getSlider());
    algoRoomA  = std::make_unique<SA>(apvts, PID::algoRoom,    algoRoomRow->getSlider());
    algoDampA  = std::make_unique<SA>(apvts, PID::algoDamping, algoDampRow->getSlider());
    algoWidthA = std::make_unique<SA>(apvts, PID::algoWidth,   algoWidthRow->getSlider());

    reverbMixRow->updateValue();
    algoRoomRow->updateValue();
    algoDampRow->updateValue();
    algoWidthRow->updateValue();

    // Attach APVTS AFTER buttons are set up
    reverbTypeA = std::make_unique<CA>(apvts, PID::reverbType, reverbTypeHidden);

    startTimerHz(30); // ghost slider updates
}

void FxPanel::timerCallback()
{
    if (processorRef.audioIdle.load(std::memory_order_relaxed)) return;
    auto& mv = processorRef.modulatedValues;
    delayTimeRow->setGhostValue(mv.delayTime.load(std::memory_order_relaxed));
    delayFbRow->setGhostValue(mv.delayFeedback.load(std::memory_order_relaxed));
    delayMixRow->setGhostValue(mv.delayMix.load(std::memory_order_relaxed));
    reverbMixRow->setGhostValue(mv.reverbMix.load(std::memory_order_relaxed));

    for (auto* r : { delayTimeRow.get(), delayFbRow.get(), delayMixRow.get(), reverbMixRow.get() })
        r->tickGhost();
}

void FxPanel::updateVisibility()
{
    // Guard: called by APVTS attachment before all components are created
    if (!reverbMixRow)
        return;

    constexpr float dimAlpha = 0.3f;

    // Delay: always visible, dimmed when OFF
    bool delayOn = delayTypeHidden.getSelectedId() > 1;
    float delayAlpha = delayOn ? 1.0f : dimAlpha;
    for (auto* r : { delayTimeRow.get(), delayFbRow.get(), delayDampRow.get(), delayMixRow.get() })
    {
        r->setAlpha(delayAlpha);
        r->setEnabled(delayOn);
    }

    // Reverb: always visible; dim params based on mode
    bool reverbOn = reverbTypeHidden.getSelectedId() > 1;
    bool algoOn   = reverbTypeHidden.getSelectedId() == 5;
    float reverbAlpha = reverbOn ? 1.0f : dimAlpha;
    // Room/Damp/Width: active only for Algo, dimmed for Convolution and OFF
    float algoParamAlpha = algoOn ? 1.0f : dimAlpha;
    for (auto* r : { algoRoomRow.get(), algoDampRow.get(), algoWidthRow.get() })
    {
        r->setAlpha(algoParamAlpha);
        r->setEnabled(algoOn);
    }
    // Mix: active whenever reverb is on
    reverbMixRow->setAlpha(reverbAlpha);
    reverbMixRow->setEnabled(reverbOn);

    resized();
    repaint();
}

float FxPanel::fs() const
{
    float topH = (getTopLevelComponent() != nullptr)
                     ? static_cast<float>(getTopLevelComponent()->getHeight()) : 800.0f;
    return juce::jlimit(12.0f, 22.0f, topH * 0.022f);
}

void FxPanel::paint(juce::Graphics& g)
{
    g.fillAll(kCard);

    // Vertical separator on left edge (between Seq and FX)
    g.setColour(kBorder);
    g.drawVerticalLine(0, 0.0f, static_cast<float>(getHeight()));

    // SwitchBox borders
    paintSwitchBoxBorder(g, delayTypeSwitchBounds);
    paintSwitchBoxBorder(g, reverbTypeSwitchBounds);

    // ── Wordmark: "T5ynth by UCDCAE AI LAB" — always visible, scales with panel ──
    if (reverbMixRow)
    {
        int spaceBelow = getHeight() - reverbMixRow->getBottom();
        int panelW = getWidth() - 16;
        // Size to fit panel width: "T5ynth by UCDCAE AI LAB" is ~22 chars
        float wmFs = std::max(6.0f, std::min(static_cast<float>(panelW) / 16.0f,
                                              static_cast<float>(spaceBelow) * 0.65f));
        int wmH = juce::roundToInt(wmFs * 1.4f);
        int wmY = reverbMixRow->getBottom() + (spaceBelow - wmH) / 2;
        g.setFont(juce::FontOptions(wmFs));

        // "T5ynth by " in dim gray, "UCDCAE AI LAB" per-letter colored — centred
        juce::String prefix = "T5ynth by  ";
        int prefixW = g.getCurrentFont().getStringWidth(prefix);

        struct LetterColor { char ch; juce::Colour col; };
        LetterColor letters[] = {
            {'U', juce::Colour(0xff667eea)}, {'C', juce::Colour(0xffe91e63)},
            {'D', juce::Colour(0xff7C4DFF)}, {'C', juce::Colour(0xffFF6F00)},
            {'A', juce::Colour(0xff4CAF50)}, {'E', juce::Colour(0xff00BCD4)},
            {' ', {}},
            {'A', juce::Colour(0xff667eea)}, {'I', juce::Colour(0xffe91e63)},
            {' ', {}},
            {'L', juce::Colour(0xff7C4DFF)}, {'A', juce::Colour(0xffFF6F00)},
            {'B', juce::Colour(0xff4CAF50)}
        };
        int tracking = juce::roundToInt(wmFs * 0.15f);

        int startX = 8;

        g.setColour(kDimmer);
        g.drawText(prefix, startX, wmY, prefixW, wmH, juce::Justification::centredLeft);

        int x = startX + prefixW;
        for (auto& lc : letters)
        {
            juce::String ch(juce::CharPointer_ASCII(&lc.ch), 1);
            int cw = g.getCurrentFont().getStringWidth(ch);
            if (lc.ch != ' ') { g.setColour(lc.col); g.drawText(ch, x, wmY, cw + 1, wmH, juce::Justification::centredLeft); }
            x += cw + tracking;
        }
    }
}

void FxPanel::resized()
{
    auto area = getLocalBounds().reduced(6, 0);
    float topH = getTopLevelComponent()
                     ? static_cast<float>(getTopLevelComponent()->getHeight()) : 800.0f;
    int headerH = juce::jlimit(14, 20, juce::roundToInt(topH * 0.022f));
    float f = static_cast<float>(headerH);

    int rowH = juce::jmin(juce::roundToInt(static_cast<float>(getHeight()) * 0.14f), 20);
    int gap = 2;

    // Reserve space at bottom for wordmark
    int wmReserve = juce::jmax(14, juce::roundToInt(static_cast<float>(getHeight()) * 0.09f));
    area.removeFromBottom(wmReserve);

    // ── DELAY header ──
    delayHeader.setFont(juce::FontOptions(f * 0.85f));
    delayHeader.setBounds(area.removeFromTop(headerH));
    area.removeFromTop(gap);

    // Delay type switchbox
    auto delaySwRow = area.removeFromTop(rowH);
    int delayCellW = delaySwRow.getWidth() / kNumDelayBtns;
    for (int i = 0; i < kNumDelayBtns; ++i)
    {
        int edges = 0;
        if (i > 0) edges |= juce::Button::ConnectedOnLeft;
        if (i < kNumDelayBtns - 1) edges |= juce::Button::ConnectedOnRight;
        delayTypeBtns[i].setConnectedEdges(edges);
        delayTypeBtns[i].setBounds(delaySwRow.removeFromLeft(delayCellW));
    }
    delayTypeSwitchBounds = delayTypeBtns[0].getBounds()
        .getUnion(delayTypeBtns[kNumDelayBtns - 1].getBounds());
    area.removeFromTop(gap);

    // Delay params — always laid out, dimmed when OFF
    {
        auto row1 = area.removeFromTop(rowH);
        auto pair1 = layoutSliderRowPairBounds(row1, *delayTimeRow, *delayFbRow, 2);
        delayTimeRow->setBounds(pair1[0]);
        delayFbRow->setBounds(pair1[1]);

        area.removeFromTop(gap);
        auto row2 = area.removeFromTop(rowH);
        auto pair2 = layoutSliderRowPairBounds(row2, *delayDampRow, *delayMixRow, 2);
        delayDampRow->setBounds(pair2[0]);
        delayMixRow->setBounds(pair2[1]);
    }

    area.removeFromTop(gap * 2);

    // ── REVERB header ──
    reverbHeader.setFont(juce::FontOptions(f * 0.85f));
    reverbHeader.setBounds(area.removeFromTop(headerH));
    area.removeFromTop(gap);

    // Reverb type switchbox
    auto revSwRow = area.removeFromTop(rowH);
    int revCellW = revSwRow.getWidth() / kNumReverbBtns;
    for (int i = 0; i < kNumReverbBtns; ++i)
    {
        int edges = 0;
        if (i > 0) edges |= juce::Button::ConnectedOnLeft;
        if (i < kNumReverbBtns - 1) edges |= juce::Button::ConnectedOnRight;
        reverbTypeBtns[i].setConnectedEdges(edges);
        reverbTypeBtns[i].setBounds(revSwRow.removeFromLeft(revCellW));
    }
    reverbTypeSwitchBounds = reverbTypeBtns[0].getBounds()
        .getUnion(reverbTypeBtns[kNumReverbBtns - 1].getBounds());
    area.removeFromTop(gap);

    // Reverb params — always 2 rows: Room+Damp, Width+Mix (dimmed when inactive)
    {
        auto row1 = area.removeFromTop(rowH);
        auto pair1 = layoutSliderRowPairBounds(row1, *algoRoomRow, *algoDampRow, 2);
        algoRoomRow->setBounds(pair1[0]);
        algoDampRow->setBounds(pair1[1]);

        area.removeFromTop(gap);
        auto row2 = area.removeFromTop(rowH);
        auto pair2 = layoutSliderRowPairBounds(row2, *algoWidthRow, *reverbMixRow, 2);
        algoWidthRow->setBounds(pair2[0]);
        reverbMixRow->setBounds(pair2[1]);
    }
}
