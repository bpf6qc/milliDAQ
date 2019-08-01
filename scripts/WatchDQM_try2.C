#include <thread>

using namespace std;

atomic<bool> UserQuits(false);

void loop() {

  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(0);

  TFile * input;

  TCanvas * interactiveWavesCanvas;
  TCanvas * interactiveAmplitudesCanvas;
  TCanvas * interactiveTriggerRatesCanvas;
  TCanvas * interactiveGlobalRatesCanvas;

  while(!UserQuits.load()) {

    input = new TFile("/home/milliqan/MilliDAQ/plots/interactiveDQM.root", "READ");

    interactiveWavesCanvas = (TCanvas*)input->Get("interactiveWavesCanvas");
    interactiveAmplitudesCanvas = (TCanvas*)input->Get("interactiveAmplitudesCanvas");
    interactiveTriggerRatesCanvas = (TCanvas*)input->Get("interactiveTriggerRatesCanvas");
    interactiveGlobalRatesCanvas = (TCanvas*)input->Get("interactiveGlobalRatesCanvas");

    interactiveWavesCanvas->Draw();
    interactiveAmplitudesCanvas->Draw();
    interactiveTriggerRatesCanvas->Draw();
    interactiveGlobalRatesCanvas->Draw();

    sleep(1);
  }

  input->Close();
}

void WatchDQM_try2() {

  cout << endl << endl << "Press enter to exit" << endl << endl;

  thread t(loop);

  cin.get();

  UserQuits.store(true);

  t.join();

  return;
}
