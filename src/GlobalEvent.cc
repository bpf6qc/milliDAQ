#include "GlobalEvent.h"

ClassImp(mdaq::GlobalEvent);

mdaq::GlobalEvent::GlobalEvent() : 
  TObject(),
  DAQEventNumber(0) {
  digitizers.resize(nDigitizers);
}

mdaq::GlobalEvent::~GlobalEvent() {
  digitizers.clear();
}

//////////////////////////////////////////////////
// Operator=
//////////////////////////////////////////////////

mdaq::GlobalEvent& mdaq::GlobalEvent::operator=(const mdaq::GlobalEvent& evt) {
  TObject::operator=(evt);

  DAQEventNumber = evt.DAQEventNumber;

  for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
    digitizers[iDigitizer] = evt.digitizers[iDigitizer];
  }

  return *this;
}

//////////////////////////////////////////////////
// User-level getters
//////////////////////////////////////////////////

// Declare a new histogram for a waveform
TH1D * mdaq::GlobalEvent::GetWaveform(const unsigned int boardIndex, const unsigned int ch, const TString name) const {
  return digitizers[boardIndex].GetWaveform(ch, name);
}

// Set an existing histogram to a waveform
void mdaq::GlobalEvent::GetWaveform(const unsigned int boardIndex, const unsigned int ch, TH1D *& histogram) const {
  histogram->Reset();
  return digitizers[boardIndex].GetWaveform(ch, histogram);
}

// Return 2-d vector of waveforms[iDigitizer][iChannel]
std::vector<std::vector<TH1D*> > mdaq::GlobalEvent::GetWaveforms(const TString name) const {
  std::vector<std::vector<TH1D*> > waves;
  for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
    waves.push_back(digitizers[iDigitizer].GetWaveforms(Form("board_%d_%s", iDigitizer, name.Data())));
  }
  return waves;
}

float mdaq::GlobalEvent::GetMinimumSample(const unsigned int boardIndex, const unsigned int ch) const {
  return digitizers[boardIndex].GetMinimumSample(ch);
}

float mdaq::GlobalEvent::GetMaximumSample(const unsigned int boardIndex, const unsigned int ch) const{
  return digitizers[boardIndex].GetMaximumSample(ch);
}

