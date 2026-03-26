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
        "Select Stable Audio model directory (contains model_index.json)",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "", true);

    fileChooser->launchAsync(juce::FileBrowserComponent::openMode
                             | juce::FileBrowserComponent::canSelectDirectories,
        [this](const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result == juce::File()) return;

            // Validate: must contain model_index.json
            if (!result.getChildFile("model_index.json").existsAsFile())
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::MessageBoxIconType::WarningIcon,
                    "Invalid Model Directory",
                    "The selected directory does not contain model_index.json.\n\n"
                    "Please select the root directory of the Stable Audio model "
                    "(e.g. stable-audio-open-1.0/).");
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
                // Create symlink: target -> selected directory
                auto symlinkResult = targetDir.createSymbolicLink(result, false);
                if (symlinkResult)
                {
                    setStatusText("Model linked — restart to load");
                    if (onModelSelected)
                        onModelSelected(result);
                }
                else
                {
                    // Symlink failed — try to copy
                    setStatusText("Copying model...");
                    repaint();

                    juce::Thread::launch([this, result, targetDir]() {
                        result.copyDirectoryTo(targetDir);
                        juce::MessageManager::callAsync([this]() {
                            setStatusText("Model copied — restart to load");
                        });
                    });
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
