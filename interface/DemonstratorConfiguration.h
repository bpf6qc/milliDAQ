#ifndef DEMONSTRATOR_CONFIGURATION_H
#define DEMONSTRATOR_CONFIGURATION_H

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>

#include "Logger.h"
#include "V1743Configuration.h"

#include "TObject.h"

#define nHVChannels 13

typedef struct HVConfiguration {
  float voltages[nHVChannels];
  bool enabled[nHVChannels];
  std::string names[nHVChannels];
} HVConfiguration;

namespace mdaq {

  class DemonstratorConfiguration : public TObject {
  public:
    DemonstratorConfiguration();
    DemonstratorConfiguration(const DemonstratorConfiguration& cfg);
    virtual ~DemonstratorConfiguration();

    DemonstratorConfiguration& operator=(DemonstratorConfiguration& cfg);

    void PrintConfiguration() const;
    void LogConfiguration() const;

    bool ReadPython(std::string filename);
    bool ReadXML(std::string filename);
    //bool ReadROOT(std::string filename); // bfrancis -- implement

    std::string GetCurrentConfigFile() { return currentConfigFile; };
    void SetCurrentConfigFile(std::string filename) { currentConfigFile = filename; };

    const unsigned int nDigitizersEnabled() const;

    // software-only parameters
    bool TFileCompressionEnabled;

    // High level trigger -- if enabled, then to actually write an event to disk
    // it needs to have >= HLTNumRequired channels/layers in a row with amplitude
    // <= HLTMaxAmplitude (volts)
    bool HLTEnabled;
    float HLTMaxAmplitude;
    unsigned int HLTNumRequired;

    // Cosmic prescale -- if enabled, only 1 out of CosmicPrescalePS events having a cosmic
    // will be written to disk. To have a cosmic it needs to have >= CosmicPrescaleNumRequired
    // channels in a vertical row with amplitude <= CosmicPrescaleMaxAmplitude (volts)
    bool CosmicPrescaleEnabled;
    float CosmicPrescaleMaxAmplitude;
    unsigned int CosmicPrescaleNumRequired;
    unsigned int CosmicPrescalePS;

    std::vector<V1743Configuration> digitizers;

    HVConfiguration hvConfig;

  private:
    std::string currentConfigFile;

  public:
    ClassDef(mdaq::DemonstratorConfiguration, 1)
      };

}

#endif
