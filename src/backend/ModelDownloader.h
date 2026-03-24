#pragma once
#include <JuceHeader.h>
#include <functional>

/**
 * Downloads and manages local model files.
 *
 * Handles downloading the T5 model checkpoint, verifying integrity,
 * and reporting progress.
 */
class ModelDownloader
{
public:
    ModelDownloader() = default;

    /** Check if the model is already downloaded. */
    bool isModelAvailable() const;

    /** Get the path to the local model directory. */
    juce::File getModelDirectory() const;

    /** Start downloading the model (async). Progress callback: 0.0-1.0. */
    void startDownload(std::function<void(float progress)> onProgress,
                       std::function<void(bool success)> onComplete);

    /** Cancel an in-progress download. */
    void cancelDownload();

private:
    bool downloading = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModelDownloader)
};
