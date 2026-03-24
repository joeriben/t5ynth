#pragma once
#include <JuceHeader.h>
#include <vector>

/**
 * Modulation matrix connecting sources to destinations.
 *
 * Sources: LFO1, LFO2, ModEnv1, ModEnv2, Velocity, Aftertouch, DriftLFO
 * Destinations: Filter cutoff, resonance, osc scan, delay time, etc.
 *
 * Stub implementation — routing logic to be added.
 */
class ModulationMatrix
{
public:
    ModulationMatrix() = default;

    void prepare(double sampleRate);
    void reset();

    /** Process modulation routing for one block. */
    void process();

    /** Add a modulation route. */
    void addRoute(int source, int destination, float amount);

    /** Remove all routes. */
    void clearRoutes();

    /** Get the current modulation offset for a destination. */
    float getModulationValue(int destination) const;

private:
    struct Route
    {
        int source = 0;
        int destination = 0;
        float amount = 0.0f;
    };

    std::vector<Route> routes;
    double sr = 44100.0;
};
