#include "InteractiveDAQ.h"

using namespace mdaq;
using namespace std;

void ReadAction() {

  // create and connect to the digitizer
  { Lock lk(theDAQMutex);
    theDAQ = new DAQ();
    theDAQ->ConnectToBoards();

    // read the current config; if you can't, reset current config to theDAQ's latest good one
    { Lock lk2(currentConfigFileMutex);
      if(!theDAQ->ReadConfiguration(currentConfigFile)) currentConfigFile = theDAQ->GetConfigurationFile();
    }

    // Initialize the board from its current DemonstratorConfiguration
    theDAQ->InitializeBoards();

    // start the run (ie allow the digitizer to accept triggers)
    theDAQ->StartRun();
  }

  // in interactive mode, if a specific run time has been requested,
  // then the read thread checks if this time has elapsed
  chrono::duration<float> runLength(runTimeRequested.load());
  chrtime::time_point initialTime;

  // now that data is coming, wake up other threads
  { Guard lk(digitizerReadyMutex);
    digitizerReady.store(true);
  }
  digitizerBecameReady.notify_all();

  mdaq::Logger::instance().bold("MilliDAQ", "Acquisition has begun!");

  if(isTimedRun.load() && runTimeRequested.load() >= 0.0) initialTime = chrtime::now();

  // until the run is paused or stopped, read data from the digitizer
  while(!UserProgramQuit.load()) {

    { Lock lk(theDAQMutex);
      theDAQ->SendSWTriggers();

      // attempt to read events from the board, appending any to the local queue
      theDAQ->Read(masterQueues);
    }

    // with 12 dedicated cores, there actually can be no break between the release of theDAQMutex and the next iteration of this loop
    this_thread::sleep_for(chrono::milliseconds(5));

    // negative time --> don't actually time the run
    if(isTimedRun.load() && runTimeRequested.load() >= 0.0) {
      if(chrtime::now() - initialTime > runLength) {
        mdaq::Logger::instance().bold("MilliDAQ", "Timed run complete! Stopping acquisition now.");
        UserStopRun.store(true);
        UserPauseRun.store(false);
      }
    }

    if(UserPrintConfiguration.load()) {
      Lock lk(theDAQMutex);
      theDAQ->GetConfiguration()->LogConfiguration();
      UserPrintConfiguration.store(false);
    }

    if(UserPrintBoardInfo.load()) {
      Lock lk(theDAQMutex);
      theDAQ->PrintBoardInfos();
      UserPrintBoardInfo.store(false);
    }

    // if this thread is told the run is paused
    while(UserPauseRun.load()) {
      // If while paused, this thread is told to stop the run
      // or the user wants to quit the program, leave
      if(UserProgramQuit.load() || UserStopRun.load()) break;

      // otherwise just wait
      this_thread::sleep_for(chrono::seconds(1));

    }

    // if this thread is told the run is stopped
    if(UserStopRun.load()) {

      // first stop acquisition entirely
      { Lock lk(theDAQMutex);
        theDAQ->StopRun();
      }

      { Lock lk(digitizerReadyMutex);
        digitizerReady.store(false);
      }

      // notify other threads that we've stopped
      { Guard lk(digitizerStoppedMutex);
        digitizerIsStopped.store(true);
      }
      digitizerHasStopped.notify_all();

      if(!UserReconfigureRun.load() && !UserRestartRun.load()) {
        mdaq::Logger::instance().bold("MilliDAQ", "Currently stopped. Issue \"start\" to resume.");
      }

      // Now wait until we're not stopped any more
      while(UserStopRun.load()) {
        // If the user wants to quit the program, leave
        if(UserProgramQuit.load()) break;

        // otherwise just wait
        this_thread::sleep_for(chrono::milliseconds(50));
      }

      // if we're told to reconfigure, that means now we reconfigure + start
      if(UserReconfigureRun.load()) {
        { Lock lk(theDAQMutex);
          theDAQ->StopRun(); // First stop

          // change the config file; if it's not a good one, keep using the latest good one in theDAQ
          { Lock lk2(currentConfigFileMutex);
            if(!theDAQ->ReadConfiguration(currentConfigFile)) currentConfigFile = theDAQ->GetConfigurationFile();
          }

          // Initialize the board from this new V1743Configuration
          theDAQ->InitializeBoards();

          // start the run (ie allow the digitizer to accept triggers)
          theDAQ->StartRun();
        }

        { Lock lk(digitizerStoppedMutex);
          digitizerIsStopped.store(false);
        }

        // now tell the other threads they can go on
        { Guard lk(digitizerReadyMutex);
          digitizerReady.store(true);
        }
        digitizerBecameReady.notify_all();

        mdaq::Logger::instance().bold("MilliDAQ", "Now acquiring events!");

        continue;
      }
      // otherwise if we weren't told to quit, we were told to start a run
      else if(!UserProgramQuit.load()) {
        { Lock lk(theDAQMutex);
          // Initialize the board from its current V1743Configuration
          theDAQ->InitializeBoards();

          // start the run (ie allow the digitizer to accept triggers)
          theDAQ->StartRun();
        }

        // reset run timers
        runLength = chrono::duration<float>(runTimeRequested.load());
        initialTime = chrtime::now();

        { Lock lk(digitizerStoppedMutex);
          digitizerIsStopped.store(false);
        }

        // notify other threads that we're ready
        { Guard lk(digitizerReadyMutex);
          digitizerReady.store(true);
        }
        digitizerBecameReady.notify_all();

        mdaq::Logger::instance().bold("MilliDAQ", "Now acquiring events!");

        continue;
      }

    } // if UserStopRun

  }

  // we're now quitting the program
  { Lock lk(theDAQMutex);
    theDAQ->StopRun();

    // close the digitizer
    theDAQ->CloseDevices();
    if(theDAQ) delete theDAQ;
  }

  return;
}

