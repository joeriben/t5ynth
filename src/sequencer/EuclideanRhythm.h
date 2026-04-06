#pragma once
#include <array>
#include <algorithm>

/**
 * Euclidean rhythm generator (Bjorklund algorithm).
 *
 * Distributes `pulses` onsets as evenly as possible across `steps` slots,
 * then applies a circular `rotation` offset.
 *
 * Based on: Toussaint, "The Euclidean Algorithm Generates Traditional
 * Musical Rhythms" (2005).
 */
namespace EuclideanRhythm
{

static constexpr int MAX_STEPS = 64;

/**
 * Generate a Euclidean rhythm pattern.
 *
 * @param steps    Total number of steps (1–64)
 * @param pulses   Number of active beats (0–steps)
 * @param rotation Circular shift applied after generation (0–steps-1)
 * @return         Fixed-size array; indices [0, steps) contain the pattern,
 *                 indices [steps, 64) are false.
 */
inline std::array<bool, MAX_STEPS> generate(int steps, int pulses, int rotation)
{
    std::array<bool, MAX_STEPS> result{};

    if (steps <= 0) return result;

    pulses = std::max(0, std::min(pulses, steps));
    if (steps > 0) rotation = ((rotation % steps) + steps) % steps;

    if (pulses == 0) return result;
    if (pulses == steps)
    {
        for (int i = 0; i < steps; ++i) result[static_cast<size_t>(i)] = true;
        return result;
    }

    // Bjorklund's algorithm — iterative grouping
    // We build the pattern in a flat work array, tracking group boundaries.
    // Equivalent to the Euclidean GCD string-construction.

    // Use Bresenham-style approach (simpler, equivalent result):
    // Place a pulse at step i if floor((i+1)*pulses/steps) > floor(i*pulses/steps)
    std::array<bool, MAX_STEPS> raw{};
    for (int i = 0; i < steps; ++i)
        raw[static_cast<size_t>(i)] =
            ((i + 1) * pulses / steps) > (i * pulses / steps);

    // Apply rotation (circular shift right by `rotation`)
    for (int i = 0; i < steps; ++i)
        result[static_cast<size_t>(i)] =
            raw[static_cast<size_t>((i - rotation + steps) % steps)];

    return result;
}

} // namespace EuclideanRhythm
