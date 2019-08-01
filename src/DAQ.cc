#include "DAQ.h"

#include <bitset>

using namespace std;

//////////////////////////////////////////////////////////////////////////
// Constructor, destructor
//////////////////////////////////////////////////////////////////////////

mdaq::DAQ::DAQ() {
  for(unsigned int i = 0; i < nDigitizers; i++) {
    DeviceHandles[i] = -1;

    CurrentEventInfos[i] = new CAEN_DGTZ_EventInfo_t();
    CurrentEvents[i] = NULL;

    ReadoutBuffers[i] = NULL;
    ReadoutBufferSizes[i] = 0;

    eventPtrs[i] = NULL;
  }

  triggerAnyBoardSW = false;
  SAMPulseAnyBoardSW = false;

  configurationFile = GetPreviousConfigurationFile();

  // bfrancis -- wip
  /*
  CAENSY5527 * theHV = new CAENSY5527();
  theHV->Login();
  theHV->GetParameters();
  for(int i = 0; i < nHVChannels; i++) {
    mdaq::Logger::instance().debug("HV", TString(Form("channel %d -- Set: %f V %f uA -- Mon: %f V %f uA -- pw = %d -- status = %d", theHV->channels[i], theHV->V0Set[i], theHV->I0Set[i], theHV->V0Mon[i], theHV->I0Mon[i], theHV->Pw[i], theHV->Status[i])));
  }
  theHV->Logout();
  delete theHV;
 */
}

mdaq::DAQ::~DAQ() {
  for(unsigned int i = 0; i < nDigitizers; i++) {
    delete CurrentEvents[i];
    delete CurrentEventInfos[i];
  }

  delete cfg;
}

//////////////////////////////////////////////////////////////////////////
// Public methods
//////////////////////////////////////////////////////////////////////////

bool mdaq::DAQ::ReadConfiguration(string filename) {
  mdaq::DemonstratorConfiguration * config = new DemonstratorConfiguration();
  mdaq::ConfigurationReader * reader = new ConfigurationReader();

  // check for *.py or *.xml
  if(filename.size() < 3) return false;

  bool success = false;

  if(std::equal(xmlSuffix.rbegin(), xmlSuffix.rend(), filename.rbegin())) {
    success = reader->ParseXML(*config, filename);
  }
  else if(std::equal(pythonSuffix.rbegin(), pythonSuffix.rend(), filename.rbegin())) {
    success = reader->ParsePython(*config, filename);
  }
  else {
    mdaq::Logger::instance().warning("DAQ", "Only XML or python configuration files are supported.");
    success = false;
  }

  if(!success) return false;

  cfg = config;
  configurationFile = filename;

  nDigitizersEnabled = cfg->nDigitizersEnabled();

  delete reader;

  return true;
}

void mdaq::DAQ::ConnectToBoards() {
  mdaq::Logger::instance().info("DAQ", "Opening connections to all V1743 digitizers...");

  // bfrancis -- this might be the extra configuration read i keep seeing...
  // check when these are read and maybe remove this?
  if(!ReadConfiguration(configurationFile)) {
    mdaq::Logger::instance().error("DAQ", "Failed to read previous configuration file " + TString(configurationFile) + " -- falling back to default /home/milliqan/MilliDAQ/config/TripleCoincidence.py");
    configurationFile = "/home/milliqan/MilliDAQ/config/Synchronize.py";

    if(!ReadConfiguration(configurationFile)) {
      mdaq::Logger::instance().error("DAQ", "Failed to read default configuration file " + TString(configurationFile) + " -- cannot continue!");
      exit(EXIT_FAILURE);
    }
  }

  for(unsigned int i = 0; i < nDigitizers; i++) ConnectToBoard(i);

  mdaq::Logger::instance().info("DAQ", "Successfully opened connections to all V1743 digitizers with file: " + TString(configurationFile));
}

void mdaq::DAQ::PrintBoardInfos() {
  for(unsigned int i = 0; i < nDigitizers; i++) PrintBoardInfo(i);
  // bfrancis -- this could be made into a single printout with two columns
}

void mdaq::DAQ::PrintRegister(const unsigned int boardIndex, uint32_t reg) {
  uint32_t data;
  ReadRegister(boardIndex, reg, &data);

  ostringstream oss;
  oss << "Value of register 0x" << hex << reg << " in board " << boardIndex << " has data = " << data << dec << endl;
  mdaq::Logger::instance().info("DAQ", TString(oss.str()));
}

void mdaq::DAQ::PrintAllRegisters(const unsigned int boardIndex) {
  // bfrancis -- should just use WDFile.c's SaveRegImage(int) from WaveDemo

  vector<uint32_t> registers;
  vector<TString> registerNames;
  for(unsigned int i = 0; i < nGroupsPerDigitizer; i++) {
    registers.push_back(0x102C + i * 0x0100);
    registerNames.push_back(Form("Group %d Pulse Enable (1.2)", i));
  }
  for(unsigned int i = 0; i < nGroupsPerDigitizer; i++) {
    registers.push_back(0x1030 + i * 0x0100);
    registerNames.push_back(Form("Group %d PostTrigger (1.3)", i));
  }
  for(unsigned int i = 0; i < nGroupsPerDigitizer; i++) {
    registers.push_back(0x1034 + i * 0x0100);
    registerNames.push_back(Form("Group %d Pulse Pattern (1.4)", i));
  }
  for(unsigned int i = 0; i < nGroupsPerDigitizer; i++) {
    registers.push_back(0x1038 + i * 0x0100);
    registerNames.push_back(Form("Group %d Trigger Gate (1.5)", i));
  }
  for(unsigned int i = 0; i < nGroupsPerDigitizer; i++) {
    registers.push_back(0x103C + i * 0x0100);
    registerNames.push_back(Form("Group %d Trigger (1.6)", i));
  }
  for(unsigned int i = 0; i < nGroupsPerDigitizer; i++) {
    registers.push_back(0x1040 + i * 0x0100);
    registerNames.push_back(Form("Group %d Sampling Frequency (1.7)", i));
  }
  for(unsigned int i = 0; i < nGroupsPerDigitizer; i++) {
    registers.push_back(0x1040 + i * 0x0100);
    registerNames.push_back(Form("Group %d Recording Depth (1.8)", i));
  }
  for(unsigned int i = 0; i < nGroupsPerDigitizer; i++) {
    registers.push_back(0x1048 + i * 0x0100);
    registerNames.push_back(Form("Group %d Charge Threshold (1.9)", i));
  }
  for(unsigned int i = 0; i < nGroupsPerDigitizer; i++) {
    registers.push_back(0x106C + i * 0x0100);
    registerNames.push_back(Form("Group %d Rate Counters Ch0 (1.15)", i));
  }
  for(unsigned int i = 0; i < nGroupsPerDigitizer; i++) {
    registers.push_back(0x1094 + i * 0x0100);
    registerNames.push_back(Form("Group %d Rate Counters Ch1 (1.20)", i));
  }

  registers.push_back(0x800);
  registerNames.push_back("Channel Configuration (1.21)");

  registers.push_back(0x8100);
  registerNames.push_back("Acquisition Control (1.30)");

  registers.push_back(0x8104);
  registerNames.push_back("Acquisition Status (1.31)");

  registers.push_back(0x810C);
  registerNames.push_back("Trigger Source Enable Mask (1.33)");

  registers.push_back(0x8110);
  registerNames.push_back("Front Panel Trigger Out Enable Mask (1.34)");

  registers.push_back(0x811C);
  registerNames.push_back("Front Panel I/O Control (1.36)");

  registers.push_back(0x8120);
  registerNames.push_back("Channel Enable Mask (1.37)");  

  registers.push_back(0x8178);
  registerNames.push_back("Board Fail Status (1.47)");

  registers.push_back(0x81A0);
  registerNames.push_back("Front Panel LVDS I/O New Features (1.48)");

  registers.push_back(0x81A0);
  registerNames.push_back("Front Panel LVDS I/O New Features (1.48)");

  uint32_t data;

  ostringstream oss;

  oss << endl << "Register values for board " << boardIndex << ": " << endl << endl;
  for(unsigned int i = 0; i < registers.size(); i++) {
    ReadRegister(boardIndex, registers[i], &data);
    oss << "0x" << hex << registers[i] << " [" << registerNames[i] << "] = " << data << dec << endl;
  }

  mdaq::Logger::instance().info("DAQ", TString(oss.str()));

  registers.clear();
  registerNames.clear();
}

