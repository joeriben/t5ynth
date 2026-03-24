#include "BackendManager.h"

BackendManager::~BackendManager()
{
    stop();
}

void BackendManager::start()
{
    // Stub: launch Python backend process
    running = false; // Will be set true once implemented
}

void BackendManager::stop()
{
    running = false;
}
