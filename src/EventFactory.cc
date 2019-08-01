#include "EventFactory.h"

mdaq::EventFactory::EventFactory(const std::string tmpFile, const std::string destDir, mdaq::DemonstratorConfiguration * config, const bool isInteractiveDQM) :
  tempFileName(tmpFile),
  destinationDirectory(destDir),
  currentConfigFileName(config->GetCurrentConfigFile()),
  cfg(config),
  interactiveMode(isInteractiveDQM)
{
  dqm = new DQM("/home/milliqan/MilliDAQ/plots/", cfg, interactiveMode);
  rng = new TRandom3(0);

  ResetEventsRecordedInRun(); // nEventsThisRun = 0
  GetPreviousRunNumber(); // example if the last saved file was run 7.3, find Run 7.3
  IterateRunNumber(); // after 7.3 the next would be 8.1
  ResetSubRunNumber(); // set the .1 in 8.1

  meta_fileOpenTime = new TString();
  meta_fileCloseTime = new TString();
  meta_configFile = new TString();
  meta_gitVersion = new TString(GIT_HASH);

  currentRunStartTime = std::chrono::system_clock::now();

#ifdef DEBUG_MATCH_PERFORMANCE
  matchDebugOutput = new TFile("/home/milliqan/matchDebug.root", "RECREATE");
  matchDebugTree = new TTree("debug", "debug");

  matchDebugTree->Branch("nPulledFromQueues", &nPulledFromQueues);
  matchDebugTree->Branch("TTT", &matching_ttts);
  matchDebugTree->Branch("boardBeingMatched", &boardBeingMatched);
#endif

#ifdef DEBUG_WRITE_PERFORMANCE
  writeDebugOutput = new TFile("/home/milliqan/writeDebug.root", "RECREATE");
  writeDebugTree = new TTree("debug", "debug");

  writeDebugTree->Branch("closestTTTDifference", &closestTTTDifference);
  writeDebugTree->Branch("nMatchesAttempted", &nMatchesAttempted);
  writeDebugTree->Branch("matchSuccessful", &matchSuccessful);
  writeDebugTree->Branch("queueAge", &timeOnQueue);
#endif

  // start the first output file
  StartFile();
}

mdaq::EventFactory::~EventFactory() {
  if(output && output->IsOpen()) FinishFile();
  if(output) delete output;
  delete dqm;
  delete rng;

  delete meta_fileOpenTime;
  delete meta_fileCloseTime;
  delete meta_configFile;
  delete meta_gitVersion;

#ifdef DEBUG_MATCH_PERFORMANCE
  matchDebugTree->Write();
  if(matchDebugOutput && matchDebugOutput->Is)
#endif
    
    }

void mdaq::EventFactory::StartFile() {
  currentFileOpenTime = std::chrono::system_clock::now();

  // open with option "update" in case we failed to move the previous file out of the temp dir
  output = new TFile(tempFileName.c_str(), "UPDATE");

  // IMPORTANT -- allowing TFile compression seems to just take too much time.
  // With 16 channels writing out and compression turned on, we only seem to be
  // able to write about 230 events/second before we max out the Queue and
  // start dropping events due to backpressure.
  // With this disabled we can write to disk faster than the digitizer can trigger.
  //
  // Need to investigate other compression algorithms (ROOT::kZLIB), split levels, bufsizes for Branch()...
  if(!cfg->TFileCompressionEnabled) output->SetCompressionLevel(0);

  CurrentEvent = new GlobalEvent();
  CurrentConfig = cfg;

  for(int i = 0; i < nDigitizers; i++) {
    BoardEvents[i].clear();
    tempBoardEvents[i] = new V1743Event();
  }

  // in case we're updating an existing file
  if(output->GetListOfKeys()->Contains("Events")) {
    data = (TTree*)output->Get("Events");
    data->SetBranchAddress("event", &CurrentEvent);
  }
  else {
    data = new TTree("Events", "Events");
    data->SetAutoSave(autoSaveEvery);
    data->Branch("event", &CurrentEvent);
  }

}

