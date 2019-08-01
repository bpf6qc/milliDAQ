#include "MilliDAQ.h"

using namespace mdaq;
using namespace std;

void emergencyClose() {
  if(!needEmergencyClose) return;
  { Lock lk(theDAQMutex);
    theDAQ->CloseDevices();
  }
  mdaq::Logger::instance().debug("MilliDAQ", "EMERGENCY CLOSE");
}

void PrintStatus(EventFactory * factory) {
  std::ostringstream oss;

  oss << "MilliDAQ program status:" << std::endl << std::endl;
  oss << "Currently: " << A_BRIGHT_BLACK;

  if(UserProgramQuit.load())                 oss << A_BRIGHT_RED << "QUITTING"; // this shouldn't be possible
  else if(UserPauseRun.load())               oss << A_YELLOW     << "PAUSED";
  else if(UserStopRun.load())                oss << A_BRIGHT_RED << "STOPPED";
  else if(!digitizerReady.load())            oss << A_YELLOW     << "STARTING, NOT READY";
  else if(UserTimedRunDuration.load() > 0.0) oss << A_GREEN      << "RUNNING TIMED";
  else if(UserNeventsRunDuration.load() > 0) oss << A_GREEN      << "RUNNING NEVENTS";
  else                                       oss << A_GREEN      << "RUNNING";

  oss << A_RESET << std::endl << std::endl;

  // if we're currently running, print out more information
  if(!UserProgramQuit.load() && !UserStopRun.load() && digitizerReady.load()) {

    { Lock lk(currentConfigFileMutex);
      oss << "Configuration          : " << currentConfigFile << std::endl;
    }

    int totalSecondsLive = factory->GetRunDuration().count();
    int nHoursLive = totalSecondsLive / 60 / 60;
    int nMinutesLive = (totalSecondsLive / 60) % 60;
    int nSecondsLive = totalSecondsLive % 60;

    oss << "Current run number     : " << factory->GetRunNumber() << "." << factory->GetSubRunNumber() << std::endl;
    oss << "Run alive for          : " << nHoursLive << "h " << nMinutesLive << "m " << nSecondsLive << "s" << std::endl;
    oss << "Events in current run  : " << factory->EventsRecordedInRun() << std::endl;
    if(nSecondsLive > 0) 
      oss << "Average recording rate : " << (float)factory->EventsRecordedInRun() / (float)totalSecondsLive << " Hz" << std::endl;

    oss << std::endl;
  }
  else {
    { Lock lk(currentConfigFileMutex);
      oss << "Configuration          : " << currentConfigFile << std::endl;
    }
  }

  mdaq::Logger::instance().info("MilliDAQ", TString(oss.str()));
}

