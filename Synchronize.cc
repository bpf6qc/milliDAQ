#include "Synchronize.h"

#define USE_GUI

#include "TFile.h"
#include "TTimeStamp.h"
#include "TTree.h"

using namespace mdaq;
using namespace std;

// bfrancis -- finalize all of this

int kbhit(void) {

  struct termios oldt, newt;
  int ch;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  ch = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

  if(ch != EOF) {
    ungetc(ch, stdin);
    return 1;
  }

  return 0;
}

enum userCommands {kUserNull,
		   kUserQuit,
		   kUserStartRun,
		   kUserCheckClockAlignment,
		   kUserReloadPLLConfiguration,
		   kUserAlignTT,
		   kUserResetPlots,
		   kUserNumCommands};

int CheckCommand() {
  if(!kbhit()) return kUserNull;

  switch(getchar()) {
  case 's':
    return kUserStartRun;
    break;
  case 'c':
    return kUserCheckClockAlignment;
    break;
  case 'r':
    return kUserReloadPLLConfiguration;
    break;
  case 't':
    return kUserAlignTT;
    break;
  case 'p':
    return kUserResetPlots;
    break;
  case 'q':
    return kUserQuit;
    break;
  }

  return kUserNull;
}

double interpolate(float * waveform, DemonstratorConfiguration * cfg) {
	
  double crosspoint = -1.0;

  const bool isRisingEdge = (cfg->digitizers[0].channels[referenceChannel].triggerPolarity == CAEN_DGTZ_TriggerOnRisingEdge);
  const double threshold = cfg->digitizers[0].channels[referenceChannel].triggerThreshold * 1000.0;

  for(uint32_t i = 0; i < cfg->digitizers[0].RecordLength - 1; i++) {

    if(isRisingEdge && waveform[i] <= threshold && waveform[i+1] > threshold) {
      crosspoint = i + ((waveform[i] - threshold) / (waveform[i] - waveform[i+1]));
      cout << "ay crosspoint = " << i << " + (" << (waveform[i] - threshold) << " / " << (waveform[i] - waveform[i+1]) << " = " << crosspoint << endl;
      break;
    }

    if(!isRisingEdge && waveform[i] >= threshold && waveform[i+1] < threshold) {
      crosspoint = i + ((threshold - waveform[i]) / (waveform[i+1] - waveform[i]));
      break;
    }
  }

  return crosspoint;
}