void mdaq::EventFactory::FinishFile() {

  if(!interactiveMode) {  
    dqm->SavePDFs();
    dqm->ResetPlots();
  }

  output->cd();
  data->Write();

  // Handle metadata tree

  // in case we're updating an existing file
  if(output->GetListOfKeys()->Contains("Metadata")) {
    metadata = (TTree*)output->Get("Metadata");
    metadata->SetBranchAddress("configuration", &CurrentConfig);
    metadata->SetBranchAddress("fileOpenTime", &meta_fileOpenTime);
    metadata->SetBranchAddress("fileCloseTime", &meta_fileCloseTime);
    metadata->SetBranchAddress("runNumber", &meta_runNumber);
    metadata->SetBranchAddress("subrunNumber", &meta_subrunNumber);
    metadata->SetBranchAddress("configFileName", &meta_configFile);
    metadata->SetBranchAddress("gitVersion", &meta_gitVersion);
  }
  else {
    metadata = new TTree("Metadata", "Metadata");
    metadata->Branch("configuration", &cfg);
    metadata->Branch("fileOpenTime", &meta_fileOpenTime);
    metadata->Branch("fileCloseTime", &meta_fileCloseTime);
    metadata->Branch("runNumber", &meta_runNumber);
    metadata->Branch("subrunNumber", &meta_subrunNumber);
    metadata->Branch("configFileName", &meta_configFile);
    metadata->Branch("gitVersion", &meta_gitVersion);
  }
  
  // whatever else we might want to save in metadata...
  // voltages, temperatures, rates?

  // figure out the open/close times

  time_t fileOpenTime = std::chrono::system_clock::to_time_t(currentFileOpenTime);
  char openTime[40];
  strftime(openTime, sizeof(openTime), "%F_%Hh%Mm%Ss", gmtime(&fileOpenTime));
  *meta_fileOpenTime = openTime;

  time_t fileCloseTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  char closeTime[40];
  strftime(closeTime, sizeof(closeTime), "%F_%Hh%Mm%Ss", gmtime(&fileCloseTime));
  *meta_fileCloseTime = closeTime;

  // meta_runNumber and meta_subrunNumber are already set

  *meta_configFile = currentConfigFileName;

  metadata->Fill();
  output->cd();
  metadata->Write();
  output->Close();

  // get base file name
  std::string croppedConfigName = currentConfigFileName;
  auto pos = croppedConfigName.rfind("/");
  if(pos != std::string::npos) croppedConfigName = croppedConfigName.substr(pos+1);

  // remove file extension
  pos = croppedConfigName.rfind(".");
  if(pos != std::string::npos) {
    croppedConfigName = croppedConfigName.substr(0, pos);
  }

  char newFileName[256];
  sprintf(newFileName, "%sMilliQan_Run%d.%d_%s.root", destinationDirectory.c_str(), meta_runNumber, meta_subrunNumber, croppedConfigName.c_str());

  int status = rename(tempFileName.c_str(), newFileName);

  RegisterFile();

  if(status != 0) {
    mdaq::Logger::instance().warning("EventFactory", "Unable to move temporary output to destination " + TString(newFileName) + ", continuing with temporary file.");
  }
  else {
    mdaq::Logger::instance().info("EventFactory", "Finished output file " + TString(newFileName));
  }

  dqm->PrintRates();
}

void mdaq::EventFactory::GetPreviousRunNumber() {

  // read the most recent run from /var/log/MilliDAQ_RunList.log
  std::fstream fin;
  fin.open("/var/log/MilliDAQ_RunList.log");

  std::string cfgPath;

  meta_runNumber = 0;
  meta_subrunNumber = 0;

  // find the most recent entry
  if(fin.is_open()) {
    while(true) {
      fin >> meta_runNumber >> meta_subrunNumber >> cfgPath;
      if(!fin.good()) break;
    }
  }

  return;
}