void mdaq::DAQ::InitializeBoards() {
  mdaq::Logger::instance().info("DAQ", "Initializing all V1743 digitizer configurations...");

  triggerAnyBoardSW = false;
  SAMPulseAnyBoardSW = false;

  for(int i = 0; i < nDigitizers; i++) {
    InitializeBoard(i);
  }

  MallocReadoutBuffers();
}

void mdaq::DAQ::StartRun() {
  ProgramSynchronization();

  for(int i = 0; i < nDigitizers; i++) {
    Try("AllocateEvent", CAEN_DGTZ_AllocateEvent(DeviceHandles[i], (void**)&CurrentEvents[i]));
  }    

  for(int i = 1; i < nDigitizers; i++) {
    Try("SWStartAcquisition", CAEN_DGTZ_SWStartAcquisition(DeviceHandles[i]));
  }

  Try("SWStartAcquisition", CAEN_DGTZ_SWStartAcquisition(DeviceHandles[0]));

  runInitialTime = chrtime::now();
}

void mdaq::DAQ::StopRun() {
  for(int i = 0; i < nDigitizers; i++) {
    Try("SWStopAcquisition", CAEN_DGTZ_SWStopAcquisition(DeviceHandles[i]));
    Try("ClearData", CAEN_DGTZ_ClearData(DeviceHandles[i]));
  }

  for(int i = 0; i < nDigitizers; i++) {
    Try("FreeEvent", CAEN_DGTZ_FreeEvent(DeviceHandles[i], (void**)&CurrentEvents[i]));
  }

  for(int i = 0; i < nDigitizers; i++) {
    Try("FreeReadoutBuffer", CAEN_DGTZ_FreeReadoutBuffer((char**)&ReadoutBuffers[i]));
  }
}

void mdaq::DAQ::SendSWTriggers() {
  if(!triggerAnyBoardSW) return;

  // If multiple boards are configured for software triggers, send only one
  // and let the board synchronization propogate it
  for(unsigned int i = 0; i < nDigitizers; i++) {
    if(TriggerTypes[i] == SYSTEM_TRIGGER_AUTO || TriggerTypes[i] == SYSTEM_TRIGGER_SOFT) {
      Try("SendSWTrigger", CAEN_DGTZ_SendSWtrigger(DeviceHandles[i]));
      break;
    }
  }
}

void mdaq::DAQ::SendSAMPulses() {

  if(!SAMPulseAnyBoardSW) return;

  for(unsigned int i = 0; i < nDigitizers; i++) {
    Try("SendSAMPulse", CAEN_DGTZ_SendSAMPulse(DeviceHandles[i]));
  }

}

void mdaq::DAQ::CloseDevices() {
  for(int i = 0; i < nDigitizers; i++) {
    Try("CloseDigitizer", CAEN_DGTZ_CloseDigitizer(DeviceHandles[i]));
  }
}

void mdaq::DAQ::Read(Queue * queues[nDigitizers], uint32_t * val) {

  unsigned int nDigitizersWithData = 0;

  // Attempt to read all digitizers' buffers
  for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
    if(ReadEventBuffer(iDigitizer)) {
      nDigitizersWithData++;
    }
  }
  
  // No data was read, so there's nothing to do
  if(nDigitizersWithData == 0) {
    val[0] = 0;
    val[1] = 0;
    return;
  }

  // Decode all events read and push them onto their own queues
  for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {

    val[iDigitizer] = GetNumberOfEvents(iDigitizer);

    for(uint32_t iEvt = 0; iEvt < val[iDigitizer]; iEvt++) {
      if(DecodeEvent(iEvt, iDigitizer)) {
        if(!queues[iDigitizer]->push(CurrentEvents[iDigitizer], CurrentEventInfos[iDigitizer])) {
          mdaq::Logger::instance().warning("DAQ", Form("Failed to push event to queues[%d], it's full!", iDigitizer));
          break; // If this queue is full, stop trying to give it a chance to empty
        }
      }
      else break; // If any events are somehow corrupted, skip this buffer entirely
    }

  }

}

void mdaq::DAQ::Read(Queue * queues[nDigitizers]) {

  unsigned int nDigitizersWithData = 0;

  // Attempt to read all digitizers' buffers
  for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {
    if(ReadEventBuffer(iDigitizer)) {
      nDigitizersWithData++;
    }
  }
  
  // No data was read, so there's nothing to do
  if(nDigitizersWithData == 0) return;

  // Decode all events read and push them onto their own queues
  for(unsigned int iDigitizer = 0; iDigitizer < nDigitizers; iDigitizer++) {

    for(uint32_t iEvt = 0; iEvt < GetNumberOfEvents(iDigitizer); iEvt++) {
      if(DecodeEvent(iEvt, iDigitizer)) {
        if(!queues[iDigitizer]->push(CurrentEvents[iDigitizer], CurrentEventInfos[iDigitizer])) {
          mdaq::Logger::instance().warning("DAQ", Form("Failed to push event to queues[%d], it's full!", iDigitizer));
          break; // If this queue is full, stop trying to give it a chance to empty
        }
      }
      else break; // If any events are somehow corrupted, skip this buffer entirely
    }

  }

}

