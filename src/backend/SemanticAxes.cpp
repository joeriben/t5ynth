#include "SemanticAxes.h"

void SemanticAxes::addAxis(const juce::String& negative, const juce::String& positive)
{
    axes.push_back({ negative, positive, 0.0f });
}

void SemanticAxes::setAxisValue(int axisIndex, float value)
{
    if (axisIndex >= 0 && axisIndex < static_cast<int>(axes.size()))
        axes[static_cast<size_t>(axisIndex)].value = juce::jlimit(-1.0f, 1.0f, value);
}

float SemanticAxes::getAxisValue(int axisIndex) const
{
    if (axisIndex >= 0 && axisIndex < static_cast<int>(axes.size()))
        return axes[static_cast<size_t>(axisIndex)].value;
    return 0.0f;
}
