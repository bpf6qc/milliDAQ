#include "DemonstratorConfiguration.h"

#include "ConfigurationReader.h"

using namespace std;

ClassImp(mdaq::DemonstratorConfiguration);

mdaq::DemonstratorConfiguration::DemonstratorConfiguration() : 
  TObject(),
  TFileCompressionEnabled(false),
  HLTEnabled(false),
  HLTMaxAmplitude(0.0),
  HLTNumRequired(1),
  CosmicPrescaleEnabled(false),
  CosmicPrescaleMaxAmplitude(0),
  CosmicPrescaleNumRequired(1),
  CosmicPrescalePS(1),
  currentConfigFile("")
{
  digitizers.resize(nDigitizers);

  for(unsigned int i = 0; i < nHVChannels; i++) {
    hvConfig.voltages[i] = 0.0;
    hvConfig.enabled[i] = false;
  }

  hvConfig.names[0] = "878box2";
  hvConfig.names[1] = "L3ML_7725";
  hvConfig.names[2] = "L3BR_7725";
  hvConfig.names[3] = "ETbox";
  hvConfig.names[4] = "878box1";
  hvConfig.names[5] = "L3TL_878";
  hvConfig.names[6] = "Slab0_878";
  hvConfig.names[7] = "ShL1L_878";
  hvConfig.names[8] = "ShL1R_878";
  hvConfig.names[9] = "ShL1T_878";
  hvConfig.names[10] = "ShL2L_878";
  hvConfig.names[11] = "ShL2T_878";
  hvConfig.names[12] = "ShL3L_878";
}

mdaq::DemonstratorConfiguration::DemonstratorConfiguration(const mdaq::DemonstratorConfiguration& cfg) : TObject(cfg) {

  TFileCompressionEnabled = cfg.TFileCompressionEnabled;

  HLTEnabled = cfg.HLTEnabled;
  HLTMaxAmplitude = cfg.HLTMaxAmplitude;
  HLTNumRequired = cfg.HLTNumRequired;

  CosmicPrescaleEnabled = cfg.CosmicPrescaleEnabled;
  CosmicPrescaleMaxAmplitude = cfg.CosmicPrescaleMaxAmplitude;
  CosmicPrescaleNumRequired = cfg.CosmicPrescaleNumRequired;
  CosmicPrescalePS = cfg.CosmicPrescalePS;

  digitizers.resize(cfg.digitizers.size());

  for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
    digitizers[iDigitizer].boardIndex  = cfg.digitizers[iDigitizer].boardIndex;
    digitizers[iDigitizer].boardEnable = cfg.digitizers[iDigitizer].boardEnable;

    digitizers[iDigitizer].LinkType       = cfg.digitizers[iDigitizer].LinkType;
    digitizers[iDigitizer].LinkNum        = cfg.digitizers[iDigitizer].LinkNum;
    digitizers[iDigitizer].ConetNode      = cfg.digitizers[iDigitizer].ConetNode;
    digitizers[iDigitizer].VMEBaseAddress = cfg.digitizers[iDigitizer].VMEBaseAddress;

    digitizers[iDigitizer].useIRQ      = cfg.digitizers[iDigitizer].useIRQ;
    digitizers[iDigitizer].irqLevel    = cfg.digitizers[iDigitizer].irqLevel;
    digitizers[iDigitizer].irqStatusId = cfg.digitizers[iDigitizer].irqStatusId;
    digitizers[iDigitizer].irqNEvents  = cfg.digitizers[iDigitizer].irqNEvents;
    digitizers[iDigitizer].irqMode     = cfg.digitizers[iDigitizer].irqMode;
    digitizers[iDigitizer].irqTimeout  = cfg.digitizers[iDigitizer].irqTimeout;

    digitizers[iDigitizer].useChargeMode          = cfg.digitizers[iDigitizer].useChargeMode;
    digitizers[iDigitizer].suppressChargeBaseline = cfg.digitizers[iDigitizer].suppressChargeBaseline;

    digitizers[iDigitizer].SAMCorrectionLevel = cfg.digitizers[iDigitizer].SAMCorrectionLevel;

    digitizers[iDigitizer].SAMFrequency     = cfg.digitizers[iDigitizer].SAMFrequency;
    digitizers[iDigitizer].secondsPerSample = cfg.digitizers[iDigitizer].secondsPerSample;

    digitizers[iDigitizer].RecordLength = cfg.digitizers[iDigitizer].RecordLength;

    digitizers[iDigitizer].TriggerType = cfg.digitizers[iDigitizer].TriggerType;

    digitizers[iDigitizer].IOLevel = cfg.digitizers[iDigitizer].IOLevel;

    digitizers[iDigitizer].MaxNumEventsBLT = cfg.digitizers[iDigitizer].MaxNumEventsBLT;

    digitizers[iDigitizer].GroupTriggerLogic         = cfg.digitizers[iDigitizer].GroupTriggerLogic;
    digitizers[iDigitizer].GroupTriggerMajorityLevel = cfg.digitizers[iDigitizer].GroupTriggerMajorityLevel;

    digitizers[iDigitizer].useTriggerCountVeto    = cfg.digitizers[iDigitizer].useTriggerCountVeto;
    digitizers[iDigitizer].triggerCountVetoWindow = cfg.digitizers[iDigitizer].triggerCountVetoWindow;

    for(unsigned int i = 0; i < digitizers[iDigitizer].groups.size(); i++)   digitizers[iDigitizer].groups[i] = cfg.digitizers[iDigitizer].groups[i];
    for(unsigned int i = 0; i < digitizers[iDigitizer].channels.size(); i++) digitizers[iDigitizer].channels[i] = cfg.digitizers[iDigitizer].channels[i];
  }

  for(unsigned int i = 0; i < nHVChannels; i++) {
    hvConfig.voltages[i] = cfg.hvConfig.voltages[i];
    hvConfig.enabled[i] = cfg.hvConfig.enabled[i];
    hvConfig.names[i] = cfg.hvConfig.names[i];
  }

}

