#pragma once
#include <JuceHeader.h>

class StatusBar : public juce::Component
{
public:
    StatusBar();
    ~StatusBar() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setStatusText(const juce::String& text);
    void setConnected(bool connected);

    /** Callbacks wired by MainPanel. */
    std::function<void()> onImportClicked;
    std::function<void()> onExportClicked;
    std::function<void()> onSettingsClicked;

private:
    juce::String statusText = "Ready";
    bool backendConnected = false;

    juce::TextButton importBtn  { "Import" };
    juce::TextButton exportBtn  { "Export" };
    juce::TextButton settingsBtn { "Settings" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StatusBar)
};
