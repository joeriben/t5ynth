#include "StatusBar.h"
#include "GuiHelpers.h"

StatusBar::StatusBar()
{
    modelButton.setColour(juce::TextButton::buttonColourId, kSurface);
    modelButton.setColour(juce::TextButton::textColourOffId, kAccent);
    modelButton.onClick = [this] { browseForModel(); };
    addAndMakeVisible(modelButton);
}

void StatusBar::browseForModel()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Select Stable Audio model directory (HuggingFace format)",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "", true);

    fileChooser->launchAsync(juce::FileBrowserComponent::openMode
                             | juce::FileBrowserComponent::canSelectDirectories,
        [this](const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result == juce::File()) return;

            // Must be HuggingFace pretrained format — embedding manipulation
            // requires separate access to text encoder + projection model
            if (!result.getChildFile("model_index.json").existsAsFile())
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::MessageBoxIconType::WarningIcon,
                    "Wrong Model Format",
                    "T5ynth needs the HuggingFace pipeline format (with model_index.json) "
                    "for embedding manipulation.\n\n"
                    "A single .safetensors checkpoint won't work — the text encoder and "
                    "projection model must be accessible as separate components.\n\n"
                    "Download with:\n"
                    "  huggingface-cli download stabilityai/stable-audio-open-1.0 "
                    "--local-dir ~/t5ynth/models/stable-audio-open-1.0");
                return;
            }

            // Target: ~/t5ynth/models/stable-audio-open-1.0
            auto targetDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                                 .getChildFile("t5ynth/models/stable-audio-open-1.0");
            auto parentDir = targetDir.getParentDirectory();

            if (!parentDir.isDirectory())
                parentDir.createDirectory();

            // Remove existing target if it's a symlink or empty dir
            if (targetDir.exists())
            {
                if (targetDir.isSymbolicLink())
                    targetDir.deleteFile();
                else if (targetDir.isDirectory() && targetDir.getNumberOfChildFiles(juce::File::findFilesAndDirectories) == 0)
                    targetDir.deleteRecursively();
            }

            if (!targetDir.exists())
            {
                auto symlinkResult = targetDir.createSymbolicLink(result, false);
                if (symlinkResult)
                {
                    setStatusText("Model linked — restart to load");
                    if (onModelSelected)
                        onModelSelected(result);
                }
                else
                {
                    setStatusText("Symlink failed — check permissions");
                }
            }
            else
            {
                setStatusText("Model directory already exists");
            }
        });
}

void StatusBar::paint(juce::Graphics& g)
{
    g.fillAll(kCard);

    float h = static_cast<float>(getHeight());
    float dotSize = juce::jmax(6.0f, h * 0.35f);
    float dotX = h * 0.4f;
    g.setColour(backendConnected ? juce::Colour(0xff4ade80) : juce::Colour(0xffef4444));
    g.fillEllipse(dotX, (h - dotSize) * 0.5f, dotSize, dotSize);

    float topH = (getTopLevelComponent() != nullptr) ? static_cast<float>(getTopLevelComponent()->getHeight()) : 800.0f;
    float fs = juce::jlimit(14.0f, 26.0f, topH * 0.030f);
    g.setColour(juce::Colour(0xffe3e3e3));
    g.setFont(juce::FontOptions(fs));
    int textX = juce::roundToInt(dotX + dotSize + h * 0.3f);
    int btnW = modelButton.getWidth();
    g.drawText(statusText, textX, 0, getWidth() - textX - btnW - 8, getHeight(),
               juce::Justification::centredLeft);
}

void StatusBar::resized()
{
    auto b = getLocalBounds();
    int btnW = 70;
    int btnH = juce::jmin(b.getHeight() - 4, 22);
    modelButton.setBounds(b.getRight() - btnW - 4, (b.getHeight() - btnH) / 2, btnW, btnH);
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