void mdaq::DAQ::SetOutputClockSignal(const unsigned int boardIndex) {

  uint32_t bitMask = 0;

  ReadRegister(boardIndex, 0x811C, &bitMask);

  // TRG-OUT Mode select:
  // 00 = Trigger (TRG-OUT propagates the internal trigger sources according to the Front Panel Trigger Out Enable Mask register (see § 1.34).
  // 01 = Motherboard probes (TRG-OUT is used to propagate signals of the motherboards, according to the bits[19:18].
  // 10 = Channel probes (TRG-OUT is used to propagate signals of the mezzanines (Channel Signal Virtual Probe).
  // 11 = S-IN propagation
  bitMask |= (1 << 16);
  bitMask &= ~(1 << 17);

  // Motherboard Virtual Probe select (to be propagated onto TRG-OUT):
  // 00 = RUN: the signal is active when the acquisition is running. This option can be used to synchronize the start/stop of the acquisition through the TRG-OUT->TR-IN or TRG-OUT->S-IN daisy chain.
  // 01 = CLKOUT: this clock is synchronous with the sampling clock of the ADC and this option can be used to align the phase of the clocks in different boards.
  // 10 = CLK Phase
  // 11 = Board BUSY
  bitMask |= (1 << 18);
  bitMask &= ~(1 << 19);

  WriteRegister(boardIndex, 0x811C, bitMask);

}

void mdaq::DAQ::UnsetOutputClockSignal(const unsigned int boardIndex) {
  uint32_t bitMask = 0;

  ReadRegister(boardIndex, 0x811C, &bitMask);

  // TRG-OUT Mode select:
  // 00 = Trigger (TRG-OUT propagates the internal trigger sources according to the Front Panel Trigger Out Enable Mask register (see § 1.34).
  // 01 = Motherboard probes (TRG-OUT is used to propagate signals of the motherboards, according to the bits[19:18].
  // 10 = Channel probes (TRG-OUT is used to propagate signals of the mezzanines (Channel Signal Virtual Probe).
  // 11 = S-IN propagation
  bitMask &= ~(1 << 16);
  bitMask &= ~(1 << 17);

  // Motherboard Virtual Probe select (to be propagated onto TRG-OUT):
  // 00 = RUN: the signal is active when the acquisition is running. This option can be used to synchronize the start/stop of the acquisition through the TRG-OUT->TR-IN or TRG-OUT->S-IN daisy chain.
  // 01 = CLKOUT: this clock is synchronous with the sampling clock of the ADC and this option can be used to align the phase of the clocks in different boards.
  // 10 = CLK Phase
  // 11 = Board BUSY
  bitMask &= ~(1 << 18);
  bitMask &= ~(1 << 19);

  WriteRegister(boardIndex, 0x811C, bitMask);

}

// bfrancis -- CAEN does not use this (neither do we right now), so is it necessary?
void mdaq::DAQ::ForceClockSync(const unsigned int boardIndex) {

  // Force the clock phase alignment
  TouchRegister(boardIndex, 0x813C);
  // Wait for the sync process to complete
  std::this_thread::sleep_for(chrono::milliseconds(1000));

}

// bfrancis -- CAEN does not use this (neither do we right now), so is it necessary?
void mdaq::DAQ::ForcePLLReload(const unsigned int boardIndex) {
  TouchRegister(boardIndex, 0xEF34);
}

//////////////////////////////////////////////////////////////////////////
// Protected methods
//////////////////////////////////////////////////////////////////////////

void mdaq::DAQ::ConnectToBoard(const unsigned int boardIndex) {

  mdaq::Logger::instance().info("DAQ", "Opening connection to V1743 at index " + TString(Form("%d", boardIndex)) + "...");

  Require("OpenDigitizer", CAEN_DGTZ_OpenDigitizer((CAEN_DGTZ_ConnectionType)cfg->digitizers[boardIndex].LinkType,
						   cfg->digitizers[boardIndex].LinkNum,
						   cfg->digitizers[boardIndex].ConetNode,
						   cfg->digitizers[boardIndex].VMEBaseAddress,
						   &DeviceHandles[boardIndex]));

  Require("GetInfo", CAEN_DGTZ_GetInfo(DeviceHandles[boardIndex], &boardInfos[boardIndex]));

  mdaq::Logger::instance().info("DAQ", "Successfully opened connection to V1743 at index " + TString(Form("%d", boardIndex)));

  connectionTimes[boardIndex] = chrtime::now();

  PrintBoardInfo(boardIndex);
}

void mdaq::DAQ::PrintBoardInfo(const unsigned int boardIndex) {

  ostringstream oss;

  oss << endl << endl;
  oss << "======================================================" << endl;
  oss << "CAEN V1743 information (board index " << boardIndex << "):" << endl;
  oss << endl;

  oss << "Model Name:              " << boardInfos[boardIndex].ModelName << endl;
  oss << "Model:                   " << boardInfos[boardIndex].Model << endl;
  oss << "Channels:                " << boardInfos[boardIndex].Channels << endl;
  oss << "Form Factor:             " << boardInfos[boardIndex].FormFactor << endl;
  oss << "Family Code:             " << boardInfos[boardIndex].FamilyCode << endl;
  oss << "ROC_FirmwareRel:         " << boardInfos[boardIndex].ROC_FirmwareRel << endl;
  oss << "AMC_FirmwareRel:         " << boardInfos[boardIndex].AMC_FirmwareRel << endl;
  oss << "SerialNumber:            " << boardInfos[boardIndex].SerialNumber << endl;
  oss << "PCB_Revision:            " << boardInfos[boardIndex].PCB_Revision << endl;
  oss << "ADC_NBits:               " << boardInfos[boardIndex].ADC_NBits << endl;
  oss << "SAMCorrectionDataLoaded: " << boardInfos[boardIndex].SAMCorrectionDataLoaded << endl;
  oss << "CommHandle:              " << boardInfos[boardIndex].CommHandle << endl;
  oss << "License:                 " << boardInfos[boardIndex].License << endl;
  oss << endl;

  time_t t_t = chrtime::to_time_t(connectionTimes[boardIndex]);
  char ch_t[40];
  strftime(ch_t, sizeof(ch_t), "%c %Z", gmtime(&t_t));

  chrono::duration<float> connectionDuration = chrtime::now() - connectionTimes[boardIndex];
  int nSecondsLive = connectionDuration.count();
  int nHoursLive = nSecondsLive / 60 / 60;
  int nMinutesLive = (nSecondsLive / 60) % 60;
  nSecondsLive = nSecondsLive % 60;

  oss << "Connection opened: " << ch_t << endl;
  oss << "Open for:          " << nHoursLive << "h " << nMinutesLive << "m " << nSecondsLive << "s" << endl;
  oss << "======================================================" << endl << endl;

  mdaq::Logger::instance().info("DAQ", TString(oss.str()));
}

