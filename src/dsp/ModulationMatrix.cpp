#include "ModulationMatrix.h"

void ModulationMatrix::prepare(double sampleRate)
{
    sr = sampleRate;
}

void ModulationMatrix::reset()
{
    // Keep routes, just reset internal state
}

void ModulationMatrix::process()
{
    // Stub: routing logic to be implemented
}

void ModulationMatrix::addRoute(int source, int destination, float amount)
{
    routes.push_back({ source, destination, amount });
}

void ModulationMatrix::clearRoutes()
{
    routes.clear();
}

float ModulationMatrix::getModulationValue(int /*destination*/) const
{
    return 0.0f; // Stub
}
