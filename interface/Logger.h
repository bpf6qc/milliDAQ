#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <chrono>
#include <ctime>

#include "TString.h"

#define  A_BLACK    ("\033[30m")
#define  A_RED      ("\033[31m")
#define  A_GREEN    ("\033[32m")
#define  A_YELLOW   ("\033[33m")
#define  A_BLUE     ("\033[34m")
#define  A_MAGENTA  ("\033[35m")
#define  A_CYAN     ("\033[36m")
#define  A_WHITE    ("\033[37m")

#define  A_BRIGHT_BLACK    ("\033[1;30m")
#define  A_BRIGHT_RED      ("\033[1;31m")
#define  A_BRIGHT_GREEN    ("\033[1;32m")
#define  A_BRIGHT_YELLOW   ("\033[1;33m")
#define  A_BRIGHT_BLUE     ("\033[1;34m")
#define  A_BRIGHT_MAGENTA  ("\033[1;35m")
#define  A_BRIGHT_CYAN     ("\033[1;36m")
#define  A_BRIGHT_WHITE    ("\033[1;37m")

#define  A_RESET    ("\033[0m")

#define LOG_FILE "/var/log/MilliDAQ.log"

namespace mdaq {

  typedef enum {
    kDebug,
    kInfo,
    kCommand,
    kBold,
    kWarning,
    kError,
    kNumSeverityLevels
  } SeverityLevel;

  class Logger {
  public:
    static Logger& instance();

    void SetUseLogFile(const bool v = false) { useLogFile = v; };

    void log(const std::string& source, const TString& message, const SeverityLevel level);

    void debug(const std::string& source, const TString& message) { log(source, message, kDebug); };
    void info(const std::string& source, const TString& message) { log(source, message, kInfo); };
    void command(const std::string& source, const TString& message) { log(source, message, kCommand); };
    void bold(const std::string& source, const TString& message) { log(source, message, kBold); };
    void warning(const std::string& source, const TString& message) { log(source, message, kWarning); };
    void error(const std::string& source, const TString& message) { log(source, message, kError); };

  protected:
    static Logger* pInstance;
    static const char* const kLogFileName;

    std::ofstream mOutputStream;

    friend class Cleanup;
    class Cleanup {
    public:
      ~Cleanup();
    };

  private:
    Logger();
    virtual ~Logger();
    Logger(const Logger&);
    Logger& operator=(const Logger&);
    static std::mutex sMutex;

    bool useLogFile = true;

    const std::string severityColors[kNumSeverityLevels] {
      A_BLUE,
	"", // regular black text
	A_GREEN,
	A_BRIGHT_BLACK,
	A_YELLOW,
	A_BRIGHT_RED
	};

    const std::string severityNames[kNumSeverityLevels] {
      "Debug",
	"Info",
	"User command",
	"Info",
	"Warning",
	"ERROR"
	};
  };

}

#endif
