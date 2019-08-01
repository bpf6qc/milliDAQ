#include "DQM.h"

mdaq::DQM::DQM(const std::string plotDest, mdaq::DemonstratorConfiguration * config, const bool isInteractive) :
  plotDir(plotDest),
  interactiveMode(isInteractive),
  cfg(config)
{

  //if(!interactiveMode) gROOT->SetBatch(true);

  // doesn't seem to work
  // disable messages like: Info in <TCanvas::Print>: pdf file /home/milliqan/MilliDAQ/plots/wave_channel_9.pdf has been created  
  gErrorIgnoreLevel = kWarning;

  gROOT->SetBatch(true);
  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(0);

  gr_ymin = -2.0;

  if(interactiveMode) {
    interactiveOutput = new TFile(plotDir + "interactiveDQM.root", "UPDATE");

    interactiveWavesCanvas = new TCanvas("interactiveWavesCanvas", "Amplitudes", 10, 10, 800, 600);
    interactiveWavesCanvas->Divide(2, 2, 0, 0);

    interactiveAmplitudesCanvas = new TCanvas("interactiveAmplitudesCanvas", "Waveforms", 10, 10, 800, 600);
    interactiveAmplitudesCanvas->Divide(2, 2, 0, 0);
    for(int i = 1; i <= 4; i++) {
      interactiveAmplitudesCanvas->cd(i);
      gPad->SetLogy(true);
    }

    interactiveTriggerRatesCanvas = new TCanvas("interactiveTriggerRatesCanvas", "Trigger Rates", 10, 10, 800, 600);
    interactiveTriggerRatesCanvas->Divide(2, 2, 0, 0);

    interactiveGlobalRatesCanvas = new TCanvas("interactiveGlobalRatesCanvas", "Global Rates", 10, 10, 800, 600);

    interactiveOutput->Write(0, TObject::kWriteDelete);
    interactiveOutput->SaveSelf();
  }

  singlePaneCanvas = new TCanvas("singlePaneCanvas", "MilliDAQ", 10, 10, 1600, 1000);

  for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
    for(int i = 0; i < nGroupsPerDigitizer; i++) {
      groupLabels[iDigitizer][i] = new TPaveText(0.4, 0.95, 0.6, 0.999, "NDC");
      groupLabels[iDigitizer][i]->SetBorderSize(1);
      groupLabels[iDigitizer][i]->AddText(Form("Group %d (board %d)", i, iDigitizer));
    }

    for(int i = 0; i < nChannelsPerDigitizer; i++) {
      channelLabels[iDigitizer][i] = new TPaveText(0.4, 0.95, 0.6, 0.999, "NDC");
      channelLabels[iDigitizer][i]->SetBorderSize(1);
      channelLabels[iDigitizer][i]->AddText(Form("Channel %d (board %d)", i, iDigitizer));
    }
  }

  dqmCanvas = new TCanvas("dqmCanvas", "MilliDAQ DQM", 10, 10, 600, 1000);
  padTop = new TPad("padTop", "padTop", 0, 0.66, 1, 1);
  padLeft = new TPad("padLeft", "padLeft", 0, 0.33, 0.5, 0.66);
  padRight = new TPad("padRight", "padRight", 0.5, 0.33, 1, 0.66);
  padBottom = new TPad("padBottom", "padBottom", 0, 0, 1, 0.33);

  padLeft->SetLogy(true);
  padRight->SetLogy(true);

  padTop->Draw();
  padLeft->Draw();
  padRight->Draw();
  padBottom->Draw();

  for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
    for(int i = 0; i < nChannelsPerDigitizer; i++) {
      TString channelName = Form("%d_%d", iDigitizer, i);

      h_waveforms[iDigitizer][i] = new TH1D("waveform_" + channelName, "waveform_" + channelName + ";Sample;[mV]", maxSamples, 0, maxSamples);
      //h_waveforms[iDigitizer][i]->SetLineColor((i % 2 == 0) ? kBlack : kRed); // colors[i]
      //h_waveforms[iDigitizer][i]->SetMarkerColor((i % 2 == 0) ? kBlack : kRed); // colors[i]
      h_waveforms[iDigitizer][i]->SetLineColor(colors[i]);
      h_waveforms[iDigitizer][i]->SetMarkerColor(colors[i]);

      h_amplitudes[iDigitizer][i] = new TH1D("amplitude_" + channelName, "amplitude_" + channelName + "Amplitude [mV];Events", 1310, -10, 1300);
      h_amplitudes[iDigitizer][i]->SetLineColor(colors[i]);
      h_amplitudes[iDigitizer][i]->SetMarkerColor(colors[i]);
    }
  }

  GlobalTriggerRate.Init("GlobalTriggerRate", kBlack);
  HLTRate.Init("HLTRate", kRed);
  CosmicRate.Init("CosmicRate", 8);
  CosmicPrescaledAwayRate.Init("CosmicPrescaledAwayRate", kBlue);

  MissedTriggerRates = new VRate();
  ChannelTriggerRates = new VRate();
  
  MissedTriggerRates->Init("MissedTriggerRate");
  ChannelTriggerRates->Init("ChannelTriggerRate");

  AverageQueueDepth = new Average();
  AverageQueueDepth->Init("AverageQueueDepth", kBlack, kRed);

  MatchingEfficiency = new Efficiency();
  MatchingEfficiency->Init("MatchingEfficiency", kBlack);

  for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
    for(int i = 0; i < nGroupsPerDigitizer / 2; i++) {
      legGroupPairLegends[iDigitizer][i] = CreateLegend();
      for(int j = 4*i; j < 4*(i+1); j++) legGroupPairLegends[iDigitizer][i]->AddEntry(MissedTriggerRates->rates[iDigitizer][j]->graph, Form("B. %d Ch. %d", iDigitizer, j), "LP");
    }
  }

  globalRatesLegend = CreateLegend();
  globalRatesLegend->AddEntry(GlobalTriggerRate.graph, "Global trigger rate", "LP");
  globalRatesLegend->AddEntry(HLTRate.graph, "HLT rate", "LP");
  globalRatesLegend->AddEntry(CosmicRate.graph, "Cosmic rate (unprescaled)", "LP");
  globalRatesLegend->AddEntry(CosmicPrescaledAwayRate.graph, "Cosmic rate (prescaled", "LP");

  averageQueueDepthLegend = CreateLegend();
  averageQueueDepthLegend->AddEntry(AverageQueueDepth->graph, "Average", "LP");
  averageQueueDepthLegend->AddEntry(AverageQueueDepth->maxGraph, "Maximum", "LP");

  initTime = std::chrono::system_clock::now();
  initTime_slowReset = initTime;
}

