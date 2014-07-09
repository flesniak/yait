#include "logger/logger.h"

#include <string>

struct options_s {
  std::string isoPath;
  std::string ripPath;
  std::string irdPath;
  enum operation_e { none, opCheck, opList, opHash } operation;
};

void usage( char* argv0 ) {
  LOG(logInfo) << "Usage: %s: [-hvq] <-c|l|m> [-i iso-file] [-d rip-directory] [-f ird-file]\n", argv0;
  LOG(logInfo) << "One of the following modes is obligatory:";
  LOG(logInfo) << " -c: Check an iso or directory against the ird (includes hash-check)";
  LOG(logInfo) << " -l: List files inside an ird, iso or directory";
  LOG(logInfo) << " -m: List and hash files inside an iso or directory";
  exit(1);
}

void parseOptions( int argc, char** argv, struct options_s* o ) {
  int o = 0;
  while( (o = getopt(argc, argv, "hvqclmi:d:f:") != -1 ) {
    switch( o ) {
      case 'v':
        if( Logger::logLevel() < Logger::logDebug )
          Logger::logLevel() = (logLevel_t)( (uint32_t)Logger::logLevel() + 1 );
        break;
      case 'q':
        if( Logger::logLevel() > Logger::logError )
          Logger::logLevel() = (logLevel_t)( (uint32_t)Logger::logLevel() - 1 );
        break;
      case 'i':
        o->isoPath = std::string(optarg);
        break;
      case 'd':
        o->dirPath = std::string(optarg);
        break;
      case 'f':
        o->irdPath = std::string(optarg);
        break;
      case 'c':
        if( o->operation != none ) {
          LOG(logError) << "Multiple operations specified!";
          usage(argv[0]);
        } else
          o->operation = opCheck;
        break;
      case 'l':
        if( o->operation != none ) {
          LOG(logError) << "Multiple operations specified!";
          usage(argv[0]);
        } else
          o->operation = opList;
        break;
      case 'm':
        if( o->operation != none ) {
          LOG(logError) << "Multiple operations specified!";
          usage(argv[0]);
        } else
          o->operation = opHash;
        break;
      case 'h':
        usage(argv[0]);
        break;
      default:
        usage(argv[0]);
    }
  }

  if( optind < argc ) {
    LOG(logError) << "Additional, unhandled non-option attributes!";
    usage(argv[0]);
  }

  //check collision between operation and file arguments
  if( o->operation == opCheck && ( irdPath.empty() || ( isoPath.empty() && dirPath.empty() ) ) ) {
    LOG(logError) << "Operation check requires an ird and either an iso or a directory";
    usage(argv[0]);
  }
  if( o->operation == opList && ( irdPath.empty() + isoPath.empty() + dirPath.empty() ) != 2 ) {
    LOG(logError) << "Operation list expects exactly one path (ird, iso or directory)";
    usage(argv[0]);
  }
  if( o->operation == opHash && ( irdPath.empty() + isoPath.empty() + dirPath.empty() ) != 2 ) {
    LOG(logError) << "Operation hash expects exactly one path (ird, iso or directory)";
    usage(argv[0]);
  }
}

int main(int argc, char** argv) {
  //initialize logger
  Logger::logLevel() = logInfo;
  Logger::facility() = new LoggerFacilityConsole;

  struct options_s options;
  parseOptions( argc, argv, &options );

  yait* y = new yait();
}
