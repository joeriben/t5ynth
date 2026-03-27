#pragma once
#include <cmath>
#include <array>
#include <random>

/**
 * Drift LFO system — offset-based parameter drift.
 *
 * Ported from AI4ArtsEd's useDriftLfo.ts concept.
 *
 * Architecture:
 * - 3 internal sine-based LFOs running at different rates
 * - Each LFO can target: alpha, sem_axis_1, sem_axis_2, sem_axis_3, wt_scan
 * - Output is an OFFSET (added to base parameter value), not absolute
 * - Rate and depth per-LFO, independently configurable
 * - tick(dt) advances all LFOs by dt seconds
 */
class DriftLFO
{
public:
    /** Target parameter indices. */
    enum Target
    {
        Alpha = 0,
        SemAxis1,
        SemAxis2,
        SemAxis3,
        WtScan,
        NumTargets
    };

    static constexpr int NUM_LFOS = 3;

    DriftLFO() = default;

    /** Reset all phases and offsets. */
    void reset();

    /** Advance all LFOs by dt seconds. */
    void tick(double dt);

    /** Get the combined offset for a given target parameter. */
    float getOffsetForTarget(int target) const;

    /** Configure a single internal LFO. */
    void setLfoRate(int lfoIndex, float hz);
    void setLfoDepth(int lfoIndex, float amount);
    void setLfoTarget(int lfoIndex, int target);

    /** Enable/disable the entire drift system. */
    void setEnabled(bool enabled) { active = enabled; }
    bool isEnabled() const { return active; }

    /** Enable auto-regeneration: randomize rate/depth on each cycle wrap. */
    void setAutoRegen(bool enabled) { autoRegen = enabled; }

private:
    struct InternalLFO
    {
        double phase = 0.0;
        float rate = 0.1f;      // Hz (current, possibly randomized)
        float depth = 0.0f;     // offset amplitude (current, possibly randomized)
        int target = Alpha;     // which parameter to modulate
        float baseRate = 0.1f;  // configured rate (center of randomization)
        float baseDepth = 0.0f; // configured depth (center of randomization)
    };

    std::array<InternalLFO, NUM_LFOS> lfos;
    bool active = false;
    bool autoRegen = false;
    std::mt19937 rng { std::random_device{}() };

    static constexpr double TWO_PI = 6.283185307179586;
};