mdaq::DQM::~DQM() {
  for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
    for(auto &leg : legGroupPairLegends[iDigitizer]) delete leg;
    for(int i = 0; i < nChannelsPerDigitizer; i++) {
      if(h_waveforms[iDigitizer][i]) delete h_waveforms[iDigitizer][i];
      if(h_amplitudes[iDigitizer][i]) delete h_amplitudes[iDigitizer][i];
    }
  }

  delete padBottom;
  delete padRight;
  delete padLeft;
  delete padTop;
  delete dqmCanvas;
  delete singlePaneCanvas;

  delete AverageQueueDepth;
  delete MatchingEfficiency;

  delete MissedTriggerRates;
  delete ChannelTriggerRates;

  if(interactiveMode) {
    delete interactiveWavesCanvas;
    delete interactiveAmplitudesCanvas;
    delete interactiveTriggerRatesCanvas;
    delete interactiveGlobalRatesCanvas;

    interactiveOutput->Close();
    delete interactiveOutput;
  }
}

void mdaq::DQM::UpdateInteractiveCanvases() {
  if(!interactiveMode) return;

  Draw(h_waveforms, legGroupPairLegends, interactiveWavesCanvas);
  Draw(h_amplitudes, legGroupPairLegends, interactiveAmplitudesCanvas);

  Draw(ChannelTriggerRates, legGroupPairLegends, interactiveTriggerRatesCanvas);

  DrawGlobalRates();

  //gSystem->ProcessEvents();
}

