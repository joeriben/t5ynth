#include "Arpeggiator.h"

void T5ynthArpeggiator::prepare(double sr, int /*samplesPerBlock*/)
{
    sampleRate = sr;
    samplesUntilNext = 0.0;
}

void T5ynthArpeggiator::processBlock(juce::AudioBuffer<float>& /*buffer*/,
                                     juce::MidiBuffer& /*midi*/)
{
    // Stub: arp logic to be implemented
}

void T5ynthArpeggiator::reset()
{
    heldNotes.clear();
    currentIndex = 0;
    samplesUntilNext = 0.0;
}

void T5ynthArpeggiator::noteOn(int midiNote, float velocity)
{
    // Avoid duplicates
    for (const auto& n : heldNotes)
        if (n.note == midiNote) return;

    heldNotes.push_back({ midiNote, velocity });
}

void T5ynthArpeggiator::noteOff(int midiNote)
{
    heldNotes.erase(
        std::remove_if(heldNotes.begin(), heldNotes.end(),
                       [midiNote](const HeldNote& n) { return n.note == midiNote; }),
        heldNotes.end());

    if (currentIndex >= static_cast<int>(heldNotes.size()))
        currentIndex = 0;
}