mdaq::DemonstratorConfiguration::~DemonstratorConfiguration() {
  digitizers.clear();
}

mdaq::DemonstratorConfiguration& mdaq::DemonstratorConfiguration::operator=(mdaq::DemonstratorConfiguration& cfg) {

  TObject::operator=(cfg);

  TFileCompressionEnabled = cfg.TFileCompressionEnabled;

  HLTEnabled = cfg.HLTEnabled;
  HLTMaxAmplitude = cfg.HLTMaxAmplitude;
  HLTNumRequired = cfg.HLTNumRequired;

  CosmicPrescaleEnabled = cfg.CosmicPrescaleEnabled;
  CosmicPrescaleMaxAmplitude = cfg.CosmicPrescaleMaxAmplitude;
  CosmicPrescaleNumRequired = cfg.CosmicPrescaleNumRequired;
  CosmicPrescalePS = cfg.CosmicPrescalePS;

  digitizers.resize(cfg.digitizers.size());

  for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
    digitizers[iDigitizer].boardIndex  = cfg.digitizers[iDigitizer].boardIndex;
    digitizers[iDigitizer].boardEnable = cfg.digitizers[iDigitizer].boardEnable;

    digitizers[iDigitizer].LinkType       = cfg.digitizers[iDigitizer].LinkType;
    digitizers[iDigitizer].LinkNum        = cfg.digitizers[iDigitizer].LinkNum;
    digitizers[iDigitizer].ConetNode      = cfg.digitizers[iDigitizer].ConetNode;
    digitizers[iDigitizer].VMEBaseAddress = cfg.digitizers[iDigitizer].VMEBaseAddress;

    digitizers[iDigitizer].useIRQ      = cfg.digitizers[iDigitizer].useIRQ;
    digitizers[iDigitizer].irqLevel    = cfg.digitizers[iDigitizer].irqLevel;
    digitizers[iDigitizer].irqStatusId = cfg.digitizers[iDigitizer].irqStatusId;
    digitizers[iDigitizer].irqNEvents  = cfg.digitizers[iDigitizer].irqNEvents;
    digitizers[iDigitizer].irqMode     = cfg.digitizers[iDigitizer].irqMode;
    digitizers[iDigitizer].irqTimeout  = cfg.digitizers[iDigitizer].irqTimeout;

    digitizers[iDigitizer].useChargeMode          = cfg.digitizers[iDigitizer].useChargeMode;
    digitizers[iDigitizer].suppressChargeBaseline = cfg.digitizers[iDigitizer].suppressChargeBaseline;

    digitizers[iDigitizer].SAMCorrectionLevel = cfg.digitizers[iDigitizer].SAMCorrectionLevel;

    digitizers[iDigitizer].SAMFrequency     = cfg.digitizers[iDigitizer].SAMFrequency;
    digitizers[iDigitizer].secondsPerSample = cfg.digitizers[iDigitizer].secondsPerSample;

    digitizers[iDigitizer].RecordLength = cfg.digitizers[iDigitizer].RecordLength;

    digitizers[iDigitizer].TriggerType = cfg.digitizers[iDigitizer].TriggerType;

    digitizers[iDigitizer].IOLevel = cfg.digitizers[iDigitizer].IOLevel;

    digitizers[iDigitizer].MaxNumEventsBLT = cfg.digitizers[iDigitizer].MaxNumEventsBLT;

    digitizers[iDigitizer].GroupTriggerLogic         = cfg.digitizers[iDigitizer].GroupTriggerLogic;
    digitizers[iDigitizer].GroupTriggerMajorityLevel = cfg.digitizers[iDigitizer].GroupTriggerMajorityLevel;

    digitizers[iDigitizer].useTriggerCountVeto    = cfg.digitizers[iDigitizer].useTriggerCountVeto;
    digitizers[iDigitizer].triggerCountVetoWindow = cfg.digitizers[iDigitizer].triggerCountVetoWindow;

    for(unsigned int i = 0; i < digitizers[iDigitizer].groups.size(); i++)   digitizers[iDigitizer].groups[i] = cfg.digitizers[iDigitizer].groups[i];
    for(unsigned int i = 0; i < digitizers[iDigitizer].channels.size(); i++) digitizers[iDigitizer].channels[i] = cfg.digitizers[iDigitizer].channels[i];
  }

  for(unsigned int i = 0; i < nHVChannels; i++) {
    hvConfig.voltages[i] = cfg.hvConfig.voltages[i];
    hvConfig.enabled[i] = cfg.hvConfig.enabled[i];
    hvConfig.names[i] = cfg.hvConfig.names[i];
  }

  currentConfigFile = cfg.currentConfigFile;

  return *this;
}