void WriteAction() {

  // Wait for reader to finish connecting and configuring the digitizer
  {
    Lock lk(digitizerReadyMutex);
    digitizerBecameReady.wait(lk, []{return digitizerReady.load();});
  }

  // define the event factory
  EventFactory * factory;
  { Lock lk(theDAQMutex);
    factory = new EventFactory("/home/milliqan/tmp/MilliQanEvents.root", "/home/milliqan/data/", theDAQ->GetConfiguration(), true);
  }

  // now run until told to quit the program
  while(!UserProgramQuit.load()) {

    factory->FlushQueuesToDisk(masterQueues);
    factory->GetDQM()->UpdateInteractiveCanvases();
    this_thread::sleep_for(chrono::milliseconds(50));

    // in interactive mode, if a target number of events is requested, the writer thread handles this check
    if(!isTimedRun.load()) {
      if(factory->EventsRecordedInRun() >= runEventsRequested) {
        mdaq::Logger::instance().bold("MilliDAQ", "Requested number of events recorded! Stopping acquisition now.");
        UserStopRun.store(true);
        UserPauseRun.store(false);
      }
    }

    // if the thread is told to rotate the file
    if(UserRotateFile.load()) {
      factory->RotateFile();
      UserRotateFile.store(false);
    }

    // if the thread is told to peek 10 events to file
    if(UserPeekFile.load()) {
      factory->PeekFile();
      UserPeekFile.store(false);
    }

    // if the thread is told to print the dqm info
    if(UserPrintRates.load()) {
      factory->GetDQM()->PrintRates();
      UserPrintRates.store(false);
    }

    // if this thread is told the run is paused
    while(UserPauseRun.load()) {
      // If while paused, this thread is told to stop the run
      // or the user wants to quit the program, leave
      if(UserProgramQuit.load() || UserStopRun.load()) break;

      // otherwise just wait
      this_thread::sleep_for(chrono::seconds(1));
    }

    // if this thread is told the run is stopped
    if(UserStopRun.load()) {

      // finish off the current output file
      factory->FlushQueuesToDisk(masterQueues);
      factory->FinishFile();
      factory->ResetEventsRecordedInRun();

      // wait until we've really stopped
      { Lock lk(digitizerStoppedMutex);
        digitizerHasStopped.wait(lk, []{return digitizerIsStopped.load();});
      }

      // Now wait until we're not stopped any more
      while(UserStopRun.load()) {
        // If the user wants to quit the program, leave
        if(UserProgramQuit.load()) break;

        // otherwise just wait
        this_thread::sleep_for(chrono::milliseconds(50));
      }

      // if we're told to reconfigure, that means stop + reconfigure + start
      if(UserReconfigureRun.load()) {
        // wait for the configuration to be applied and the run restarted, then update EventFactory's cfg
        {
          Lock lk(digitizerReadyMutex);
          digitizerBecameReady.wait(lk, []{return digitizerReady.load();});
        }

        // change the configuration in the output files and start a new file
        { Lock lk(theDAQMutex);
          factory->SetConfiguration(theDAQ->GetConfiguration());
        }

        // runNumber++, subRunNumber = 1, eventNumber = 1, factory->StartFile()
        factory->DefineNewRun();

        continue;
      }

      // otherwise if we weren't told to quit, we were told to start a run
      else if(!UserProgramQuit.load()) {
        // wait for the configuration to be applied, then update EventFactory's cfg
        { Lock lk(digitizerReadyMutex);
          digitizerBecameReady.wait(lk, []{return digitizerReady.load();});
        }

        { Lock lk(theDAQMutex);
          factory->SetConfiguration(theDAQ->GetConfiguration());
        }

        // runNumber++, subRunNumber = 1, eventNumber = 1, factory->StartFile()
        factory->DefineNewRun();

        continue;
      }

    } // if UserStopRun

  } // while !quit

  delete factory;

  return;
}

