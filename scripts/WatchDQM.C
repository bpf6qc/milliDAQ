#include <thread>

using namespace std;

atomic<bool> UserQuits(false);

void loop() {

  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(0);

  TFile * input = new TFile("/home/milliqan/MilliDAQ/plots/interactiveDQM.root", "READ");

  TCanvas * interactiveWavesCanvas = new TCanvas("interactiveWavesCanvas", "Waveforms", 10, 10, 800, 600);
  interactiveWavesCanvas->Divide(2, 2, 0, 0);

  TCanvas * interactiveTriggerRatesCanvas = new TCanvas("interactiveTriggerRatesCanvas", "Trigger Rates", 10, 10, 800, 600);
  interactiveTriggerRatesCanvas->Divide(2, 2, 0, 0);

  TCanvas * interactiveGlobalRatesCanvas = new TCanvas("interactiveGlobalRatesCanvas", "Global Rates", 10, 10, 800, 600);

  // objects
  TH1D * h_waveforms[16];
    
  TGraphErrors * gr_ChannelTriggerCounts[16];

  TGraphErrors * gr_GlobalTriggerCount;
  TGraphErrors * gr_HLTCount;
  TGraphErrors * gr_CosmicCount;
  TGraphErrors * gr_CosmicPrescaledAwayCount;

  // extra copy for legends?
  TH1D * h_waveforms_leg[16];
    
  TGraphErrors * gr_ChannelTriggerCounts_leg[16];

  TGraphErrors * gr_GlobalTriggerCount_leg;
  TGraphErrors * gr_HLTCount_leg;
  TGraphErrors * gr_CosmicCount_leg;
  TGraphErrors * gr_CosmicPrescaledAwayCount_leg;

  TLegend * legGroupPairLegends[4];
  TLegend * globalRatesLegend;

  bool haveDrawn = false;

  while(!UserQuits.load()) {
    input->ReadKeys();

    /*
      for(int i = 0; i < 16; i++) {
      delete input->FindObject(Form("waveform_%d", i));
      }
      for(int i = 0; i < 16; i++) {
      delete input->FindObject(Form("ChannelTriggerCounts_%d", i));
      }

      delete input->FindObject("globalTriggerCount");
      delete input->FindObject("HLTCount");
      delete input->FindObject("CosmicCount");
      delete input->FindObject("CosmicPrescaledAwayCount");
    */
		
    for(int i = 0; i < 16; i++) {
      h_waveforms[i] = (TH1D*)input->Get(Form("waveform_%d", i));
      h_waveforms[i]->SetDirectory(0);
      if(!haveDrawn) h_waveforms_leg[i] = (TH1D*)input->Get(Form("waveform_%d_leg", i));

      TGraphErrors * gr = (TGraphErrors*)input->Get(Form("ChannelTriggerCounts_%d", i));
      gr_ChannelTriggerCounts[i] = (TGraphErrors*)gr->Clone(Form("ChannelTriggerCounts_%d", i));
      if(!haveDrawn) gr_ChannelTriggerCounts_leg[i] = (TGraphErrors*)gr->Clone(Form("ChannelTriggerCounts_%d_leg", i));
      //gr_ChannelTriggerCounts[i]->SetDirectory(0);
    }
    TGraphErrors * gr_a = (TGraphErrors*)input->Get("globalTriggerCount");
    gr_GlobalTriggerCount = (TGraphErrors*)gr_a->Clone("globalTriggerCount");
    if(!haveDrawn) gr_GlobalTriggerCount_leg = (TGraphErrors*)gr_a->Clone("globalTriggerCount_leg");
    //gr_GlobalTriggerCount->SetDirectory(0);

    TGraphErrors * gr_b = (TGraphErrors*)input->Get("HLTCount");
    gr_HLTCount = (TGraphErrors*)gr_b->Clone("HLTCount");
    if(!haveDrawn) gr_HLTCount_leg = (TGraphErrors*)gr_b->Clone("HLTCount_leg");
    //gr_HLTCount->SetDirectory(0);

    TGraphErrors * gr_c = (TGraphErrors*)input->Get("CosmicCount");
    gr_CosmicCount = (TGraphErrors*)gr_c->Clone("CosmicCount");
    if(!haveDrawn) gr_CosmicCount_leg = (TGraphErrors*)gr_c->Clone("CosmicCount_leg");
    //gr_CosmicCount->SetDirectory(0):

    TGraphErrors * gr_d = (TGraphErrors*)input->Get("CosmicPrescaledAwayCount");
    gr_CosmicPrescaledAwayCount = (TGraphErrors*)gr_d->Clone("CosmicPrescaledAwayCount");
    if(!haveDrawn) gr_CosmicPrescaledAwayCount_leg = (TGraphErrors*)gr_d->Clone("CosmicPrescaledAwayCount_leg");
    //gr_CosmicPrescaledAwayCount->SetDirectory(0);

    interactiveWavesCanvas->cd();

    if(!haveDrawn) {
      for(int i = 0; i < 4; i++) {
	legGroupPairLegends[i] = new TLegend(0.75, 0.7, 0.95, 0.95, NULL, "brNDC");
	legGroupPairLegends[i]->SetBorderSize(0);
	legGroupPairLegends[i]->SetFillColor(0);
	legGroupPairLegends[i]->SetFillStyle(0);
	legGroupPairLegends[i]->SetTextFont(42);
	legGroupPairLegends[i]->SetTextSize(0.04);
	for(int j = 4*i; j < 4*(i+1); j++) legGroupPairLegends[i]->AddEntry(gr_ChannelTriggerCounts_leg[j], Form("Ch. %d", j), "LP");
      }

      globalRatesLegend = new TLegend(0.6, 0.7, 0.95, 0.95, NULL, "brNDC");
      globalRatesLegend->SetBorderSize(0);
      globalRatesLegend->SetFillStyle(0);
      globalRatesLegend->SetFillStyle(0);
      globalRatesLegend->SetTextFont(42);
      globalRatesLegend->SetTextSize(0.04);
      globalRatesLegend->AddEntry(gr_GlobalTriggerCount_leg, "Global trigger rate", "LP");
      globalRatesLegend->AddEntry(gr_HLTCount_leg, "HLT rate", "LP");
      globalRatesLegend->AddEntry(gr_CosmicCount_leg, "Cosmic rate (unprescaled)", "LP");
      globalRatesLegend->AddEntry(gr_CosmicPrescaledAwayCount_leg, "Cosmic rate (prescaled", "LP");

      haveDrawn = true;
    }

    // waveforms
    double ymin = -10.0;
    double ymax = 10.0;

    for(int i = 0; i < 16; i++) {
      double thisMin = h_waveforms[i]->GetBinContent(h_waveforms[i]->GetMinimumBin()) * 1.10;
      double thisMax = h_waveforms[i]->GetBinContent(h_waveforms[i]->GetMaximumBin()) * 1.10;
      if(thisMin < ymin) ymin = thisMin;
      if(thisMax > ymax) ymax = thisMax;
    }

    for(int i = 0; i < 4; i++) {
      interactiveWavesCanvas->cd(i+1);
      h_waveforms[4*i]->GetYaxis()->SetRangeUser(ymin, ymax);
      h_waveforms[4*i]->Draw("hist");
      for(int j = 1; j < 4; j++) h_waveforms[4*i + j]->Draw("hist same");
      legGroupPairLegends[i]->Draw("same");
    }

    interactiveWavesCanvas->Modified();
    interactiveWavesCanvas->Update();

    interactiveTriggerRatesCanvas->cd();

    // trigger rates
    double gr_ChannelTriggerCounts_ymax = 5.0;

    for(int i = 0; i < 16; i++) {
      for(int j = std::max(0, gr_ChannelTriggerCounts[i]->GetN() - 290); j < gr_ChannelTriggerCounts[i]->GetN(); j++) {
	float val = gr_ChannelTriggerCounts[i]->GetY()[j];
	if(val > gr_ChannelTriggerCounts_ymax) gr_ChannelTriggerCounts_ymax = val;
      }
    }

    for(int i = 0; i < 4; i++) {
      interactiveTriggerRatesCanvas->cd(i+1);
      gr_ChannelTriggerCounts[4*i]->Draw("ALP");
      float val = (gr_ChannelTriggerCounts[4*i]->GetN() > 0) ? gr_ChannelTriggerCounts[4*i]->GetX()[gr_ChannelTriggerCounts[4*i]->GetN() - 1] : 0;
      gr_ChannelTriggerCounts[4*i]->GetXaxis()->SetRangeUser(std::max(val - 290.0, 0.0), val + 10.0);
      gr_ChannelTriggerCounts[4*i]->GetYaxis()->SetRangeUser(-2.0, 1.1 * gr_ChannelTriggerCounts_ymax);
      for(int j = 1; j < 4; j++) gr_ChannelTriggerCounts[4*i + j]->Draw("LP same");
      legGroupPairLegends[i]->Draw("same");
    }

    interactiveTriggerRatesCanvas->Modified();
    interactiveTriggerRatesCanvas->Update();

    // global rates
    ymax = -1.e6;
    if(gr_GlobalTriggerCount->GetN() == 0) ymax = 10.0;
    else {
      for(int j = std::max(0, gr_GlobalTriggerCount->GetN() - 290); j < gr_GlobalTriggerCount->GetN(); j++) {
	float val = gr_GlobalTriggerCount->GetY()[j];
	if(val > ymax) ymax = val;
      }
      for(int j = std::max(0, gr_HLTCount->GetN() - 290); j < gr_HLTCount->GetN(); j++) {
	float val = gr_HLTCount->GetY()[j];
	if(val > ymax) ymax = val;
      }
      for(int j = std::max(0, gr_CosmicCount->GetN() - 290); j < gr_CosmicCount->GetN(); j++) {
	float val = gr_CosmicCount->GetY()[j];
	if(val > ymax) ymax = val;
      }
      for(int j = std::max(0, gr_CosmicPrescaledAwayCount->GetN() - 290); j < gr_CosmicPrescaledAwayCount->GetN(); j++) {
	float val = gr_CosmicPrescaledAwayCount->GetY()[j];
	if(val > ymax) ymax = val;
      }
    }

    interactiveGlobalRatesCanvas->cd();

    gr_GlobalTriggerCount->Draw("ALP");
    gr_GlobalTriggerCount->GetYaxis()->SetTitle("[Hz]");
    gr_GlobalTriggerCount->GetYaxis()->SetRangeUser(-1, 1.1 * ymax);
    gr_GlobalTriggerCount->GetXaxis()->SetTitle("Time [s]");
    float val = (gr_GlobalTriggerCount->GetN() > 0) ? gr_GlobalTriggerCount->GetX()[gr_GlobalTriggerCount->GetN() - 1] : 0;
    gr_GlobalTriggerCount->GetXaxis()->SetRangeUser(std::max(val - 290.0, 0.0), val + 10.0);

    gr_HLTCount->Draw("LP same");
    gr_CosmicCount->Draw("LP same");
    gr_CosmicPrescaledAwayCount->Draw("LP same");

    globalRatesLegend->Draw("same");
		
    interactiveGlobalRatesCanvas->Modified();
    interactiveGlobalRatesCanvas->Update();

    gSystem->ProcessEvents();

    sleep(1);
  }

  input->Close();
}

void WatchDQM() {

  cout << endl << endl << "Press enter to exit" << endl << endl;

  thread t(loop);

  cin.get();

  UserQuits.store(true);

  t.join();

  return;
}
