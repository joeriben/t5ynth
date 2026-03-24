#include "DelayLine.h"

void T5ynthDelayLine::prepare(double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    delayLine.prepare(spec);
    delayLine.setDelay(static_cast<float>(delayTimeMs * 0.001 * sr));
    prepared = true;
}

void T5ynthDelayLine::processBlock(juce::AudioBuffer<float>& buffer)
{
    if (!prepared || wetMix == 0.0f)
        return;

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    const float dryMix = 1.0f - wetMix;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buffer.getWritePointer(ch);

        for (int i = 0; i < numSamples; ++i)
        {
            float drySample = data[i];
            float delayed = delayLine.popSample(ch);
            delayLine.pushSample(ch, drySample + delayed * feedback);
            data[i] = drySample * dryMix + delayed * wetMix;
        }
    }
}

void T5ynthDelayLine::reset()
{
    delayLine.reset();
}

void T5ynthDelayLine::setTime(float ms)
{
    delayTimeMs = ms;
    if (prepared)
        delayLine.setDelay(static_cast<float>(ms * 0.001 * sr));
}

void T5ynthDelayLine::setFeedback(float fb)
{
    feedback = juce::jlimit(0.0f, 0.95f, fb);
}

void T5ynthDelayLine::setMix(float mix)
{
    wetMix = juce::jlimit(0.0f, 1.0f, mix);
}
