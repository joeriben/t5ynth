#pragma once
#include <JuceHeader.h>
#include <functional>

/**
 * Manages the lifecycle of the local Python backend process.
 *
 * - Locates the Python interpreter (venv or system)
 * - Starts server.py via juce::ChildProcess
 * - Polls /health until the server is ready
 * - Kills the process on shutdown
 */
class BackendManager : private juce::Timer
{
public:
    enum class Status { Stopped, Starting, Running, Failed };

    using StatusCallback = std::function<void(Status)>;

    BackendManager();
    ~BackendManager() override;

    void start();
    void stop();

    Status getStatus() const { return status.load(); }
    bool isRunning() const { return status.load() == Status::Running; }
    juce::String getEndpointUrl() const { return endpointUrl; }

    void setStatusCallback(StatusCallback cb);

private:
    void timerCallback() override;
    void notifyStatus();

    juce::File findPythonExecutable() const;
    juce::File findBackendDirectory() const;

    std::unique_ptr<juce::ChildProcess> childProcess;
    std::atomic<Status> status { Status::Stopped };
    juce::String endpointUrl { "http://127.0.0.1:17803" };

    StatusCallback statusCallback;
    int healthCheckAttempts = 0;

    static constexpr int maxHealthCheckAttempts = 30;   // 30 x 500ms = 15s
    static constexpr int healthCheckIntervalMs = 500;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BackendManager)
};