void GSystemAction() {
  TApplication * theApp = new TApplication("theApp", 0, 0);
  theApp->SetReturnFromRun(true);

  /*
    { Guard lk(controlMutex);
    control = new DAQControl();
    controlExists.store(true);
    }
    controlDeclared.notify_one();
  */

  while(!UserProgramQuit.load()) {
    gSystem->ProcessEvents();
    this_thread::sleep_for(chrono::milliseconds(10));
  }

  delete theApp;
}

int CheckCommand(int fileDescriptor, string& argument) {

  ssize_t nbytes;
  char buffer[256];

  while((nbytes = read(fileDescriptor, buffer, sizeof(buffer) - 1)) > 0) {

    // commands without arguments
    if(strncmp(buffer, "quit", 4) == 0)                 return kProgramQuit;
    if(strncmp(buffer, "pause", 5) == 0)                return kPauseRun;
    if(strncmp(buffer, "unpause", 7) == 0)              return kUnpauseRun;
    if(strncmp(buffer, "start", 5) == 0)                return kStartRun;
    if(strncmp(buffer, "restart", 7) == 0)              return kRestartRun;
    if(strncmp(buffer, "stop", 4) == 0)                 return kStopRun;
    if(strncmp(buffer, "print configuration", 19) == 0) return kPrintConfiguration;
    if(strncmp(buffer, "print board", 11) == 0)         return kPrintBoardInfo;
    if(strncmp(buffer, "print rates", 11) == 0)          return kPrintRates;
    if(strncmp(buffer, "dqm", 3) == 0)                  return kPlotDQM;
    if(strncmp(buffer, "rotate", 6) == 0)               return kRotateFile;
    if(strncmp(buffer, "peek", 4) == 0)                 return kPeekFile;
    if(strncmp(buffer, "interactive_togglewaves", 23) == 0) return kInteractiveToggleSaveWaveforms;

    // "reconfigure newFile.xml" -- return arguments = "newFile.xml"
    if(strncmp(buffer, "reconfigure", 11) == 0) {
      argument = buffer;
      argument = argument.substr(12, nbytes-1 - 12);
      return kReconfigureRun;
    }

    // "interactive_configure newFile.xml"
    if(strncmp(buffer, "interactive_configure", 21) == 0) {
      argument = buffer;
      argument = argument.substr(22, nbytes-1 - 22);
      return kInteractiveReadConfiguration;
    }

    // "interactive_configure 300"
    if(strncmp(buffer, "interactive_timed", 17) == 0) {
      argument = buffer;
      argument = argument.substr(18, nbytes-1 - 18);
      return kInteractiveStartRunSetTime;
    }

    // "interactive_nevents 10000"
    if(strncmp(buffer, "interactive_nevents", 18) == 0) {
      argument = buffer;
      argument = argument.substr(19, nbytes-1 - 19);
      return kInteractiveStartRunSetEvents;
    }

    return kNullCommand;
  }

  return kNullCommand;
}

