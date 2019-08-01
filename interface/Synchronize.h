#ifndef SYNCHRONIZE_H
#define SYNCHRONIZE_H

#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include <string>
#include <fstream>
#include <math.h>
#include <thread>

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/timeb.h>

#include "DAQ.h"
#include "DemonstratorConstants.h"
#include "Logger.h"

#include "TCanvas.h"
#include "TH1D.h"
#include "TApplication.h"
#include "TSystem.h"

namespace mdaq {
  DAQ * theDAQ;
  Queue * masterQueues[nDigitizers];
  std::vector<mdaq::V1743Event*> BoardEvents[nDigitizers];

  std::atomic<bool> UserQuitsProgram(false);
  std::atomic<bool> UserStartsRun(false);
  std::atomic<bool> UserAlignsTT(false);
  std::atomic<bool> UserResetsPlots(false);

  chrtime::time_point previousTime;
  std::chrono::duration<float> deltaT;
  int nMillisecondsLive;

  double TTT[nDigitizers];
  double previousTTT[nDigitizers];

  unsigned int missingEdge[nDigitizers];
  uint64_t numRollovers[nDigitizers];
  double edgeTime[nDigitizers];

  double meanT, sigmaT;
  double meanTTT, sigmaTTT;
  unsigned int nMeasurements;

  const int matchingWindow = 200 * 10.0; // [ns]

  const unsigned int referenceChannel = 0;

  bool alignTT;
}


#endif
