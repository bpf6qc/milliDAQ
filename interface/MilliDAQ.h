#ifndef MILLIDAQ_H
#define MILLIDAQ_H

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <limits.h>
#include <iostream>
#include <iomanip>

#include "DAQ.h"
#include "EventFactory.h"
#include "Logger.h"
#include "Commands.h"

#define FIFO_FILE "/tmp/MilliDAQ_FIFO"

typedef std::lock_guard<std::mutex>  Guard;
typedef std::unique_lock<std::mutex> Lock;

typedef enum {
  kConnecting,
  kQuitting,
  kPaused,
  kStopped,
  kStartingNotReady,
  kRunningTimed,
  kRunningNEvents,
  kRunning,
  kNumRunStates
} RunState_t;

namespace mdaq {

  ////////////////////////////////////////////////////////////////////
  // Global objects -- needed by multiple threads
  ////////////////////////////////////////////////////////////////////

  // the acquisition state and all methods
  DAQ * theDAQ;
  std::mutex theDAQMutex;

  // the master event queues
  // ReadAction() will append events, and WriteAction() will remove events
  Queue * masterQueues[nDigitizers];
  std::mutex masterQueueMutex;

  bool needEmergencyClose = false;

  ////////////////////////////////////////////////////////////////////
  // Inter-thread user command flags
  ////////////////////////////////////////////////////////////////////

  // the user wants to quit the program: all threads must end cleanly
  std::atomic<bool> UserProgramQuit(false);

  // the user wants to pause the run: threads will wait until resumed
  std::atomic<bool> UserPauseRun(false);  // only allowed when running

  // the user wants to start, restart, or stop the run: similar to pause/resume but will force reconfigure and new output files
  std::atomic<bool> UserRestartRun(false); // only allowed when running; effectively stop + start
  std::atomic<bool> UserStopRun(false);    // only allowed when running or paused

  // the user wants to start a timed or target-nevents run: same as start, but automatically sends stop when target time/nevents is reached
  std::atomic<float>  UserTimedRunDuration(-1.0); // duration of run in seconds; negative means untimed run
  std::atomic<int>    UserNeventsRunDuration(-1); // target nevents of run; negative means no limit

  // the user wants to reconfigure the digitizer: this means stop, read new configuration, start
  std::atomic<bool> UserReconfigureRun(false);

  // on-demand information requests
  std::atomic<bool> UserPrintConfiguration(false); // print the current V1743Configuration object
  std::atomic<bool> UserPrintBoardInfo(false);     // print the BoardInfo object
  std::atomic<bool> UserPrintRates(false);         // print things like trigger rates, queue usage, events recorded, etc
  std::atomic<bool> UserPrintStatus(false);        // print the overall status of the DAQ like running/stopped, number of events in run, etc
  std::atomic<bool> UserPlotDQM(false);            // force the DQM to save PDFs immediately
  std::atomic<bool> UserRotateFile(false);         // force the current file to be rotated immediately
  std::atomic<bool> UserPeekFile(false);           // on top of the normal recording, store the next 10 events in a small file

  ////////////////////////////////////////////////////////////////////
  // Inter-thread messages
  ////////////////////////////////////////////////////////////////////

  // condition sent when we're done connecting to and/or configuring the digitizer
  std::atomic<bool>       digitizerReady(false);
  std::condition_variable digitizerBecameReady;
  std::mutex              digitizerReadyMutex;

  std::atomic<bool>       digitizerIsStopped(false);
  std::condition_variable digitizerHasStopped;
  std::mutex              digitizerStoppedMutex;

  std::atomic<bool>       safeToQuit(false);
  std::condition_variable becameSafeToQuit;
  std::mutex              safeToQuitMutex;

  std::string currentConfigFile = "/home/milliqan/MilliDAQ/config/externalTrigger.xml";
  std::mutex currentConfigFileMutex;

}

#endif
