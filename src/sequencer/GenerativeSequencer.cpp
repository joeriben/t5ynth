#include "GenerativeSequencer.h"
#include "EuclideanRhythm.h"
#include "ScaleQuantizer.h"

// ─── Timing ────────────────────────────────────────────────────────────────

double T5ynthGenerativeSequencer::stepDurationSamples() const
{
    return sampleRate_ * 60.0 / bpm_ * static_cast<double>(DIVISION_FACTORS[division_]);
}

constexpr float T5ynthGenerativeSequencer::DIVISION_FACTORS[];

void T5ynthGenerativeSequencer::prepare(double sr, int /*samplesPerBlock*/)
{
    sampleRate_ = sr;
    samplesUntilNextStep = 0.0;
    samplesUntilGateOff = -1.0;
}

// ─── Parameter setters ─────────────────────────────────────────────────────

void T5ynthGenerativeSequencer::setSteps(int s)
{
    s = juce::jlimit(2, MAX_STEPS, s);
    if (s != numSteps) { numSteps = s; patternDirty = true; }
}

void T5ynthGenerativeSequencer::setPulses(int p)
{
    p = juce::jlimit(1, numSteps, p);
    if (p != numPulses) { numPulses = p; patternDirty = true; }
}

void T5ynthGenerativeSequencer::setRotation(int r)
{
    r = numSteps > 0 ? ((r % numSteps) + numSteps) % numSteps : 0;
    if (r != rotation) { rotation = r; patternDirty = true; }
}

void T5ynthGenerativeSequencer::setMutation(float rate)
{
    mutationRate = juce::jlimit(0.0f, 1.0f, rate);
}

void T5ynthGenerativeSequencer::setRange(int octaves)
{
    octaves = juce::jlimit(1, 4, octaves);
    if (octaves != rangeOctaves) { rangeOctaves = octaves; patternDirty = true; }
}

void T5ynthGenerativeSequencer::setScale(int type, int root)
{
    // If Off, default to Major for generative mode
    if (type <= 0) type = 1;
    if (type != scaleType || root != scaleRoot)
    {
        scaleType = type;
        scaleRoot = root;
        patternDirty = true;
    }
}

void T5ynthGenerativeSequencer::setGate(float g) { gate_ = juce::jlimit(0.1f, 1.0f, g); }
void T5ynthGenerativeSequencer::setBpm(double b) { bpm_ = b; }
void T5ynthGenerativeSequencer::setDivision(int d) { division_ = juce::jlimit(0, 4, d); }

// ─── Start / Stop ──────────────────────────────────────────────────────────

void T5ynthGenerativeSequencer::start()
{
    if (!running)
    {
        running = true;
        samplesUntilNextStep = 0.0;
        currentStep = 0;
        scheduledStep = 0;
        cycleCount = 0;
        patternDirty = true;  // rebuild on start
    }
}

void T5ynthGenerativeSequencer::stop()
{
    running = false;
    samplesUntilGateOff = -1.0;
    currentStep = 0;
    scheduledStep = 0;
    currentStepForGui.store(-1, std::memory_order_relaxed);
}

void T5ynthGenerativeSequencer::reset()
{
    stop();
}

// ─── Pattern generation ────────────────────────────────────────────────────

void T5ynthGenerativeSequencer::computeGaps(int* gaps, int* gapCount) const
{
    // Find gaps between consecutive pulses (circular)
    *gapCount = 0;
    int firstPulse = -1;
    int prevPulse = -1;

    for (int i = 0; i < numSteps; ++i)
    {
        if (eucPattern[static_cast<size_t>(i)])
        {
            if (firstPulse < 0) firstPulse = i;
            if (prevPulse >= 0)
                gaps[(*gapCount)++] = i - prevPulse;
            prevPulse = i;
        }
    }
    // Wrap-around gap: from last pulse to first pulse
    if (firstPulse >= 0 && prevPulse >= 0 && *gapCount > 0)
        gaps[(*gapCount)++] = numSteps - prevPulse + firstPulse;
    else if (firstPulse >= 0)
        gaps[(*gapCount)++] = numSteps; // single pulse
}

