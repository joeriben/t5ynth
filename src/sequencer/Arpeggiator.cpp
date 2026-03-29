#include "Arpeggiator.h"

void T5ynthArpeggiator::prepare(double sr, int /*samplesPerBlock*/)
{
    sampleRateVal = sr;
    samplesUntilNext = 0.0;
}

void T5ynthArpeggiator::rebuildIntervals()
{
    intervals.clear();

    // Expand chord across octave range
    for (int oct = 0; oct < octaveRange; ++oct)
        for (int i = 0; i < NUM_CHORD_NOTES; ++i)
            intervals.push_back(CHORD_INTERVALS[i] + oct * 12);

    // Apply pattern ordering
    switch (mode)
    {
        case Mode::Up:
            // Already in order
            break;
        case Mode::Down:
            std::reverse(intervals.begin(), intervals.end());
            break;
        case Mode::UpDown:
        {
            if (intervals.size() <= 1) break;
            auto up = intervals;
            // Down part: reverse, omitting endpoints to avoid double-stepping
            auto down = std::vector<int>(up.rbegin() + 1, up.rend() - 1);
            intervals.reserve(up.size() + down.size());
            intervals.insert(intervals.end(), down.begin(), down.end());
            break;
        }
        case Mode::Random:
            fisherYatesShuffle();
            break;
    }
}

void T5ynthArpeggiator::fisherYatesShuffle()
{
    for (int i = static_cast<int>(intervals.size()) - 1; i > 0; --i)
    {
        int j = rng.nextInt(i + 1);
        std::swap(intervals[static_cast<size_t>(i)], intervals[static_cast<size_t>(j)]);
    }
}

void T5ynthArpeggiator::setMode(Mode m)
{
    mode = m;
    rebuildIntervals();
}

void T5ynthArpeggiator::setBaseNote(int midiNote, float velocity)
{
    baseNote = midiNote;
    baseVelocity = velocity;
    active = true;
    currentIndex = 0;
    samplesUntilNext = 0.0;
    rebuildIntervals();
}

void T5ynthArpeggiator::stopArp()
{
    active = false;
    currentIndex = 0;
    samplesUntilNext = 0.0;
}

void T5ynthArpeggiator::processBlock(juce::AudioBuffer<float>& buffer,
                                     juce::MidiBuffer& midi)
{
    if (!active || intervals.empty())
    {
        if (lastPlayedNote >= 0)
        {
            midi.addEvent(juce::MidiMessage::noteOff(1, lastPlayedNote), 0);
            lastPlayedNote = -1;
        }
        return;
    }

    // Calculate step duration in samples
    double quarterNoteSamples = sampleRateVal * 60.0 / bpm;
    double samplesPerStep = quarterNoteSamples * static_cast<double>(RATE_FACTORS[rate]);

    const int numSamples = buffer.getNumSamples();
    int samplePos = 0;

    while (samplePos < numSamples)
    {
        if (samplesUntilNext <= 0.0)
        {
            // Note-off for previous
            if (lastPlayedNote >= 0)
            {
                midi.addEvent(juce::MidiMessage::noteOff(1, lastPlayedNote), samplePos);
                lastPlayedNote = -1;
            }

            int idx = currentIndex % static_cast<int>(intervals.size());
            int midiNote = juce::jlimit(0, 127, baseNote + intervals[static_cast<size_t>(idx)]);
            int velInt = juce::jlimit(1, 127, juce::roundToInt(baseVelocity * 127.0f));

            midi.addEvent(juce::MidiMessage::noteOn(1, midiNote,
                          static_cast<juce::uint8>(velInt)), samplePos);
            lastPlayedNote = midiNote;
            currentIndex++;

            // Rebuild intervals on each cycle for random pattern freshness
            if (mode == Mode::Random && currentIndex % static_cast<int>(intervals.size()) == 0)
                fisherYatesShuffle();

            samplesUntilNext += samplesPerStep;
        }

        int samplesToProcess = juce::jmin(numSamples - samplePos,
                                          static_cast<int>(std::ceil(samplesUntilNext)));
        samplesToProcess = std::max(1, samplesToProcess);
        samplesUntilNext -= samplesToProcess;
        samplePos += samplesToProcess;
    }
}

void T5ynthArpeggiator::reset()
{
    active = false;
    currentIndex = 0;
    samplesUntilNext = 0.0;
    lastPlayedNote = -1;
    intervals.clear();
}
