#ifndef INTERACTIVEDAQ_H
#define INTERACTIVEDAQ_H

//#include "DAQControl.h"
#include "MilliDAQ.h"
#include "TApplication.h"
//#include "Keyboard.h"

namespace mdaq {

  std::atomic<bool> isTimedRun(true);
  std::atomic<float> runTimeRequested(0.0);
  std::atomic<unsigned int> runEventsRequested(0);

  std::atomic<bool> saveWaveforms(true);

  const std::string xmlSuffix = ".xml";
  const std::string pythonSuffix = ".py";

  /*
  // condition sent when we're done connecting to and/or configuring the digitizer
  std::atomic<bool>       controlExists(false);
  std::condition_variable controlDeclared;
  std::mutex              controlMutex;
  DAQControl * control;
  */
}

#endif
