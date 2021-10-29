#include "Logger.h"

std::shared_ptr<AbstractLogger> cspotGlobalLogger;

void setDefaultLogger() {
    cspotGlobalLogger = std::make_shared<CspotLogger>();
}