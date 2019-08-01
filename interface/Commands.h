#ifndef COMMANDS_H
#define COMMANDS_H

namespace mdaq {

  typedef enum {
    kNullCommand,
    kProgramQuit,
    kPauseRun,
    kUnpauseRun,
    kStartRun,
    kRestartRun,
    kStopRun,
    kReconfigureRun,
    kPrintConfiguration,
    kPrintBoardInfo,
    kPrintRates,
    kPrintStatus,
    kPlotDQM,
    kRotateFile,
    kPeekFile,
    kInteractiveStartRunSetTime,
    kInteractiveStartRunSetEvents,
    kInteractiveReadConfiguration,
    kInteractiveToggleSaveWaveforms,
    kStartRunSetTime,
    kStartRunSetEvents,

    // future commands
    // kReconfigure,       // this will replace reconfigurerun, but if it's unsuccessful the daq DOESN'T restart automatically, it STOPS instead
    // kToggleWaveforms,   // on by default, if turned off this fills the ROOT file with zeroes -- check that this actually saves space?

    kNumMessages
  } MessageType;

}

#endif
