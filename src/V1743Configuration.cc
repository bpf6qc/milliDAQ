#include "V1743Configuration.h"

using namespace std;

ClassImp(mdaq::V1743Configuration);

mdaq::V1743Configuration::V1743Configuration() : TObject() {
  groups.resize(nGroupsPerDigitizer);
  channels.resize(nChannelsPerDigitizer);
}

mdaq::V1743Configuration::V1743Configuration(const mdaq::V1743Configuration& cfg) : TObject(cfg) {

  boardIndex = cfg.boardIndex;
  boardEnable = cfg.boardEnable;

  LinkType = cfg.LinkType;
  LinkNum = cfg.LinkNum;
  ConetNode = cfg.ConetNode;
  VMEBaseAddress = cfg.VMEBaseAddress;

  useIRQ = cfg.useIRQ;
  irqLevel = cfg.irqLevel;
  irqStatusId = cfg.irqStatusId;
  irqNEvents = cfg.irqNEvents;
  irqMode = cfg.irqMode;
  irqTimeout = cfg.irqTimeout;

  useChargeMode = cfg.useChargeMode;
  suppressChargeBaseline = cfg.suppressChargeBaseline;

  SAMCorrectionLevel = cfg.SAMCorrectionLevel;

  SAMFrequency = cfg.SAMFrequency;
  secondsPerSample = cfg.secondsPerSample;

  RecordLength = cfg.RecordLength;

  TriggerType = cfg.TriggerType;

  IOLevel = cfg.IOLevel;

  MaxNumEventsBLT = cfg.MaxNumEventsBLT;

  GroupTriggerLogic = cfg.GroupTriggerLogic;
  GroupTriggerMajorityLevel = cfg.GroupTriggerMajorityLevel;

  useTriggerCountVeto = cfg.useTriggerCountVeto;
  triggerCountVetoWindow = cfg.triggerCountVetoWindow;

  for(unsigned int i = 0; i < groups.size(); i++) groups[i] = cfg.groups[i];
  for(unsigned int i = 0; i < channels.size(); i++) channels[i] = cfg.channels[i];
}

mdaq::V1743Configuration::~V1743Configuration() {
  groups.clear();
  channels.clear();
}

mdaq::V1743Configuration& mdaq::V1743Configuration::operator=(mdaq::V1743Configuration& cfg) {

  TObject::operator=(cfg);

  boardIndex = cfg.boardIndex;
  boardEnable = cfg.boardEnable;

  LinkType = cfg.LinkType;
  LinkNum = cfg.LinkNum;
  ConetNode = cfg.ConetNode;
  VMEBaseAddress = cfg.VMEBaseAddress;

  useIRQ = cfg.useIRQ;
  irqLevel = cfg.irqLevel;
  irqStatusId = cfg.irqStatusId;
  irqNEvents = cfg.irqNEvents;
  irqMode = cfg.irqMode;
  irqTimeout = cfg.irqTimeout;

  useChargeMode = cfg.useChargeMode;
  suppressChargeBaseline = cfg.suppressChargeBaseline;

  SAMCorrectionLevel = cfg.SAMCorrectionLevel;

  SAMFrequency = cfg.SAMFrequency;
  secondsPerSample = cfg.secondsPerSample;

  RecordLength = cfg.RecordLength;

  TriggerType = cfg.TriggerType;

  IOLevel = cfg.IOLevel;

  MaxNumEventsBLT = cfg.MaxNumEventsBLT;

  GroupTriggerLogic = cfg.GroupTriggerLogic;
  GroupTriggerMajorityLevel = cfg.GroupTriggerMajorityLevel;

  useTriggerCountVeto = cfg.useTriggerCountVeto;
  triggerCountVetoWindow = cfg.triggerCountVetoWindow;

  for(unsigned int i = 0; i < groups.size(); i++) groups[i] = cfg.groups[i];
  for(unsigned int i = 0; i < channels.size(); i++) channels[i] = cfg.channels[i];

  return *this;
}

