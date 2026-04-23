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
 * - Stride-based permutation maps pulses to scale degrees (melody)
 * - Turing Machine mutation evolves the pattern every cycle
 * - Per-pulse probability adds rhythmic breath
 * - Pulse count and rotation drift organically over time
 *
 * Three layers of generativity:
 * 1. Euclidean structure = seed (rhythm + melodic stride)
 * 2. Turing mutation = generator (notes drift ±1-4 scale degrees per cycle)
 * 3. Pulse probability = rhythmic breath (ghost notes, skipped pulses)
 *
 * Phase 1 refactor (polyphonic-gen-seq feature):
 * Per-pattern state has been moved into a nested Strand struct, with
 * strands[MAX_STRANDS] held at the class level. In Phase 1 only strand 0
 * is active — runtime behavior is unchanged from the mono sequencer.
 * Subsequent phases add PitchField, roles, and multi-strand scheduling.
 *
 * References:
 * - Toussaint, G.T. (2005). "The Euclidean Algorithm Generates Traditional
 *   Musical Rhythms." Proc. BRIDGES: Mathematical Connections in Art, Music
 *   and Science, pp. 47–56.
 * - Conceptual inspiration: Music Thing Modular "Turing Machine" by
 *   Tom Whitwell — LFSR with adjustable mutation probability for
 *   pattern evolution. https://musicthing.co.uk/Turing-Machine/
 */
class T5ynthGenerativeSequencer
{
public:
    static constexpr int MAX_STEPS   = 32;
    static constexpr int MAX_STRANDS = 4;

    T5ynthGenerativeSequencer() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi);
    void reset();

    // Strand-0 legacy API — routed internally to strands[0].
    void setSteps(int steps);
    void setPulses(int pulses);
    void setRotation(int rotation);
    void setMutation(float rate);
    void setFixSteps(bool fixed);
    void setFixPulses(bool fixed);
    void setFixRotation(bool fixed);
    void setFixMutation(bool fixed);
    void setBaseMutation(float rate);

    /** Seed strand 0's pattern from step sequencer data (skips Euclidean generation). */
    void seedFromSteps(const int* midiNotes, const bool* enabled, int count);

    // Shared / global
    void setRange(int octaves);
    void setScale(int scaleType, int root);
    void setGate(float gate);
    void setBpm(double bpm);
    void setDivision(int div);

    void start();
    void stop();
    bool isRunning() const { return running; }

    /** Atomic step position for GUI polling (tracks strand 0). */
    std::atomic<int> currentStepForGui { -1 };

    /** Current note pattern for GUI visualization (tracks strand 0). */
    struct StepInfo { int midiNote; bool isPulse; bool fired; };
    std::array<std::atomic<int>, MAX_STEPS> notePatternForGui{};
    std::atomic<int> numStepsForGui { 0 };

    /** Effective (post-drift) values for GUI polling (tracks strand 0). */
    std::atomic<int>   effectiveStepsForGui   { 0 };
    std::atomic<int>   effectivePulsesForGui  { 0 };
    std::atomic<float> effectiveMutationForGui{ 0.0f };

private:
    /** Per-strand pattern + playback state. */
    struct Strand
    {
        // Euclidean parameters
        int   numSteps     = 8;
        int   numPulses    = 5;
        int   rotation     = 0;
        float mutationRate = 0.15f;

        // Playback state
        int    currentStep           = 0;
        int    scheduledStep         = 0;
        double samplesUntilNextStep  = 0.0;
        double samplesUntilGateOff   = -1.0;
        int    lastPlayedNote        = -1;
        int    cycleCount            = 0;

        // Pattern data
        std::array<bool, MAX_STEPS> eucPattern{};
        std::array<int,  MAX_STEPS> notePattern{};
        std::array<bool, MAX_STEPS> accentPattern{};
        std::array<int,  MAX_STEPS> degreePattern{};
        bool patternDirty  = true;
        bool patternSeeded = false;

        // Euclidean drift state
        int  driftCycle       = 0;
        int  basePulses       = 5;
        int  pulseDriftAccum  = 0;
        bool pulseDriftUp     = true;

        // Steps drift state
        int  baseSteps        = 8;
        int  stepsDriftAccum  = 0;
        bool stepsDriftUp     = true;

        // Mutation drift state
        float baseMutation       = 0.15f;
        int   mutationDriftPhase = 0;

        // Fix flags
        bool fixSteps    = true;
        bool fixPulses   = false;
        bool fixRotation = false;
        bool fixMutation = true;
    };

    std::array<Strand, MAX_STRANDS> strands{};

    // Timing (shared across strands)
    double bpm_         = 120.0;
    double sampleRate_  = 44100.0;
    float  gate_        = 0.8f;
    int    division_    = 3;
    static constexpr float DIVISION_FACTORS[] = { 4.0f, 2.0f, 1.0f, 0.5f, 0.25f };
    static constexpr int   DRIFT_PERIOD       = 8;  // drift evaluated over 8-cycle windows

    // Pitch context (shared)
    int rangeOctaves = 2;
    int scaleType    = 1;   // Major (not Off)
    int scaleRoot    = 0;   // C

    // Playback global
    bool running = false;

    // Shared RNG (accessed only from the audio thread)
    std::mt19937 rng { static_cast<std::mt19937::result_type>(std::random_device{}()) };

    // Internal helpers — all operate on a passed Strand reference.
    double stepDurationSamples() const;
    void   rebuildPattern(Strand& s);
    void   mutatePattern(Strand& s);
    void   computeGaps(const Strand& s, int* gaps, int* gapCount) const;
    void   publishStrandToGui(const Strand& s);
    void   mutateNotes(Strand& s, float rate, int totalDegrees, int scaleEnum, int baseNote);
    void   applyEuclideanDrift(Strand& s);
    void   applyStepsDrift(Strand& s);
    void   applyMutationDrift(Strand& s);
    void   addPulse(Strand& s);
    void   removePulse(Strand& s);
};
