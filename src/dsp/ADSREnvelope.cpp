#include "ADSREnvelope.h"
#include <algorithm>

void ADSREnvelope::prepare(double sampleRate)
{
    sr = sampleRate;
    recalculate();
    state = State::Idle;
    currentLevel = 0.0f;
    bypassed = false;
}

void ADSREnvelope::recalculate()
{
    // Linear attack rate (samples)
    float attackSamples = std::max(1.0f, attackMs * 0.001f * static_cast<float>(sr));
    attackRate = 1.0f / attackSamples;

    // Exponential decay: RC-style coefficient
    float decaySamples = std::max(1.0f, decayMs * 0.001f * static_cast<float>(sr));
    decayRate = std::exp(-1.0f / decaySamples);

    // Exponential release: RC-style coefficient
    float releaseSamples = std::max(1.0f, releaseMs * 0.001f * static_cast<float>(sr));
    releaseRate = std::exp(-1.0f / releaseSamples);
}

void ADSREnvelope::noteOn(float velocity)
{
    targetVelocity = velocity;
    state = State::Attack;
    bypassed = false;
    // Don't reset currentLevel — allows retrigger from current position
}

void ADSREnvelope::noteOff()
{
    if (state != State::Idle)
        state = State::Release;
}

void ADSREnvelope::bypass()
{
    bypassed = true;
    currentLevel = 1.0f;
    state = State::Sustain;
}

float ADSREnvelope::processSample()
{
    if (bypassed)
        return 1.0f;

    switch (state)
    {
        case State::Idle:
            return 0.0f;

        case State::Attack:
            currentLevel += attackRate * targetVelocity;
            if (currentLevel >= targetVelocity)
            {
                currentLevel = targetVelocity;
                state = State::Decay;
            }
            return currentLevel;

        case State::Decay:
        {
            // Exponential decay toward sustain level
            float target = sustainLevel * targetVelocity;
            currentLevel = target + (currentLevel - target) * decayRate;
            // Transition to sustain when close enough
            if (std::abs(currentLevel - target) < 0.001f)
            {
                currentLevel = target;
                state = State::Sustain;
            }
            return currentLevel;
        }

        case State::Sustain:
            return currentLevel;

        case State::Release:
        {
            // Exponential release toward zero
            currentLevel *= releaseRate;
            if (currentLevel < 0.0001f)
            {
                currentLevel = 0.0f;
                state = State::Idle;
            }
            return currentLevel;
        }
    }

    return 0.0f;
}
