#ifndef V1743_CONFIGURATION_H
#define V1743_CONFIGURATION_H

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>

#include "DemonstratorConstants.h"
#include "Logger.h"

#include "TObject.h"

namespace mdaq {

  typedef enum {
    SYSTEM_TRIGGER_SOFT,
    SYSTEM_TRIGGER_NORMAL,
    SYSTEM_TRIGGER_AUTO,
    SYSTEM_TRIGGER_EXTERN,
    SYSTEM_TRIGGER_EXTERN_AND_NORMAL,
    SYSTEM_TRIGGER_EXTERN_OR_NORMAL,
    SYSTEM_TRIGGER_NONE,
    SYSTEM_NUM_TRIGGER_TYPES
  } TriggerType_t;

  struct GroupConfiguration {
    unsigned int logic; // CAEN_DGTZ_TrigerLogic_t
    uint16_t coincidenceWindow;
    uint8_t triggerDelay;
  };

  struct ChannelConfiguration {
    bool enable;

    bool testPulseEnable;
    unsigned short testPulsePattern;
    unsigned int testPulseSource; // CAEN_DGTZ_SAMPulseSourceType_t

    bool triggerEnable;
    double triggerThreshold;
    long triggerPolarity; // CAEN_DGTZ_TriggerPolarity_t

    double dcOffset;

    unsigned int chargeStartCell;
    unsigned short chargeLength;
    long enableChargeThreshold; // CAEN_DGTZ_EnaDis_t
    float chargeThreshold;
  };

  class V1743Configuration : public TObject {
  public:
    V1743Configuration();
    V1743Configuration(const V1743Configuration& cfg);
    virtual ~V1743Configuration();

    V1743Configuration& operator=(V1743Configuration& cfg);

    void PrintConfiguration() const;
    void LogConfiguration() const;

    unsigned int boardIndex;
    bool boardEnable;

    // connection parameters
    long LinkType; // CAEN_DGTZ_ConnectionType
    int LinkNum;
    int ConetNode;
    uint32_t VMEBaseAddress;

    // IRQ parameters
    bool useIRQ;
    uint8_t irqLevel;
    uint32_t irqStatusId;
    uint16_t irqNEvents;
    long irqMode; // CAEN_DGTZ_IRQMode_t
    uint32_t irqTimeout;

    // charge mode
    bool useChargeMode;
    long suppressChargeBaseline; // CAEN_DGTZ_EnaDis_t

    unsigned int SAMCorrectionLevel; // CAEN_DGTZ_SAM_CORRECTION_LEVEL_t

    long SAMFrequency; // CAEN_DGTZ_SAMFrequency_t
    float secondsPerSample;

    uint32_t RecordLength;

    TriggerType_t TriggerType;

    long IOLevel; // CAEN_DGTZ_IOLevel_t

    uint32_t MaxNumEventsBLT;

    unsigned int GroupTriggerLogic; // CAEN_DGTZ_TrigerLogic_t
    uint32_t GroupTriggerMajorityLevel;

    bool useTriggerCountVeto;
    uint32_t triggerCountVetoWindow;

    std::vector<GroupConfiguration> groups;
    std::vector<ChannelConfiguration> channels;

    ClassDef(mdaq::V1743Configuration, 2)
      };

}

#endif
