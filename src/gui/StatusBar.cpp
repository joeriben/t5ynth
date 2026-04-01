#include "StatusBar.h"
#include "GuiHelpers.h"

StatusBar::StatusBar()
{
    auto setupBtn = [this](juce::TextButton& btn) {
        btn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        btn.setColour(juce::TextButton::textColourOffId, kDim);
        addAndMakeVisible(btn);
    };
    setupBtn(importBtn);
    setupBtn(exportBtn);
    setupBtn(settingsBtn);

    importBtn.onClick   = [this] { if (onImportClicked)   onImportClicked(); };
    exportBtn.onClick   = [this] { if (onExportClicked)   onExportClicked(); };
    settingsBtn.onClick = [this] { if (onSettingsClicked)  onSettingsClicked(); };
}

void StatusBar::paint(juce::Graphics& g)
{
    g.fillAll(kCard);

    float h = static_cast<float>(getHeight());
    float dotSize = juce::jmax(5.0f, h * 0.30f);
    float dotX = 8.0f;
    g.setColour(backendConnected ? juce::Colour(0xff4ade80) : juce::Colour(0xffef4444));
    g.fillEllipse(dotX, (h - dotSize) * 0.5f, dotSize, dotSize);

    float fs = juce::jlimit(10.0f, 14.0f, h * 0.55f);
    g.setColour(juce::Colour(0xffe3e3e3));
    g.setFont(juce::FontOptions(fs));
    int textX = juce::roundToInt(dotX + dotSize + 6.0f);
    int textW = importBtn.getX() - textX - 4;
    g.drawText(statusText, textX, 0, textW, getHeight(),
               juce::Justification::centredLeft);
}

void StatusBar::resized()
{
    auto b = getLocalBounds();
    int btnW = 64;
    int btnH = b.getHeight();

    settingsBtn.setBounds(b.removeFromRight(btnW).withHeight(btnH));
    exportBtn.setBounds(b.removeFromRight(btnW).withHeight(btnH));
    importBtn.setBounds(b.removeFromRight(btnW).withHeight(btnH));
}

void StatusBar::setStatusText(const juce::String& text)
{
    statusText = text;
    repaint();
}

void StatusBar::setConnected(bool connected)
{
    backendConnected = connected;
    repaint();
}
