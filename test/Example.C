void Example() {

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

  mdaq::V1743Configuration * cfg = new mdaq::V1743Configuration();
  TTree * metadata = (TTree*)input->Get("Metadata");
  metadata->SetBranchAddress("configuration", &cfg);
  metadata->GetEntry(0);
  cfg->PrintConfiguration();

  mdaq::Event * evt = new mdaq::Event();
  TTree * events = (TTree*)input->Get("Events");
  events->SetBranchAddress("event", &evt);
  events->GetEntry(0);

  TCanvas * canWaves = new TCanvas("waves", "waves", 10, 10, 800, 600);

  TH1D * waveforms[16];
  for(int ch = 0; ch < 16; ch++) {
    waveforms[ch] = (TH1D*)evt->GetWaveform(ch, Form("wave_ch%d", ch));
    waveforms[ch]->SetLineColor(colors[ch]);
  }

  waveforms[0]->GetXaxis()->SetTitle("Sample number");
  waveforms[0]->GetYaxis()->SetTitle("Sample [mV]");
  waveforms[0]->GetYaxis()->SetRangeUser(-75.0, 15.0);
  waveforms[0]->Draw();
  for(int ch = 1; ch < 16; ch++) waveforms[ch]->Draw("same");

  TCanvas * canAmplitudes = new TCanvas("amplitudes", "amplitudes", 10, 10, 800, 600);
  canAmplitudes->SetLogy(true);

  TH1D * amplitudes[16];
  for(int ch = 0; ch < 16; ch++) {
    amplitudes[ch] = new TH1D(Form("amplitude_ch%d", ch), Form("amplitude_ch%d", ch), 100, 0, 1250);
    amplitudes[ch]->SetLineColor(colors[ch]);
  }

  for(int i = 0; i < events->GetEntries(); i++) {
    events->GetEntry(i);

    for(int ch = 0; ch < 16; ch++) amplitudes[ch]->Fill(-1.0 * evt->GetMinimumSample(ch));
  }

  amplitudes[0]->GetXaxis()->SetTitle("Amplitude [mV]");
  amplitudes[0]->GetYaxis()->SetRangeUser(0.5, 3.e3);
  amplitudes[0]->Draw();
  for(int ch = 1; ch < 16; ch++) amplitudes[ch]->Draw("same");

}