void mdaq::DemonstratorConfiguration::PrintConfiguration() const {

  cout << endl;
  cout << "======================================================" << endl;
  cout << "DemonstratorConfiguration: " << currentConfigFile << endl;
  cout << endl;

  cout << "General (software-only) parameters:" << endl;
  cout << "\tTFileCompression: " << (TFileCompressionEnabled ? "enabled" : "disabled") << endl;
  cout << "\tHLT: ";
  if(HLTEnabled) cout << "enabled (" << HLTNumRequired << "+ channels with amplitude <= " << HLTMaxAmplitude << " mV)" << endl;
  else cout << "disabled" << endl;
  cout << "\tCosmicPrescale: ";
  if(CosmicPrescaleEnabled) cout << "enabled (Apply prescale " << CosmicPrescalePS << " to events with " << CosmicPrescaleNumRequired << "+ channels with amplitude <= " << CosmicPrescaleMaxAmplitude << " mV)" << endl;
  else cout << "disabled" << endl;
  cout << endl;

  cout << "======================================================" << endl;
  cout << "High voltage:" << endl;
  cout << endl;
  for(unsigned int i = 0; i < nHVChannels; i++) {
    cout << hvConfig.names[i] << ": " << hvConfig.voltages[i] << " V -- " << (hvConfig.enabled[i]  ? "ON" : "OFF") << endl;
  }
  cout << endl;

  cout << "======================================================" << endl;
  cout << "Digitizers:" << endl;
  cout << endl;

  for(auto &digitizer : digitizers) digitizer.PrintConfiguration();
}

void mdaq::DemonstratorConfiguration::LogConfiguration() const {

  ostringstream oss;

  oss << endl;
  oss << "======================================================" << endl;
  oss << "DemonstratorConfiguration: " << currentConfigFile << endl;
  oss << endl;

  oss << "General (software-only) parameters:" << endl;
  oss << "\tTFileCompression: " << (TFileCompressionEnabled ? "enabled" : "disabled") << endl;
  oss << "\tHLT: ";
  if(HLTEnabled) oss << "enabled (" << HLTNumRequired << "+ channels with amplitude <= " << HLTMaxAmplitude << " mV)" << endl;
  else oss << "disabled" << endl;
  oss << "\tCosmicPrescale: ";
  if(CosmicPrescaleEnabled) oss << "enabled (Apply prescale " << CosmicPrescalePS << " to events with " << CosmicPrescaleNumRequired << "+ channels with amplitude <= " << CosmicPrescaleMaxAmplitude << " mV)" << endl;
  else oss << "disabled" << endl;
  oss << endl;

  oss << "======================================================" << endl;
  oss << "High voltage:" << endl;
  oss << endl;
  for(unsigned int i = 0; i < nHVChannels; i++) {
    oss << hvConfig.names[i] << ": " << hvConfig.voltages[i] << " V -- " << (hvConfig.enabled[i]  ? "ON" : "OFF") << endl;
  }
  oss << endl;

  oss << "======================================================" << endl;
  oss << "Digitizers:" << endl;
  oss << endl;

  mdaq::Logger::instance().info("DemonstratorConfiguration", TString(oss.str()));

  for(auto &digitizer : digitizers) digitizer.LogConfiguration();
}

bool mdaq::DemonstratorConfiguration::ReadPython(string filename) {

  mdaq::DemonstratorConfiguration cfg;
  mdaq::ConfigurationReader reader;
  
  if(reader.ParsePython(cfg, filename)) {
    currentConfigFile = filename;
    *this = cfg;
    return true;
  }

  return false;
}

bool mdaq::DemonstratorConfiguration::ReadXML(string filename) {

  mdaq::DemonstratorConfiguration cfg;
  mdaq::ConfigurationReader reader;

  if(reader.ParseXML(cfg, filename)) {
    currentConfigFile = filename;
    *this = cfg;
    return true;
  }

  return false;
}

const unsigned int mdaq::DemonstratorConfiguration::nDigitizersEnabled() const {
  unsigned int nEnabled = 0;
  for(auto &digitizer : digitizers) {
    if(digitizer.boardEnable) {
      nEnabled++;
    }
  }
  return nEnabled;
}
