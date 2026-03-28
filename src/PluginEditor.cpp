#include "PluginEditor.h"
#include "PluginProcessor.h"

T5ynthEditor::T5ynthEditor(T5ynthProcessor& processor)
    : AudioProcessorEditor(processor),
      processorRef(processor),
      mainPanel(processor)
{
    setLookAndFeel(&lookAndFeel);
    addAndMakeVisible(mainPanel);

    setSize(1200, 800);
    setResizable(true, true);
    setResizeLimits(1050, 700, 2400, 1600);
    getConstrainer()->setFixedAspectRatio(3.0 / 2.0);
}

T5ynthEditor::~T5ynthEditor()
{
    setLookAndFeel(nullptr);
}

void T5ynthEditor::paint(juce::Graphics&)
{
}

void T5ynthEditor::resized()
{
    mainPanel.setBounds(getLocalBounds());
}

void T5ynthEditor::parentHierarchyChanged()
{
    AudioProcessorEditor::parentHierarchyChanged();
    if (!settingsInjected)
        injectSettingsButton();
}

void T5ynthEditor::injectSettingsButton()
{
    // Find the JUCE standalone content component (our parent)
    // and place Settings button next to its Options button
    auto* parent = getParentComponent();
    if (parent == nullptr) return;

    // The JUCE standalone content component has an "Options" TextButton as child
    for (int i = 0; i < parent->getNumChildComponents(); ++i)
    {
        auto* child = parent->getChildComponent(i);
        if (auto* btn = dynamic_cast<juce::TextButton*>(child))
        {
            if (btn->getButtonText().containsIgnoreCase("option"))
            {
                // Found the Options button — add Settings right next to it
                auto& settings = mainPanel.settingsButton;
                parent->addAndMakeVisible(settings);

                // Position: same height, right next to Options
                auto optBounds = btn->getBounds();
                int settingsW = 70;
                settings.setBounds(optBounds.getRight() + 4, optBounds.getY(),
                                   settingsW, optBounds.getHeight());
                settingsInjected = true;
                return;
            }
        }
    }
}
