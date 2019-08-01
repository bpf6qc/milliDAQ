#ifndef DAQ_H
#define DAQ_H

#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include <string>
#include <fstream>
#include <thread>

#include "CAENDigitizer.h"
#include "CAENSY5527.h"
#include "ConfigurationReader.h"
#include "DemonstratorConfiguration.h"
#include "DemonstratorConstants.h"
#include "GlobalEvent.h"
#include "Logger.h"
#include "V1743ErrorMessages.h"
#include "Queue.h"

typedef std::chrono::system_clock chrtime;

namespace mdaq {

  class DAQ {
  public:
    DAQ();
    virtual ~DAQ();

    std::string GetConfigurationFile() { return configurationFile; }

    bool ReadConfiguration(std::string filename);
    DemonstratorConfiguration * GetConfiguration() { return cfg; }

    void ConnectToBoards();
    void PrintBoardInfos();

    void PrintRegister(const unsigned int boardIndex, uint32_t reg);
    void PrintAllRegisters(const unsigned int boardIndex);

    void InitializeBoards();

    void StartRun();
    void StopRun();

    void SendSWTriggers();
    void SendSAMPulses(); 

    void CloseDevices();

    // Read the event buffers for all digitizers, and push all events read
    // onto each digitizer's queue
    void Read(Queue * queues[nDigitizers]);
    void Read(Queue * queues[nDigitizers], uint32_t * val);

    // Enable and disable the internal trigger clock to be put out on the TRG-OUT connection
    // Used only by Synchronize to choose PLL firmware delay
    void SetOutputClockSignal(const unsigned int boardIndex);
    void UnsetOutputClockSignal(const unsigned int boardIndex);

    // Methods to force the PLL lock and reload the PLL configuration
    // Expert use only; CAEN's synchronized WaveDemo did not call these
    void ForceClockSync(const unsigned int boardIndex);
    void ForcePLLReload(const unsigned int boardIndex);

  protected:
    void ConnectToBoard(const unsigned int boardIndex);
    void PrintBoardInfo(const unsigned int boardIndex);

    void InitializeBoard(const unsigned int boardIndex);

    void MallocReadoutBuffers();

    void ProgramSynchronization(); // mar18
    void CAEN_ProgramSynchronization();

    //////////////////////////////////////////////////////////////////////////
    // Wrapper functions for CAENDigitizer library methods
    // Called in InitializeBoard
    //////////////////////////////////////////////////////////////////////////
    void SetTriggerDelay           (const unsigned int boardIndex);
    void SetPulserParameters       (const unsigned int boardIndex);
    void SetSamplingFrequency      (const unsigned int boardIndex);
    void SetTriggerThresholds      (const unsigned int boardIndex);
    void SetTriggerCountVeto       (const unsigned int boardIndex);
    void SetTriggerPolarities      (const unsigned int boardIndex);
    void SetTriggerSource          (const unsigned int boardIndex);
    void CAEN_SetTriggerSource     (const unsigned int boardIndex);
    void SetIOLevel                (const unsigned int boardIndex);
    void SetChannelDCOffsets       (const unsigned int boardIndex);
    void SetCorrectionLevel        (const unsigned int boardIndex);
    void SetMaxNumEventsBLT        (const unsigned int boardIndex);
    void SetRecordLength           (const unsigned int boardIndex);
    void SetAnalogMonitorOutput    (const unsigned int boardIndex);
    void SetChannelPairTriggerLogic(const unsigned int boardIndex);
    void SetGroupEnableMask        (const unsigned int boardIndex);
    void SetGroupTriggerLogic      (const unsigned int boardIndex);
    void SetIRQPolicy              (const unsigned int boardIndex);

    // IRQ methods called in Read
    bool IRQWait(const unsigned int boardIndex);
    void RearmInterrupt(const unsigned int boardIndex);

    // Data-handling methods called in Read
    bool ReadEventBuffer(const unsigned int boardIndex);
    uint32_t GetNumberOfEvents(const unsigned int boardIndex);
    bool DecodeEvent(int eventNumber, const unsigned int boardIndex);

    // Board failure state checks
    const bool BoardFailStatus(const unsigned int boardIndex);
    const bool BoardPLLLockLoss(const unsigned int boardIndex);

  private:
    //////////////////////////////////////////////////////////////////////////
    // Wrappers for all CAENDigitizer library calls
    // Prints log messages (or quits for Require) in case of error
    //////////////////////////////////////////////////////////////////////////
    bool Try(char const * name, CAEN_DGTZ_ErrorCode code, bool verbose = false);
    bool Require(char const * name, CAEN_DGTZ_ErrorCode code, bool verbose = false);

    //////////////////////////////////////////////////////////////////////////
    // Wrappers for accessing V1743 registers
    // Only use TouchRegister for registers where data is meaningless, ie "a write access" causes something to happen
    //////////////////////////////////////////////////////////////////////////
    void ReadRegister(const unsigned int boardIndex, uint16_t reg, uint32_t * data, bool verbose = false);
    void WriteRegister(const unsigned int boardIndex, uint16_t reg, uint32_t data, bool verbose = false);
    void TouchRegister(const unsigned int boardIndex, uint16_t reg);

    // Convert a value in [Volts] to the internal DAC value
    int DACValue(double value) { return (int)((MAX_DAC_RAW_VALUE - value) / (MAX_DAC_RAW_VALUE - MIN_DAC_RAW_VALUE) * 0xFFFF); };

    // Read /var/log/MilliDAQ_RunList.log to find the last-used config
    std::string GetPreviousConfigurationFile() const;

    // Build a bitmask for the channels/groups with triggers enabled
    const uint32_t ChannelTriggerMask(const unsigned int boardIndex);
    const uint32_t GroupTriggerMask(const unsigned int boardIndex);

    //////////////////////////////////////////////////////////////////////////
    // Class members
    //////////////////////////////////////////////////////////////////////////

    int DeviceHandles[nDigitizers];

    TriggerType_t TriggerTypes[nDigitizers];

    uint32_t * ReadoutBuffers[nDigitizers];
    uint32_t ReadoutBufferSizes[nDigitizers];
    uint32_t MaxReadoutBufferSizes[nDigitizers];
    uint32_t DPPReadoutBufferSizes[nDigitizers];
    char * eventPtrs[nDigitizers];

    CAEN_DGTZ_BoardInfo_t boardInfos[nDigitizers];

    CAEN_DGTZ_X743_EVENT_t * CurrentEvents[nDigitizers];
    CAEN_DGTZ_DPP_X743_Event_t * DPPEvents[nDigitizers][nChannelsPerDigitizer];

    CAEN_DGTZ_EventInfo_t * CurrentEventInfos[nDigitizers];

    DemonstratorConfiguration * cfg;

    chrtime::time_point connectionTimes[nDigitizers];
    chrtime::time_point runInitialTime;

    std::string configurationFile;

    const mdaq::V1743ErrorMessages errorMessages;

    const std::string xmlSuffix = ".xml";
    const std::string pythonSuffix = ".py";

    bool triggerAnyBoardSW;  // if any board has a software trigger enabled
    bool SAMPulseAnyBoardSW; // if any board has a software source for SAM pulses

    unsigned int nDigitizersEnabled;
  };

}

#endif