void mdaq::DAQ::InitializeBoard(const unsigned int boardIndex) {

  if(DeviceHandles[boardIndex] == -1) {
    mdaq::Logger::instance().warning("DAQ", "Cannot initialize board at index " + TString(Form("%d as it has no connected device!", boardIndex)));
    return;
  }

  Try("Reset", CAEN_DGTZ_Reset(DeviceHandles[boardIndex]));

  if(BoardFailStatus(boardIndex)) return;

  SetGroupEnableMask(boardIndex);
  SetTriggerDelay(boardIndex);
  SetSamplingFrequency(boardIndex);
  SetPulserParameters(boardIndex);
  SetTriggerThresholds(boardIndex);
  SetTriggerSource(boardIndex);
  SetTriggerPolarities(boardIndex);
  SetChannelDCOffsets(boardIndex);
  SetCorrectionLevel(boardIndex);
  SetMaxNumEventsBLT(boardIndex);
  SetRecordLength(boardIndex);
  SetIOLevel(boardIndex);

  for(int i = 0; i < nDigitizers; i++) {
    Try("SetAcquisitionMode", CAEN_DGTZ_SetAcquisitionMode(DeviceHandles[i], CAEN_DGTZ_SW_CONTROLLED));
  }

  // any remaining generic address writes

  // bfrancis -- if something above has failed, logger prints something but the configuration continues...should make it stop completely and reset?
  // in principle the config's IsValid() check should catch anything incorrect

  // bfrancis -- below are settings not applied by CAEN

  SetAnalogMonitorOutput(boardIndex);

  SetTriggerCountVeto(boardIndex);

  SetChannelPairTriggerLogic(boardIndex);
  SetGroupTriggerLogic(boardIndex);

  SetIRQPolicy(boardIndex);

  mdaq::Logger::instance().info("DAQ", "Successfully configured V1743 digitizer with file " + TString(configurationFile) + " at index " + TString(Form("%d", boardIndex)));
}

void mdaq::DAQ::MallocReadoutBuffers() {
  for(unsigned int i = 0; i < nDigitizers; i++) {
    Try("MallocReadoutBuffer", CAEN_DGTZ_MallocReadoutBuffer(DeviceHandles[i], (char**)&ReadoutBuffers[i], &MaxReadoutBufferSizes[i]));
  }
}

void mdaq::DAQ::CAEN_ProgramSynchronization() {
  uint32_t d32;

  for(unsigned int i = 0; i < nDigitizers; i++) {
    if(i == 0) {
      // master
      // set the start mode to software signal
      ReadRegister(i, CAEN_DGTZ_ACQ_CONTROL_ADD, &d32);
      WriteRegister(i, CAEN_DGTZ_ACQ_CONTROL_ADD, (d32 & 0xFFFFFFF8) | 0x0); // RUN_START_ON_SOFTWARE_COMMAND = 0x0
      // 0, 1, 2 off

      // enable the VetoIn
      ReadRegister(i, CAEN_DGTZ_ACQ_CONTROL_ADD, &d32);
      WriteRegister(i, CAEN_DGTZ_ACQ_CONTROL_ADD, (d32 & 0xFFFFEDFF) | 0x1200);
      // 9, 12 off; 9, 12 on
    }
    else {
      // slave
      // set the start mode to LVDS signal
      ReadRegister(i, CAEN_DGTZ_ACQ_CONTROL_ADD, &d32);
      WriteRegister(i, CAEN_DGTZ_ACQ_CONTROL_ADD, (d32 & 0xFFFFFFF8) | 0x7); // RUN_START_ON_LVDS_IO
      // 0, 1, 2 off; 0, 1, 2 on

      // enable the VetoIn and BusyIn
      ReadRegister(i, CAEN_DGTZ_ACQ_CONTROL_ADD, &d32);
      WriteRegister(i, CAEN_DGTZ_ACQ_CONTROL_ADD, (d32 & 0xFFFFECFF) | 0x1300);
      // 8, 9, 12 off; 8, 9, 12 on
    }

    // set the LVDS mode
    ReadRegister(i, CAEN_DGTZ_FRONT_PANEL_IO_CTRL_ADD, &d32);
    WriteRegister(i, CAEN_DGTZ_FRONT_PANEL_IO_CTRL_ADD, (d32 & 0xFFFFFEC1) | 0x120);
    // 1-5, 8 off; 5, 8 on

    WriteRegister(i, 0x81A0, 0x2200);
    // 9, 13 on; all others off

    // set the run start delay (0 for the last slave, 2 for the next slave, and so forth until the master)
    WriteRegister(i, 0x8170, 0x2 * (nDigitizers - 1 - i));
  }
}

void mdaq::DAQ::ProgramSynchronization() {
  uint32_t d32;

  for(unsigned int i = 0; i < nDigitizers; i++) {
    if(i == 0) {
      // master
      // set the start mode to software signal
      ReadRegister(i, CAEN_DGTZ_ACQ_CONTROL_ADD, &d32);
      WriteRegister(i, CAEN_DGTZ_ACQ_CONTROL_ADD, (d32 & 0xFFFFFFF8) | 0x0); // RUN_START_ON_SOFTWARE_COMMAND = 0x0
      // 0, 1, 2 off

      // enable the VetoIn
      ReadRegister(i, CAEN_DGTZ_ACQ_CONTROL_ADD, &d32);
      WriteRegister(i, CAEN_DGTZ_ACQ_CONTROL_ADD, (d32 & 0xFFFFEDFF) | 0x1200);
      // 9, 12 off; 9, 12 on
    }
    else {
      // slave
      // set the start mode to LVDS signal
      ReadRegister(i, CAEN_DGTZ_ACQ_CONTROL_ADD, &d32);
      WriteRegister(i, CAEN_DGTZ_ACQ_CONTROL_ADD, (d32 & 0xFFFFFFF8) | 0x7); // RUN_START_ON_LVDS_IO
      // 0, 1, 2 off; 0, 1, 2 on

      // enable the VetoIn and BusyIn
      ReadRegister(i, CAEN_DGTZ_ACQ_CONTROL_ADD, &d32);
      WriteRegister(i, CAEN_DGTZ_ACQ_CONTROL_ADD, (d32 & 0xFFFFECFF) | 0x1300);
      // 8, 9, 12 off; 8, 9, 12 on
    }

    // set the LVDS mode
    ReadRegister(i, CAEN_DGTZ_FRONT_PANEL_IO_CTRL_ADD, &d32);
    WriteRegister(i, CAEN_DGTZ_FRONT_PANEL_IO_CTRL_ADD, (d32 & 0xFFFFFEC1) | 0x104);
    // 1-5, 8 off; 2, 8 on

    WriteRegister(i, 0x81A0, 0x22);
    // 1, 5

    // set the run start delay (0 for the last slave, 2 for the next slave, and so forth until the master)
    WriteRegister(i, 0x8170, 0x2 * (nDigitizers - 1 - i));
  }
}

