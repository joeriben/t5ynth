#include "StepSequencer.h"

void T5ynthStepSequencer::prepare(double sr, int /*samplesPerBlock*/)
{
    sampleRate = sr;
    samplesUntilNextStep = 0.0;
}

void T5ynthStepSequencer::processBlock(juce::AudioBuffer<float>& /*buffer*/,
                                       juce::MidiBuffer& /*midi*/)
{
    if (!running)
        return;

    // Stub: step advance and MIDI generation to be implemented
}

void T5ynthStepSequencer::reset()
{
    currentStep = 0;
    samplesUntilNextStep = 0.0;
    running = false;
}

void T5ynthStepSequencer::setNumSteps(int s)
{
    numSteps = juce::jlimit(1, MAX_STEPS, s);
}

void T5ynthStepSequencer::setStepNote(int step, int midiNote)
{
    if (step >= 0 && step < MAX_STEPS)
        steps[static_cast<size_t>(step)].note = midiNote;
}

void T5ynthStepSequencer::setStepVelocity(int step, float velocity)
{
    if (step >= 0 && step < MAX_STEPS)
        steps[static_cast<size_t>(step)].velocity = juce::jlimit(0.0f, 1.0f, velocity);
}

void T5ynthStepSequencer::setStepEnabled(int step, bool enabled)
{
    if (step >= 0 && step < MAX_STEPS)
        steps[static_cast<size_t>(step)].enabled = enabled;
}
