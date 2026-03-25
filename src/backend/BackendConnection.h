#pragma once
#include <JuceHeader.h>
#include "GenerationRequest.h"
#include <functional>
#include <queue>

/**
 * HTTP connection to the T5ynth Python backend.
 *
 * Runs all network I/O on a dedicated background thread.
 * Delivers results to the message thread via MessageManager::callAsync.
 */
class BackendConnection : public juce::Thread
{
public:
    struct GenerationResult
    {
        bool success = false;
        juce::AudioBuffer<float> audioBuffer;
        double sampleRate = 44100.0;
        int seed = -1;
        float generationTimeMs = 0.0f;
        juce::String errorMessage;
    };

    using GenerationCallback = std::function<void(GenerationResult)>;
    using AxesCallback = std::function<void(bool success, juce::var axesData)>;
    using ConnectionCallback = std::function<void(bool connected)>;

    BackendConnection();
    ~BackendConnection() override;

    void setEndpoint(const juce::String& url);
    juce::String getEndpoint() const;
    bool isConnected() const { return connected.load(); }

    /** Synchronous health check (callable from any thread). */
    bool checkHealth();

    /** Async: POST /api/cross_aesthetic/synth */
    void requestGeneration(const GenerationRequest& request, GenerationCallback callback);

    /** Async: GET /api/cross_aesthetic/semantic_axes */
    void fetchSemanticAxes(AxesCallback callback);

    void setConnectionCallback(ConnectionCallback cb);

    // juce::Thread
    void run() override;

private:
    /** HTTP GET, returns response body (empty on failure). */
    juce::String httpGet(const juce::String& path, int& statusCode);

    /** HTTP POST with JSON body, returns response body (empty on failure). */
    juce::String httpPost(const juce::String& path, const juce::String& jsonBody, int& statusCode);

    /** Decode base64-encoded WAV into an AudioBuffer. */
    static bool decodeBase64Wav(const juce::String& base64,
                                juce::AudioBuffer<float>& buffer,
                                double& sampleRate);

    void enqueueWork(std::function<void()> work);

    std::atomic<bool> connected { false };
    juce::String endpointUrl { "http://127.0.0.1:17803" };
    juce::CriticalSection endpointLock;

    std::queue<std::function<void()>> workQueue;
    juce::CriticalSection queueLock;
    juce::WaitableEvent workAvailable { false };

    ConnectionCallback connectionCallback;
    juce::CriticalSection callbackLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BackendConnection)
};
