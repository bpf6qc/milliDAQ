#ifndef GLOBAL_EVENT_H
#define GLOBAL_EVENT_H

#include <vector>
#include <chrono>
#include <ctime>
#include <string>

#include "DemonstratorConstants.h"
#include "V1743Event.h"

#include "TH1.h"
#include "TObject.h"
#include "TString.h"
#include "TTimeStamp.h"

namespace mdaq {

  class GlobalEvent : public TObject {
  public:
    GlobalEvent();
    virtual ~GlobalEvent();

    GlobalEvent& operator=(const GlobalEvent& evt);

    void Reset() {
      for(auto &dgtz : digitizers) dgtz.Reset();
    }

    //////////////////////////////////////////////////
    // DAQ setters
    //////////////////////////////////////////////////

    void SetEventNumber(const uint32_t evtNum) { DAQEventNumber = evtNum; };

    //////////////////////////////////////////////////
    // User-level getters
    //////////////////////////////////////////////////

    // Declare a new histogram for a waveform
    TH1D * GetWaveform(const unsigned int boardIndex, const unsigned int ch, const TString name) const;

    // Set an existing histogram to a waveform
    void GetWaveform(const unsigned int boardIndex, const unsigned int ch, TH1D *& histogram) const;

    // Return 2-d vector of waveforms[iDigitizer][iChannel]
    std::vector<std::vector<TH1D*> > GetWaveforms(const TString name) const;

    // Get the min/max of waveforms[boardIndex][ch]
    float GetMinimumSample(const unsigned int boardIndex, const unsigned int ch) const;
    float GetMaximumSample(const unsigned int boardIndex, const unsigned int ch) const;

    //////////////////////////////////////////////////
    // Class members
    //////////////////////////////////////////////////

    // software-derived members
    uint32_t DAQEventNumber;

    std::vector<V1743Event> digitizers;

    //////////////////////////////////////////////////

    ClassDef(mdaq::GlobalEvent, 1)
      };

}

#endif

