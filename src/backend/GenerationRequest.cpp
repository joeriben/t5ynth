#include "GenerationRequest.h"

juce::String GenerationRequest::toJson() const
{
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("prompt", prompt);
    obj->setProperty("alpha", static_cast<double>(alpha));
    obj->setProperty("magnitude", static_cast<double>(magnitude));
    obj->setProperty("noise", static_cast<double>(noise));
    obj->setProperty("seed", seed);

    return juce::JSON::toString(juce::var(obj.get()));
}
