#pragma once
#include <JuceHeader.h>
#include <functional>

/**
 * HTTP connection to the T5ynth backend.
 *
 * Sends generation requests and receives audio/latent data.
 * Runs network I/O on a background thread.
 */
class BackendConnection
{
public:
    BackendConnection() = default;

    /** Check if connected to a running backend. */
    bool isConnected() const { return connected; }

    /** Set the backend endpoint URL. */
    void setEndpoint(const juce::String& url) { endpointUrl = url; }

    /** Test connectivity. */
    void ping();

    /** Send a generation request (async). */
    void requestGeneration(const juce::String& prompt,
                           std::function<void(bool success)> callback);

private:
    bool connected = false;
    juce::String endpointUrl = "http://localhost:17850";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BackendConnection)
};