void mdaq::DQM::UpdateDQM(mdaq::GlobalEvent * evt, unsigned int depth, bool firesHLT, bool prescaledAwayCosmic, bool firesCosmic) {

  ++GlobalTriggerRate;
  if(firesHLT) ++HLTRate;
  if(prescaledAwayCosmic) ++CosmicPrescaledAwayRate;
  if(firesCosmic) ++CosmicRate;

  for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
    for(int iChannel = 0; iChannel < nChannelsPerDigitizer; iChannel++) {
      MissedTriggerRates->Increment(iDigitizer, iChannel, evt->digitizers[iDigitizer].TriggerCount[iChannel]);

      // bfrancis -- we don't always use the falling edge, so add a check for that
      if(evt->GetMinimumSample(iDigitizer, iChannel) < cfg->digitizers[iDigitizer].channels[iChannel].triggerThreshold * 1000.0) {
        ChannelTriggerRates->Increment(iDigitizer, iChannel);
      }
    }
  }

  *AverageQueueDepth += depth;
}

void mdaq::DQM::UpdateWaveforms(mdaq::GlobalEvent * evt) {
  for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
    for(int iChannel = 0; iChannel < nChannelsPerDigitizer; iChannel++) {
      evt->GetWaveform(iDigitizer, iChannel, h_waveforms[iDigitizer][iChannel]);
      h_amplitudes[iDigitizer][iChannel]->Fill(-1.0 * evt->GetMinimumSample(iDigitizer, iChannel));
    }
  }
}

void mdaq::DQM::GraphAndResetCounters() {

  std::chrono::duration<float> deltaT = std::chrono::system_clock::now() - initTime;
  int nMillisecondsLive = std::chrono::duration_cast<std::chrono::milliseconds>(deltaT).count();

  if(nMillisecondsLive < 5000) return; // graph points every 5 seconds

  GlobalTriggerRate.AddPoint(nMillisecondsLive);
  HLTRate.AddPoint(nMillisecondsLive);
  CosmicRate.AddPoint(nMillisecondsLive);
  CosmicPrescaledAwayRate.AddPoint(nMillisecondsLive);

  MissedTriggerRates->AddPoint(nMillisecondsLive);
  ChannelTriggerRates->AddPoint(nMillisecondsLive);

  AverageQueueDepth->AddPoint(nMillisecondsLive);

  MatchingEfficiency->AddPoint(nMillisecondsLive);

  initTime = std::chrono::system_clock::now();
}

void mdaq::DQM::ResetPlots() {

  GlobalTriggerRate.ResetGraph();
  HLTRate.ResetGraph();
  CosmicRate.ResetGraph();
  CosmicPrescaledAwayRate.ResetGraph();

  MissedTriggerRates->ResetGraphs();
  ChannelTriggerRates->ResetGraphs();

  for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
    for(int i = 0; i < nChannelsPerDigitizer; i++) {
      h_waveforms[iDigitizer][i]->Reset();
      h_amplitudes[iDigitizer][i]->Reset();
    }
  }

  AverageQueueDepth->ResetGraphs();

  MatchingEfficiency->ResetGraphs();
}

void mdaq::DQM::SaveWaveformPDFs() {

  findRange(h_waveforms);

  singlePaneCanvas->cd();

  for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
    for(int i = 0; i < nChannelsPerDigitizer; i++) {
      h_waveforms[iDigitizer][i]->Draw("hist");
      channelLabels[iDigitizer][i]->Draw("same");
      singlePaneCanvas->SaveAs(plotDir + Form("wave_board%d_channel%d.pdf", iDigitizer, i));
    }
  }

  h_waveforms[0][0]->GetYaxis()->SetRangeUser(h_ymin, h_ymax);
  h_waveforms[0][0]->Draw("hist");

  for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
    for(int i = (iDigitizer == 0 ? 1 : 0); i < nChannelsPerDigitizer; i++) {
      h_waveforms[iDigitizer][i]->Draw("hist same");
      //groupLabels[iDigitizer][i]->Draw("same");
    }
  }
  singlePaneCanvas->SaveAs(plotDir + "allWaves.pdf");
}

