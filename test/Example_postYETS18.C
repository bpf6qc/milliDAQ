void Example_postYETS18() {

  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(0);

  int colors[16] = {kBlack, kRed,
                    kGreen+2, kBlue,
                    kOrange-3, kPink-8,
                    kAzure+7, kTeal+5,
                    kGray+2, kYellow+3,
                    kMagenta+1, kSpring+7,
                    kPink+2, kOrange+3,
                    kBlue-7, kRed-6};

  //gSystem->Load("../libMilliDAQ.so"); // N.B. now in ./.rootlogon.C

  TFile * input = new TFile("MilliQan_Run42.1_TripleCoincidence.root", "READ");

  mdaq::DemonstratorConfiguration * cfg = new mdaq::DemonstratorConfiguration();
  TTree * metadata = (TTree*)input->Get("Metadata");
  metadata->SetBranchAddress("configuration", &cfg);
  metadata->GetEntry(0);
  cfg->PrintConfiguration();

  mdaq::GlobalEvent * evt = new mdaq::GlobalEvent();
  TTree * events = (TTree*)input->Get("Events");
  events->SetBranchAddress("event", &evt);
  events->GetEntry(0);

  TCanvas * canWaves0 = new TCanvas("waves0", "waves0", 10, 10, 800, 600);

  TH1D * waveforms_board0[16];
  TH1D * waveforms_board1[16];
  for(int ch = 0; ch < 16; ch++) {
    waveforms_board0[ch] = (TH1D*)evt->GetWaveform(0, ch, Form("wave_b0_ch%d", ch));
    waveforms_board0[ch]->SetLineColor(colors[ch]);

    waveforms_board1[ch] = (TH1D*)evt->GetWaveform(1, ch, Form("wave_b1_ch%d", ch));
    waveforms_board1[ch]->SetLineColor(colors[ch]);
  }

  waveforms_board0[0]->GetXaxis()->SetTitle("Sample number");
  waveforms_board0[0]->GetYaxis()->SetTitle("Sample [mV]");
  waveforms_board0[0]->GetYaxis()->SetRangeUser(-75.0, 15.0);
  waveforms_board0[0]->Draw();
  for(int ch = 1; ch < 16; ch++) waveforms_board0[ch]->Draw("same");

  TCanvas * canWaves1 = new TCanvas("waves1", "waves1", 10, 10, 800, 600);

  waveforms_board1[0]->GetXaxis()->SetTitle("Sample number");
  waveforms_board1[0]->GetYaxis()->SetTitle("Sample [mV]");
  waveforms_board1[0]->GetYaxis()->SetRangeUser(-75.0, 15.0);
  waveforms_board1[0]->Draw();
  for(int ch = 1; ch < 16; ch++) waveforms_board1[ch]->Draw("same");

  TH1D * amplitudes_board0[16];
  TH1D * amplitudes_board1[16];

  for(int ch = 0; ch < 16; ch++) {
    amplitudes_board0[ch] = new TH1D(Form("amplitude_b0_ch%d", ch), Form("amplitude_b0_ch%d", ch), 100, 0, 1250);
    amplitudes_board0[ch]->SetLineColor(colors[ch]);

    amplitudes_board1[ch] = new TH1D(Form("amplitude_b1_ch%d", ch), Form("amplitude_b1_ch%d", ch), 100, 0, 1250);
    amplitudes_board1[ch]->SetLineColor(colors[ch]);
  }

  for(int i = 0; i < events->GetEntries(); i++) {
    events->GetEntry(i);

    for(int ch = 0; ch < 16; ch++) {
      amplitudes_board0[ch]->Fill(-1.0 * evt->GetMinimumSample(0, ch));
      amplitudes_board1[ch]->Fill(-1.0 * evt->GetMinimumSample(1, ch));
    }
  }

  TCanvas * canAmplitudes0 = new TCanvas("amplitudes", "amplitudes", 10, 10, 800, 600);
  canAmplitudes0->SetLogy(true);

  amplitudes_board0[0]->GetXaxis()->SetTitle("Amplitude [mV]");
  amplitudes_board0[0]->GetYaxis()->SetRangeUser(0.5, 3.e3);
  amplitudes_board0[0]->Draw();
  for(int ch = 1; ch < 16; ch++) amplitudes_board0[ch]->Draw("same");

  TCanvas * canAmplitudes1 = new TCanvas("amplitudes", "amplitudes", 10, 10, 800, 600);
  canAmplitudes1->SetLogy(true);

  amplitudes_board1[0]->GetXaxis()->SetTitle("Amplitude [mV]");
  amplitudes_board1[0]->GetYaxis()->SetRangeUser(0.5, 3.e3);
  amplitudes_board1[0]->Draw();
  for(int ch = 1; ch < 16; ch++) amplitudes_board1[ch]->Draw("same");

}
