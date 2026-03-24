#include "StateVariableFilter.h"

void T5ynthFilter::prepare(double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    blockSize = samplesPerBlock;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    filter.prepare(spec);
    filter.setCutoffFrequency(20000.0f);
    filter.setResonance(1.0f / std::sqrt(2.0f)); // Butterworth default
    prepared = true;
}

void T5ynthFilter::processBlock(juce::AudioBuffer<float>& buffer)
{
    if (!prepared)
        return;

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    filter.process(context);
}

void T5ynthFilter::reset()
{
    filter.reset();
}

void T5ynthFilter::setCutoff(float hz)
{
    filter.setCutoffFrequency(hz);
}

void T5ynthFilter::setResonance(float q)
{
    filter.setResonance(q);
}

void T5ynthFilter::setType(int type)
{
    switch (type)
    {
        case 0:
            filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            break;
        case 1:
            filter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
            break;
        case 2:
            filter.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
            break;
        default:
            filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            break;
    }
}