int main(int argc, char** argv) {

  mdaq::Logger::instance().SetUseLogFile(false); // send messages to stdout instead of file
  mdaq::Logger::instance().bold("MilliDAQ", "Program has started.\n\n");

  if(argc == 2) {
    string userInput = argv[1];

    DemonstratorConfiguration * testCfg = new DemonstratorConfiguration();
    mdaq::ConfigurationReader * reader = new ConfigurationReader();

    if(userInput.size() >= 3) {
      if((std::equal(xmlSuffix.rbegin(), xmlSuffix.rend(), userInput.rbegin()) && reader->ParseXML(*testCfg, userInput)) ||
         (std::equal(pythonSuffix.rbegin(), pythonSuffix.rend(), userInput.rbegin()) && reader->ParsePython(*testCfg, userInput))) {
        currentConfigFile = userInput;
      }
    }

    delete reader;
    delete testCfg;
  }

  // define a timer for how long we've been running
  //chrtime::time_point programStarted = chrtime::now();
  //chrono::duration<float> programDuration(0);

  // define the masterQueues
  for(unsigned int i = 0; i < nDigitizers; i++) masterQueues[i] = new Queue();

  // define the FIFO for user commands
  int fileDescriptor = open(FIFO_FILE, O_NONBLOCK | O_RDWR);
  if(fileDescriptor == -1) {
    mdaq::Logger::instance().error("MilliDAQ", "Failed to open FIFO at " + TString(FIFO_FILE));
    return EXIT_FAILURE;
  }

  fd_set fileDescriptorSet;

  // zero-length timer for a non-blocking FIFO read attempt
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 0;

  int rv;
  string commandArgument;

  //thread theAppSession(GSystemAction);

  /*
    {
    Lock lk(controlMutex);
    controlDeclared.wait(lk, []{return controlExists.load();});
    }
  */

  // without a run started yet, user options are a bit different and the threads haven't started yet
  while(true) {
    this_thread::sleep_for(chrono::milliseconds(50));

    //int command = control->GetCurrentCommand();

    FD_ZERO(&fileDescriptorSet);
    FD_SET(fileDescriptor, &fileDescriptorSet);

    rv = select(fileDescriptor+1, &fileDescriptorSet, NULL, NULL, &tv);

    if(rv == -1) {
      mdaq::Logger::instance().error("MilliDAQ", "Failed to select() from FIFO!");
      return EXIT_FAILURE;
    }

    if(FD_ISSET(fileDescriptor, &fileDescriptorSet)) {

      int command = CheckCommand(fileDescriptor, commandArgument);

      if(command == kNullCommand) continue;

      if(command == kProgramQuit) {
        for(unsigned int i = 0; i < nDigitizers; i++) delete masterQueues[i];
        //delete control;

        mdaq::Logger::instance().command("MilliDAQ", "Recieved quit command. Quitting now.");

        return EXIT_SUCCESS;
      }

      if(command == kInteractiveReadConfiguration) {
        mdaq::Logger::instance().command("MilliDAQ", "Recieved command interactive_configure with file: " + TString(commandArgument) + " - will stop and restart with new configuration.");

        DemonstratorConfiguration * testCfg = new DemonstratorConfiguration();
        mdaq::ConfigurationReader * reader = new ConfigurationReader();

        if(commandArgument.size() >= 3) {
          if((std::equal(xmlSuffix.rbegin(), xmlSuffix.rend(), commandArgument.rbegin()) && reader->ParseXML(*testCfg, commandArgument)) ||
             (std::equal(pythonSuffix.rbegin(), pythonSuffix.rend(), commandArgument.rbegin()) && reader->ParsePython(*testCfg, commandArgument))) {
            currentConfigFile = commandArgument;
          }
        }

        delete reader;
        delete testCfg;

        continue;
      }

      if(command == kInteractiveStartRunSetTime) {
        mdaq::Logger::instance().command("MilliDAQ", "Recieved command to begin timed run, starting now!");

        float commandArgumentFloat;
        try {
          commandArgumentFloat = stof(commandArgument);
        } catch(...) {
          cout << "User input not a valid float..." << endl << endl;
          continue;
        }
        isTimedRun.store(true);
        runTimeRequested.store(commandArgumentFloat);
        break;
      }

      if(command == kInteractiveStartRunSetEvents) {
        mdaq::Logger::instance().command("MilliDAQ", "Recieved command to begin nevents run, starting now!");

        int commandArgumentInt;
        try {
          commandArgumentInt = stoi(commandArgument);
        } catch(...) {
          cout << "User input not a valid integer..." << endl << endl;
          continue;
        }
        isTimedRun.store(false);
        runEventsRequested.store(commandArgumentInt);
        break;
      }

      if(command == kInteractiveToggleSaveWaveforms) {
        mdaq::Logger::instance().warning("MilliDAQ", "Toggling of saving waveforms is not yet implemented...");
        continue;
      }

      if(command >= kPauseRun && command <= kPeekFile) {
        mdaq::Logger::instance().warning("MilliDAQ", "This command is not possible yet in InteractiveDAQ, no run has begun yet!");
        continue;
      }

    } // if FD_ISSET

  }

  // now we've requested some type of run, so we start the threads and begin the first run
  thread writer(WriteAction);
  thread reader(ReadAction);

  // Wait for reader to finish connecting and configuring the digitizer
  // this is to prevent user commands during connection/configuration
  {
    Lock lk(digitizerReadyMutex);
    digitizerBecameReady.wait(lk, []{return digitizerReady.load();});
  }

  // Until we are told to quit the program, the only thing main() is responsible for
  // is to occasionally check stdin for commands and handle any it finds
  while(true) {
    this_thread::sleep_for(chrono::milliseconds(200));

    FD_ZERO(&fileDescriptorSet);
    FD_SET(fileDescriptor, &fileDescriptorSet);

    rv = select(fileDescriptor+1, &fileDescriptorSet, NULL, NULL, &tv);

    if(rv == -1) {
      mdaq::Logger::instance().error("MilliDAQ", "Failed to select() from FIFO!");
      return EXIT_FAILURE;
    }

    //int command = control->GetCurrentCommand();

    if(FD_ISSET(fileDescriptor, &fileDescriptorSet)) {

      int command = CheckCommand(fileDescriptor, commandArgument);

      if(command == kNullCommand) continue;

      if(command == kProgramQuit) {
        UserProgramQuit.store(true);
        mdaq::Logger::instance().command("MilliDAQ", "Recieved quit command. Quitting now.");
        break;
      }

      if(command == kPauseRun) {
        if(UserPauseRun.load() || UserStopRun.load()) {
          mdaq::Logger::instance().warning("MilliDAQ", "Recieved pause command while currently not running -- cannot pause!");
          continue;
        }

        UserPauseRun.store(true);
        mdaq::Logger::instance().command("MilliDAQ", "Recieved pause command. Pausing acquisition now.");
        mdaq::Logger::instance().bold("MilliDAQ", "Currently paused. Issue \"unpause\" to resume.");
        continue;
      }

      if(command == kUnpauseRun) {
        if(!UserPauseRun.load()) {
          mdaq::Logger::instance().warning("MilliDAQ", "Recieved unpause command while not paused -- nothing to do!");
          continue;
        }

        UserPauseRun.store(false);
        mdaq::Logger::instance().command("MilliDAQ", "Recieved unpause. Resuming acquisition now.");
        continue;
      }

      if(command == kStartRun) {
        mdaq::Logger::instance().warning("MilliDAQ", "Recieved start command in InteractiveDAQ -- choose instead interactive_timed or interactive_nevents!");
        continue;
      }

      /* commands not yet handled
	 kReconfigureRun,
	 kPrintConfiguration,
	 kPrintBoardInfo,
	 kPrintRates,
	 kPlotDQM,
      */
      
      if(command == kInteractiveStartRunSetTime) {
        if(!UserStopRun.load()) {
          mdaq::Logger::instance().warning("MilliDAQ", "New run requested while not stopped -- nothing to do!");
          continue;
        }

        float commandArgumentFloat;
        try {
          commandArgumentFloat = stof(commandArgument);
        } catch(...) {
          cout << "User input not a valid float..." << endl << endl;
          continue;
        }
        isTimedRun.store(true);
        runTimeRequested.store(commandArgumentFloat);

        digitizerReady.store(false);
        UserStopRun.store(false);

        // Wait for reader to finish connecting and configuring the digitizer
        // this is to prevent user commands during connection/configuration
        {
          Lock lk(digitizerReadyMutex);
          digitizerBecameReady.wait(lk, []{return digitizerReady.load();});
        }

        continue;
      }

      if(command == kInteractiveStartRunSetEvents) {
        if(!UserStopRun.load()) {
          mdaq::Logger::instance().warning("MilliDAQ", "New run requested while not stopped -- nothing to do!");
          continue;
        }

        int commandArgumentInt;
        try {
          commandArgumentInt = stoi(commandArgument);
        } catch(...) {
          cout << "User input not a valid integer..." << endl << endl;
          continue;
        }
        isTimedRun.store(false);
        runEventsRequested.store(commandArgumentInt);

        digitizerReady.store(false);
        UserStopRun.store(false);

        // Wait for reader to finish connecting and configuring the digitizer
        // this is to prevent user commands during connection/configuration
        {
          Lock lk(digitizerReadyMutex);
          digitizerBecameReady.wait(lk, []{return digitizerReady.load();});
        }

        continue;
      }

      if(command == kInteractiveReadConfiguration) {
        if(!UserStopRun.load()) {
          mdaq::Logger::instance().warning("MilliDAQ", "New run requested while not stopped -- nothing to do!");
          continue;
        }

        // change the config file; if it's not good, keep using the latest good one in theDAQ
        { Lock lk2(currentConfigFileMutex);
          if(!theDAQ->ReadConfiguration(commandArgument)) currentConfigFile = theDAQ->GetConfigurationFile();
          else currentConfigFile = commandArgument;
        }

        continue;
      }

      if(command == kRestartRun) {
        if(UserStopRun.load()) {
          mdaq::Logger::instance().warning("MilliDAQ", "Recieved restart command while stopped -- use start instead!");
          continue;
        }
        if(UserPauseRun.load()) {
          mdaq::Logger::instance().warning("MilliDAQ", "Recieved restart command while paused -- use unpause or stop instead!");
          continue;
        }

        mdaq::Logger::instance().command("MilliDAQ", "Recieved restart command. First stopping run...");
        UserRestartRun.store(true);
        UserStopRun.store(true);

        // wait for the digitizer to actually stop
        { Lock lk(digitizerStoppedMutex);
          digitizerHasStopped.wait(lk);
        }

        mdaq::Logger::instance().info("MilliDAQ", "Restarting run from restart command.");
        UserStopRun.store(false);

        // Wait for reader to finish connecting and configuring the digitizer
        // this is to prevent user commands during connection/configuration
        {
          Lock lk(digitizerReadyMutex);
          digitizerBecameReady.wait(lk, []{return digitizerReady.load();});
        }

        continue;
      }

      if(command == kStopRun) {
        if(UserStopRun.load()) {
          mdaq::Logger::instance().warning("MilliDAQ", "Recieved stop command while already stopped -- nothing to do!");
          continue;
        }

        mdaq::Logger::instance().command("MilliDAQ", "Recieved stop command. Stopping acquisition now.");
        UserStopRun.store(true);
        UserPauseRun.store(false); // if you stop while paused, turn this false too for when we restart later

        continue;
      }

      if(command == kRotateFile) {
        mdaq::Logger::instance().command("MilliDAQ", "Recieved rotate file command. Will immediately rotate file.");
        UserRotateFile.store(true);
      }

      if(command == kPeekFile) {
        mdaq::Logger::instance().command("MilliDAQ", "Recieved peek file command. Will store next 10 events in peekFile.root.");
        UserPeekFile.store(true);
      }

      if(command == kInteractiveToggleSaveWaveforms) {
        mdaq::Logger::instance().warning("MilliDAQ", "Toggling of saving waveforms is not yet implemented...");
        continue;
      }

    } // if FD_ISSET

  }

  // wait for the threads to finish
  //theAppSession.join();
  writer.join();
  reader.join();

  for(unsigned int i = 0; i < nDigitizers; i++) delete masterQueues[i];
  //delete control;

  mdaq::Logger::instance().bold("MilliDAQ", "Program has exited.");

  return EXIT_SUCCESS;
}
