#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <random>

/**
 * Generative Euclidean Sequencer — alternative to step sequencer.
 *
 * Generates both rhythm AND melody from a single Euclidean pattern:
 * - Euclidean distribution determines which steps are pulses (rhythm)
 * - Gaps between pulses determine scale-degree jumps (melody → self-similar)
 * - Turing Machine mutation evolves the pattern every cycle
 * - Per-pulse probability adds rhythmic breath
 *
 * Three layers of generativity:
 * 1. Euclidean structure = seed (rhythm + melodic intervals)
 * 2. Turing mutation = generator (notes drift ±1 scale degree per cycle)
 * 3. Pulse probability = rhythmic breath (ghost notes, skipped pulses)
 */
class T5ynthGenerativeSequencer
{
public:
    static constexpr int MAX_STEPS = 32;

    T5ynthGenerativeSequencer() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi);
    void reset();

    // Parameters — call from processBlock before processing
    void setSteps(int steps);
    void setPulses(int pulses);
    void setRotation(int rotation);
    void setMutation(float rate);
    void setRange(int octaves);
    void setScale(int scaleType, int root);
    void setGate(float gate);
    void setBpm(double bpm);
    void setDivision(int div);

    void start();
    void stop();
    bool isRunning() const { return running; }

    /** Atomic step position for GUI polling. */
    std::atomic<int> currentStepForGui { -1 };

    /** Current note pattern for GUI visualization (read from GUI thread). */
    struct StepInfo { int midiNote; bool isPulse; bool fired; };
    std::array<std::atomic<int>, MAX_STEPS> notePatternForGui{};
    std::atomic<int> numStepsForGui { 0 };

private:
    // Euclidean parameters
    int numSteps = 8;
    int numPulses = 5;
    int rotation = 0;
    float mutationRate = 0.15f;
    int rangeOctaves = 2;
    int scaleType = 1;  // Major (not Off)
    int scaleRoot = 0;  // C

    // Timing (shared with step sequencer)
    double bpm_ = 120.0;
    double sampleRate_ = 44100.0;
    float gate_ = 0.8f;
    int division_ = 3;
    static constexpr float DIVISION_FACTORS[] = { 4.0f, 2.0f, 1.0f, 0.5f, 0.25f };

    // Playback state
    bool running = false;
    int currentStep = 0;
    int scheduledStep = 0;
    double samplesUntilNextStep = 0.0;
    double samplesUntilGateOff = -1.0;
    int lastPlayedNote = -1;
    int cycleCount = 0;

    // Pattern data
    std::array<bool, MAX_STEPS> eucPattern{};    // true = pulse position
    std::array<int, MAX_STEPS> notePattern{};    // MIDI note per step (0 = rest)
    std::array<float, MAX_STEPS> velocityPattern{}; // velocity per step
    std::array<int, MAX_STEPS> degreePattern{};  // scale degree index per pulse (for mutation)
    bool patternDirty = true;

    // Turing Machine state
    std::mt19937 rng { 42 };

    // Internal
    double stepDurationSamples() const;
    void rebuildPattern();
    void mutatePattern();
    void computeGaps(int* gaps, int* gapCount) const;
    void publishPatternToGui();
};
