#include "DriftLFO.h"

void DriftLFO::reset()
{
    for (auto& lfo : lfos)
    {
        lfo.phase = 0.0;
        lfo.rate = lfo.baseRate;
        lfo.depth = lfo.baseDepth;
    }
}

void DriftLFO::tick(double dt)
{
    if (!active)
        return;

    for (auto& lfo : lfos)
    {
        lfo.phase += static_cast<double>(lfo.rate) * dt;

        // Wrap phase to prevent precision loss over long runs
        if (lfo.phase >= 1.0)
        {
            lfo.phase -= std::floor(lfo.phase);

            // Regenerate rate/depth on cycle completion
            if (autoRegen && lfo.baseDepth > 0.0f)
            {
                std::uniform_real_distribution<float> rateDist(
                    lfo.baseRate * 0.5f, lfo.baseRate * 1.5f);
                std::uniform_real_distribution<float> depthDist(
                    lfo.baseDepth * 0.5f, lfo.baseDepth * 1.5f);
                lfo.rate = rateDist(rng);
                lfo.depth = depthDist(rng);
            }
        }
    }
}

float DriftLFO::getOffsetForTarget(int target) const
{
    if (!active || target < 0 || target >= NumTargets)
        return 0.0f;

    float totalOffset = 0.0f;

    for (const auto& lfo : lfos)
    {
        if (lfo.target == target && lfo.depth != 0.0f)
        {
            float value = static_cast<float>(std::sin(lfo.phase * TWO_PI));
            totalOffset += value * lfo.depth;
        }
    }

    return totalOffset;
}

void DriftLFO::setLfoRate(int lfoIndex, float hz)
{
    if (lfoIndex >= 0 && lfoIndex < NUM_LFOS)
    {
        auto& lfo = lfos[static_cast<size_t>(lfoIndex)];
        lfo.baseRate = hz;
        if (!autoRegen)
            lfo.rate = hz;
    }
}

void DriftLFO::setLfoDepth(int lfoIndex, float amount)
{
    if (lfoIndex >= 0 && lfoIndex < NUM_LFOS)
    {
        auto& lfo = lfos[static_cast<size_t>(lfoIndex)];
        lfo.baseDepth = amount;
        if (!autoRegen)
            lfo.depth = amount;
    }
}

void DriftLFO::setLfoTarget(int lfoIndex, int target)
{
    if (lfoIndex >= 0 && lfoIndex < NUM_LFOS && target >= 0 && target < NumTargets)
        lfos[static_cast<size_t>(lfoIndex)].target = target;
}