void T5ynthGenerativeSequencer::rebuildPattern()
{
    // 1. Generate Euclidean rhythm
    auto fullPattern = EuclideanRhythm::generate(numSteps, numPulses, rotation);
    for (int i = 0; i < MAX_STEPS; ++i)
        eucPattern[static_cast<size_t>(i)] = (i < numSteps) && fullPattern[static_cast<size_t>(i)];

    // 2. Compute gaps between pulses → interval jumps
    int gaps[MAX_STEPS];
    int gapCount = 0;
    computeGaps(gaps, &gapCount);

    // 3. Build scale degrees for melody
    auto scale = static_cast<ScaleQuantizer::Scale>(
        juce::jlimit(1, static_cast<int>(ScaleQuantizer::COUNT) - 1, scaleType));
    int dpOct = ScaleQuantizer::degreesPerOctave(scale);
    int totalDegrees = dpOct * rangeOctaves;
    int baseNote = 48; // C3

    // 4. Assign notes using stride-based permutation through the scale
    //    degree[i] = (pulseIndex * stride) % totalDegrees
    //    Stride derived from Euclidean params (numSteps - numPulses).
    //    If coprime with totalDegrees → visits every degree before repeating.
    //    Creates a wide-ranging, non-sequential, structured melody.
    int stride = juce::jmax(1, numSteps - numPulses);
    // Ensure coprime with totalDegrees for maximum coverage
    while (stride > 1 && totalDegrees % stride == 0)
        stride++;
    if (stride >= totalDegrees) stride = 1;

    int pulseIdx = 0;
    notePattern.fill(0);
    velocityPattern.fill(0.0f);
    degreePattern.fill(0);

    for (int i = 0; i < numSteps; ++i)
    {
        if (eucPattern[static_cast<size_t>(i)])
        {
            int degree = (pulseIdx * stride) % totalDegrees;
            int midiNote = ScaleQuantizer::degreeToMidi(degree, scaleRoot, scale, baseNote);
            notePattern[static_cast<size_t>(i)] = midiNote;
            degreePattern[static_cast<size_t>(i)] = degree;

            // Velocity: accent based on gap length
            float gapFrac = (gapCount > 0)
                ? static_cast<float>(gaps[pulseIdx % gapCount]) / static_cast<float>(numSteps)
                : 0.5f;
            velocityPattern[static_cast<size_t>(i)] = juce::jlimit(0.4f, 1.0f, 0.55f + gapFrac * 0.6f);
            pulseIdx++;
        }
    }

    patternDirty = false;
    publishPatternToGui();
}

void T5ynthGenerativeSequencer::mutatePattern()
{
    if (mutationRate <= 0.0f) return;

    auto scale = static_cast<ScaleQuantizer::Scale>(
        juce::jlimit(1, static_cast<int>(ScaleQuantizer::COUNT) - 1, scaleType));
    int dpOct = ScaleQuantizer::degreesPerOctave(scale);
    int totalDegrees = dpOct * rangeOctaves;
    int baseNote = 48;

    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    std::uniform_int_distribution<int> dirDist(0, 1);

    // ── Rotation drift: circularly shift pattern arrays by 1 step ──
    //    Does NOT rebuild — preserves accumulated Turing mutations.
    if (dist(rng) < mutationRate * 0.5f && numSteps > 1)
    {
        // Rotate all arrays left by 1
        auto shiftLeft = [&](auto& arr) {
            auto first = arr[0];
            for (int i = 0; i < numSteps - 1; ++i)
                arr[static_cast<size_t>(i)] = arr[static_cast<size_t>(i + 1)];
            arr[static_cast<size_t>(numSteps - 1)] = first;
        };
        shiftLeft(eucPattern);
        shiftLeft(notePattern);
        shiftLeft(velocityPattern);
        shiftLeft(degreePattern);
    }

    // ── Pulse count drift: with P=mutation/3, shift pulse count ±1 ──
    if (dist(rng) < mutationRate * 0.33f)
    {
        int dir = dirDist(rng) == 0 ? -1 : 1;
        int newPulses = juce::jlimit(1, numSteps, numPulses + dir);
        if (newPulses != numPulses)
        {
            numPulses = newPulses;
            rebuildPattern();  // recomputes euclidean + melody from new pulse count
            return;            // rebuildPattern already calls publishPatternToGui
        }
    }

    // ── Note mutation: Turing Machine — mutate N random pulses per cycle ──
    //    N scales with evolve rate and pulse count: 1-4 notes per cycle.
    //    Each mutated note shifts ±1-2 scale degrees.
    {
        int pulseIndices[MAX_STEPS];
        int pulseCount = 0;
        for (int i = 0; i < numSteps; ++i)
            if (eucPattern[static_cast<size_t>(i)])
                pulseIndices[pulseCount++] = i;

        // How many notes to mutate this cycle (always at least attempt 1)
        int numMutations = juce::jmax(1, juce::roundToInt(mutationRate * static_cast<float>(pulseCount) * 0.25f));

        if (pulseCount > 0)
        {
            std::uniform_int_distribution<int> pickDist(0, pulseCount - 1);
            std::uniform_int_distribution<int> jumpDist(1, 4);  // ±1 to ±4 degrees

            for (int m = 0; m < numMutations; ++m)
            {
                if (dist(rng) > mutationRate) continue;

                int idx = pulseIndices[pickDist(rng)];
                int dir = dirDist(rng) == 0 ? -1 : 1;
                int jump = jumpDist(rng);
                int newDeg = degreePattern[static_cast<size_t>(idx)] + dir * jump;
                // Wrap within range (not clamp — allows full exploration)
                newDeg = ((newDeg % totalDegrees) + totalDegrees) % totalDegrees;
                degreePattern[static_cast<size_t>(idx)] = newDeg;
                notePattern[static_cast<size_t>(idx)] =
                    ScaleQuantizer::degreeToMidi(newDeg, scaleRoot, scale, baseNote);
            }
        }
    }

    publishPatternToGui();
}

