#include "ModelDownloader.h"

bool ModelDownloader::isModelAvailable() const
{
    return getModelDirectory().getChildFile("model.safetensors").existsAsFile();
}

juce::File ModelDownloader::getModelDirectory() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("T5ynth")
        .getChildFile("models");
}

void ModelDownloader::startDownload(std::function<void(float)> /*onProgress*/,
                                    std::function<void(bool)> onComplete)
{
    // Stub: download logic to be implemented
    if (onComplete)
        onComplete(false);
}

void ModelDownloader::cancelDownload()
{
    downloading = false;
}