void mdaq::EventFactory::FlushQueuesToDisk(mdaq::Queue * queues[nDigitizers]) {

  // bfrancis -- extremely important, must consider this and improve matching if possible

  // first pull all events from all board queues to our local copies
  for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
    BoardEvents[iDigitizer].clear();
    while(queues[iDigitizer]->pull(tempBoardEvents[iDigitizer])) {
      BoardEvents[iDigitizer].push_back(tempBoardEvents[iDigitizer]);
    }
#ifdef DEBUG_MATCH_PERFORMANCE
    nPulledFromQueues[iDigitizer] = BoardEvents[iDigitizer].size();
#endif
  }

  // now we match amongst these queues in the TriggerTimeTag (TTT)

  for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {

    // work at this board's events until they're all gone
    while(BoardEvents[iDigitizer].size() > 0) {

      // reset all the V1743Event objects in case some don't exist
      // this zeroes out all data and sets the flag DataPresent to false
      CurrentEvent->Reset();

      bool commitToWrite = false;

#ifdef DEBUG_MATCH_PERFORMANCE
      boardBeingMatched = iDigitizer;
      matching_ttts[iDigitizer] = matchTTT;
#endif

#ifdef DEBUG_WRITE_PERFORMANCE
      closestTTTDifference = -1.0;
      nMatchesAttempted = 0;
#endif


      double matchTTT = BoardEvents[iDigitizer].front()->GetTTTNanoseconds();

      // now search through all other boards to find a matching TTT
      for(unsigned int j = 0; j < nDigitizers; j++) {
        if(j == iDigitizer) continue; // don't match an event to itself

        // look through all events in this other board, and try to find a matching TTT
        for(unsigned int jEvent = 0; jEvent < BoardEvents[j].size(); jEvent++) {
          double otherTTT = BoardEvents[j][jEvent]->GetTTTNanoseconds();

#ifdef DEBUG_MATCH_PERFORMANCE
          matching_ttts[j] = otherTTT;
          matchTiming->Fill();
#endif

#ifdef DEBUG_WRITE_PERFORMANCE
          if(closestTTTDifference < 0 || fabs(otherTTT - matchTTT) < closestTTTDifference) closestTTTDifference = fabs(otherTTT - matchTTT);
          nMatchesAttempted++;
#endif

          if(fabs(otherTTT - matchTTT) < matchingTTTRequirement) {
            // this is now a matching event, so we set CurrentEvent to use each V1743Event and remove them from future matching
            CurrentEvent->digitizers[iDigitizer] = BoardEvents[iDigitizer].front();
            CurrentEvent->digitizers[j] = BoardEvents[j][jEvent];

            BoardEvents[iDigitizer].erase(BoardEvents[iDigitizer].begin(), BoardEvents[iDigitizer].begin() + 1); 
            BoardEvents[j].erase(BoardEvents[j].begin() + jEvent, BoardEvents[j].begin() + jEvent + 1);

            commitToWrite = true;

            dqm->UpdateMatchingEfficiency(true);

            break;
          }

        } // loop other events in other digitizer

      } // loop over other digitizers

      // if no match was found for this event, then we see how long it's been queued for
      if(!commitToWrite) {

	std::chrono::duration<float> deltaT = std::chrono::system_clock::now() - BoardEvents[iDigitizer].front()->GetTimestampAsChrono();
	int nMillisecondsOld = std::chrono::duration_cast<std::chrono::milliseconds>(deltaT).count();

	if(nMillisecondsOld < maxQueueTime) {
          // within our local copies of the queues (BoardEvents), there is no match for this event.
          // but it's not too old, so we want to push it back onto the global queues and remove it from the local ones
          // next time we call FlushQueuesToDisk, it will be in the queues again
          queues[iDigitizer]->push(BoardEvents[iDigitizer].front());
          BoardEvents[iDigitizer].erase(BoardEvents[iDigitizer].begin(), BoardEvents[iDigitizer].begin() + 1); 
	}
	else {
	  // bfrancis -- disabled jun 28th 2018; verbose output, can be observed by non-100% matching efficiency printout in status
          //mdaq::Logger::instance().warning("EventFactory", TString(Form("Writing incomplete event from board %d due to age %f seconds!", iDigitizer, nMillisecondsOld/1000.)));

          CurrentEvent->digitizers[iDigitizer] = BoardEvents[iDigitizer].front();
          BoardEvents[iDigitizer].erase(BoardEvents[iDigitizer].begin(), BoardEvents[iDigitizer].begin() + 1); 

          commitToWrite = true;

          dqm->UpdateMatchingEfficiency(false);

#ifdef DEBUG_WRITE_PERFORMANCE
          matchSuccessful = false;
          timeOnQueue = nMillisecondsOld;
          writeDebug->Fill();
#endif
	}
      }
#ifdef DEBUG_WRITE_PERFORMANCE
      else {
        matchSuccessful = true;

        std::chrono::duration<float> deltaT = std::chrono::system_clock::now() - BoardEvents[iDigitizer].front()->GetTimestampAsChrono();
        timeOnQueue = std::chrono::duration_cast<std::chrono::milliseconds>(deltaT).count();

        writeDebug->Fill();
      }
#endif

      // if we've matched this event, or are going to write an incomplete event due to age, then we continue to HLT checks
      // otherwise we're done here and will not write this event, so go back to the top of the while loop
      if(!commitToWrite) continue;

      bool firesHLT = PassHLT();
      bool prescaledAwayCosmic = false;
      bool firesCosmic = PassCosmicPrescale(prescaledAwayCosmic);

      // bfrancis -- after adding a second AverageQueueDepth, give this both queues
      dqm->UpdateAll(CurrentEvent, queues[0]->GuessSize(), firesHLT, prescaledAwayCosmic, firesCosmic);

      // if enabled, apply the HLT and cosmic prescale
      if(!firesHLT && !firesCosmic) continue;

      nEventsThisRun++;
      CurrentEvent->SetEventNumber(nEventsThisRun);

      data->Fill();

      if(data->GetEntries() >= maxNumberEventsPerFile) {
        RotateFile();
        break;
      }

      // bfrancis -- reconsider this requency, it can get annoying at higher rates
      if(data->GetEntries() % 1000 == 0) dqm->PrintRates(); // print slower counters for rates
      dqm->GraphAndResetCounters(); // update rate graphs with faster counters; inserts a new point on all versus-time graphs if it's been >1 second since the last point

      // if there are still events in this particular board, we go back to the while() and continue to work
    } // end while(BoardEvents[iDigitizer].size() > 0)

    // now BoardEvents[iDigitizer] is empty, but there still could be events in other boards -- we go back to the for() and continue to work
  } // end loop over iDigitizer

  // finally all queues are empty!
}

