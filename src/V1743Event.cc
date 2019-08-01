#include "V1743Event.h"

ClassImp(mdaq::V1743Event);

mdaq::V1743Event& mdaq::V1743Event::operator=(const mdaq::V1743Event& evt) {
  TObject::operator=(evt);

  // loop over channels
  for(unsigned int iChannel = 0; iChannel < nChannelsPerDigitizer; iChannel++) {
    TriggerCount[iChannel] = evt.TriggerCount[iChannel];
    TimeCount[iChannel]    = evt.TimeCount[iChannel];

    for(unsigned int iSample = 0; iSample < maxSamples; iSample++) {
      waveform[iChannel][iSample] = evt.waveform[iChannel][iSample];
    }
  }

  // members by group
  for(unsigned int iGroup = 0; iGroup < nGroupsPerDigitizer; iGroup++) {
    EventId[iGroup] = evt.EventId[iGroup];
    TDC[iGroup]     = evt.TDC[iGroup];

#ifdef SAVE_OPTIONAL_VALUES
    PosEdgeTimeStamp[iGroup] = evt.PosEdgeTimeStamp[iGroup];
    NegEdgeTimeStamp[iGroup] = evt.NegEdgeTimeStamp[iGroup];
    PeakIndex[iGroup]        = evt.PeakIndex[iGroup];
    Peak[iGroup]             = evt.Peak[iGroup];
    Baseline[iGroup]         = evt.Baseline[iGroup];

    for(unsigned int iChargeEvent = 0; iChargeEvent < nEventsChargeMode; iChargeEvent++) {
      StartIndexCell[iGroup][iChargeEvent] = evt.StartIndexCell[iGroup][iChargeEvent];
      Charge[iGroup][iChargeEvent]         = evt.Charge[iGroup][iChargeEvent];
    }
#endif
  }

  // EventInfo

  EventSize      = evt.EventSize;
  BoardId        = evt.BoardId;
  Pattern        = evt.Pattern;
  ChannelMask    = evt.ChannelMask;
  EventCounter   = evt.EventCounter;
  TriggerTimeTag = evt.TriggerTimeTag;

  TriggerTimeTagRollovers = evt.TriggerTimeTagRollovers;

  DataPresent = evt.DataPresent;

  tdcClockTick = evt.tdcClockTick;
  nanosecondsPerSample = evt.nanosecondsPerSample;

  return *this;
}

mdaq::V1743Event& mdaq::V1743Event::operator=(const mdaq::V1743Event * evt) {
  TObject::operator=(*evt);

  // loop over channels
  for(unsigned int iChannel = 0; iChannel < nChannelsPerDigitizer; iChannel++) {
    TriggerCount[iChannel] = evt->TriggerCount[iChannel];
    TimeCount[iChannel]    = evt->TimeCount[iChannel];

    for(unsigned int iSample = 0; iSample < maxSamples; iSample++) {
      waveform[iChannel][iSample] = evt->waveform[iChannel][iSample];
    }
  }

  // members by group
  for(unsigned int iGroup = 0; iGroup < nGroupsPerDigitizer; iGroup++) {
    EventId[iGroup] = evt->EventId[iGroup];
    TDC[iGroup]     = evt->TDC[iGroup];

#ifdef SAVE_OPTIONAL_VALUES
    PosEdgeTimeStamp[iGroup] = evt->PosEdgeTimeStamp[iGroup];
    NegEdgeTimeStamp[iGroup] = evt->NegEdgeTimeStamp[iGroup];
    PeakIndex[iGroup]        = evt->PeakIndex[iGroup];
    Peak[iGroup]             = evt->Peak[iGroup];
    Baseline[iGroup]         = evt->Baseline[iGroup];

    for(unsigned int iChargeEvent = 0; iChargeEvent < nEventsChargeMode; iChargeEvent++) {
      StartIndexCell[iGroup][iChargeEvent] = evt->StartIndexCell[iGroup][iChargeEvent];
      Charge[iGroup][iChargeEvent]         = evt->Charge[iGroup][iChargeEvent];
    }
#endif
  }

  // EventInfo

  EventSize      = evt->EventSize;
  BoardId        = evt->BoardId;
  Pattern        = evt->Pattern;
  ChannelMask    = evt->ChannelMask;
  EventCounter   = evt->EventCounter;
  TriggerTimeTag = evt->TriggerTimeTag;

  TriggerTimeTagRollovers = evt->TriggerTimeTagRollovers;

  DataPresent = evt->DataPresent;

  tdcClockTick = evt->tdcClockTick;
  nanosecondsPerSample = evt->nanosecondsPerSample;

  return *this;
}