void mdaq::V1743Configuration::PrintConfiguration() const {

  cout << endl;
  cout << "======================================================" << endl;
  cout << "V1743Configuration: " << endl;
  cout << endl;

  cout << "Board index: " << boardIndex << endl;
  cout << "Board enabled: " << (boardEnable ? "true" : "false") << endl << endl;

  cout << "Global parameters:" << endl << endl;
  cout << "\tLinkType = " << LinkType << endl;
  cout << "\tLinkNum, ConetNode, VMEBaseAddress = " << LinkNum << ", " << ConetNode << ", " << VMEBaseAddress << endl << endl;

  cout << "\tIRQ:" << endl;
  cout << "\t\tuseIRQ = " << (useIRQ ? "true" : "false") << endl;
  cout << "\t\tLevel = " << (int)irqLevel << endl;
  cout << "\t\tStatusId = " << irqStatusId << endl;
  cout << "\t\tNEvents = " << irqNEvents << endl;
  cout << "\t\tMode = " << irqMode << endl;
  cout << "\t\tTimeout = " << irqTimeout << endl << endl;

  cout << "\tuseChargeMode = " << (useChargeMode ? "true" : "false") << endl;
  cout << "\tsurpressChargeBaseline = " << suppressChargeBaseline << endl << endl;

  cout << "\tSAMCorrectionLevel = " << SAMCorrectionLevel << endl;
  cout << "\tSAMFrequency = " << SAMFrequency << " (" << secondsPerSample << " s/sample)" << endl;
  cout << "\tRecordLength = " << RecordLength << endl;
  cout << "\tTriggerType = " << TriggerType << endl;
  cout << "\tIOLevel = " << IOLevel << endl;
  cout << "\tMaxNumEventsBLT = " << MaxNumEventsBLT << endl << endl;

  cout << "\tGroupTriggerLogic = " << GroupTriggerLogic << endl;
  cout << "\tGroupTriggerMajorityLevel = " << GroupTriggerMajorityLevel << endl;

  cout << "\tuseTriggerCountVeto = " << (useTriggerCountVeto ? "true" : "false") << endl;
  cout << "\ttriggerCountVetoWindow = " << triggerCountVetoWindow << " (ns)" << endl;

  cout << "Groups: " << endl << endl;

  for(unsigned int i = 0; i < groups.size(); i++) {
    cout << "\tGroup " << i << ":" << endl;
    cout << "\t\tlogic = " << groups[i].logic << endl;
    cout << "\t\tcoincidenceWindow = " << groups[i].coincidenceWindow << endl;
    cout << "\t\ttriggerDelay = " << (int)groups[i].triggerDelay << endl;
  }

  cout << "Channels: " << endl << endl;

  for(unsigned int i = 0; i < channels.size(); i++) {
    cout << "\tChannel " << i << ":" << endl;
    cout << "\t\tenable = " << (channels[i].enable ? "true" : "false") << endl;
    cout << "\t\ttestPulseEnable = " << (channels[i].testPulseEnable ? "true" : "false") << endl;
    cout << "\t\ttestPulsePatter = " << channels[i].testPulsePattern << endl;
    cout << "\t\ttestPulseSource = " << channels[i].testPulseSource << endl;
    cout << "\t\ttriggerEnable = " << (channels[i].triggerEnable ? "true" : "false") << endl;
    cout << "\t\ttriggerThreshold = " << channels[i].triggerThreshold << endl;
    cout << "\t\ttriggerPolarity = " << channels[i].triggerPolarity << endl;
    cout << "\t\tdcOffset = " << channels[i].dcOffset << endl;
    cout << "\t\tchargeStartCell = " << channels[i].chargeStartCell << endl;
    cout << "\t\tchargeLength = " << channels[i].chargeLength << endl;
    cout << "\t\tenableChargeThreshold = " << channels[i].enableChargeThreshold << endl;
    cout << "\t\tchargeThreshold = " << channels[i].chargeThreshold << endl;
  }

  cout << "======================================================" << endl << endl;
}


