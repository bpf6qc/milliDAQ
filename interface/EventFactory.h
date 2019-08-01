#ifndef EVENT_FACTORY_H
#define EVENT_FACTORY_H

#include <chrono>
#include <ctime>
#include <fstream>
#include <string>

#include "CAENDigitizer.h"
#include "DemonstratorConfiguration.h"
#include "DQM.h"
#include "GitVersion.h"
#include "GlobalEvent.h"
#include "Logger.h"
#include "Queue.h"

#include "TFile.h"
#include "TTree.h"
#include "TString.h"
#include "TRandom3.h"

namespace mdaq {

  class EventFactory {
  public:
    EventFactory(const std::string tmpFile, const std::string destDir, DemonstratorConfiguration * config, const bool isInteractiveDQM = false);
    virtual ~EventFactory();

    void SetConfiguration(mdaq::DemonstratorConfiguration * config) {
      cfg = config;
      dqm->SetConfiguration(config);
      currentConfigFileName = config->GetCurrentConfigFile();
    };
    DemonstratorConfiguration * GetConfiguration() { return cfg; };

    Long64_t GetEntries() { return data->GetEntries(); };

    void FlushQueuesToDisk(mdaq::Queue * queues[nDigitizers]);

    bool FilterHLT(mdaq::GlobalEvent * evt);
    bool FilterCosmics(mdaq::GlobalEvent * evt);

    void StartFile();
    void FinishFile();
    void RotateFile() {
      FinishFile();
      IterateSubRunNumber();
      StartFile();
    };

    void PeekFile();

    void ResetEventsRecordedInRun()         { nEventsThisRun = 0; };
    unsigned long int EventsRecordedInRun() { return nEventsThisRun; };

    void IterateRunNumber()    { meta_runNumber++; };
    void IterateSubRunNumber() { meta_subrunNumber++; };
    void ResetSubRunNumber()   { meta_subrunNumber = 1; };

    int GetRunNumber()    { return meta_runNumber; };
    int GetSubRunNumber() { return meta_subrunNumber; };

    std::chrono::duration<float> GetRunDuration() { return std::chrono::system_clock::now() - currentRunStartTime; };

    void DefineNewRun() {
      IterateRunNumber();
      ResetSubRunNumber();
      ResetEventsRecordedInRun();
      currentRunStartTime = std::chrono::system_clock::now();
      StartFile();

#ifdef MATCH_TIMING_PERFORMANCE
      matchTimingOutput = new TFile("/home/milliqan/matchTimingPerformance.root", "RECREATE");
      matchTiming = new TTree("reads", "Timing perforamnce for each read attempt");

      matchTiming->Branch("nPulledFromQueues", &nPulledFromQueues);
      matchTiming->Branch("matching_ttts", &matching_ttts);
      matchTiming->Branch("nMatchesAttempted", &nMatchesAttempted);
      matchTiming->Branch("boardBeingMatched", &boardBeingMatched);
      matchTiming->Branch("matchSuccessful", &matchSuccessful);
#endif
    };

#ifdef MATCH_TIMING_PERFORMANCE
    void ClosePerformanceFile() {
      matchTimingOutput->cd();
      matchTiming->Write();
      matchTimingOutput->Close();
    };
#endif

    void RegisterFile();

    mdaq::DQM * GetDQM() { return dqm; };

  private:

    void GetPreviousRunNumber();

    const bool PassHLT() const;
    const bool PassCosmicPrescale(bool& prescaledAwayCosmic) const;

    const std::string tempFileName;
    const std::string destinationDirectory;

    std::string currentConfigFileName;

    mdaq::DemonstratorConfiguration * cfg;
    mdaq::DQM * dqm;

    const bool interactiveMode;

    TFile * output;
    TTree * data;
    TTree * metadata;

    unsigned long int nEventsThisRun;

    // Branch members Events
    mdaq::GlobalEvent * CurrentEvent;

    std::vector<mdaq::V1743Event*> BoardEvents[nDigitizers];
    mdaq::V1743Event * tempBoardEvents[nDigitizers];

    // Branch members Metadata
    mdaq::DemonstratorConfiguration * CurrentConfig;
    TString * meta_fileOpenTime;
    TString * meta_fileCloseTime;
    int meta_runNumber;
    int meta_subrunNumber;
    TString * meta_configFile;
    TString * meta_gitVersion;

    std::chrono::system_clock::time_point currentFileOpenTime;

    std::chrono::system_clock::time_point currentRunStartTime;

    // random number generator for the cosmic prescale
    TRandom3 * rng;

    const Long64_t autoSaveEvery = 10000; // save every 1000 events
    const Long64_t maxNumberEventsPerFile = 10000; // start a new file every 10,000 events

    // bfrancis -- test
    const int matchingTTTRequirement = 200 * 10.0; // [ns]

    // bfrancis -- test
    const int maxQueueTime = 3000; 

#ifdef DEBUG_MATCH_PERFORMANCE
    TFile * matchDebugOutput;
    TTree * matchDebugTree;

    int32_t nPulledFromQueues[nDigitizers];
    double matching_ttts[nDigitizers];
    int boardBeingMatched;
#endif

#ifdef DEBUG_WRITE_PERFORMANCE
    TFile * writeDebugOutput;
    TTree * writeDebugTree;

    double closestTTTDifference;
    int nMatchesAttempted;
    bool matchSuccessful;
    int timeOnQueue;
#endif

  };

}

#endif
