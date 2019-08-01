#ifndef CAEN_SY5527_H
#define CAEN_SY5527_H

// need this to use CAENHVWrapper.h
#define UNIX

#define nHVSlots 2

#include "CAENHVWrapper.h"

#include "DemonstratorConfiguration.h"
#include "Logger.h"

// bfrancis -- completely redo with new HV cabling

class CAENSY5527 {
 public:
  CAENSY5527();
  virtual ~CAENSY5527();

  const bool Login();
  const bool Logout();

  const bool GetParameters();
  const bool SetParameters(mdaq::DemonstratorConfiguration * cfg);

  float V0Set[nHVChannels];
  float I0Set[nHVChannels];

  float V0Mon[nHVChannels];
  float I0Mon[nHVChannels];

  bool Pw[nHVChannels];

  uint32_t Status[nHVChannels];

  uint32_t BdStatus[nHVSlots];

  unsigned short slots[nHVChannels];
  unsigned short channels[nHVChannels];

 private:

  const bool Require(char const * name, CAENHVRESULT code, bool verbose = false);
  const bool Try(char const * name, CAENHVRESULT code, bool verbose = false);

  int SystemHandle;

  const CAENHV_SYSTEM_TYPE_t systemType = (CAENHV_SYSTEM_TYPE_t)SY5527;
  const int linkType = LINKTYPE_TCPIP;
	
  char ipAddress[30];
  char userName[30];
  char passwd[30];
};

#endif