/////////////////////////////////////////////////////////////////////////
// Protected wrapper functions for CAENDigitizer library methods
// Called in InitializeBoard
//////////////////////////////////////////////////////////////////////////

void mdaq::DAQ::SetTriggerDelay(const unsigned int boardIndex) {
  for(int i = 0; i < nGroupsPerDigitizer; i++) {
    Try("SetSAMPostTriggerSize", CAEN_DGTZ_SetSAMPostTriggerSize(DeviceHandles[boardIndex], i, cfg->digitizers[boardIndex].groups[i].triggerDelay & 0xFF));
  }
}

void mdaq::DAQ::SetPulserParameters(const unsigned int boardIndex) {
  for(int i = 0; i < nChannelsPerDigitizer; i++) {
    if(cfg->digitizers[boardIndex].channels[i].testPulseEnable) {
      Try("EnableSAMPulseGen", CAEN_DGTZ_EnableSAMPulseGen(DeviceHandles[boardIndex],
                                                           i,
                                                           cfg->digitizers[boardIndex].channels[i].testPulsePattern,
                                                           (CAEN_DGTZ_SAMPulseSourceType_t)cfg->digitizers[boardIndex].channels[i].testPulseSource));

      if(cfg->digitizers[boardIndex].channels[i].testPulseSource == CAEN_DGTZ_SAMPulseSoftware) SAMPulseAnyBoardSW = true;
    }
    else Try("DisableSAMPulseGen", CAEN_DGTZ_DisableSAMPulseGen(DeviceHandles[boardIndex], i));
  }
}

void mdaq::DAQ::SetSamplingFrequency(const unsigned int boardIndex) {
  Try("SetSAMSamplingFrequency", CAEN_DGTZ_SetSAMSamplingFrequency(DeviceHandles[boardIndex], (CAEN_DGTZ_SAMFrequency_t)cfg->digitizers[boardIndex].SAMFrequency));
}

void mdaq::DAQ::SetTriggerThresholds(const unsigned int boardIndex) {
  for(int i = 0; i < nChannelsPerDigitizer; i++) {
    Try("SetChannelTriggerThreshold", CAEN_DGTZ_SetChannelTriggerThreshold(DeviceHandles[boardIndex],
									   i,
									   DACValue(cfg->digitizers[boardIndex].channels[i].triggerThreshold + cfg->digitizers[boardIndex].channels[i].dcOffset)));
  }
}

void mdaq::DAQ::SetTriggerCountVeto(const unsigned int boardIndex) {
  // "It is possible to configure the board to inhibit the trigger counting within an adjustable
  //  time window after the digitization starts. This option can be used to reject unwanted
  //  after-pulses..."

  // This is configurable for all channels individually, but for now configure all channels the same
  CAEN_DGTZ_EnaDis_t enableVeto = cfg->digitizers[boardIndex].useTriggerCountVeto ? CAEN_DGTZ_ENABLE : CAEN_DGTZ_DISABLE;
  for(int i = 0; i < nChannelsPerDigitizer; i++) {
    Try("SetSAMTriggerCountVetoParam", CAEN_DGTZ_SetSAMTriggerCountVetoParam(DeviceHandles[boardIndex], i, enableVeto, cfg->digitizers[boardIndex].triggerCountVetoWindow));
  }
}

void mdaq::DAQ::SetTriggerPolarities(const unsigned int boardIndex) {
  for(int i = 0; i < nChannelsPerDigitizer; i++) {
    Try("SetTriggerPolarity", CAEN_DGTZ_SetTriggerPolarity(DeviceHandles[boardIndex],
                                                           i,
                                                           (CAEN_DGTZ_TriggerPolarity_t)cfg->digitizers[boardIndex].channels[i].triggerPolarity));
  }
}

void mdaq::DAQ::CAEN_SetTriggerSource(const unsigned int boardIndex) {
  TriggerTypes[boardIndex] = cfg->digitizers[boardIndex].TriggerType;

  // first disable triggers on all channels
  uint32_t channelMask = (1 << nChannelsPerDigitizer) - 1; // all channels
  Try("SetChannelSelfTrigger", CAEN_DGTZ_SetChannelSelfTrigger(DeviceHandles[boardIndex], CAEN_DGTZ_TRGMODE_DISABLED, channelMask));

  switch(TriggerTypes[boardIndex]) {
  case SYSTEM_TRIGGER_SOFT:
    Try("SetExtTriggerInputMode", CAEN_DGTZ_SetExtTriggerInputMode(DeviceHandles[boardIndex], CAEN_DGTZ_TRGMODE_DISABLED));
    break;
  case SYSTEM_TRIGGER_NORMAL:
    channelMask = 0;
    for(unsigned int i = 0; i < nChannelsPerDigitizer / 2; i++) {
      channelMask += (cfg->digitizers[boardIndex].channels[2*i].triggerEnable + ((cfg->digitizers[boardIndex].channels[2*i + 1].triggerEnable) << 1)) << (2*i);
    }
    Try("SetChannelSelfTrigger", CAEN_DGTZ_SetChannelSelfTrigger(DeviceHandles[boardIndex], CAEN_DGTZ_TRGMODE_ACQ_ONLY, channelMask));
    Try("SetExtTriggerInputMode", CAEN_DGTZ_SetExtTriggerInputMode(DeviceHandles[boardIndex], CAEN_DGTZ_TRGMODE_DISABLED));
    break;
  case SYSTEM_TRIGGER_AUTO:
    channelMask = 0;
    for(unsigned int i = 0; i < nChannelsPerDigitizer / 2; i++) {
      channelMask += (cfg->digitizers[boardIndex].channels[2*i].triggerEnable + ((cfg->digitizers[boardIndex].channels[2*i + 1].triggerEnable) << 1)) << (2*i);
    }
    Try("SetChannelSelfTrigger", CAEN_DGTZ_SetChannelSelfTrigger(DeviceHandles[boardIndex], CAEN_DGTZ_TRGMODE_ACQ_ONLY, channelMask));
    Try("SetExtTriggerInputMode", CAEN_DGTZ_SetExtTriggerInputMode(DeviceHandles[boardIndex], CAEN_DGTZ_TRGMODE_ACQ_ONLY));
    break;
  case SYSTEM_TRIGGER_EXTERN:
    Try("SetSWTriggerMode", CAEN_DGTZ_SetSWTriggerMode(DeviceHandles[boardIndex], CAEN_DGTZ_TRGMODE_ACQ_ONLY));
    Try("SetExtTriggerInputMode", CAEN_DGTZ_SetExtTriggerInputMode(DeviceHandles[boardIndex], CAEN_DGTZ_TRGMODE_ACQ_ONLY));
    channelMask = 0;
    for(unsigned int i = 0; i < nChannelsPerDigitizer / 2; i++) {
      channelMask += (cfg->digitizers[boardIndex].channels[2*i].triggerEnable + ((cfg->digitizers[boardIndex].channels[2*i + 1].triggerEnable) << 1)) << (2*i);
    }
    Try("SetChannelSelfTrigger", CAEN_DGTZ_SetChannelSelfTrigger(DeviceHandles[boardIndex], CAEN_DGTZ_TRGMODE_EXTOUT_ONLY, channelMask));
    break;
  default:
    mdaq::Logger::instance().warning("DAQ", TString(Form("Unknown group trigger logic selected for board %d", boardIndex)));
    break;    
  } // switch

}

