#ifndef DQM_H
#define DQM_H

#include "DemonstratorConfiguration.h"
#include "DemonstratorConstants.h"
#include "GlobalEvent.h"
#include "GraphUtilities.h"
#include "Logger.h"

#include <chrono>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

#include "TROOT.h"
#include "TSystem.h"
#include "TStyle.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TLine.h"
#include "TLegend.h"
#include "TPaveText.h"
#include "TString.h"
#include "TStyle.h"
#include "TGraphErrors.h"
#include "TMath.h"
#include "TFile.h"
#include "TError.h"

// bfrancis -- split up allWaves into digitizers, and add second AverageQueueDepth

namespace mdaq {

  class DQM {
  public:
    DQM(const std::string plotDest, mdaq::DemonstratorConfiguration * config, const bool isInteractive = false);
    virtual ~DQM();

    void SetConfiguration(mdaq::DemonstratorConfiguration * config) {
      cfg = config;
    };
    DemonstratorConfiguration * GetConfiguration() { return cfg; };

    void UpdateInteractiveCanvases();

    void UpdateDQM(mdaq::GlobalEvent * evt, unsigned int depth, bool firesHLT, bool prescaledAwayCosmic, bool firesCosmic);
    void UpdateWaveforms(mdaq::GlobalEvent * evt);
    void UpdateAll(mdaq::GlobalEvent * evt, unsigned int depth, bool firesHLT, bool prescaledAwayCosmic, bool firesCosmic) {
      UpdateDQM(evt, depth, firesHLT, prescaledAwayCosmic, firesCosmic);
      UpdateWaveforms(evt);
    };

    void UpdateMatchingEfficiency(const bool passes) { *MatchingEfficiency += passes; };

    void GraphAndResetCounters();
    void ResetPlots();

    // Write PDFs
    void SaveWaveformPDFs();
    void SaveAmplitudePDFs();
    void SaveRatePDFs();
    void SaveDQMPDFs();

    void SavePDFs() {
      SaveWaveformPDFs();
      SaveAmplitudePDFs();
      SaveRatePDFs();
      SaveDQMPDFs();
    };

    void PrintRates();

  private:
    TLegend * CreateLegend() {
      TLegend * leg = new TLegend(0.75, 0.7, 0.95, 0.95, NULL, "brNDC");
      leg->SetBorderSize(0);
      leg->SetFillStyle(0);
      leg->SetFillStyle(0);
      leg->SetTextFont(42);
      leg->SetTextSize(0.04);
      return leg;
    };

    void findRange(TGraphErrors * graphs[nDigitizers][nChannelsPerDigitizer]);
    void findRange(TH1D * histograms[nDigitizers][nChannelsPerDigitizer]);
    void findRange(Rate r1, Rate r2, Rate r3, Rate r4);
    void findRange(Average * avg);
    void findRange(Efficiency * eff);
    void findRange(VRate * vr);

    void Draw(TH1D * histograms[nDigitizers][nChannelsPerDigitizer], TLegend * legends[nDigitizers][nGroupsPerDigitizer / 2], TCanvas * canvas);
    void Draw(VRate * rates, TLegend * legends[nDigitizers][nGroupsPerDigitizer / 2], TCanvas * canvas);
    void DrawGlobalRates();
    void DrawQueueDepth();
    void DrawMatchingEfficiency();

    const int graphsPerPlot = 4;

    TFile * interactiveOutput;
    TCanvas * interactiveWavesCanvas;
    TCanvas * interactiveAmplitudesCanvas;
    TCanvas * interactiveTriggerRatesCanvas;
    TCanvas * interactiveGlobalRatesCanvas;

    TCanvas * singlePaneCanvas;
    TCanvas * dqmCanvas;
    TPad * padTop;
    TPad * padLeft;
    TPad * padRight;
    TPad * padBottom;

    const TString plotDir;
    const bool interactiveMode;

    mdaq::DemonstratorConfiguration * cfg;

    TPaveText * groupLabels[nDigitizers][nGroupsPerDigitizer];
    TPaveText * channelLabels[nDigitizers][nChannelsPerDigitizer];

    TH1D * h_waveforms[nDigitizers][nChannelsPerDigitizer];
    TH1D * h_amplitudes[nDigitizers][nChannelsPerDigitizer];

    // Global counts (across all channels)
    Rate GlobalTriggerRate; // number of triggers across all boards
    Rate HLTRate; // triggers passing the HLT, if it's enabled
    Rate CosmicRate; // triggers passing the Cosmic prescale, if it's enabled
    Rate CosmicPrescaledAwayRate; // triggers passing the Cosmic prescale, but were ignored due to the PS   

    float h_ymin;
    float h_ymax;

    float gr_ymin;
    float gr_ymax;
    float gr_xmin;
    float gr_xmax;

    // Channel-by-channel counts
    VRate * MissedTriggerRates;
    VRate * ChannelTriggerRates;

    Average * AverageQueueDepth;

    Efficiency * MatchingEfficiency;

    std::chrono::system_clock::time_point initTime;           // initial time for rates
    std::chrono::system_clock::time_point initTime_slowReset; // initial time for info printer

    TLegend * legGroupPairLegends[nDigitizers][nGroupsPerDigitizer / 2];
    TLegend * globalRatesLegend;
    TLegend * averageQueueDepthLegend;

    int colors[nChannelsPerDigitizer] = {
      kBlack, kRed,
      kGreen+2, kBlue,
      kOrange-3, kPink-8,
      kAzure+7, kTeal+5,
      kGray+2, kYellow+3,
      kMagenta+1, kSpring+7,
      kPink+2, kOrange+3,
      kBlue-7, kRed-6
    };
    
  };

}

#endif