void mdaq::V1743Configuration::LogConfiguration() const {

  ostringstream oss;

  oss << endl;
  oss << "======================================================" << endl;
  oss << "V1743Configuration: " << endl;
  oss << endl;

  oss << "Board index: " << boardIndex << endl;
  oss << "Board enabled: " << (boardEnable ? "true" : "false") << endl << endl;

  oss << "Global parameters:" << endl << endl;
  oss << "\tLinkType = " << LinkType << endl;
  oss << "\tLinkNum, ConetNode, VMEBaseAddress = " << LinkNum << ", " << ConetNode << ", " << VMEBaseAddress << endl << endl;

  oss << "\tIRQ:" << endl;
  oss << "\t\tuseIRQ = " << (useIRQ ? "true" : "false") << endl;
  oss << "\t\tLevel = " << (int)irqLevel << endl;
  oss << "\t\tStatusId = " << irqStatusId << endl;
  oss << "\t\tNEvents = " << irqNEvents << endl;
  oss << "\t\tMode = " << irqMode << endl;
  oss << "\t\tTimeout = " << irqTimeout << endl << endl;

  oss << "\tuseChargeMode = " << (useChargeMode ? "true" : "false") << endl;
  oss << "\tsurpressChargeBaseline = " << suppressChargeBaseline << endl << endl;

  oss << "\tSAMCorrectionLevel = " << SAMCorrectionLevel << endl;
  oss << "\tSAMFrequency = " << SAMFrequency << " (" << secondsPerSample << " s/sample)" << endl;
  oss << "\tRecordLength = " << RecordLength << endl;
  oss << "\tTriggerType = " << TriggerType << endl;
  oss << "\tIOLevel = " << IOLevel << endl;
  oss << "\tMaxNumEventsBLT = " << MaxNumEventsBLT << endl << endl;

  oss << "\tGroupTriggerLogic = " << GroupTriggerLogic << endl;
  oss << "\tGroupTriggerMajorityLevel = " << GroupTriggerMajorityLevel << endl;

  oss << "\tuseTriggerCountVeto = " << (useTriggerCountVeto ? "true" : "false") << endl;
  oss << "\ttriggerCountVetoWindow = " << triggerCountVetoWindow << " (ns)" << endl;

  oss << "Groups: " << endl << endl;

  for(unsigned int i = 0; i < groups.size(); i++) {
    oss << "\tGroup " << i << ":" << endl;
    oss << "\t\tlogic = " << groups[i].logic << endl;
    oss << "\t\tcoincidenceWindow = " << groups[i].coincidenceWindow << endl;
    oss << "\t\ttriggerDelay = " << (int)groups[i].triggerDelay << endl;
  }

  oss << "Channels: " << endl << endl;

  for(unsigned int i = 0; i < channels.size(); i++) {
    oss << "\tChannel " << i << ":" << endl;
    oss << "\t\tenable = " << (channels[i].enable ? "true" : "false") << endl;
    oss << "\t\ttestPulseEnable = " << (channels[i].testPulseEnable ? "true" : "false") << endl;
    oss << "\t\ttestPulsePatter = " << channels[i].testPulsePattern << endl;
    oss << "\t\ttestPulseSource = " << channels[i].testPulseSource << endl;
    oss << "\t\ttriggerEnable = " << (channels[i].triggerEnable ? "true" : "false") << endl;
    oss << "\t\ttriggerThreshold = " << channels[i].triggerThreshold << endl;
    oss << "\t\ttriggerPolarity = " << channels[i].triggerPolarity << endl;
    oss << "\t\tdcOffset = " << channels[i].dcOffset << endl;
    oss << "\t\tchargeStartCell = " << channels[i].chargeStartCell << endl;
    oss << "\t\tchargeLength = " << channels[i].chargeLength << endl;
    oss << "\t\tenableChargeThreshold = " << channels[i].enableChargeThreshold << endl;
    oss << "\t\tchargeThreshold = " << channels[i].chargeThreshold << endl;
  }

  oss << "======================================================" << endl << endl;

  mdaq::Logger::instance().info("V1743Configuration", TString(oss.str()));
}
