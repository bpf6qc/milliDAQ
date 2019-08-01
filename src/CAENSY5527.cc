#include "CAENSY5527.h"

CAENSY5527::CAENSY5527() {

  strcpy(ipAddress, "169.254.216.22");
  strcpy(userName, "admin");
  strcpy(passwd, "milliHV");

  SystemHandle = -1;

  for(unsigned int i = 0; i < 6; i++) slots[i] = 2;
  for(unsigned int i = 6; i < 13; i++) slots[i] = 4;

  // 878box2 = 2.006
  slots[0] = 2;
  channels[0] = 6;

  // L3ML_7725 = 2.007
  slots[1] = 2;
  channels[1] = 7;

  // L3BR_7725 = 2.008
  slots[2] = 2;
  channels[2] = 8;

  // ETbox = 2.009
  slots[3] = 2;
  channels[3] = 9;

  // 878box1 = 2.010
  slots[4] = 2;
  channels[4] = 10;

  // L3TL_878 = 2.011
  slots[5] = 2;
  channels[5] = 11;

  // Slab0_878 = 4.000
  slots[6] = 4;
  channels[6] = 0;

  // ShL1L_878 = 4.001
  slots[7] = 4;
  channels[7] = 1;

  // ShL1R_878 = 4.002
  slots[8] = 4;
  channels[8] = 2;

  // ShL1T_878 = 4.003
  slots[9] = 4;
  channels[9] = 3;

  // ShL2L_878 = 4.004
  slots[10] = 4;
  channels[10] = 4;

  // ShL2T_878 = 4.005
  slots[11] = 4;
  channels[11] = 5;

  // ShL3L_878 = 4.011
  slots[12] = 4;
  channels[12] = 11;


  for(unsigned int i = 0; i < nHVChannels; i++) {
    V0Set[i] = 0;
    I0Set[i] = 0;

    V0Mon[i] = 0;
    I0Mon[i] = 0;

    Pw[i] = false;

    Status[i] = 0x0;
  }

  for(unsigned int i = 0; i < nHVSlots; i++) {
    BdStatus[i] = 0x0;
  }

}

CAENSY5527::~CAENSY5527() {

}

const bool CAENSY5527::Try(char const * name, CAENHVRESULT code, bool verbose) {
  if(code || verbose) {
    mdaq::Logger::instance().debug("CAENSY5527::Try", TString(name) + ": " + TString(CAENHV_GetError(SystemHandle)));
  }
  return (code == CAENHV_OK);
}

const bool CAENSY5527::Require(char const * name, CAENHVRESULT code, bool verbose) {
  if(!Try(name, code, verbose)) {
    mdaq::Logger::instance().error("CAENSY5527::Require", "FATAL -- " + TString(name) + ": " + TString(CAENHV_GetError(SystemHandle)));
    exit(EXIT_FAILURE);
  }
  return true;
}

const bool CAENSY5527::Login() {
  return Try("InitSystem", 
	     CAENHV_InitSystem(systemType, linkType, ipAddress, userName, passwd, &SystemHandle));
}

const bool CAENSY5527::Logout() {
  return Try("DeinitSystem", 
	     CAENHV_DeinitSystem(SystemHandle));
}

