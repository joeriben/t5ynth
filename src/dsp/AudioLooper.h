#pragma once
#include <JuceHeader.h>

/**
 * Simple audio looper for AI-generated audio playback.
 *
 * Loads a buffer and plays it back in a loop with sample-rate conversion.
 */
class AudioLooper
{
public:
    AudioLooper() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    /** Load audio data for looped playback. */
    void loadBuffer(const juce::AudioBuffer<float>& buffer, double bufferSampleRate);

    /** Process a block (adds to buffer). */
    void processBlock(juce::AudioBuffer<float>& output);

    /** True if audio has been loaded. */
    bool hasAudio() const { return audioLoaded; }

    /** Start playback. */
    void play() { playing = true; }

    /** Stop playback and reset position. */
    void stop() { playing = false; readPosition = 0.0; }

    bool isPlaying() const { return playing; }

private:
    juce::AudioBuffer<float> loopBuffer;
    double playbackSampleRate = 44100.0;
    double bufferOriginalSR = 44100.0;
    double readPosition = 0.0;
    bool audioLoaded = false;
    bool playing = true;
};