void mdaq::V1743Event::Reset() {
  // loop over channels
  for(unsigned int iChannel = 0; iChannel < nChannelsPerDigitizer; iChannel++) {
    TriggerCount[iChannel] = 0;
    TimeCount[iChannel]    = 0;

    for(unsigned int iSample = 0; iSample < maxSamples; iSample++) {
      waveform[iChannel][iSample] = 0;
    }
  }

  // members by group
  for(unsigned int iGroup = 0; iGroup < nGroupsPerDigitizer; iGroup++) {
    EventId[iGroup] = 0;
    TDC[iGroup]     = 0;

#ifdef SAVE_OPTIONAL_VALUES
    PosEdgeTimeStamp[iGroup] = 0;
    NegEdgeTimeStamp[iGroup] = 0;
    PeakIndex[iGroup]        = 0;
    Peak[iGroup]             = 0;
    Baseline[iGroup]         = 0;

    for(unsigned int iChargeEvent = 0; iChargeEvent < nEventsChargeMode; iChargeEvent++) {
      StartIndexCell[iGroup][iChargeEvent] = 0;
      Charge[iGroup][iChargeEvent]         = 0;
    }
#endif
  }

  // EventInfo

  EventSize      = 0;
  BoardId        = 0;
  Pattern        = 0;
  ChannelMask    = 0;
  EventCounter   = 0;
  TriggerTimeTag = 0;

  TriggerTimeTagRollovers = 0;

  DataPresent = false;
}

void mdaq::V1743Event::SetTimestamp(const std::chrono::system_clock::time_point tp) {
  auto nsecs = std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch());

  DAQTimeStamp = std::chrono::system_clock::to_time_t(tp);
  DAQTimeStamp.SetNanoSec(nsecs.count() % (int)1e9);
}

const std::chrono::system_clock::time_point mdaq::V1743Event::GetTimestampAsChrono() const {
  std::chrono::system_clock::time_point tp = std::chrono::system_clock::from_time_t(DAQTimeStamp.GetSec());
  tp += std::chrono::nanoseconds(DAQTimeStamp.GetNanoSec());
  return tp;
}

// Declare a new histogram for waveform[ch] with name "name"; x-axis is in units of [ns]
TH1D * mdaq::V1743Event::GetWaveform(const unsigned int ch, const TString name) const {
  TH1D * wave = new TH1D(name, name, maxSamples, 0, maxSamples * nanosecondsPerSample);

  if(ch >= nChannelsPerDigitizer) return wave;

  for(unsigned int i = 0; i < maxSamples; i++) {
    wave->SetBinContent(i+1, waveform[ch][i]);
  }
  
  return wave;
}

// Set an existing histogram to waveform[ch]
void mdaq::V1743Event::GetWaveform(const unsigned int ch, TH1D *& histogram) const {
  if(ch >= nChannelsPerDigitizer) return;

  histogram->Reset();
  for(unsigned int i = 0; i < maxSamples; i++) {
    histogram->SetBinContent(i+1, waveform[ch][i]);
  }
}

// Return vector of waveform[iChannel] with names "name_<channel number>"
std::vector<TH1D*> mdaq::V1743Event::GetWaveforms(const TString name) const {
  std::vector<TH1D*> waves;

  for(unsigned int iChannel = 0; iChannel < nChannelsPerDigitizer; iChannel++) {
    waves.push_back(GetWaveform(iChannel, name + Form("_%d", iChannel)));
  }

  return waves;
}

float mdaq::V1743Event::GetMinimumSample(const unsigned int ch) const {
  float minSample = 1.e6;
  if(ch >= nChannelsPerDigitizer) return minSample;

  for(auto sample : waveform[ch]) {
    if(sample < minSample) minSample = sample;
  }

  return minSample;
}

float mdaq::V1743Event::GetMaximumSample(const unsigned int ch) const {
  float maxSample = -1.e6;
  if(ch >= nChannelsPerDigitizer) return maxSample;

  for(auto sample : waveform[ch]) {
    if(sample > maxSample) maxSample = sample;
  }

  return maxSample;
}