const bool CAENSY5527::GetParameters() {

  // CAENHVRESULT CAENHV_GetChParam(
  //	int                  handle,      // in
  //  unsigned short       slot,        // in
  //  const char           *ParName,    // in
  //  unsigned short       ChNum,       // in
  //  const unsigned short *ChList,     // in
  //  void                 *ParValList) // out

  unsigned short channels_slot2[6];
  for(unsigned int i = 0; i < 6; i++) channels_slot2[i] = channels[i];

  float V0Set_slot2[6];
  float I0Set_slot2[6];
  float V0Mon_slot2[6];
  float I0Mon_slot2[6];
  bool  Pw_slot2[6];
  uint32_t Status_slot2[6];

  if(!Try("GetChParam", CAENHV_GetChParam(SystemHandle, 2, "V0Set",  6, channels_slot2, &V0Set_slot2))) return false;
  if(!Try("GetChParam", CAENHV_GetChParam(SystemHandle, 2, "I0Set",  6, channels_slot2, &I0Set_slot2))) return false;
  if(!Try("GetChParam", CAENHV_GetChParam(SystemHandle, 2, "VMon",  6, channels_slot2, &V0Mon_slot2))) return false;
  if(!Try("GetChParam", CAENHV_GetChParam(SystemHandle, 2, "IMon",  6, channels_slot2, &I0Mon_slot2))) return false;
  if(!Try("GetChParam", CAENHV_GetChParam(SystemHandle, 2, "Pw",     6, channels_slot2, &Pw_slot2))) return false;
  if(!Try("GetChParam", CAENHV_GetChParam(SystemHandle, 2, "Status", 6, channels_slot2, &Status_slot2))) return false;

  unsigned short channels_slot4[7];
  for(unsigned int i = 6; i < 13; i++) channels_slot4[i-6] = channels[i];

  float V0Set_slot4[7];
  float I0Set_slot4[7];
  float V0Mon_slot4[7];
  float I0Mon_slot4[7];
  bool  Pw_slot4[7];
  uint32_t Status_slot4[7];

  if(!Try("GetChParam", CAENHV_GetChParam(SystemHandle, 4, "V0Set",  7, channels_slot4, &V0Set_slot4))) return false;
  if(!Try("GetChParam", CAENHV_GetChParam(SystemHandle, 4, "I0Set",  7, channels_slot4, &I0Set_slot4))) return false;
  if(!Try("GetChParam", CAENHV_GetChParam(SystemHandle, 4, "VMon",  7, channels_slot4, &V0Mon_slot4))) return false;
  if(!Try("GetChParam", CAENHV_GetChParam(SystemHandle, 4, "IMon",  7, channels_slot4, &I0Mon_slot4))) return false;
  if(!Try("GetChParam", CAENHV_GetChParam(SystemHandle, 4, "Pw",     7, channels_slot4, &Pw_slot4))) return false;
  if(!Try("GetChParam", CAENHV_GetChParam(SystemHandle, 4, "Status", 7, channels_slot4, &Status_slot4))) return false;

  // CAENHVRESULT CAENHV_GetBdParam(
  //	int                  handle,      // in
  //  unsigned short       slotNum,     // in
  //  const unsigned short *slotList,   // in
  //  const char           *ParName,    // out
  //  void                 *ParValList) // out

  if(!Try("GetBdStatus", CAENHV_GetBdParam(SystemHandle, nHVSlots, slots, "BdStatus", &BdStatus))) return false;

  for(unsigned int i = 0; i < 6; i++) {
    V0Set[i] = V0Set_slot2[i];
    I0Set[i] = I0Set_slot2[i];
    V0Mon[i] = V0Mon_slot2[i];
    I0Mon[i] = I0Mon_slot2[i];
    Pw[i] = Pw_slot2[i];
    Status[i] = Status_slot2[i];
  }

  for(unsigned int i = 6; i < 13; i++) {
    V0Set[i] = V0Set_slot4[i-6];
    I0Set[i] = I0Set_slot4[i-6];
    V0Mon[i] = V0Mon_slot4[i-6];
    I0Mon[i] = I0Mon_slot4[i-6];
    Pw[i] = Pw_slot4[i-6];
    Status[i] = Status_slot4[i-6];
  }

  return true;

}

const bool CAENSY5527::SetParameters(mdaq::DemonstratorConfiguration * cfg) {
  /* durp	
     GetParameters();

     bool settingsChanged = false;

     if(V0Set[R878_channel] != cfg->hvConfig.voltage_R878 || Pw[R878_channel] != cfg->hvConfig.enable_R878) {
     V0Set[R878_channel] = cfg->hvConfig.voltage_R878;
     Pw[R878_channel]    = cfg->hvConfig.enable_R878;
     settingsChanged = true;
     }

     if(V0Set[ET_channel] != cfg->hvConfig.voltage_ET || Pw[ET_channel] != cfg->hvConfig.enable_ET) {
     V0Set[ET_channel] = cfg->hvConfig.voltage_ET;
     Pw[ET_channel]    = cfg->hvConfig.enable_ET;
     settingsChanged = true;
     }

     if(V0Set[R7725_channel] != cfg->hvConfig.voltage_R7725 || Pw[R7725_channel] != cfg->hvConfig.enable_R7725) {
     V0Set[R7725_channel] = cfg->hvConfig.voltage_R7725;
     Pw[R7725_channel]    = cfg->hvConfig.enable_R7725;
     settingsChanged = true;
     }

     if(!settingsChanged) return true;

     // CAENHVRESULT CAENHV_SetChParam(
     //  int                   handle,    // in
     //  unsigned short        slot,      // in
     //  const char            *ParName,  // in
     //  unsigned short        ChNum,     // in
     //  const unsigned short  *ChList,   // in
     //  void                  *ParValue) // in

     if(!Try("SetChParam", CAENHV_SetChParam(SystemHandle, slots[0], "V0Set", nHVChannels, channels, &V0Set))) return false;
     if(!Try("SetChParam", CAENHV_SetChParam(SystemHandle, slots[0], "Pw", nHVChannels, channels, &Pw))) return false;
  */
  return true;
}
