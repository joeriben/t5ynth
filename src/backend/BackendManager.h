#pragma once
#include <JuceHeader.h>

/**
 * Manages the lifecycle of the local Python backend process.
 *
 * Responsibilities:
 * - Locates or downloads the T5 model
 * - Starts/stops the inference server
 * - Health checks
 * - Provides the BackendConnection endpoint
 */
class BackendManager
{
public:
    BackendManager() = default;
    ~BackendManager();

    /** Start the backend process. */
    void start();

    /** Stop the backend process. */
    void stop();

    /** Check if the backend is running and healthy. */
    bool isRunning() const { return running; }

    /** Get the backend URL (e.g., http://localhost:17850). */
    juce::String getEndpointUrl() const { return endpointUrl; }

private:
    bool running = false;
    juce::String endpointUrl = "http://localhost:17850";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BackendManager)
};
