#include "Logger.h"

#include <stdexcept>

typedef std::lock_guard<std::mutex> Lock;

const char* const mdaq::Logger::kLogFileName = LOG_FILE;
mdaq::Logger * mdaq::Logger::pInstance = nullptr;
std::mutex mdaq::Logger::sMutex;

mdaq::Logger& mdaq::Logger::instance() {
  static Cleanup cleanup;
  Lock lk(sMutex);
  if(pInstance == nullptr) pInstance = new Logger();
  return *pInstance;
}

mdaq::Logger::Cleanup::~Cleanup() {
  Lock lk(mdaq::Logger::sMutex);
  delete mdaq::Logger::pInstance;
  mdaq::Logger::pInstance = nullptr;
}

mdaq::Logger::~Logger() {
  mOutputStream.close();
}

mdaq::Logger::Logger() {
  useLogFile = true;
  mOutputStream.open(kLogFileName, std::ios_base::app);
  if(!mOutputStream.good()) {
    std::cout << "[Logger]: Unable to initialize the log file at " << kLogFileName << " !!! Using stdout instead!" << std::endl;
    SetUseLogFile(false);
    //throw std::runtime_error("Unable to initialize the Logger!");
  }
}

void mdaq::Logger::log(const std::string& source, const TString& message, const SeverityLevel level) {
  time_t t_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  char ch_t[40];
  strftime(ch_t, sizeof(ch_t), "%c %Z", gmtime(&t_t));

  std::string color;

  Lock lk(sMutex);
  if(useLogFile) {
    mOutputStream << ch_t
                  << severityColors[level]
                  << " " << severityNames[level] << ": [" << source << "]: "
                  << message
                  << A_RESET
                  << std::endl;
  }
  else {
    std::cout << ch_t
              << severityColors[level]
              << " " << severityNames[level] << ": [" << source << "]: "
              << message
              << A_RESET
              << std::endl;
    std::cout.flush();
  }

}