void mdaq::DQM::SaveAmplitudePDFs() {

  findRange(h_amplitudes);

  singlePaneCanvas->cd();

  for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
    for(int i = 0; i < nChannelsPerDigitizer; i++) {
      h_amplitudes[iDigitizer][i]->Draw("hist");
      channelLabels[iDigitizer][i]->Draw("same");
      singlePaneCanvas->SaveAs(plotDir + Form("amplitudes_board%d_channel%d.pdf", iDigitizer, i));
    }
  }

  h_amplitudes[0][0]->GetYaxis()->SetRangeUser(h_ymin, h_ymax);
  h_amplitudes[0][0]->Draw("hist");

  for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
    for(int i = (iDigitizer == 0 ? 1 : 0); i < nChannelsPerDigitizer; i++) {
      h_amplitudes[iDigitizer][i]->Draw("hist same");
      //groupLabels[iDigitizer][i]->Draw("same");
    }
  }
  singlePaneCanvas->SaveAs(plotDir + "allAmplitudes.pdf");

}

void mdaq::DQM::SaveRatePDFs() {
  singlePaneCanvas->cd();

  // plot missed trigger rates, 4 channels at a time
  findRange(MissedTriggerRates);

  for(int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
    for(int i = 0; i < nChannelsPerDigitizer / graphsPerPlot; i++) {
      for(int j = 0; j < graphsPerPlot; j++) {
        if(j == 0) {
          MissedTriggerRates->rates[iDigitizer][graphsPerPlot*i + j]->graph->Draw("ALP");

          MissedTriggerRates->rates[iDigitizer][graphsPerPlot*i + j]->graph->GetXaxis()->SetRangeUser(gr_xmin, gr_xmax);
          MissedTriggerRates->rates[iDigitizer][graphsPerPlot*i + j]->graph->GetYaxis()->SetRangeUser(gr_ymin, gr_ymax);
        }
        else MissedTriggerRates->rates[iDigitizer][graphsPerPlot*i + j]->graph->Draw("LP same");
      }
      legGroupPairLegends[iDigitizer][i]->Draw("same");
      singlePaneCanvas->SaveAs(plotDir + "MissedTriggerRates_groups" + Form("%d-%d", i, i+1) + ".pdf");
    }
  }

  // plot channel trigger rates, 4 channels at a time
  findRange(ChannelTriggerRates);

  for(int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
    for(int i = 0; i < nChannelsPerDigitizer / graphsPerPlot; i++) {
      for(int j = 0; j < graphsPerPlot; j++) {
        if(j == 0) {
          ChannelTriggerRates->rates[iDigitizer][graphsPerPlot*i + j]->graph->Draw("ALP");
          ChannelTriggerRates->rates[iDigitizer][graphsPerPlot*i + j]->graph->GetXaxis()->SetRangeUser(gr_xmin, gr_xmax);
          ChannelTriggerRates->rates[iDigitizer][graphsPerPlot*i + j]->graph->GetYaxis()->SetRangeUser(gr_ymin, gr_ymax);
        }
        else ChannelTriggerRates->rates[iDigitizer][graphsPerPlot*i + j]->graph->Draw("LP same");
      }
      legGroupPairLegends[iDigitizer][i]->Draw("same");
      singlePaneCanvas->SaveAs(plotDir + "ChannelTriggerRates_groups" + Form("%d-%d", i, i+1) + ".pdf");
    }
  }

  DrawGlobalRates();

  DrawQueueDepth();

  DrawMatchingEfficiency();

}

void mdaq::DQM::SaveDQMPDFs() {

  // bfrancis -- intend for this to be a single unified canvas with all the relevent information for DQM

}

