#pragma once
#include <cmath>

/**
 * ADSR envelope with exponential release.
 *
 * Ported from AI4ArtsEd's useEnvelope.ts.
 * Runs per-sample in the audio thread (no heap allocation).
 */
class ADSREnvelope
{
public:
    ADSREnvelope() = default;

    void prepare(double sampleRate);

    void setAttack(float ms)  { attackMs = ms; recalculate(); }
    void setDecay(float ms)   { decayMs = ms; recalculate(); }
    void setSustain(float level) { sustainLevel = level; }
    void setRelease(float ms) { releaseMs = ms; recalculate(); }

    void noteOn(float velocity = 1.0f);
    void noteOff();

    /** Bypass envelope (gain = 1 immediately, for non-MIDI playback). */
    void bypass();

    /** Enable looping: envelope restarts Attack when reaching Sustain. */
    void setLooping(bool shouldLoop) { looping = shouldLoop; }

    /** Process one sample, returns gain value 0–1. */
    float processSample();

    /** True if envelope is in idle state (not producing output). */
    bool isIdle() const { return state == State::Idle; }

private:
    enum class State { Idle, Attack, Decay, Sustain, Release };
    State state = State::Idle;

    float attackMs = 10.0f;
    float decayMs = 100.0f;
    float sustainLevel = 0.8f;
    float releaseMs = 200.0f;

    // Per-sample coefficients
    float attackRate = 0.0f;
    float decayRate = 0.0f;
    float releaseRate = 0.0f;

    float currentLevel = 0.0f;
    float targetVelocity = 1.0f;
    double sr = 44100.0;
    bool bypassed = false;
    bool looping = false;

    void recalculate();
};