void mdaq::DAQ::SetTriggerSource(const unsigned int boardIndex) {
  TriggerTypes[boardIndex] = cfg->digitizers[boardIndex].TriggerType;

  // first disable triggers on all channels
  uint32_t channelMask = (1 << nChannelsPerDigitizer) - 1; // all channels
  Try("SetChannelSelfTrigger", CAEN_DGTZ_SetChannelSelfTrigger(DeviceHandles[boardIndex], CAEN_DGTZ_TRGMODE_DISABLED, channelMask));

  // note: in all trigger modes, an external TTL signal will always cause an event.
  //       the choice of trigger mode rather determines what causes a TTL signal output
  //       so be careful with the function generator!

  switch(TriggerTypes[boardIndex]) {
  case SYSTEM_TRIGGER_SOFT:
    triggerAnyBoardSW = true;
    Try("SetSWTriggerMode", CAEN_DGTZ_SetSWTriggerMode(DeviceHandles[boardIndex], CAEN_DGTZ_TRGMODE_EXTOUT_ONLY));
    Try("SetExtTriggerInputMode", CAEN_DGTZ_SetExtTriggerInputMode(DeviceHandles[boardIndex], CAEN_DGTZ_TRGMODE_ACQ_ONLY));
    break;
  case SYSTEM_TRIGGER_NORMAL:
    Try("SetSWTriggerMode", CAEN_DGTZ_SetSWTriggerMode(DeviceHandles[boardIndex], CAEN_DGTZ_TRGMODE_DISABLED));
    Try("SetExtTriggerInputMode", CAEN_DGTZ_SetExtTriggerInputMode(DeviceHandles[boardIndex], CAEN_DGTZ_TRGMODE_ACQ_ONLY));
    Try("SetChannelSelfTrigger", CAEN_DGTZ_SetChannelSelfTrigger(DeviceHandles[boardIndex], CAEN_DGTZ_TRGMODE_EXTOUT_ONLY, ChannelTriggerMask(boardIndex)));

    uint32_t d32;
    ReadRegister(boardIndex, CAEN_DGTZ_FP_TRIGGER_OUT_ENABLE_ADD, &d32);

    if(cfg->digitizers[boardIndex].GroupTriggerLogic == CAEN_DGTZ_LOGIC_OR) {
      // turn off 8-12
      WriteRegister(boardIndex, CAEN_DGTZ_FP_TRIGGER_OUT_ENABLE_ADD, (d32 & 0xFFFFE0FF) | 0x0); 
    }
    else if(cfg->digitizers[boardIndex].GroupTriggerLogic == CAEN_DGTZ_LOGIC_AND) {
      // turn off 8-12; turn on 8
      WriteRegister(boardIndex, CAEN_DGTZ_FP_TRIGGER_OUT_ENABLE_ADD, (d32 & 0xFFFFE0FF) | 0x100);
    }
    else if(cfg->digitizers[boardIndex].GroupTriggerLogic == CAEN_DGTZ_LOGIC_MAJORITY) {
      // turn off 0-12; turn on group mask for 0-7, turn on 9, and set [12:10] to the majority level
      uint32_t d32_new = GroupTriggerMask(boardIndex) + (1 << 9) + (cfg->digitizers[boardIndex].GroupTriggerMajorityLevel << 10);
      WriteRegister(boardIndex, CAEN_DGTZ_FP_TRIGGER_OUT_ENABLE_ADD, (d32 & 0xFFFFE000) | d32_new);
    }
    else {
      mdaq::Logger::instance().warning("DAQ", TString(Form("Unknown group trigger logic selected for board %d", boardIndex)));
    }

    break;
  case SYSTEM_TRIGGER_AUTO:
    Try("SetSWTriggerMode", CAEN_DGTZ_SetSWTriggerMode(DeviceHandles[boardIndex], CAEN_DGTZ_TRGMODE_EXTOUT_ONLY));
    Try("SetExtTriggerInputMode", CAEN_DGTZ_SetExtTriggerInputMode(DeviceHandles[boardIndex], CAEN_DGTZ_TRGMODE_ACQ_ONLY));
    Try("SetChannelSelfTrigger", CAEN_DGTZ_SetChannelSelfTrigger(DeviceHandles[boardIndex], CAEN_DGTZ_TRGMODE_EXTOUT_ONLY, ChannelTriggerMask(boardIndex)));
    break;
  case SYSTEM_TRIGGER_EXTERN:
    Try("SetSWTriggerMode", CAEN_DGTZ_SetSWTriggerMode(DeviceHandles[boardIndex], CAEN_DGTZ_TRGMODE_DISABLED));
    Try("SetExtTriggerInputMode", CAEN_DGTZ_SetExtTriggerInputMode(DeviceHandles[boardIndex], CAEN_DGTZ_TRGMODE_ACQ_ONLY));
    break;
  case SYSTEM_TRIGGER_EXTERN_AND_NORMAL:
    // bfrancis -- think about this one...
    mdaq::Logger::instance().warning("DAQ", "For the moment externalAndNormal trigger mode is not supported.");
  case SYSTEM_TRIGGER_EXTERN_OR_NORMAL:
    // bfrancis -- think about this one...
    mdaq::Logger::instance().warning("DAQ", "For the moment externalOrNormal trigger mode is not supported.");    
  case SYSTEM_TRIGGER_NONE:
    // bfrancis -- for now this is identical to external, but perhaps users will want to force a de-synchronization?
    // need to re-support the disabling of a board again
    Try("SetSWTriggerMode", CAEN_DGTZ_SetSWTriggerMode(DeviceHandles[boardIndex], CAEN_DGTZ_TRGMODE_DISABLED));
    Try("SetExtTriggerInputMode", CAEN_DGTZ_SetExtTriggerInputMode(DeviceHandles[boardIndex], CAEN_DGTZ_TRGMODE_ACQ_ONLY));
    break;
  default:
    mdaq::Logger::instance().warning("DAQ", TString(Form("Unknown trigger mode selected for board at index %d!", boardIndex)));
    break;    
  } // switch
}