void mdaq::DQM::PrintRates() {

  std::chrono::duration<float> deltaT = std::chrono::system_clock::now() - initTime_slowReset;
  int nMillisecondsLive = std::chrono::duration_cast<std::chrono::milliseconds>(deltaT).count();

  if(nMillisecondsLive == 0) {
    mdaq::Logger::instance().warning("DQM", "Print rates requested too quickly (0 ms)!");
    return;
  }

  std::ostringstream oss;
  oss << "Since last update:" << std::endl
      << "\tTotal board triggers: " << GlobalTriggerRate.slowCount << " (" << (float)GlobalTriggerRate.slowCount / nMillisecondsLive * 1000 << "/s)" << std::endl
      << "\tHLT fires: " << HLTRate.slowCount << " (" << (float)HLTRate.slowCount / nMillisecondsLive * 1000 << "/s)" << std::endl
      << "\tCosmics: " << (CosmicRate.slowCount + CosmicPrescaledAwayRate.slowCount) << " (" << (float)(CosmicRate.slowCount + CosmicPrescaledAwayRate.slowCount) / nMillisecondsLive * 1000 << "/s)" << std::endl
      << "\tCosmics after prescale: " << CosmicRate.slowCount << " (" << (float)CosmicRate.slowCount / nMillisecondsLive * 1000 << "/s)"
      << std::endl;

/* bfrancis -- disable jun 28th 2018; very annoying, verbose output!
  for(int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
    for(int i = 0; i < nChannelsPerDigitizer; i++) {
      if(MissedTriggerRates->rates[iDigitizer][i]->slowCount > 0) {
        oss << A_YELLOW
            << "Missed triggers for channel " << i
            << ": " << MissedTriggerRates->rates[iDigitizer][i]->slowCount
            << "(" << (float)MissedTriggerRates->rates[iDigitizer][i]->slowCount / nMillisecondsLive * 1000 << "/s)"
            << A_RESET
            << std::endl;
      }
    }
  }
*/

  oss << "\tAverage queue depth: " << AverageQueueDepth->GetValue()
      << " (max: " << AverageQueueDepth->maxMeasurement << ")"
      << std::endl;

  oss << "\tMatching efficiency: " << MatchingEfficiency->GetValue() * 100.0 << " %" << std::endl;

  mdaq::Logger::instance().info("DQM", TString(oss.str()));

  GlobalTriggerRate.ResetSlow();
  HLTRate.ResetSlow();
  CosmicRate.ResetSlow();
  CosmicPrescaledAwayRate.ResetSlow();

  MissedTriggerRates->ResetSlow();
  ChannelTriggerRates->ResetSlow();

  AverageQueueDepth->ResetSlow();
  MatchingEfficiency->ResetSlow();

  initTime_slowReset = std::chrono::system_clock::now();
}

void mdaq::DQM::findRange(VRate * vr) {
  gr_ymax = 5.0;
  gr_xmin = 0.0;
  gr_xmax = 0.0;

  int n = vr->rates[0][0]->graph->GetN();

  if(n > 0) gr_xmax = vr->rates[0][0]->graph->GetX()[n - 1];

  if(gr_xmax > 290.0) gr_xmin = gr_xmax - 290.0;
  gr_xmax += 10.0;

  vr->rates[0][0]->graph->GetXaxis()->SetRangeUser(gr_xmin, gr_xmax);

  for(auto &v : vr->rates) {

    for(auto &r : v) {

      for(int i = n-1; i >= 0; i--) {

        if(r->graph->GetX()[i] < gr_xmin) {
	  break;
	}
        float val = r->graph->GetY()[i];
        if(val > gr_ymax) gr_ymax = val;
      }

    }
  }

  gr_ymax *= 1.1;

  for(auto &v : vr->rates) {
    for(auto &r : v) {

      if(!r->graph) {
	continue;
      }
      r->graph->Draw();

      if(r->graph->GetXaxis()) r->graph->GetXaxis()->SetRangeUser(gr_xmin, gr_xmax);

      if(r->graph->GetYaxis()) r->graph->GetYaxis()->SetRangeUser(gr_ymin, gr_ymax);

    }
  }

  return;
}

void mdaq::DQM::findRange(TH1D * histograms[nDigitizers][nChannelsPerDigitizer]) {
  h_ymin = -10.0;
  h_ymax = 10.0;

  for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
    for(int iChannel = 0; iChannel < nChannelsPerDigitizer; iChannel++) {
      double thisMin = histograms[iDigitizer][iChannel]->GetBinContent(histograms[iDigitizer][iChannel]->GetMinimumBin()) * 1.10;
      double thisMax = histograms[iDigitizer][iChannel]->GetBinContent(histograms[iDigitizer][iChannel]->GetMaximumBin()) * 1.10;
      if(thisMin < h_ymin) h_ymin = thisMin;
      if(thisMax > h_ymax) h_ymax = thisMax;
    }
  }

  return;
}