void ReadAction() {

#ifdef READ_TIMING_PERFORMANCE
  TFile * readTimingOutput = new TFile("/home/milliqan/readTimingPerformance.root", "RECREATE");
  
  TTree * readTiming = new TTree("reads", "Timing perforamnce for each read attempt");
  
  uint32_t nEventsRead[nDigitizers];
  TTimeStamp readTimeStamp;
  int queueDepths[nDigitizers];

  readTiming->Branch("nEventsRead", &nEventsRead);
  readTiming->Branch("timestamp", &readTimeStamp);
  readTiming->Branch("queueDepths", &queueDepths);
#endif

  // local timing information for timed runs (DAQCommand timedStart)
  // these are in the read thread rather than write thread so that we 
  // time the digitizer reading instead of how long we're writing to 
  // disk, which could be different
  chrono::duration<float> runDuration;
  chrtime::time_point runStartTime;

  // create and connect to the digitizer
  { Lock lk(theDAQMutex);
    theDAQ = new DAQ();
    
    // theDAQ, in its constructor, will have tried to read the previous config from the runlist
    // so here we see what it found for its initial config file
    // we also set the time units in the queues to be applied to events as they are created
    { Lock lk2(currentConfigFileMutex);
      currentConfigFile = theDAQ->GetConfigurationFile();
    }
    
    theDAQ->ConnectToBoards();
    needEmergencyClose = true; // bfrancis -- consider if anything more needs to happen in case of a crash
    atexit(emergencyClose);

    { Lock lk2(masterQueueMutex);
      for(unsigned int i = 0; i < nDigitizers; i++) masterQueues[i]->SetTimeUnits(theDAQ->GetConfiguration()->digitizers[i].SAMFrequency);
    }

    // Initialize the boards from the current DemonstratorConfiguration
    theDAQ->InitializeBoards();

    // Arming the LVDS start on slave boards seems to need a few seconds, so we wait
    this_thread::sleep_for(chrono::seconds(3));

    // Start the acquisition
    theDAQ->StartRun();
  }

  // now that data is coming, wake up other threads
  { Guard lk(digitizerReadyMutex);
    digitizerReady.store(true);
  }
  digitizerBecameReady.notify_all();

  mdaq::Logger::instance().bold("MilliDAQ", "Acquisition has begun!");

  // until the run is paused or stopped, read data from the digitizer
  while(!UserProgramQuit.load()) {

    // process one read cycle of the digitizer
    { Lock lk(theDAQMutex);
      theDAQ->SendSWTriggers(); // if software trigger is being used, send one here

      // attempt to read events from the board, appending any to the local queue
#ifdef READ_TIMING_PERFORMANCE
      theDAQ->Read(masterQueues, nEventsRead);

      auto nsecs = std::chrono::duration_cast<std::chrono::nanoseconds>(chrtime::now().time_since_epoch());
      readTimeStamp = std::chrono::system_clock::to_time_t(chrtime::now());
      readTimeStamp.SetNanoSec(nsecs.count() % (int)1e9);

      for(int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) queueDepths[iDigitizer] = masterQueues[iDigitizer]->GuessSize();

      readTiming->Fill();
#else
      theDAQ->Read(masterQueues);
#endif
    }

    // with 12 dedicated cores, there actually can be no break between the release of theDAQMutex and the next iteration of this loop
    this_thread::sleep_for(chrono::milliseconds(5));

    // if the run is a timed run (DAQCommand timedStart), check the timer
    if(UserTimedRunDuration.load() > 0.0) {
      if(chrtime::now() - runStartTime > runDuration) {
        mdaq::Logger::instance().bold("MilliDAQ", "Timed run complete! Stopping acquisition now.");

        // reset UserTimedRunDuration to a negative value, so that the next run isn't automatically a timed run as well
        UserTimedRunDuration.store(-1.0);

        // signal acquisition to stop
        UserStopRun.store(true);
        UserPauseRun.store(false);
      }
    }

    // DAQCommand print configuration
    if(UserPrintConfiguration.load()) {
      Lock lk(theDAQMutex);
      theDAQ->GetConfiguration()->LogConfiguration();
      UserPrintConfiguration.store(false);
    }

    // DAQCommand print board
    if(UserPrintBoardInfo.load()) {
      Lock lk(theDAQMutex);
      theDAQ->PrintBoardInfos();
      UserPrintBoardInfo.store(false);
    }

    // if this thread is told the run is paused (DAQCommand pause)
    while(UserPauseRun.load()) {
      // If while paused, this thread is told to stop the run
      // or the user wants to quit the program, leave
      if(UserProgramQuit.load() || UserStopRun.load()) break;

      // otherwise just wait
      this_thread::sleep_for(chrono::seconds(1));
    }

    // if this thread is told the run is stopped for any reason
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

#ifdef READ_TIMING_PERFORMANCE
      // the run is ending, so we write the timing performance tree
      readTimingOutput->cd();
      readTiming->Write();
      readTimingOutput->Close();
#endif

      // Now wait until we're not stopped any more
      while(UserStopRun.load()) {
        // If the user wants to quit the program, leave
        if(UserProgramQuit.load()) break;

        // otherwise just wait
        this_thread::sleep_for(chrono::milliseconds(50));
      }

      // if we're told to reconfigure, we do that now
      if(UserReconfigureRun.load()) {
        { Lock lk(theDAQMutex);
          Lock lk2(currentConfigFileMutex);

          // change the configuration
          if(!theDAQ->ReadConfiguration(currentConfigFile)) {
            // if we failed to read a proper file, get the name of the old one
            currentConfigFile = theDAQ->GetConfigurationFile();
          }
          else {
            // if we successfully changed the configuration, we set the time units in the queues
            // to be applied to events as they are created
            Lock lk3(masterQueueMutex);
            for(unsigned int i = 0; i < nDigitizers; i++) masterQueues[i]->SetTimeUnits(theDAQ->GetConfiguration()->digitizers[i].SAMFrequency);
          }          

          // Initialize the board from its current DemonstratorConfiguration
          theDAQ->InitializeBoards();

          // Arming the LVDS start on slave boards seems to need a few seconds, so we wait
          this_thread::sleep_for(chrono::seconds(3));

          // Start the acquisition
          theDAQ->StartRun();
        }

        // currently "DAQCommand reconfigure" only executes a reconfigure-then-start,
        // so we don't handle timed runs -- there is no command for 'reconfigure + timed'

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
      // otherwise if we weren't told to quit, we were told to start a run
      else if(!UserProgramQuit.load()) {

        { Lock lk(theDAQMutex);
          // Initialize the board from its current V1743Configuration
          theDAQ->InitializeBoards();

	  // Arming the LVDS start on slave boards seems to need a few seconds, so we wait
	  this_thread::sleep_for(chrono::seconds(3));

          // start the run (ie allow the digitizer to accept triggers)
          theDAQ->StartRun();
        }

        // if the new run is a timed run (DAQCommand timedStart), define timers
        // this happens just after theDAQ->StartRun() when the digitizer is
        // first allowed to accept triggers
        if(UserTimedRunDuration.load() > 0.0) {
          runStartTime = chrtime::now();
          runDuration = chrono::duration<float>(UserTimedRunDuration.load());
        }

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

#ifdef READ_TIMING_PERFORMANCE
      // bfrancis -- this is in the wrong place

      // a new run is starting, so we open a new timing performance output file
      readTimingOutput = new TFile("/home/milliqan/readTimingPerformance.root", "RECREATE");

      readTiming = new TTree("reads", "Timing perforamnce for each read attempt");
  
      readTiming->Branch("nEventsRead", &nEventsRead);
      readTiming->Branch("timestamp", &readTimeStamp);
      readTiming->Branch("queueDepths", &queueDepths);
#endif

    } // if UserStopRun

  }

  // if theDAQ is deleted before factory is deleted in WriteAction(),
  // then the configuration file is deleted before EventFactory::FinishFile() has
  // written it to the Metadata tree. so we wait for factory to be deleted
  { Lock lk(safeToQuitMutex);
    becameSafeToQuit.wait(lk, []{return safeToQuit.load();});
  }

  // we're now quitting the program
  { Lock lk(theDAQMutex);
    theDAQ->StopRun();

    // close the digitizers
    theDAQ->CloseDevices();
    needEmergencyClose = false; // bfrancis -- consider if anything more needs to happen in case of crash
    if(theDAQ) delete theDAQ;
  }

#ifdef READ_TIMING_PERFORMANCE
  if(readTimingOutput) {
    // the run is ending, so we write the timing performance tree
    readTimingOutput->cd();
    readTiming->Write();
    readTimingOutput->Close();
  }
#endif

  return;
}

void WriteAction() {

  // Wait for reader to finish connecting and configuring the digitizer
  { Lock lk(digitizerReadyMutex);
    digitizerBecameReady.wait(lk, []{return digitizerReady.load();});
  }

  // define the event factory
  EventFactory * factory;
  { Lock lk(theDAQMutex);
    factory = new EventFactory("/home/milliqan/tmp/MilliQanEvents.root", "/home/milliqan/data/", theDAQ->GetConfiguration());
  }

  // now run until told to quit the program
  while(!UserProgramQuit.load()) {

    // flush the entire current event queue to disk
    factory->FlushQueuesToDisk(masterQueues);

    this_thread::sleep_for(chrono::milliseconds(50));

    // if the run targets Nevents (DAQCommand nEventsStart), check the Nevents
    if(UserNeventsRunDuration.load() > 0) {
      if(factory->EventsRecordedInRun() > (unsigned int)UserNeventsRunDuration.load()) {
        mdaq::Logger::instance().bold("MilliDAQ", "Nevents run complete! Stopping acquisition now.");

        // reset UserNeventsRunDuration to a negative value, so that the next run isn't automatically an Nevents run as well
        UserNeventsRunDuration.store(-1);

        // signal acquisition to stop
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

    if(UserPrintStatus.load()) {
      PrintStatus(factory);
      UserPrintStatus.store(false);
    }

    if(UserPlotDQM.load()) {
      factory->GetDQM()->SavePDFs();
      UserPlotDQM.store(false);
    }

    // if this thread is told the run is paused
    while(UserPauseRun.load()) {
      // If while paused, this thread is told to stop the run
      // or the user wants to quit the program, leave
      if(UserProgramQuit.load() || UserStopRun.load()) break;

      if(UserPrintStatus.load()) {
        PrintStatus(factory);
        UserPrintStatus.store(false);
      }

      // otherwise just wait
      this_thread::sleep_for(chrono::seconds(1));
    }

    // if this thread is told the run is stopped
    if(UserStopRun.load()) {

      // finish off the current output file
      factory->FlushQueuesToDisk(masterQueues);
      factory->FinishFile();

#ifdef MATCH_TIME_PERFORMANCE
      factory->ClosePerformanceFile();
#endif

      // wait until we've really stopped
      { Lock lk(digitizerStoppedMutex);
        digitizerHasStopped.wait(lk, []{return digitizerIsStopped.load();});
      }

      // destroy any leftover events so that they won't remain in the next run
      // bfrancis -- this was disabled March 21st 2018; seems to break the queue
      //             so that it appears full
      //for(unsigned int i = 0; i < nDigitizers; i++) masterQueues[i]->Destroy();

      // Now wait until we're not stopped any more
      while(UserStopRun.load()) {
        // If the user wants to quit the program, leave
        if(UserProgramQuit.load()) break;

        if(UserPrintStatus.load()) {
          PrintStatus(factory);
          UserPrintStatus.store(false);
        }

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

  // alert ReadAction() that it's now safe to delete theDAQ
  { Guard lk(safeToQuitMutex);
    safeToQuit.store(true);
  }
  becameSafeToQuit.notify_all();

  return;
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
    if(strncmp(buffer, "print rates", 11) == 0)         return kPrintRates;
    if(strncmp(buffer, "print status", 12) == 0)        return kPrintStatus;
    if(strncmp(buffer, "dqm", 3) == 0)                  return kPlotDQM;
    if(strncmp(buffer, "rotate", 6) == 0)               return kRotateFile;
    if(strncmp(buffer, "peek", 4) == 0)                 return kPeekFile;

    // "reconfigure newFile.xml" -- return arguments = "newFile.xml"
    if(strncmp(buffer, "reconfigure", 11) == 0) {
      argument = buffer;
      argument = argument.substr(12, nbytes-1 - 12);
      return kReconfigureRun;
    }

    // "timedStart 300"
    if(strncmp(buffer, "timedStart", 10) == 0) {
      argument = buffer;
      argument = argument.substr(11, nbytes-1 - 11);
      return kStartRunSetTime;
    }

    // "nEventsStart 10000"
    if(strncmp(buffer, "nEventsStart", 12) == 0) {
      argument = buffer;
      argument = argument.substr(13, nbytes-1 - 13);
      return kStartRunSetEvents;
    }

    return kNullCommand;
  }

  return kNullCommand;
}

int main(int argc, char** argv) {

  mdaq::Logger::instance().bold("MilliDAQ", "Program has started.\n\n");

  // define a timer for how long we've been running
  //chrtime::time_point programStarted = chrtime::now();
  //chrono::duration<float> programDuration(0);

  // define the masterQueues
  { Lock lk(masterQueueMutex);
    for(unsigned int i = 0; i < nDigitizers; i++) masterQueues[i] = new Queue();
  }

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

  // upon first starting, we want to apply a default configuration and start a run immediately
  // this is in case of a program crash, we can automatically restart it without needing a command

  // start threads
  thread writer(WriteAction);
  thread reader(ReadAction);

  // Wait for reader to finish connecting and configuring the digitizer
  // this is to prevent user commands during connection/configuration
  {
    Lock lk(digitizerReadyMutex);
    digitizerBecameReady.wait(lk, []{return digitizerReady.load();});
  }

  // Until we are told to quit the program, the only thing main() is responsible for
  // is to occasionally check the FIFO for commands and handle any it finds
  while(true) {

    this_thread::sleep_for(chrono::milliseconds(200));

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
        if(!UserStopRun.load()) {
          mdaq::Logger::instance().warning("MilliDAQ", "Recieved start command while not stopped -- nothing to do!");
          continue;
        }

        mdaq::Logger::instance().command("MilliDAQ", "Recieved start command. Starting acquisition now.");

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

      if(command == kStartRunSetTime) {
        if(!UserStopRun.load()) {
          mdaq::Logger::instance().warning("MilliDAQ", "Recieved timedStart command while not stopped -- nothing to do!");
          continue;
        }

        float timedRunDuration;

        try {
          timedRunDuration = stof(commandArgument);
        } catch(...) {
          mdaq::Logger::instance().warning("MilliDAQ", "Recieved timedStart command with non-numeric argument. Ignoring...");
          continue;
        }

        if(timedRunDuration <= 0.0) {
          mdaq::Logger::instance().warning("MilliDAQ", "Recieved timedStart command with non-positive duration. Ignoring...");
          continue;
        }

        mdaq::Logger::instance().command("MilliDAQ", "Recieved timedStart command with duration: " + TString(commandArgument) + " seconds. Starting acquisition now.");

        UserTimedRunDuration.store(timedRunDuration); // this being now non-negative, the read thread will check a timer
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

      if(command == kStartRunSetEvents) {
        if(!UserStopRun.load()) {
          mdaq::Logger::instance().warning("MilliDAQ", "Recieved nEventsStart command while not stopped -- nothing to do!");
          continue;
        }

        int runTargetNevents;

        try {
          runTargetNevents = stoi(commandArgument);
        } catch(...) {
          mdaq::Logger::instance().warning("MilliDAQ", "Recieved nEventsStart command with non-integer argument. Ignoring...");
          continue;
        }

        if(runTargetNevents <= 0) {
          mdaq::Logger::instance().warning("MilliDAQ", "Recieved nEventsStart command with non-positive Nevents. Ignoring...");
          continue;
        }

        mdaq::Logger::instance().command("MilliDAQ", "Recieved nEventsStart command with target: " + TString(commandArgument) + " events. Starting acquisition now.");

        UserNeventsRunDuration.store(runTargetNevents); // this being now non-negative, the write thread will check the Nevents
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

      if(command == kReconfigureRun) {
        if(UserPauseRun.load()) {
          mdaq::Logger::instance().warning("MilliDAQ", "Recieved reconfigure command while paused -- please stop or unpause first!");
          continue;
        }

        mdaq::Logger::instance().command("MilliDAQ", "Recieved reconfigure command with file: " + TString(commandArgument) + " - will stop and apply new configuration.");

        { Lock lk(currentConfigFileMutex);
          currentConfigFile = commandArgument;
        }

        UserReconfigureRun.store(true);
        UserStopRun.store(true);

        // wait for the digitizer to actually stop
        { Lock lk(digitizerStoppedMutex);
          digitizerHasStopped.wait(lk, []{return digitizerIsStopped.load();});
        }

        // don't restart immediately, wait a short period
        this_thread::sleep_for(chrono::seconds(3));

	// tell threads to start up again
        UserStopRun.store(false);

        // wait for the configuration to be applied and the run restarted, then update EventFactory's cfg
        { Lock lk(digitizerReadyMutex);
          digitizerBecameReady.wait(lk, []{return digitizerReady.load();});
        }

        UserReconfigureRun.store(false);

        continue;
      }

      if(command == kPrintConfiguration) {
        if(UserPauseRun.load()) {
          mdaq::Logger::instance().warning("MilliDAQ", "Recieved print configuration command while paused -- please unpause first!");
          continue;
        }
        if(UserStopRun.load()) {
          mdaq::Logger::instance().warning("MilliDAQ", "Recieved print configuration command while stopped -- please start a run first!");
          continue;
        }

        mdaq::Logger::instance().command("MilliDAQ", "Recieved print configuration command.");
        UserPrintConfiguration.store(true);
      }
      if(command == kPrintBoardInfo) {
        if(UserPauseRun.load()) {
          mdaq::Logger::instance().warning("MilliDAQ", "Recieved print board info command while paused -- please unpause first!");
          continue;
        }
        if(UserStopRun.load()) {
          mdaq::Logger::instance().warning("MilliDAQ", "Recieved print board info command while stopped -- please start a run first!");
          continue;
        }

        mdaq::Logger::instance().command("MilliDAQ", "Recieved print board info command.");
        UserPrintBoardInfo.store(true);
      }
      if(command == kPrintRates) {
        mdaq::Logger::instance().command("MilliDAQ", "Recieved print rates command.");
        UserPrintRates.store(true);
      }
      if(command == kPrintStatus) {
        mdaq::Logger::instance().command("MilliDAQ", "Recieved print status command.");
        UserPrintStatus.store(true);
      }
      if(command == kPlotDQM) {
        mdaq::Logger::instance().command("MilliDAQ", "Recieved plot dqm command. Writing PDFs to /home/milliqan/MilliDAQ/plots/ ...");
        UserPlotDQM.store(true);
      }
      if(command == kRotateFile) {
        mdaq::Logger::instance().command("MilliDAQ", "Recieved rotate file command. Will immediately rotate file.");
        UserRotateFile.store(true);
      }
      if(command == kPeekFile) {
        mdaq::Logger::instance().command("MilliDAQ", "Recieved peek file command. Will store next 10 events in peekFile.root.");
        UserPeekFile.store(true);
      }
    }

  }

  close(fileDescriptor);

  // wait for the threads to finish
  writer.join();
  reader.join();

  for(unsigned int i = 0; i < nDigitizers; i++) delete masterQueues[i];

  mdaq::Logger::instance().bold("MilliDAQ", "Program has exited.");

  return EXIT_SUCCESS;
}

