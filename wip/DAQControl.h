#ifndef DAQCONTROL_H
#define DAQCONTROL_H

#include <mutex>

#include <TGButton.h>
#include <TGButtonGroup.h>
#include <TGLabel.h>
#include <TGNumberEntry.h>
#include <TG3DLine.h>
#include <TApplication.h>

#include "Commands.h"

namespace mdaq {

  class DAQControl : public TGMainFrame {

  public:
    DAQControl();
    virtual ~DAQControl();

    mdaq::MessageType GetCurrentCommand();

    void Reconfigure() { SetCurrentCommand(kInteractiveReadConfiguration); };
    void ToggleSaveWaveforms(Bool_t v) { SetCurrentCommand(kInteractiveToggleSaveWaveforms); };
    void Pause() { SetCurrentCommand(kPauseRun); };
    void Unpause() { SetCurrentCommand(kUnpauseRun); };
    void StartTimed() { SetCurrentCommand(kInteractiveStartRunSetTime); };
    void StartNEvents() { SetCurrentCommand(kInteractiveStartRunSetEvents); };
    void Stop() { SetCurrentCommand(kStopRun); };
    void Restart() { SetCurrentCommand(kRestartRun); };
    void Rotate() { SetCurrentCommand(kRotateFile); };
    void Peek() { SetCurrentCommand(kPeekFile); };
    void Quit() { SetCurrentCommand(kProgramQuit); };

  protected:
    TGTextButton * b_readConfig;
    TGCheckButton * b_toggleSaveWaves;
    TGTextButton * b_start_timed;
    TGTextButton * b_start_nevents;
    TGTextButton * b_pause;
    TGTextButton * b_unpause;
    TGTextButton * b_stop;
    TGTextButton * b_restart;
    TGTextButton * b_rotate;
    TGTextButton * b_peek;
    TGTextButton * b_quit;

  private:
    std::mutex theMutex;

    void SetCurrentCommand(mdaq::MessageType v);

    bool isPaused = false;
    Bool_t saveWaveforms = true;

    mdaq::MessageType currentCommand;


  public:
    ClassDef(mdaq::DAQControl, 1)
      };

}

#endif