void RunAction() {

  if(UserQuitsProgram.load()) return;

#ifdef USE_GUI
  // allocate some histograms for analysis...
  TCanvas * canvas = new TCanvas("canvas", "canvas", 10, 10, 800, 600);
  TCanvas * waveCanvas = new TCanvas("waveCanvas", "waveCanvas", 10, 10, 800, 600);
#else
  cout << "USE_GUI is not defined..." << endl;
#endif

  TH1D * h_deltaT = new TH1D("deltaT", "deltaT", 4000, -200, 200);
  TH1D * wave_b0_ch1 = new TH1D("b0_ch1", "b0_ch1;Sample;[mV]", 1024, 0, 1024);
  TH1D * wave_b1_ch1 = new TH1D("b0_ch1", "b0_ch1;Sample;[mV]", 1024, 0, 1024);
  TH1D * h_wideDeltaT = new TH1D("wideDeltaT", "wideDeltaT", 50000, 0, 1000000);

  wave_b0_ch1->SetLineColor(1);
  wave_b1_ch1->SetLineColor(8);

  TTree * tree_read = new TTree("readAccesses", "readAccesses"); // filled every time DAQ::Read is called
  uint32_t nEventsRead[nDigitizers];
  TTimeStamp readTimeStamp;
  int queueDepths[nDigitizers];
  tree_read->Branch("nevents_0", &nEventsRead[0]);
  tree_read->Branch("nevents_1", &nEventsRead[1]);
  tree_read->Branch("readTimeStamp", &readTimeStamp);
  tree_read->Branch("queueDepth_0", &queueDepths[0]);
  tree_read->Branch("queueDepth_1", &queueDepths[1]);

  TTree * tree_match = new TTree("matchAttempts", "matchAttempts"); // filled every time a match between boards is attempted
  uint32_t nPulledFromQueues[nDigitizers];
  double matching_ttts[nDigitizers];
  int nMatchesAttempted;
  int boardBeingMatched;
  bool matchSuccessful;
  tree_match->Branch("nPulledFromQueues_0", &nPulledFromQueues[0]);
  tree_match->Branch("nPulledFromQueues_1", &nPulledFromQueues[1]);
  tree_match->Branch("matching_ttts_0", &matching_ttts[0]);
  tree_match->Branch("matching_ttts_1", &matching_ttts[1]);
  tree_match->Branch("nMatchesAttempted", &nMatchesAttempted);
  tree_match->Branch("boardBeingMatched", &boardBeingMatched);
  tree_match->Branch("matchSuccessful", &matchSuccessful);

  TTree * tree_write = new TTree("writes", "writes"); // filled every time a matched event is 'written' to disk
  double t_ttt0, t_ttt1;
  int nMillisecondsOnQueue[nDigitizers];
  tree_write->Branch("ttt0", &t_ttt0);
  tree_write->Branch("ttt1", &t_ttt1);
  tree_write->Branch("wave_b0_ch1", &wave_b0_ch1);
  tree_write->Branch("wave_b1_ch1", &wave_b1_ch1);
  tree_write->Branch("edgeTime_0", &edgeTime[0]);
  tree_write->Branch("edgeTime_1", &edgeTime[1]);
  tree_write->Branch("nMillisecondsOnQueue_0", &nMillisecondsOnQueue[0]);
  tree_write->Branch("nMillisecondsOnQueue_1", &nMillisecondsOnQueue[1]);

  for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
    missingEdge[iDigitizer] = 0;
    numRollovers[iDigitizer] = 0;
    edgeTime[iDigitizer] = 0;
  }

  bool haveFirstEvent = false;
  double tttOffset = 0;

  cout << "********************************************" << endl;
  cout << "** Starting run..." << endl;
	
  theDAQ->StartRun();

  cout << "** Started!" << endl;
  cout << "**" << endl;
  cout << "** [t]: align trigger time" << endl;
  cout << "** [r]: reset plots" << endl;
  cout << "** [q]: quit" << endl;
  cout << "********************************************" << endl << endl;

  previousTime = chrtime::now();

  int trigCount[nDigitizers] = {0, 0};
  int matchingEvents = 0;

  meanT = 0;
  meanTTT = 0;
  sigmaT = 0;
  sigmaTTT = 0;
  nMeasurements = 0;

  while(!UserQuitsProgram.load()) {
    ///////////////////////////////////////////////////////
    // calculate and print trigger rates each second
    ///////////////////////////////////////////////////////

    deltaT = chrtime::now() - previousTime;
    nMillisecondsLive = std::chrono::duration_cast<std::chrono::milliseconds>(deltaT).count();	

    if(nMillisecondsLive > 1000) {
      cout << "Event rate = " << (float)trigCount[0] / (float)nMillisecondsLive * 1000 << "  /  " << (float)trigCount[1] / (float)nMillisecondsLive * 1000 << " Hz" << endl;
      cout << "Matched events = " << 100 * (float)matchingEvents / (float)trigCount[0] << "  /  " << 100 * (float)matchingEvents / (float)trigCount[1] << "%" << endl;
			
      if(matchingEvents == 0) cout << "Missing edges: no edge found" << endl;
      else {
	for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
	  cout << "Board " << iDigitizer << ": Missing edges = " << 100 * (float)missingEdge[iDigitizer] / (float)matchingEvents << "%" << endl;
	  missingEdge[iDigitizer] = 0;
	}
      }

      trigCount[0] = 0;
      trigCount[1] = 0;
      matchingEvents = 0;

      if(nMeasurements == 0) {
	cout << "DeltaT edges:     mean = ----- +/- -----" << endl;
	cout << "DeltaT time tags: mean = ----- +/- -----" << endl;
      }
      else {
	cout << "DeltaT edges:     mean = " << meanT / nMeasurements << " +/- " << sqrt(sigmaT/nMeasurements - meanT*meanT/nMeasurements/nMeasurements) << endl;
	cout << "DeltaT time tags: mean = " << meanTTT / nMeasurements << " +/- " << sqrt(sigmaTTT/nMeasurements - meanTTT*meanTTT/nMeasurements/nMeasurements) << endl;
      }

#ifdef USE_GUI
      canvas->cd();
      // bfrancis -- fix histogram ranges now that things are actually synchronized
      // h_deltaT->Draw("hist");
      h_wideDeltaT->Draw("hist");
      canvas->Update();
      gSystem->ProcessEvents();

      waveCanvas->cd();
      wave_b0_ch1->Draw();
      wave_b1_ch1->Draw("same");
      waveCanvas->Update();
      gSystem->ProcessEvents();
#endif

      // update the time
      previousTime = chrtime::now();
      cout << endl << endl;
    }

    ///////////////////////////////////////////////////////
    // read data from the digitizers
    ///////////////////////////////////////////////////////

    theDAQ->Read(masterQueues, nEventsRead);

    auto nsecs = std::chrono::duration_cast<std::chrono::nanoseconds>(chrtime::now().time_since_epoch());
    readTimeStamp = std::chrono::system_clock::to_time_t(chrtime::now());
    readTimeStamp.SetNanoSec(nsecs.count() % (int)1e9);
    queueDepths[0] = masterQueues[0]->GuessSize();
    queueDepths[1] = masterQueues[1]->GuessSize();
    tree_read->Fill();

    trigCount[0] += nEventsRead[0];
    trigCount[1] += nEventsRead[1];

    ///////////////////////////////////////////////////////
    // analyze data
    ///////////////////////////////////////////////////////

    // first pull all events from all board queues to our local copy
    for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
      BoardEvents[iDigitizer].clear();
      masterQueues[iDigitizer]->pull(BoardEvents[iDigitizer]);

      nPulledFromQueues[iDigitizer] = BoardEvents[iDigitizer].size();
    }

    // now we match amongst these queues in the TriggerTimeTag (TTT)

    for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
  
      boardBeingMatched = iDigitizer;

      // work at this board's events until they're all gone
      while(BoardEvents[iDigitizer].size() > 0) {

	double matchTTT = BoardEvents[iDigitizer].front()->GetTTTNanoseconds();

	bool foundMatch = false;

	nMatchesAttempted = 0;
	matching_ttts[iDigitizer] = matchTTT;
  
	// now search through all other boards to find a matching TTT
	for(unsigned int j = 0; j < nDigitizers; j++) {

	  if(j == iDigitizer) continue; // don't match an event to itself

	  // look through all events in this other board, and try to find a matching TTT
	  for(unsigned int jEvent = 0; jEvent < BoardEvents[j].size(); jEvent++) {

	    double otherTTT = BoardEvents[j][jEvent]->GetTTTNanoseconds();

	    if(!haveFirstEvent) {
	      haveFirstEvent = true;
	      tttOffset = (iDigitizer == 0) ? otherTTT-matchTTT : matchTTT-otherTTT;
				
	      cout << endl << endl << endl;
	      cout << "THE TTT OFFSET tttOffset = " << tttOffset << endl;
	      cout << "TTT[0] = " << ((iDigitizer == 0) ? matchTTT : otherTTT) << endl;
	      cout << "TTT[1] = " << ((iDigitizer == 1) ? matchTTT : otherTTT) << endl;
				
	    }
				
	    if(iDigitizer == 0) otherTTT -= tttOffset;
	    else otherTTT += tttOffset;

	    matching_ttts[j] = otherTTT;
	    nMatchesAttempted++;
	    matchSuccessful = (fabs(otherTTT - matchTTT) < matchingWindow);
	    tree_match->Fill();

	    if(false)
	      cout << "comparing matchTTT(" << matchTTT 
		   << ") to otherTTT = " << otherTTT 
		   << " -- difference of " << otherTTT-matchTTT 
		   << "   " << (matchSuccessful ? "MATCHED" : "not matched")
		   << endl;
  
	    if(fabs(otherTTT - matchTTT) < matchingWindow) {

	      // this is now a fully matched event; we set the final event to use them and remove them from future consideration  
	              
	      // set waveform histograms
	      if(iDigitizer == 0) {
		BoardEvents[iDigitizer].front()->GetWaveform(referenceChannel, wave_b0_ch1);
		BoardEvents[j].front()->GetWaveform(referenceChannel, wave_b1_ch1);
	      }
	      else {
		BoardEvents[j].front()->GetWaveform(referenceChannel, wave_b0_ch1);
		BoardEvents[iDigitizer].front()->GetWaveform(referenceChannel, wave_b1_ch1);
	      }

	      matchingEvents++;

	      edgeTime[iDigitizer] = interpolate(BoardEvents[iDigitizer].front()->waveform[referenceChannel], theDAQ->GetConfiguration());
	      edgeTime[j] = interpolate(BoardEvents[j][jEvent]->waveform[referenceChannel], theDAQ->GetConfiguration());

	      bool allEdgesFound = true;

	      if(edgeTime[iDigitizer] < 0) {
		missingEdge[iDigitizer]++;
		allEdgesFound = false;
	      }
	      if(edgeTime[j] < 0) {
		missingEdge[j]++;
		allEdgesFound = false;
	      }

	      t_ttt0 = (iDigitizer == 0) ? matchTTT : otherTTT;
	      t_ttt1 = (iDigitizer == 1) ? matchTTT : otherTTT;

	      nMillisecondsOnQueue[iDigitizer] = std::chrono::duration_cast<std::chrono::milliseconds>(
												       chrtime::now() - BoardEvents[iDigitizer].front()->GetTimestampAsChrono()).count();
	      nMillisecondsOnQueue[j] = std::chrono::duration_cast<std::chrono::milliseconds>(
											      chrtime::now() - BoardEvents[j][jEvent]->GetTimestampAsChrono()).count();

	      tree_write->Fill();

	      if(allEdgesFound) {
		double deltaTTT = matchTTT - otherTTT;
		meanTTT += deltaTTT;
		sigmaTTT += deltaTTT * deltaTTT;
	
		double deltaT = (matchTTT + edgeTime[iDigitizer] * theDAQ->GetConfiguration()->digitizers[iDigitizer].secondsPerSample*1.e9) - (otherTTT + edgeTime[j] * theDAQ->GetConfiguration()->digitizers[j].secondsPerSample*1.e9);
		cout << "deltaT = (" << matchTTT << " + " << edgeTime[iDigitizer] << " * " << theDAQ->GetConfiguration()->digitizers[iDigitizer].secondsPerSample*1.e9 << ") - " << endl;
		cout << "\t- (" << otherTTT << " + " << edgeTime[j] << " * " << theDAQ->GetConfiguration()->digitizers[j].secondsPerSample*1.e9 << endl;
		cout << "\t= " << deltaT << endl;

		meanT += deltaT;
		sigmaT += deltaT * deltaT;
	
		nMeasurements++;
	
		h_deltaT->Fill(deltaT);	
	      }

	      // now remove these events from consideration
	      BoardEvents[iDigitizer].erase(BoardEvents[iDigitizer].begin(), BoardEvents[iDigitizer].begin() + 1); 
	      BoardEvents[j].erase(BoardEvents[j].begin() + jEvent, BoardEvents[j].begin() + jEvent + 1);

	      foundMatch = true;

	      break;
	    }
	  }
  
	}

	// if you're here and you haven't found a match, that means this event is by itself -- add it back to the queue unless it's super old???
	if(!foundMatch) {

	  int nMillisecondsOld = std::chrono::duration_cast<std::chrono::milliseconds>(
										       chrtime::now() - BoardEvents[iDigitizer].front()->GetTimestampAsChrono()).count();

	  if(nMillisecondsOld < 5000) {
	    if(false) cout << "re-queueing an event from board " << iDigitizer << endl;
	    masterQueues[iDigitizer]->push(BoardEvents[iDigitizer].front());
	  }
	  else cout << A_BRIGHT_RED << "KILLING a super-old event from board " << iDigitizer << A_RESET << endl;

	  BoardEvents[iDigitizer].erase(BoardEvents[iDigitizer].begin(), BoardEvents[iDigitizer].begin() + 1); 	

	}

      } // while BoardEvents[iDigitizer] has events

    } // for all digitizers

    if(UserAlignsTT.load()) {
      // bfrancis -- this doesn't do anything
      UserAlignsTT.store(false);
    }


    if(UserResetsPlots.load()) {
      h_deltaT->Reset();
      meanT = 0;
      meanTTT = 0;
      sigmaT = 0;
      sigmaTTT = 0;
      nMeasurements = 0;
      UserResetsPlots.store(false);
    }

  }

  theDAQ->StopRun();
  theDAQ->CloseDevices();

  cout << endl << endl
       << "After the run has ended, there remain "
       << masterQueues[0]->GuessSize()
       << " / "
       << masterQueues[1]->GuessSize()
       << " events on each queue that we are abandoning!"
       << endl << endl;

  TFile * output = new TFile("output.root", "RECREATE");
  h_deltaT->Write();
  h_wideDeltaT->Write();
  tree_read->Write();
  tree_match->Write();
  tree_write->Write();
  output->Close();
  delete output;
  delete tree_read;
  delete tree_match;
  delete tree_write;

  delete h_deltaT;
  delete wave_b0_ch1;
  delete wave_b1_ch1;

