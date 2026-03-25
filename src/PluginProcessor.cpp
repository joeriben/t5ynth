#include "PluginProcessor.h"
#include "PluginEditor.h"

T5ynthProcessor::T5ynthProcessor()
    : AudioProcessor(BusesProperties()
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "T5ynth", createParameterLayout())
{
    backendManager.setStatusCallback([this](BackendManager::Status s)
    {
        if (s == BackendManager::Status::Running)
        {
            backendConnection.setEndpoint(backendManager.getEndpointUrl());
            backendConnection.checkHealth();
        }
    });

    backendManager.start();
}

T5ynthProcessor::~T5ynthProcessor()
{
    backendManager.stop();
}

juce::AudioProcessorValueTreeState::ParameterLayout T5ynthProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Oscillator
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"osc_scan", 1}, "Scan Position",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    // Amplitude Envelope
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"amp_attack", 1}, "Attack",
        juce::NormalisableRange<float>(0.0f, 5000.0f, 0.1f, 0.3f), 10.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"amp_decay", 1}, "Decay",
        juce::NormalisableRange<float>(0.0f, 5000.0f, 0.1f, 0.3f), 100.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"amp_sustain", 1}, "Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"amp_release", 1}, "Release",
        juce::NormalisableRange<float>(0.0f, 10000.0f, 0.1f, 0.3f), 200.0f));

    // Filter
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filter_cutoff", 1}, "Filter Cutoff",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.25f), 20000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filter_resonance", 1}, "Filter Resonance",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"filter_type", 1}, "Filter Type",
        juce::StringArray{"Lowpass", "Highpass", "Bandpass"}, 0));

    // Delay
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"delay_time", 1}, "Delay Time",
        juce::NormalisableRange<float>(0.0f, 5000.0f, 0.1f), 300.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"delay_feedback", 1}, "Delay Feedback",
        juce::NormalisableRange<float>(0.0f, 0.95f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"delay_mix", 1}, "Delay Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    // Reverb
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"reverb_mix", 1}, "Reverb Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    // Generation
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"gen_alpha", 1}, "Alpha",
        juce::NormalisableRange<float>(-2.0f, 2.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"gen_magnitude", 1}, "Magnitude",
        juce::NormalisableRange<float>(0.1f, 5.0f, 0.01f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"gen_noise", 1}, "Noise",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    return { params.begin(), params.end() };
}

void T5ynthProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    wavetableOsc.prepare(sampleRate, samplesPerBlock);
    looper.prepare(sampleRate, samplesPerBlock);
    ampEnvelope.prepare(sampleRate);
    modEnvelope1.prepare(sampleRate);
    modEnvelope2.prepare(sampleRate);
    lfo1.prepare(sampleRate);
    lfo2.prepare(sampleRate);
    filter.prepare(sampleRate, samplesPerBlock);
    delay.prepare(sampleRate, samplesPerBlock);
    reverb.prepare(sampleRate, samplesPerBlock);
    limiter.prepare(sampleRate, samplesPerBlock);
}

void T5ynthProcessor::releaseResources()
{
    wavetableOsc.reset();
    looper.reset();
}

void T5ynthProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // Process MIDI
    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            currentNote = static_cast<float>(msg.getNoteNumber());
            currentVelocity = msg.getFloatVelocity();
            noteIsOn = true;
            ampEnvelope.noteOn(currentVelocity);
            wavetableOsc.setFrequency(juce::MidiMessage::getMidiNoteInHertz(msg.getNoteNumber()));
        }
        else if (msg.isNoteOff())
        {
            noteIsOn = false;
            ampEnvelope.noteOff();
        }
    }

    const int numSamples = buffer.getNumSamples();

    // Generate audio based on engine mode
    if (engineMode == EngineMode::Wavetable && wavetableOsc.hasFrames())
    {
        auto scanParam = parameters.getRawParameterValue("osc_scan");
        wavetableOsc.setScanPosition(scanParam->load());

        for (int i = 0; i < numSamples; ++i)
        {
            float sample = wavetableOsc.processSample();
            float envGain = ampEnvelope.processSample();
            sample *= envGain;

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.setSample(ch, i, sample);
        }
    }
    else if (engineMode == EngineMode::Looper && looper.hasAudio())
    {
        looper.processBlock(buffer);

        // Apply envelope
        for (int i = 0; i < numSamples; ++i)
        {
            float envGain = ampEnvelope.processSample();
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.setSample(ch, i, buffer.getSample(ch, i) * envGain);
        }
    }

    // Filter
    filter.processBlock(buffer);

    // Delay
    delay.processBlock(buffer);

    // Reverb
    reverb.processBlock(buffer);

    // Limiter
    limiter.processBlock(buffer);
}

void T5ynthProcessor::loadGeneratedAudio(const juce::AudioBuffer<float>& audioBuffer, double sr)
{
    looper.loadBuffer(audioBuffer, sr);
    wavetableOsc.extractFramesFromBuffer(audioBuffer, sr);
}

juce::AudioProcessorEditor* T5ynthProcessor::createEditor()
{
    return new T5ynthEditor(*this);
}

void T5ynthProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void T5ynthProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new T5ynthProcessor();
}
