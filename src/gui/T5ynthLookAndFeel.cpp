#include "T5ynthLookAndFeel.h"
#include "GuiHelpers.h"

T5ynthLookAndFeel::T5ynthLookAndFeel()
{
    // Derive from shared constants
    const auto background   = kBg;
    const auto surface      = kSurface;
    const auto surfaceLight = kBorder;
    const auto textPrimary  = juce::Colour(0xffe3e3e3);
    const auto textDim      = kDim;
    const auto accent       = kAccent;

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