void mdaq::DQM::findRange(Average * avg) {
  gr_ymax = 10.0;
  gr_xmin = 0.0;
  gr_xmax = 0.0;

  int n = avg->maxGraph->GetN();
  if(n > 0) gr_xmax = avg->maxGraph->GetX()[n - 1];

  if(gr_xmax > 290.0) gr_xmin = gr_xmax - 290.0;
  gr_xmax += 10.0;

  for(int i = n-1; i >= 0; i--) {
    if(avg->maxGraph->GetX()[i] < gr_xmin) break;
    if(avg->maxGraph->GetY()[i] > gr_ymax) gr_ymax = avg->maxGraph->GetY()[i];
  }
}

void mdaq::DQM::findRange(Efficiency * eff) {
  gr_ymin = -5.0;
  gr_ymax = 110.0;
  gr_xmin = 0.0;
  gr_xmax = 0.0;

  int n = eff->graph->GetN();
  if(n > 0) gr_xmax = eff->graph->GetX()[n - 1];

  if(gr_xmax > 290.0) gr_xmin = gr_xmax - 290.0;
  gr_xmax += 10.0;
}

void mdaq::DQM::findRange(Rate r1, Rate r2, Rate r3, Rate r4) {
  gr_ymax = 10.0;
  gr_xmin = 0.0;
  gr_xmax = 0.0;

  int n = r1.graph->GetN();
  if(n > 0) gr_xmax = r1.graph->GetX()[n - 1];
  if(gr_xmax > 290.0) gr_xmin = gr_xmax - 290.0;
  gr_xmax += 10.0;

  for(int i = n-1; i >= 0; i--) {
    if(r1.graph->GetX()[i] < gr_xmin) break;

    if(r1.graph->GetY()[i] > gr_ymax) gr_ymax = r1.graph->GetY()[i];
    if(r2.graph->GetY()[i] > gr_ymax) gr_ymax = r2.graph->GetY()[i];
    if(r3.graph->GetY()[i] > gr_ymax) gr_ymax = r3.graph->GetY()[i];
    if(r4.graph->GetY()[i] > gr_ymax) gr_ymax = r4.graph->GetY()[i];
  }
}

void mdaq::DQM::Draw(TH1D * histograms[nDigitizers][nChannelsPerDigitizer], TLegend * legends[nDigitizers][nGroupsPerDigitizer / 2], TCanvas * canvas) {

  findRange(histograms);

  for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {

    for(int i = 0; i < nChannelsPerDigitizer / graphsPerPlot; i++) {
      interactiveWavesCanvas->cd(i+1);
      histograms[iDigitizer][graphsPerPlot*i]->GetYaxis()->SetRangeUser(h_ymin, h_ymax);

      if(iDigitizer == 0) {
        histograms[iDigitizer][graphsPerPlot*i]->Draw("hist");
        for(int j = 1; j < graphsPerPlot; j++) histograms[iDigitizer][graphsPerPlot*i + j]->Draw("hist same");
      }
      else {
        for(int j = 0; j < graphsPerPlot; j++) histograms[iDigitizer][graphsPerPlot*i + j]->Draw("hist same");
      }

      legends[iDigitizer][i]->Draw("same");
    }

  }

  if(canvas) {
    interactiveOutput->cd();
    canvas->Write(0, TObject::kWriteDelete);
    for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
      for(int i = 0; i < nChannelsPerDigitizer / graphsPerPlot; i++) {
        for(int j = 0; j < graphsPerPlot; j++) histograms[iDigitizer][graphsPerPlot*i + j]->Write(0, TObject::kWriteDelete);
      }
    }
    interactiveOutput->SaveSelf();
  }

  return;
}

