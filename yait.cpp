#include "yait.h"
#include "logger/logger.h"

yait::yait() {
  if( Logger:facility() == 0 ) {
    Logger::logLevel() = logInfo;
    Logger::facility() = new LoggerFacilityConsole;
    LOG(logWarning) << "Logger was not initialized yet, so yait initialized it on its own.";
    LOG(logWarning) << "To control yait's logging behaviour, initialize it yourself.";
  }
}
