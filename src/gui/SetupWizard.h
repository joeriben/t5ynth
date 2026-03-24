#pragma once
#include <JuceHeader.h>

/**
 * First-run setup wizard.
 *
 * Guides the user through model download and backend configuration.
 */
class SetupWizard : public juce::Component
{
public:
    SetupWizard();
    ~SetupWizard() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SetupWizard)
};