void mdaq::DQM::Draw(VRate * vr, TLegend * legends[nDigitizers][nGroupsPerDigitizer / 2], TCanvas * canvas) {

  findRange(vr);

  for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {

    for(int i = 0; i < nChannelsPerDigitizer / graphsPerPlot; i++) {
      canvas->cd(i+1);
      vr->rates[iDigitizer][graphsPerPlot*i]->graph->Draw("ALP");

      for(int j = 1; j < graphsPerPlot; j++) vr->rates[iDigitizer][graphsPerPlot*i + j]->graph->Draw("LP same");
      legends[iDigitizer][i]->Draw("same");
    }

  }

  interactiveOutput->cd();
  canvas->Write(0, TObject::kWriteDelete);
  for(auto &v : vr->rates) {
    for(auto &r : v) {
      r->graph->Write(0, TObject::kWriteDelete);
    }
  }
  interactiveOutput->SaveSelf();

}

void mdaq::DQM::DrawGlobalRates() {

  gr_ymax = 5.0;
  gr_xmin = 0.0;
  gr_xmax = 0.0;

  int n = GlobalTriggerRate.graph->GetN();
  if(n > 0) gr_xmax = GlobalTriggerRate.graph->GetX()[n - 1];
  if(gr_xmax > 290.0) gr_xmin = gr_xmax - 290.0;
  gr_xmax += 10.0;

  if(n == 0) gr_ymax = 10.0;
  else {

    for(int i = n-1; i >= 0; i--) {

      if(GlobalTriggerRate.graph->GetX()[i] < gr_xmin) break;

      if(GlobalTriggerRate.graph->GetY()[i] > gr_ymax) {
	gr_ymax = GlobalTriggerRate.graph->GetY()[i];
      }

      if(HLTRate.graph->GetY()[i] > gr_ymax) {
	gr_ymax = HLTRate.graph->GetY()[i];
      }

      if(CosmicRate.graph->GetY()[i] > gr_ymax) {
	gr_ymax = CosmicRate.graph->GetY()[i];
      }

      if(CosmicPrescaledAwayRate.graph->GetY()[i] > gr_ymax) {
	gr_ymax = CosmicPrescaledAwayRate.graph->GetY()[i];
      }

    } // reverse loop over graph points

  } // n != 0

  singlePaneCanvas->cd();

  GlobalTriggerRate.graph->Draw("ALP");
  GlobalTriggerRate.graph->GetYaxis()->SetTitle("[Hz]");
  GlobalTriggerRate.graph->GetYaxis()->SetRangeUser(-1, 1.1 * gr_ymax);
  GlobalTriggerRate.graph->GetXaxis()->SetTitle("Time [s]");
  GlobalTriggerRate.graph->GetXaxis()->SetRangeUser(gr_xmin, gr_ymin);

  HLTRate.graph->Draw("LP same");
  CosmicRate.graph->Draw("LP same");
  CosmicPrescaledAwayRate.graph->Draw("LP same");

  globalRatesLegend->Draw("same");

  singlePaneCanvas->SaveAs(plotDir + "GlobalRates.pdf");

}

void mdaq::DQM::DrawQueueDepth() {

  findRange(AverageQueueDepth);

  singlePaneCanvas->cd();

  AverageQueueDepth->graph->Draw("ALP");
  AverageQueueDepth->graph->GetYaxis()->SetTitle("Depth [Events]");
  AverageQueueDepth->graph->GetYaxis()->SetRangeUser(gr_ymin, gr_ymax);
  AverageQueueDepth->graph->GetXaxis()->SetTitle("Time [s]");
  AverageQueueDepth->graph->GetXaxis()->SetRangeUser(gr_xmin, gr_xmax);
  AverageQueueDepth->maxGraph->Draw("LP same");
  averageQueueDepthLegend->Draw("same");

  singlePaneCanvas->SaveAs(plotDir + "QueueUsage.pdf");

}

void mdaq::DQM::DrawMatchingEfficiency() {
  findRange(MatchingEfficiency);

  singlePaneCanvas->cd();

  MatchingEfficiency->graph->Draw("ALP");
  MatchingEfficiency->graph->GetYaxis()->SetTitle("Efficiency [%]");
  MatchingEfficiency->graph->GetYaxis()->SetRangeUser(gr_ymin, gr_ymax);
  MatchingEfficiency->graph->GetXaxis()->SetTitle("Time [s]");
  MatchingEfficiency->graph->GetXaxis()->SetRangeUser(gr_xmin, gr_xmax);

  singlePaneCanvas->SaveAs(plotDir + "MatchingEfficiency.pdf");
}
