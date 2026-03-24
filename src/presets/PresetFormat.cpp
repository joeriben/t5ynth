#include "PresetFormat.h"

bool PresetFormat::saveToFile(const juce::File& file,
                              juce::AudioProcessorValueTreeState& parameters)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());

    if (xml == nullptr)
        return false;

    return xml->writeTo(file);
}

bool PresetFormat::loadFromFile(const juce::File& file,
                                juce::AudioProcessorValueTreeState& parameters)
{
    auto xml = juce::XmlDocument::parse(file);

    if (xml == nullptr)
        return false;

    auto state = juce::ValueTree::fromXml(*xml);

    if (!state.isValid())
        return false;

    parameters.replaceState(state);
    return true;
}

juce::File PresetFormat::getPresetsDirectory()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("T5ynth")
        .getChildFile("presets");
}