#ifdef USE_GUI
  delete canvas;
  delete waveCanvas;
#endif
}

int main(int argc, char** argv) {

  TApplication * theApp = new TApplication("theApp", 0, 0);
  theApp->SetReturnFromRun(true);

  for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
    TTT[iDigitizer] = 0;
    previousTTT[iDigitizer] = 0;
  }

  // set all logging to stdout instead of /var/log/MilliDAQ.log
  Logger::instance().SetUseLogFile(false);

  // define the masterQueues
  masterQueues[0] = new Queue();
  masterQueues[1] = new Queue();

  cout << endl;
  cout << "********************************************" << endl;
  cout << "** Synchronize milliQan digitizers..." << endl;
  cout << "********************************************" << endl << endl;

  // define theDAQ and configure it
  theDAQ = new DAQ();
  if(!theDAQ->ReadConfiguration("/home/milliqan/MilliDAQ/config/caen.py")) {
    Logger::instance().error("Synchronize", "A valid configuration is required!");
    return EXIT_FAILURE;
  }

  // open some kind of command reading thing...

  theDAQ->ConnectToBoards();
  theDAQ->InitializeBoards();

  // Arming the LVDS start on slave boards seems to need a few seconds, so we wait
  this_thread::sleep_for(chrono::seconds(3));
    
  cout << "********************************************" << endl;
  cout << "** Configuration complete!" << endl;
  cout << "**" << endl;
  cout << "** [s]: start run" << endl;
  cout << "** [c]: check clock alignment" << endl;
  cout << "** [q]: quit" << endl;
  cout << "********************************************" << endl << endl;

  while(!UserStartsRun.load() && !UserQuitsProgram.load()) {

    switch(CheckCommand()) {
    case kUserNull:
      sleep(50);
      continue;
    case kUserQuit:
      UserQuitsProgram.store(true);
      break;
    case kUserCheckClockAlignment:
      {
	for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) theDAQ->SetOutputClockSignal(iDigitizer);
	cout << "********************************************" << endl;
	cout << "** Trigger clocks are now available on TRG-OUT!" << endl;
	cout << "**" << endl;
	cout << "** [r]: reload PLL configuration" << endl;
	cout << "** [s]: start run" << endl;
	cout << "** [q]: quit" << endl;
	cout << "********************************************" << endl << endl;

	int cmd = CheckCommand();
	while(cmd != kUserStartRun && cmd != kUserQuit) {
	  if(cmd == kUserReloadPLLConfiguration) {
	    theDAQ->ForcePLLReload(0);
	    theDAQ->ForceClockSync(0);
	    cout << "PLL reloaded!" << endl;
	  }
	  cmd = CheckCommand();
	}

	if(cmd == kUserQuit) {
	  UserQuitsProgram.store(true);
	  break;
	}

	if(cmd == kUserStartRun) {
	  UserStartsRun.store(true);
	  for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) theDAQ->UnsetOutputClockSignal(iDigitizer);
	  break;
	}

	break;
      }
    case kUserStartRun:
      UserStartsRun.store(true);
      break;
    }

  }

  thread runThread(RunAction);
	
  while(!UserQuitsProgram.load()) {

    this_thread::sleep_for(chrono::milliseconds(200));

    ///////////////////////////////////////////////////////
    // check user commands
    ///////////////////////////////////////////////////////

    switch(CheckCommand()) {
    case kUserNull:
      break;
    case kUserQuit:
      UserQuitsProgram.store(true);
      break;
    case kUserAlignTT:
      UserAlignsTT.store(true);
      break;
    case kUserResetPlots:
      UserResetsPlots.store(true);
      break;
    }

  } // end while(!UserQuits)

  ///////////////////////////////////////////////////////
  // stop run and clean up
  ///////////////////////////////////////////////////////

  runThread.join();

  delete theDAQ;	

  return EXIT_SUCCESS;
}

