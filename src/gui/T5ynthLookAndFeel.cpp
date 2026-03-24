#include "T5ynthLookAndFeel.h"

T5ynthLookAndFeel::T5ynthLookAndFeel()
{
    // Core dark palette
    const auto background   = juce::Colour(0xff0a0a0a);
    const auto surface      = juce::Colour(0xff1a1a1a);
    const auto surfaceLight = juce::Colour(0xff2a2a2a);
    const auto textPrimary  = juce::Colour(0xffe3e3e3);
    const auto textDim      = juce::Colour(0xff888888);
    const auto accent       = juce::Colour(0xff4a9eff);

    // Window / general
    setColour(juce::ResizableWindow::backgroundColourId, background);
    setColour(juce::DocumentWindow::backgroundColourId, background);

    // Labels
    setColour(juce::Label::textColourId, textPrimary);
    setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

    // Text editor
    setColour(juce::TextEditor::backgroundColourId, surface);
    setColour(juce::TextEditor::textColourId, textPrimary);
    setColour(juce::TextEditor::outlineColourId, surfaceLight);
    setColour(juce::TextEditor::focusedOutlineColourId, accent);

    // Sliders
    setColour(juce::Slider::backgroundColourId, surface);
    setColour(juce::Slider::trackColourId, accent);
    setColour(juce::Slider::thumbColourId, textPrimary);
    setColour(juce::Slider::textBoxTextColourId, textPrimary);
    setColour(juce::Slider::textBoxBackgroundColourId, surface);
    setColour(juce::Slider::textBoxOutlineColourId, surfaceLight);

    // Buttons
    setColour(juce::TextButton::buttonColourId, surface);
    setColour(juce::TextButton::buttonOnColourId, accent);
    setColour(juce::TextButton::textColourOffId, textPrimary);
    setColour(juce::TextButton::textColourOnId, textPrimary);

    // ComboBox
    setColour(juce::ComboBox::backgroundColourId, surface);
    setColour(juce::ComboBox::textColourId, textPrimary);
    setColour(juce::ComboBox::outlineColourId, surfaceLight);

    // PopupMenu
    setColour(juce::PopupMenu::backgroundColourId, surface);
    setColour(juce::PopupMenu::textColourId, textPrimary);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, accent);
    setColour(juce::PopupMenu::highlightedTextColourId, textPrimary);

    // Scrollbar
    setColour(juce::ScrollBar::thumbColourId, surfaceLight);

    // Tooltip
    setColour(juce::TooltipWindow::backgroundColourId, surface);
    setColour(juce::TooltipWindow::textColourId, textDim);

    // Default font
    setDefaultSansSerifTypefaceName("Inter");
}