// returns true if event passes the HLT
const bool mdaq::EventFactory::PassHLT() const {
  if(!cfg->HLTEnabled) return true;

  // FIXME: this should require all "triggering" channels to be in a row, ie pointing at the IP
  // currently it just looks at all channels regardless of position

  unsigned int nChannelsPerDigitizerTriggering = 0;
  for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
    for(unsigned int i = 0; i < nChannelsPerDigitizer; i++) {
      if(CurrentEvent->GetMinimumSample(iDigitizer, i) < cfg->HLTMaxAmplitude) nChannelsPerDigitizerTriggering++;
    }
  }

  return (nChannelsPerDigitizerTriggering >= cfg->HLTNumRequired);
}

// returns true if event passes a prescaled cosmic trigger
const bool mdaq::EventFactory::PassCosmicPrescale(bool& prescaledAwayCosmic) const {
  
  prescaledAwayCosmic = false;

  if(!cfg->CosmicPrescaleEnabled) return true;

  // FIXME: this should require all "triggering" channels to be in a vertical line, ie pointing straight up/down
  // currently it just looks at all channels regardless of position

  unsigned int nCosmicChannels = 0;
  for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
    for(unsigned int i = 0; i < nChannelsPerDigitizer; i++) {
      if(CurrentEvent->GetMinimumSample(iDigitizer, i) < cfg->CosmicPrescaleMaxAmplitude) nCosmicChannels++;
    }
  }

  if(nCosmicChannels >= cfg->CosmicPrescaleNumRequired) {
    if(rng->Uniform() < 1./cfg->CosmicPrescalePS) return true;
    else {
      prescaledAwayCosmic = true;
      return false;
    }
  }

  return false;

}

// Take the previous 10 events and make a smaller file immediately
void mdaq::EventFactory::PeekFile() {

  TFile * peekFile = new TFile("/home/milliqan/peekEvents.root", "RECREATE");
  if(!peekFile || peekFile->IsZombie()) {
    mdaq::Logger::instance().warning("EventFactory", "Failed to open /home/milliqan/peekEvents.root");
    return;
  }

  TTree * newMetadata = metadata->CloneTree();
  TTree * newData = data->CloneTree(0);

  int currentN = data->GetEntries();
  for(int i = std::max(currentN-10, 0); i < currentN; i++) {
    data->GetEntry(i);
    newData->Fill();
  }

  peekFile->cd();
  newMetadata->Write();
  newData->Write();

  peekFile->Close();

  output->cd();

  mdaq::Logger::instance().info("EventFactory", "Wrote last 10 events to /home/milliqan/peekEvents.root");
}

void mdaq::EventFactory::RegisterFile() {
  std::fstream fout;
  fout.open("/var/log/MilliDAQ_RunList.log", std::fstream::out | std::fstream::app);
  fout << meta_runNumber << "\t" << meta_subrunNumber << "\t" << currentConfigFileName << std::endl;
  fout.close();
}