void T5ynthGenerativeSequencer::publishPatternToGui()
{
    numStepsForGui.store(numSteps, std::memory_order_relaxed);
    for (int i = 0; i < MAX_STEPS; ++i)
        notePatternForGui[static_cast<size_t>(i)].store(
            i < numSteps ? notePattern[static_cast<size_t>(i)] : 0,
            std::memory_order_relaxed);
}

// ─── Process Block ─────────────────────────────────────────────────────────

void T5ynthGenerativeSequencer::processBlock(juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midi)
{
    if (!running)
    {
        if (lastPlayedNote >= 0)
        {
            midi.addEvent(juce::MidiMessage::noteOff(1, lastPlayedNote), 0);
            lastPlayedNote = -1;
        }
        currentStep = 0;
        currentStepForGui.store(-1, std::memory_order_relaxed);
        return;
    }

    if (patternDirty)
        rebuildPattern();

    const double stepDur = stepDurationSamples();
    const int numSamples = buffer.getNumSamples();
    int samplePos = 0;

    std::uniform_real_distribution<float> probDist(0.0f, 1.0f);

    while (samplePos < numSamples)
    {
        int remaining = numSamples - samplePos;

        double nextEvent = samplesUntilNextStep;
        bool gateOffFirst = false;
        if (samplesUntilGateOff >= 0.0 && samplesUntilGateOff <= samplesUntilNextStep)
        {
            nextEvent = samplesUntilGateOff;
            gateOffFirst = true;
        }

        if (nextEvent > static_cast<double>(remaining))
        {
            samplesUntilNextStep -= remaining;
            if (samplesUntilGateOff >= 0.0)
                samplesUntilGateOff -= remaining;
            samplePos = numSamples;
            break;
        }

        int advance = std::max(0, static_cast<int>(nextEvent));
        samplePos += advance;
        samplesUntilNextStep -= advance;
        if (samplesUntilGateOff >= 0.0)
            samplesUntilGateOff -= advance;

        int eventPos = juce::jmin(samplePos, numSamples - 1);

        if (gateOffFirst)
        {
            if (lastPlayedNote >= 0)
            {
                midi.addEvent(juce::MidiMessage::noteOff(1, lastPlayedNote), eventPos);
                lastPlayedNote = -1;
            }
            samplesUntilGateOff = -1.0;
            continue;
        }

        // Step boundary
        int stepIdx = scheduledStep % numSteps;

        // Cycle boundary → mutate
        if (stepIdx == 0 && scheduledStep > 0)
            mutatePattern();

        // Note-off for previous
        if (lastPlayedNote >= 0)
        {
            midi.addEvent(juce::MidiMessage::noteOff(1, lastPlayedNote), eventPos);
            lastPlayedNote = -1;
        }

        // Determine if this step should fire
        bool isPulse = eucPattern[static_cast<size_t>(stepIdx)];
        float fireProb = isPulse
            ? (0.90f + 0.10f * (1.0f - mutationRate))   // pulse: high probability
            : (mutationRate * 0.15f);                     // rest: ghost note probability

        bool shouldFire = (probDist(rng) < fireProb)
                       && notePattern[static_cast<size_t>(stepIdx)] > 0;

        if (shouldFire)
        {
            int note = notePattern[static_cast<size_t>(stepIdx)];
            float vel = velocityPattern[static_cast<size_t>(stepIdx)];

            // Ghost notes are quieter
            if (!isPulse) vel *= 0.4f;

            int velInt = juce::jlimit(1, 127, juce::roundToInt(vel * 127.0f));
            midi.addEvent(juce::MidiMessage::noteOn(1, note,
                          static_cast<juce::uint8>(velInt)), eventPos);
            lastPlayedNote = note;
            samplesUntilGateOff = static_cast<double>(gate_) * stepDur;
        }
        else
        {
            samplesUntilGateOff = -1.0;
        }

        currentStep = stepIdx;
        currentStepForGui.store(currentStep, std::memory_order_relaxed);
        scheduledStep++;
        samplesUntilNextStep += stepDur;
    }
}