void mdaq::DAQ::SetIOLevel(const unsigned int boardIndex) {
  Try("SetIOLevel", CAEN_DGTZ_SetIOLevel(DeviceHandles[boardIndex], (CAEN_DGTZ_IOLevel_t)cfg->digitizers[boardIndex].IOLevel));
}

void mdaq::DAQ::SetChannelDCOffsets(const unsigned int boardIndex) {

  for(int i = 0; i < nChannelsPerDigitizer; i++) {
    Try("SetChannelDCOffset", CAEN_DGTZ_SetChannelDCOffset(DeviceHandles[boardIndex], i, DACValue(cfg->digitizers[boardIndex].channels[i].dcOffset)));
  }

}

void mdaq::DAQ::SetCorrectionLevel(const unsigned int boardIndex) {
  Try("SetSAMCorrectionLevel", CAEN_DGTZ_SetSAMCorrectionLevel(DeviceHandles[boardIndex], (CAEN_DGTZ_SAM_CORRECTION_LEVEL_t)cfg->digitizers[boardIndex].SAMCorrectionLevel));
}

void mdaq::DAQ::SetMaxNumEventsBLT(const unsigned int boardIndex) {
  Try("SetMaxNumEventsBLT", CAEN_DGTZ_SetMaxNumEventsBLT(DeviceHandles[boardIndex], cfg->digitizers[boardIndex].MaxNumEventsBLT));
}

void mdaq::DAQ::SetRecordLength(const unsigned int boardIndex) {
  Try("SetRecordLength", CAEN_DGTZ_SetRecordLength(DeviceHandles[boardIndex], cfg->digitizers[boardIndex].RecordLength));
}

void mdaq::DAQ::SetAnalogMonitorOutput(const unsigned int boardIndex) {

  // Analog Monitor Output mode:
  // 000 = Trigger Majority Mode
  // 001 = Test Mode
  // 010 = reserved
  // 011 = Buffer Occupancy Mode
  // 100 = Voltage Level Mode

  WriteRegister(boardIndex, 0x8144, 0x3); // buffer occupancy
  WriteRegister(boardIndex, 0x81B4, 0x4); // set buffer occupancy gain to 2^4 (x 1V/1024 = 0.976 mV per step)
}

void mdaq::DAQ::SetChannelPairTriggerLogic(const unsigned int boardIndex) {
  for (uint32_t i = 0; i < nGroupsPerDigitizer; i++) {
    Try("SetChannelPairTriggerLogic", CAEN_DGTZ_SetChannelPairTriggerLogic(DeviceHandles[boardIndex],
                                                                           (uint32_t)2*i,
                                                                           (uint32_t)(2*i)+1,
                                                                           (CAEN_DGTZ_TrigerLogic_t)cfg->digitizers[boardIndex].groups[i].logic,
                                                                           cfg->digitizers[boardIndex].groups[i].coincidenceWindow));
  }
}

void mdaq::DAQ::SetGroupEnableMask(const unsigned int boardIndex) {
  uint32_t groupMask = 0;
  for(int i = 0; i < nChannelsPerDigitizer; i++) {
    if(cfg->digitizers[boardIndex].channels[i].enable) {
      groupMask |= (1 << (i / 2));
    }
  }

  Try("SetGroupEnableMask", CAEN_DGTZ_SetGroupEnableMask(DeviceHandles[boardIndex], groupMask));
}

void mdaq::DAQ::SetGroupTriggerLogic(const unsigned int boardIndex) {
  Try("SetGroupTriggerLogic", CAEN_DGTZ_SetTriggerLogic(DeviceHandles[boardIndex], (CAEN_DGTZ_TrigerLogic_t)cfg->digitizers[boardIndex].GroupTriggerLogic, cfg->digitizers[boardIndex].GroupTriggerMajorityLevel));
}

void mdaq::DAQ::SetIRQPolicy(const unsigned int boardIndex) {
  CAEN_DGTZ_EnaDis_t enableIRQ = cfg->digitizers[boardIndex].useIRQ ? CAEN_DGTZ_ENABLE : CAEN_DGTZ_DISABLE;
  Try("SetInterruptConfig", CAEN_DGTZ_SetInterruptConfig(DeviceHandles[boardIndex],
							 enableIRQ,
							 cfg->digitizers[boardIndex].irqLevel,
							 cfg->digitizers[boardIndex].irqStatusId,
							 cfg->digitizers[boardIndex].irqNEvents,
							 (CAEN_DGTZ_IRQMode_t)cfg->digitizers[boardIndex].irqMode));
}

/////////////////////////////////////////////////////////////////////////

bool mdaq::DAQ::IRQWait(const unsigned int boardIndex) {
  return (CAEN_DGTZ_IRQWait(DeviceHandles[boardIndex], cfg->digitizers[boardIndex].irqTimeout) == CAEN_DGTZ_Success);
}

void mdaq::DAQ::RearmInterrupt(const unsigned int boardIndex) {
  Try("RearmInterrupt", CAEN_DGTZ_RearmInterrupt(DeviceHandles[boardIndex]));
}

/////////////////////////////////////////////////////////////////////////

bool mdaq::DAQ::ReadEventBuffer(const unsigned int boardIndex) {

  // if using IRQ, wait
  if(cfg->digitizers[boardIndex].useIRQ) IRQWait(boardIndex);

  bool success = Try("ReadData", CAEN_DGTZ_ReadData(DeviceHandles[boardIndex],
                                                    CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT,
                                                    (char*)ReadoutBuffers[boardIndex],
                                                    &ReadoutBufferSizes[boardIndex])) &&
    ReadoutBufferSizes[boardIndex] > 0;

  if(cfg->digitizers[boardIndex].useIRQ) RearmInterrupt(boardIndex);      

  return success; 

}

