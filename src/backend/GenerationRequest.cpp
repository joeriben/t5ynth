#include "GenerationRequest.h"

juce::String GenerationRequest::toJson() const
{
    auto obj = new juce::DynamicObject();

    obj->setProperty("prompt_a", promptA);

    if (promptB.isNotEmpty())
        obj->setProperty("prompt_b", promptB);

    obj->setProperty("alpha", static_cast<double>(alpha));
    obj->setProperty("magnitude", static_cast<double>(magnitude));
    obj->setProperty("noise_sigma", static_cast<double>(noiseSigma));
    obj->setProperty("duration_seconds", static_cast<double>(durationSeconds));
    obj->setProperty("start_position", static_cast<double>(startPosition));
    obj->setProperty("steps", steps);
    obj->setProperty("cfg_scale", static_cast<double>(cfgScale));
    obj->setProperty("seed", seed);

    if (!axes.empty())
    {
        auto axesObj = new juce::DynamicObject();
        for (const auto& [name, value] : axes)
            axesObj->setProperty(name, static_cast<double>(value));
        obj->setProperty("axes", juce::var(axesObj));
    }

    if (!dimensionOffsets.empty())
    {
        auto offsetsObj = new juce::DynamicObject();
        for (const auto& [dim, offset] : dimensionOffsets)
            offsetsObj->setProperty(juce::String(dim), static_cast<double>(offset));
        obj->setProperty("dimension_offsets", juce::var(offsetsObj));
    }

    return juce::JSON::toString(juce::var(obj));
}
