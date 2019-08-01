#include "DAQControl.h"

ClassImp(mdaq::DAQControl);

mdaq::DAQControl::DAQControl() : TGMainFrame(gClient->GetRoot(), 10, 10, kHorizontalFrame) {

  currentCommand = kNullCommand;

  SetCleanup(kDeepCleanup);

  TGHorizontalFrame * contents = new TGHorizontalFrame(this);
  AddFrame(contents, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 5, 5));

  b_readConfig = new TGTextButton(contents, "    Read\nconfiguration");
  b_readConfig->Resize(100, 100);
  b_readConfig->ChangeOptions(b_readConfig->GetOptions() | kFixedSize);
  contents->AddFrame(b_readConfig, new TGLayoutHints(kLHintsCenterX | kLHintsCenterY, 5, 5, 5, 5));
  b_readConfig->Connect("Pressed()", "mdaq::DAQControl", this, "Reconfigure()");

  b_toggleSaveWaves = new TGCheckButton(contents, "Save Waveforms");
  b_toggleSaveWaves->SetOn();
  b_toggleSaveWaves->Connect("Toggled(Bool_t)", "mdaq::DAQControl", this, "ToggleSaveWaveforms(Bool_t)");
  contents->AddFrame(b_toggleSaveWaves, new TGLayoutHints(kLHintsCenterX | kLHintsCenterY, 5, 5, 5, 5));

  b_start_timed = new TGTextButton(contents, "   Start\nTimed Run");
  b_start_timed->Resize(100, 100);
  b_start_timed->ChangeOptions(b_start_timed->GetOptions() | kFixedSize);
  contents->AddFrame(b_start_timed, new TGLayoutHints(kLHintsCenterX | kLHintsCenterY, 5, 5, 5, 5));
  b_start_timed->Connect("Pressed()", "mdaq::DAQControl", this, "StartTimed()");

  b_start_nevents = new TGTextButton(contents, "   Start\nNEvents Run");
  b_start_nevents->Resize(100, 100);
  b_start_nevents->ChangeOptions(b_start_nevents->GetOptions() | kFixedSize);
  contents->AddFrame(b_start_nevents, new TGLayoutHints(kLHintsCenterX | kLHintsCenterY, 5, 5, 5, 5));
  b_start_nevents->Connect("Pressed()", "mdaq::DAQControl", this, "StartNEvents()");

  b_pause = new TGTextButton(contents, "Pause");
  b_pause->Resize(100, 100);
  b_pause->ChangeOptions(b_pause->GetOptions() | kFixedSize);
  contents->AddFrame(b_pause, new TGLayoutHints(kLHintsCenterX | kLHintsCenterY, 5, 5, 5, 5));
  b_pause->Connect("Pressed()", "mdaq::DAQControl", this, "Pause()");

  b_unpause = new TGTextButton(contents, "Unpause");
  b_unpause->Resize(100, 100);
  b_unpause->ChangeOptions(b_unpause->GetOptions() | kFixedSize);
  contents->AddFrame(b_unpause, new TGLayoutHints(kLHintsCenterX | kLHintsCenterY, 5, 5, 5, 5));
  b_unpause->Connect("Pressed()", "mdaq::DAQControl", this, "Unpause()");

  b_stop = new TGTextButton(contents, "Stop");
  b_stop->Resize(100, 100);
  b_stop->ChangeOptions(b_stop->GetOptions() | kFixedSize);
  contents->AddFrame(b_stop, new TGLayoutHints(kLHintsCenterX | kLHintsCenterY, 5, 5, 5, 5));
  b_stop->Connect("Pressed()", "mdaq::DAQControl", this, "Stop()");

  b_restart = new TGTextButton(contents, "Restart");
  b_restart->Resize(100, 100);
  b_restart->ChangeOptions(b_restart->GetOptions() | kFixedSize);
  contents->AddFrame(b_restart, new TGLayoutHints(kLHintsCenterX | kLHintsCenterY, 5, 5, 5, 5));
  b_restart->Connect("Pressed()", "mdaq::DAQControl", this, "Restart()");

  b_rotate = new TGTextButton(contents, "Rotate");
  b_rotate->Resize(100, 100);
  b_rotate->ChangeOptions(b_rotate->GetOptions() | kFixedSize);
  contents->AddFrame(b_rotate, new TGLayoutHints(kLHintsCenterX | kLHintsCenterY, 5, 5, 5, 5));
  b_rotate->Connect("Pressed()", "mdaq::DAQControl", this, "Rotate()");
  
  b_peek = new TGTextButton(contents, "Peek");
  b_peek->Resize(100, 100);
  b_peek->ChangeOptions(b_peek->GetOptions() | kFixedSize);
  contents->AddFrame(b_peek, new TGLayoutHints(kLHintsCenterX | kLHintsCenterY, 5, 5, 5, 5));
  b_peek->Connect("Pressed()", "mdaq::DAQControl", this, "Peek()");

  b_quit = new TGTextButton(contents, "Quit");
  b_quit->Resize(100, 100);
  b_quit->ChangeOptions(b_quit->GetOptions() | kFixedSize);
  contents->AddFrame(b_quit, new TGLayoutHints(kLHintsCenterX | kLHintsCenterY, 5, 5, 5, 5));
  b_quit->Connect("Pressed()", "mdaq::DAQControl", this, "Quit()");

  DontCallClose();

  MapSubwindows();
  Resize();

  SetWMSizeHints(GetDefaultWidth(), GetDefaultHeight(), 1000, 1000, 0 ,0);
  SetWindowName("Interactive DAQ Control");
  MapRaised();

}

mdaq::DAQControl::~DAQControl() {
  delete b_readConfig;
  delete b_toggleSaveWaves;
  delete b_start_timed;
  delete b_start_nevents;
  delete b_pause;
  delete b_unpause;
  delete b_stop;
  delete b_restart;
  delete b_rotate;
  delete b_peek;
  delete b_quit;
}

mdaq::MessageType mdaq::DAQControl::GetCurrentCommand() { 
  mdaq::MessageType val;
  { std::unique_lock<std::mutex> lk(theMutex);
    val = currentCommand;
    currentCommand = kNullCommand;
  }
  return val; 
}

void mdaq::DAQControl::SetCurrentCommand(mdaq::MessageType v) {
  std::unique_lock<std::mutex> lk(theMutex);
  currentCommand = v;
}