uint32_t mdaq::DAQ::GetNumberOfEvents(const unsigned int boardIndex) {
  uint32_t numEvents = 0;
  Try("GetNumEvents", CAEN_DGTZ_GetNumEvents(DeviceHandles[boardIndex], (char*)ReadoutBuffers[boardIndex], ReadoutBufferSizes[boardIndex], &numEvents));
  return numEvents;
}

bool mdaq::DAQ::DecodeEvent(int eventNumber, const unsigned int boardIndex) {

  bool succeeded = Try("GetEventInfo", CAEN_DGTZ_GetEventInfo(DeviceHandles[boardIndex],
							      (char*)ReadoutBuffers[boardIndex],
							      ReadoutBufferSizes[boardIndex],
							      eventNumber,
							      CurrentEventInfos[boardIndex],
							      &eventPtrs[boardIndex]));

  if(succeeded) succeeded = Try("DecodeEvent", CAEN_DGTZ_DecodeEvent(DeviceHandles[boardIndex],
                                                                     eventPtrs[boardIndex],
                                                                     (void**)&CurrentEvents[boardIndex]));

  return succeeded;
}

/////////////////////////////////////////////////////////////////////////

const bool mdaq::DAQ::BoardFailStatus(const unsigned int boardIndex) {
  uint32_t d32;
  ReadRegister(boardIndex, 0x8178, &d32);
  if((d32 & 0xF) != 0) {
    mdaq::Logger::instance().warning("DAQ", "cannot initialize board at index " + TString(Form("%d due to board fail status!", boardIndex)));
    return true;
  }

  return false;
}

const bool mdaq::DAQ::BoardPLLLockLoss(const unsigned int boardIndex) {
  uint32_t d32;
  ReadRegister(boardIndex, 0x8178, &d32);
  if((d32 & 0x10) != 0) {
    mdaq::Logger::instance().warning("DAQ", "The PLL lock has been lost on board index " + TString(Form("%d!!", boardIndex)));
    return true;
  }

  return false;
}

/////////////////////////////////////////////////////////////////////////
// Private methods
/////////////////////////////////////////////////////////////////////////

bool mdaq::DAQ::Try(char const * name, CAEN_DGTZ_ErrorCode code, bool verbose) {
  if(code || verbose) {
    mdaq::Logger::instance().debug("DAQ::Try", TString(name) + ": " + TString(errorMessages.message(code)));
  }
  return (code == CAEN_DGTZ_Success);
}

bool mdaq::DAQ::Require(char const * name, CAEN_DGTZ_ErrorCode code, bool verbose) {
  if(!Try(name, code, verbose)) {
    mdaq::Logger::instance().error("DAQ::Require", "FATAL -- " + TString(name) + ": " + TString(errorMessages.message(code)));
    exit(EXIT_FAILURE);
  }
  return true;
}

/////////////////////////////////////////////////////////////////////////

void mdaq::DAQ::ReadRegister(const unsigned int boardIndex, uint16_t reg, uint32_t * data, bool verbose) {
  CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_ReadRegister(DeviceHandles[boardIndex], reg, data);
  if(ret) {
    mdaq::Logger::instance().warning("DAQ::ReadRegister", "Could not read from register " + TString(Form("%x at board index %d: %s", reg, boardIndex, errorMessages.message(ret).c_str())));
    return;
  }

  if(verbose) mdaq::Logger::instance().debug("DAQ::ReadRegister", "Successfully read from register " + TString(Form("%x at board index %d", reg, boardIndex)));
}

void mdaq::DAQ::WriteRegister(const unsigned int boardIndex, uint16_t reg, uint32_t data, bool verbose) {
  CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_WriteRegister(DeviceHandles[boardIndex], reg, data);
  if(ret) {
    mdaq::Logger::instance().warning("DAQ::WriteRegister", "Could not write to register " + TString(Form("%x at board index %d: %s", reg, boardIndex, errorMessages.message(ret).c_str())));
    return;
  }

  uint32_t value = 0;
  CAEN_DGTZ_ReadRegister(DeviceHandles[boardIndex], reg, &value);
  if(value != data) {
    mdaq::Logger::instance().warning("DAQ::WriteRegister", "Could not write to register " + TString(Form("%x at board index %d: tried to write %d but still reads %d", reg, boardIndex, data, value)));
    return;
  }

  if(verbose) mdaq::Logger::instance().debug("DAQ::WriteRegister", "Successfully wrote " + TString(Form("%d to register %x at board index %d", data, reg, boardIndex)));
}

void mdaq::DAQ::TouchRegister(const unsigned int boardIndex, uint16_t reg) {
  // Similar to WriteRegister, "Touch" is used to write to registers whose values don't change after being written to
  // ie, WriteRegister but without error-checking
  CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_WriteRegister(DeviceHandles[boardIndex], reg, (uint32_t)1);
  if(ret) {
    mdaq::Logger::instance().warning("DAQ::WriteRegister", "Could not write to register " + TString(Form("%x at board index %d: %s", reg, boardIndex, errorMessages.message(ret).c_str())));
    return;
  }
}

/////////////////////////////////////////////////////////////////////////

std::string mdaq::DAQ::GetPreviousConfigurationFile() const {

  // read the most recent run from /var/log/MilliDAQ_RunList.log
  std::fstream fin;
  fin.open("/var/log/MilliDAQ_RunList.log");

  // define cfgPath as the default configuration in case RunList.log can't be open or is empty
  std::string cfgPath = "/home/milliqan/MilliDAQ/config/TripleCoincidence.py";

  int runNumber = 0;
  int subrunNumber = 0;

  // find the most recent entry
  if(fin.is_open()) {
    while(true) {
      fin >> runNumber >> subrunNumber >> cfgPath;
      if(!fin.good()) break;
    }
  }

  return cfgPath;
}

const uint32_t mdaq::DAQ::ChannelTriggerMask(const unsigned int boardIndex) {
  uint32_t channelMask = 0;
  for(unsigned int i = 0; i < nChannelsPerDigitizer / 2; i++) {
    channelMask += (cfg->digitizers[boardIndex].channels[2*i].triggerEnable + ((cfg->digitizers[boardIndex].channels[2*i + 1].triggerEnable) << 1)) << (2*i);    }
  return channelMask;
}

const uint32_t mdaq::DAQ::GroupTriggerMask(const unsigned int boardIndex) {
  uint32_t groupMask = 0;
  for(unsigned int i = 0; i < nGroupsPerDigitizer; i++) {
    if(cfg->digitizers[boardIndex].channels[2*i].triggerEnable || cfg->digitizers[boardIndex].channels[2*i + 1].triggerEnable) {
      groupMask |= (1 << i);
    }
  }
  return groupMask;
}
