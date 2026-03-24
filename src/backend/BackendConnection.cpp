#include "BackendConnection.h"

void BackendConnection::ping()
{
    // Stub: HTTP GET to endpointUrl/health
    connected = false;
}

void BackendConnection::requestGeneration(const juce::String& /*prompt*/,
                                          std::function<void(bool)> callback)
{
    // Stub: will POST to backend and invoke callback
    if (callback)
        callback(false);
}
