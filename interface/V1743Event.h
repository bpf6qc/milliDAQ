#ifndef V1743_EVENT_H
#define V1743_EVENT_H

#include <vector>
#include <chrono>
#include <ctime>
#include <string>

#include "DemonstratorConstants.h"

#include "TH1.h"
#include "TObject.h"
#include "TString.h"
#include "TTimeStamp.h"

// Some values in the CAEN event are only relevant for different
// firmware modes that we won't be using. 
// Comment this line out to save a small amount of disk space
// N.B.: Additionally this is disabled because the structure of charge mode events is different -- one acquisition is 256 events
//       Having multiple tree fill actions per acquisiton isn't handled right now; to be added
//#define SAVE_OPTIONAL_VALUES

namespace mdaq {

  class V1743Event : public TObject {
  public:
  V1743Event() : TObject() {};
    virtual ~V1743Event() {};

    V1743Event& operator=(const V1743Event& evt);
    V1743Event& operator=(const V1743Event * evt);

    void Reset();

    void SetTimestamp(const std::chrono::system_clock::time_point tp);
    const std::chrono::system_clock::time_point GetTimestampAsChrono() const;

    const double GetTTTNanoseconds() const {
      // 32-bit counter where the bit [31] is a rollover flag
      return ((TriggerTimeTagRollovers << 31) + (TriggerTimeTag & 0x7FFFFFFF)) * triggerClockTick;
    };

    const double GetTDCNanoseconds(const unsigned int group) const {
      // 40-bit counter
      if(group >= nGroupsPerDigitizer) return -1.0;
      return ((TDCRollovers[group] << 40) + (TDC[group] & 0xFFFFFFFFFF)) * tdcClockTick;
    };

    //////////////////////////////////////////////////
    // User-level getters
    //////////////////////////////////////////////////

    // Declare a new histogram for waveform[ch] with name "name"
    TH1D * GetWaveform(const unsigned int ch, const TString name) const;

    // Set an existing histogram to waveform[ch]
    void GetWaveform(const unsigned int ch, TH1D *& histogram) const;

    // Return vector of waveform[iChannel] with names "name_<channel number>"
    std::vector<TH1D*> GetWaveforms(const TString name) const;

    // Get the min/max of waveform[ch]
    float GetMinimumSample(const unsigned int ch) const;
    float GetMaximumSample(const unsigned int ch) const;

    //////////////////////////////////////////////////
    // Class members
    //////////////////////////////////////////////////

    // TO DO: make these things all protected again and provide getters
    // like TimeCount(channel) = TimeCount[8][2] 2d to 1d things

    // software-derived members
    TTimeStamp DAQTimeStamp;

    // hit rate monitor counts/times
    uint16_t TriggerCount[nChannelsPerDigitizer];
    uint16_t TimeCount[nChannelsPerDigitizer];

    // an event id (?)
    uint8_t  EventId[nGroupsPerDigitizer];

    // the trigger TDC
    uint64_t TDC[nGroupsPerDigitizer];

#ifdef SAVE_OPTIONAL_VALUES
    // These values are only calculated by "DPP" or digital pulse processing
    // firmware, which we are not using; these are always zero.
    float    PosEdgeTimeStamp[nGroupsPerDigitizer];
    float    NegEdgeTimeStamp[nGroupsPerDigitizer];
    uint16_t PeakIndex[nGroupsPerDigitizer];
    float    Peak[nGroupsPerDigitizer];
    float    Baseline[nGroupsPerDigitizer];

    // These are only relevent for charge mode (which we won't use)
    // Really there should be a second "ChargeModeEvent" class that uses
    // exclusively these values
    uint16_t StartIndexCell[nChannelsPerDigitizer][nEventsChargeMode];
    float    Charge[nChannelsPerDigitizer][nEventsChargeMode];
#endif

    // EventInfo members
    uint32_t EventSize;
    uint32_t BoardId;
    uint32_t Pattern;
    uint32_t ChannelMask;
    uint32_t EventCounter;
    uint32_t TriggerTimeTag;

    uint64_t TriggerTimeTagRollovers;
    uint64_t TDCRollovers[nGroupsPerDigitizer];

    bool DataPresent;

    // the digitized waveforms [channel][sample]
    float waveform[nChannelsPerDigitizer][maxSamples];

    const double triggerClockTick = 10.0; // [ns]
    double tdcClockTick; // [ns]

    float nanosecondsPerSample;

    //////////////////////////////////////////////////

    ClassDef(mdaq::V1743Event, 2)
      };

}

#endif

