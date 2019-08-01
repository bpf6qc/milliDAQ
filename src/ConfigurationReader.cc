#include "ConfigurationReader.h"

#include "Logger.h"

#include <iostream>

#ifndef NO_BOOST_INSTALL
using namespace boost;
using namespace property_tree;
#endif

using namespace std;

namespace mdaq {

  // in the Python/C API's concrete data layer, only a few python-->C castings are available. we'll use:
  // long PyInt_AsLong(PyObject*)
  // double PyFloat_AsDouble(PyObject*)
  // and use C castings to do the conversions
#ifndef NO_PYTHON_API_INSTALL
  template<> long 
  ConfigurationReader::getParameter<long>(PyObject * obj, const char * name) {
    pValue = PyObject_GetAttrString(obj, name);
    if(!pValue) PyErr_Print();
    long value = PyLong_AsLong(pValue);
    Py_DECREF(pValue);
    return value;
  }

  template<> bool 
  ConfigurationReader::getParameter<bool>(PyObject * obj, const char * name) {
    return (bool)getParameter<long>(obj, name);
  }

  template<> int 
  ConfigurationReader::getParameter<int>(PyObject * obj, const char * name) {
    return (int)getParameter<long>(obj, name);
  }

  template<> unsigned short // uint16_t
  ConfigurationReader::getParameter<unsigned short>(PyObject * obj, const char * name) {
    return (unsigned short)getParameter<long>(obj, name);
  }

  template<> unsigned int // uint32_t
  ConfigurationReader::getParameter<unsigned int>(PyObject * obj, const char * name) {
    return (unsigned int)getParameter<long>(obj, name);
  }

  template<> TriggerType_t
  ConfigurationReader::getParameter<TriggerType_t>(PyObject * obj, const char * name) {
    return (TriggerType_t)getParameter<long>(obj, name);
  }

  template<> uint8_t 
  ConfigurationReader::getParameter<uint8_t>(PyObject * obj, const char * name) {
    return (uint8_t)getParameter<long>(obj, name);
  }

  template<> double
  ConfigurationReader::getParameter<double>(PyObject * obj, const char * name) {
    pValue = PyObject_GetAttrString(obj, name);
    if(!pValue) PyErr_Print();
    double value = PyFloat_AsDouble(pValue);
    Py_DECREF(pValue);
    return value;
  }

  template<> float
  ConfigurationReader::getParameter<float>(PyObject * obj, const char * name) {
    return (float)getParameter<double>(obj, name);
  }

  const bool ConfigurationReader::callMethod(PyObject * obj, const std::string name) {
    char * cname = new char[name.size() + 1];
    std::strncpy(cname, name.c_str(), name.size());

    PyObject * pMethod = PyObject_CallMethod(obj, cname, NULL);
    if(!pMethod) PyErr_Print();

    bool value = PyInt_AsLong(pMethod);
    Py_DECREF(pMethod);
    return value;
  }
#endif // NO_PYTHON_API_INSTALL

  const bool ConfigurationReader::ParsePython(DemonstratorConfiguration &configuration, const std::string filename) {
#ifndef NO_PYTHON_API_INSTALL

    // Initialize the Python Interpreter
    Py_Initialize();
    //PySys_SetArgv(0, NULL);

    std::string rawname = filename;
    rawname = rawname.substr(filename.find_last_of("/") + 1);
    rawname = rawname.substr(0, rawname.size() - 3);

    const char * cfilename = rawname.c_str();

    // Build the name object for the module name ("File" for File.py")
    pName = PyString_FromString(cfilename); 
    if(IsError(pName)) return false;

    // Load the module object (import File)
    pModule = PyImport_Import(pName);
    if(IsError(pModule)) return false;

    // Borrow reference to the module's dict (File.__dict__)
    pDict = PyModule_GetDict(pModule);
    if(IsError(pDict)) return false;

    // Borrow reference to the cfg object (File.__dict__["cfg"])
    pConfig = PyDict_GetItemString(pDict, "cfg");
    if(IsError(pConfig)) return false;

    // Check if the entire configuration is valid before continuing; all parameters should make sense
    // cfg.IsValid()
    if(!callMethod(pConfig, "IsValid")) {
      Logger::instance().warning("ConfigurationReader::Parse", "Invalid configuration requested for parsing.");
      return false;
    }

    // Create a new configuration object and start reading values
    DemonstratorConfiguration cfg;

    cfg.TFileCompressionEnabled = getParameter<bool>(pConfig, "TFileCompressionEnabled");
		
    cfg.HLTEnabled      = getParameter<bool>        (pConfig, "HLTEnabled");
    cfg.HLTMaxAmplitude = getParameter<float>       (pConfig, "HLTMaxAmplitude");
    cfg.HLTNumRequired  = getParameter<unsigned int>(pConfig, "HLTNumRequired");

    cfg.CosmicPrescaleEnabled      = getParameter<bool>        (pConfig, "CosmicPrescaleEnabled");
    cfg.CosmicPrescaleMaxAmplitude = getParameter<float>       (pConfig, "CosmicPrescaleMaxAmplitude");
    cfg.CosmicPrescaleNumRequired  = getParameter<unsigned int>(pConfig, "CosmicPrescaleNumRequired");
    cfg.CosmicPrescalePS           = getParameter<unsigned int>(pConfig, "CosmicPrescalePS");

    // Borrow reference to cfg.Digitizers
    pDigitizers = PyObject_GetAttrString(pConfig, "Digitizers");
    if(IsError(pDigitizers)) return false;

    // loop over cfg.Digitizers
    for(Py_ssize_t iDigitizer = 0; iDigitizer < PyList_Size(pDigitizers); iDigitizer++) {

      // Borrow reference to cfg.Digitizers[iDigitizer]
      pDigitizer = PyList_GetItem(pDigitizers, iDigitizer);

      // Borrow references to all classes in cfg.Digitizers[iDigitizer]
      pConnection        = PyObject_GetAttrString(pDigitizer, "Connection");
      pIRQPolicy         = PyObject_GetAttrString(pDigitizer, "IRQPolicy");
      pChargeMode        = PyObject_GetAttrString(pDigitizer, "ChargeMode");
      pSAMCorrection     = PyObject_GetAttrString(pDigitizer, "SAMCorrection");
      pTriggerType       = PyObject_GetAttrString(pDigitizer, "TriggerType");
      pIOLevel           = PyObject_GetAttrString(pDigitizer, "IOLevel");
      pGroupTriggerLogic = PyObject_GetAttrString(pDigitizer, "GroupTriggerLogic");
      pTriggerCountVeto  = PyObject_GetAttrString(pDigitizer, "TriggerCountVeto");

      // Create a new V1743 configuration object and start reading values
      V1743Configuration dgtz;

      dgtz.boardIndex  = getParameter<unsigned int>(pDigitizer, "boardIndex");
      dgtz.boardEnable = getParameter<bool>        (pDigitizer, "boardEnable");

      dgtz.LinkType       = getParameter<long>     (pConnection, "type");
      dgtz.LinkNum        = getParameter<int>      (pConnection, "linkNum");
      dgtz.ConetNode      = getParameter<int>      (pConnection, "conetNode");
      dgtz.VMEBaseAddress = getParameter<uint32_t> (pConnection, "vmeBaseAddress");

      dgtz.useIRQ      = getParameter<bool>    (pIRQPolicy, "use");
      dgtz.irqLevel    = getParameter<uint8_t> (pIRQPolicy, "level");
      dgtz.irqStatusId = getParameter<uint32_t>(pIRQPolicy, "status_id");
      dgtz.irqNEvents  = getParameter<uint16_t>(pIRQPolicy, "nevents");
      dgtz.irqMode     = getParameter<long>    (pIRQPolicy, "mode");
      dgtz.irqTimeout  = getParameter<uint32_t>(pIRQPolicy, "timeout");

      dgtz.useChargeMode          = getParameter<bool>(pChargeMode, "use");
      dgtz.suppressChargeBaseline = getParameter<long>(pChargeMode, "suppressBaseline");

      dgtz.SAMCorrectionLevel = getParameter<unsigned int>(pSAMCorrection, "level");

      double samFreq = getParameter<double>(pDigitizer, "SAMFrequency");

      if(samFreq == 3.2) {
	dgtz.SAMFrequency = 0L;
	dgtz.secondsPerSample = 1. / (3.2e9);
      }
      else if(samFreq == 1.6) {
	dgtz.SAMFrequency = 1L;
	dgtz.secondsPerSample = 1. / (1.6e9);
      }
      else if(samFreq == 0.8) {
	dgtz.SAMFrequency = 2L;
	dgtz.secondsPerSample = 1. / (0.8e9);
      }
      else if(samFreq == 0.4) {
	dgtz.SAMFrequency = 3L;
	dgtz.secondsPerSample = 1. / (0.4e9);
      }

      dgtz.RecordLength = getParameter<uint32_t>(pDigitizer, "RecordLength");

      dgtz.TriggerType = getParameter<TriggerType_t>(pTriggerType, "type");

      dgtz.IOLevel = getParameter<long>(pIOLevel, "type");

      dgtz.MaxNumEventsBLT = getParameter<uint32_t>(pDigitizer, "MaxNumEventsBLT");

      dgtz.GroupTriggerLogic         = getParameter<unsigned int>(pGroupTriggerLogic, "logic");
      dgtz.GroupTriggerMajorityLevel = getParameter<uint32_t>    (pGroupTriggerLogic, "majorityLevel");

      dgtz.useTriggerCountVeto    = getParameter<bool>    (pTriggerCountVeto, "enabled");
      dgtz.triggerCountVetoWindow = getParameter<uint32_t>(pTriggerCountVeto, "vetoWindow");

      // Borrow reference to cfg.Digitizers[iDigitizer].groups
      pGroups = PyObject_GetAttrString(pDigitizer, "groups");
      if(IsError(pGroups)) return false;

      // loop over cfg.Digitizers[iDigitizer].Groups
      for(Py_ssize_t iGroup = 0; iGroup < PyList_Size(pGroups); iGroup++) {
	// Borrow reference to cfg.Digitizers[iDigitizer].Groups[iGroup]
	pGroup = PyList_GetItem(pGroups, iGroup);

	dgtz.groups[iGroup].logic             = getParameter<unsigned int>(pGroup, "logic");
	dgtz.groups[iGroup].coincidenceWindow = getParameter<uint16_t>    (pGroup, "coincidenceWindow");
	dgtz.groups[iGroup].triggerDelay      = getParameter<uint8_t>     (pGroup, "triggerDelay");
      }

      // Borrow reference to cfg.Digitizers[iDigitizer].channels
      pChannels = PyObject_GetAttrString(pDigitizer, "channels");
      if(IsError(pChannels)) return false;

      // loop over cfg.Digitizers[iDigitizer].Channels
      for(Py_ssize_t iChannel = 0; iChannel < PyList_Size(pChannels); iChannel++) {
	// Borrow reference to cfg.Digitizers[iDigitizer].Channels[iChannel]
	pChannel = PyList_GetItem(pChannels, iChannel);

	dgtz.channels[iChannel].enable = getParameter<bool>(pChannel, "enable");
				
	dgtz.channels[iChannel].testPulseEnable  = getParameter<bool>          (pChannel, "testPulseEnable");
	dgtz.channels[iChannel].testPulsePattern = getParameter<unsigned short>(pChannel, "testPulsePattern");
	dgtz.channels[iChannel].testPulseSource  = getParameter<unsigned int>  (pChannel, "testPulseSource");

	dgtz.channels[iChannel].triggerEnable    = getParameter<bool>  (pChannel, "triggerEnable");
	dgtz.channels[iChannel].triggerThreshold = getParameter<double>(pChannel, "triggerThreshold");
	dgtz.channels[iChannel].triggerPolarity  = getParameter<long>  (pChannel, "triggerPolarity");

	dgtz.channels[iChannel].dcOffset = getParameter<double>(pChannel, "dcOffset");

	dgtz.channels[iChannel].chargeStartCell       = getParameter<unsigned int>  (pChannel, "chargeStartCell");
	dgtz.channels[iChannel].chargeLength          = getParameter<unsigned short>(pChannel, "chargeLength");
	dgtz.channels[iChannel].enableChargeThreshold = getParameter<long>    (pChannel, "enableChargeThreshold");			
	dgtz.channels[iChannel].chargeThreshold       = getParameter<float>   (pChannel, "chargeThreshold");
      }

      cfg.digitizers[iDigitizer] = dgtz;

    } // end loop over cfg.Digitizers

    // Borrow reference to cfg.HighVoltage
    pHighVoltage = PyObject_GetAttrString(pConfig, "HighVoltage");
    if(IsError(pHighVoltage)) return false;

    // Borrow reference to cfg.HighVoltage.voltages
    pVoltageDict = PyObject_GetAttrString(pHighVoltage, "voltages");
    if(IsError(pVoltageDict)) return false;

    for(unsigned int i = 0; i < nHVChannels; i++) {
      pValue = PyDict_GetItemString(pVoltageDict, cfg.hvConfig.names[i].c_str());
      if(!pValue) {
	PyErr_Print();
	return false;
      }
      cfg.hvConfig.voltages[i] = PyFloat_AsDouble(pValue);
      Py_DECREF(pValue);
    }

    // Borrow reference to cfg.HighVoltage.enabled
    pEnabledDict = PyObject_GetAttrString(pHighVoltage, "enabled");
    if(IsError(pEnabledDict)) return false;

    for(unsigned int i = 0; i < nHVChannels; i++) {
      pValue = PyDict_GetItemString(pEnabledDict, cfg.hvConfig.names[i].c_str());
      if(!pValue) {
	PyErr_Print();
	return false;
      }
      cfg.hvConfig.enabled[i] = (bool)PyLong_AsLong(pValue);
      Py_DECREF(pValue);
    }

    cfg.SetCurrentConfigFile(filename);

    // set the configuration object and return success
    configuration = cfg;

    Logger::instance().info("ConfigurationReader::ParsePython", "Successfully read " + TString(filename));

    // Clean up
    Py_DECREF(pModule);
    Py_DECREF(pName);

    // Finish the Python Interpreter
    Py_Finalize();

    return true;
#else // NO_PYTHON_API_INSTALL
    Logger::instance().warning("DemonstratorConfiguration::ReadPython", "This function is not implemented without the Python/C API.");
    return false;
#endif
  }

  const bool ConfigurationReader::ParseXML(DemonstratorConfiguration &configuration, const std::string filename) {
#ifndef NO_BOOST_INSTALL
    ptree pt;

    try {
      read_xml(filename, pt);
    }
    catch(const ptree_error& e) {
      Logger::instance().warning("DemonstratorConfiguration::ParseXML", TString(e.what()));
      return false;
    }

    // Set values in a new object, so that in case something is bad
    // the original is untouched
    DemonstratorConfiguration cfg;

    string str_;

    for(const auto& i : pt.get_child("config.general")) {
      string name;
      ptree sub_pt;
      tie(name, sub_pt) = i;

      if(name == "TFileCompression") {
	cfg.TFileCompressionEnabled = sub_pt.get<bool>("<xmlattr>.enabled");
      }

      if(name == "HighLevelTrigger") {
	cfg.HLTEnabled = sub_pt.get<bool>("<xmlattr>.enabled");
	cfg.HLTMaxAmplitude = sub_pt.get<float>("<xmlattr>.maxAmplitude");
	cfg.HLTNumRequired = sub_pt.get<unsigned int>("<xmlattr>.numRequired");
      }

      if(name == "CosmicPrescale") {
	cfg.CosmicPrescaleEnabled = sub_pt.get<bool>("<xmlattr>.enabled");
	cfg.CosmicPrescaleMaxAmplitude = sub_pt.get<float>("<xmlattr>.maxAmplitude");
	cfg.CosmicPrescaleNumRequired = sub_pt.get<unsigned int>("<xmlattr>.numRequired");
	cfg.CosmicPrescalePS = sub_pt.get<unsigned int>("<xmlattr>.prescale");
	if(cfg.CosmicPrescalePS == 0) {
	  Logger::instance().warning("ConfigurationReader::ParseXML", "Invalid cosmic prescale value (can't be zero!) read from " + TString(filename));
	  return false;
	}

      }

    }

    for(const auto& iBoard : pt.get_child("config.boards")) {
      string boardTagName;
      ptree board_pt;
      tie(boardTagName, board_pt) = iBoard;
      if(boardTagName != "board") continue;

      V1743Configuration dgtz;

      dgtz.boardEnable = board_pt.get<bool>("<xmlattr>.enable");
      dgtz.boardIndex  = board_pt.get<unsigned int>("<xmlattr>.index");

      for(const auto& i : board_pt.get_child("common")) {
	string name;
	ptree sub_pt;
	tie(name, sub_pt) = i;

	if(name == "connectionType") {
	  dgtz.LinkNum        = sub_pt.get<int>("<xmlattr>.linkNum");
	  dgtz.ConetNode      = sub_pt.get<int>("<xmlattr>.conetNode");
	  dgtz.VMEBaseAddress = sub_pt.get<uint32_t>("<xmlattr>.vmeBaseAddress");

	  str_ = sub_pt.get<string>("<xmlattr>.type"); // CAEN_DGTZ_ConnectionType
	  if(str_ == "USB")              dgtz.LinkType = 0L;
	  else if(str_ == "opticalLink") dgtz.LinkType = 1L;
	  else {
	    Logger::instance().warning("DemonstratorConfiguration::ParseXML", "Invalid connection type read from " + TString(filename));
	    return false;
	  }
	}

	if(name == "IRQ") {
	  dgtz.useIRQ      = sub_pt.get<bool>("<xmlattr>.use");
	  dgtz.irqLevel    = sub_pt.get<uint8_t>("<xmlattr>.level");
	  dgtz.irqStatusId = sub_pt.get<uint32_t>("<xmlattr>.status_id");
	  dgtz.irqNEvents  = sub_pt.get<uint16_t>("<xmlattr>.nevents");
	  dgtz.irqTimeout  = sub_pt.get<uint32_t>("<xmlattr>.timeout");

	  str_ = sub_pt.get<string>("<xmlattr>.mode"); // CAEN_DGTZ_IRQMode_t
	  if(str_ == "RORA")      dgtz.irqMode = 0L;
	  else if(str_ == "ROAK") dgtz.irqMode = 1L;
	  else {
	    Logger::instance().warning("DemonstratorConfiguration::ParseXML", "Invalid IRQ mode read from " + TString(filename));
	    return false;
	  }
	}

	if(name == "chargeMode") {
	  dgtz.useChargeMode = sub_pt.get<bool>("<xmlattr>.use");
	  dgtz.suppressChargeBaseline = sub_pt.get<long>("<xmlattr>.suppressBaseline"); // CAEN_DGTZ_EnaDis_t
	}

	if(name == "samCorrection") {
	  str_ = sub_pt.get<string>("<xmlattr>.level"); // CAEN_DGTZ_SAM_CORRECTION_LEVEL_t
	  if(str_ == "all")                     dgtz.SAMCorrectionLevel = 3;
	  else if(str_ == "INL")                dgtz.SAMCorrectionLevel = 2;
	  else if(str_ == "pedestalOnly")       dgtz.SAMCorrectionLevel = 1;
	  else if(str_ == "correctionDisabled") dgtz.SAMCorrectionLevel = 0;
	  else {
	    Logger::instance().warning("DemonstratorConfiguration::ParseXML", "Invalid SAM correction level read from " + TString(filename));
	    return false;
	  }
	}

	if(name == "samFrequency") {
	  str_ = sub_pt.get<string>("<xmlattr>.rate"); // CAEN_DGTZ_SAMFrequency_t
	  if(str_ == "3.2") {
	    dgtz.SAMFrequency = 0L;
	    dgtz.secondsPerSample = 1. / (3.2e9);
	  }
	  else if(str_ == "1.6") {
	    dgtz.SAMFrequency = 1L;
	    dgtz.secondsPerSample = 1. / (1.6e9);
	  }
	  else if(str_ == "0.8") {
	    dgtz.SAMFrequency = 2L;
	    dgtz.secondsPerSample = 1. / (0.8e9);
	  }
	  else if(str_ == "0.4") {
	    dgtz.SAMFrequency = 3L;
	    dgtz.secondsPerSample = 1. / (0.4e9);
	  }
	  else {
	    Logger::instance().warning("DemonstratorConfiguration::ParseXML", "Invalid SAM frequency read from " + TString(filename));
	    return false;
	  }
	}

	if(name == "recordLength") {
	  dgtz.RecordLength = sub_pt.get<unsigned int>("<xmlattr>.length");
	  if(dgtz.RecordLength % 16 != 0 || dgtz.RecordLength < 4 * 16 || dgtz.RecordLength <= 0) {
	    Logger::instance().warning("DemonstratorConfiguration::ParseXML", "Invalid record length read from " + TString(filename));
	    return false;
	  }
	}

	if(name == "triggerType") {
	  str_ = sub_pt.get<string>("<xmlattr>.type");
	  if(str_ == "software")               dgtz.TriggerType = SYSTEM_TRIGGER_SOFT;
	  else if(str_ == "normal")            dgtz.TriggerType = SYSTEM_TRIGGER_NORMAL;
	  else if(str_ == "auto")              dgtz.TriggerType = SYSTEM_TRIGGER_AUTO;
	  else if(str_ == "external")          dgtz.TriggerType = SYSTEM_TRIGGER_EXTERN;
	  else if(str_ == "externalAndNormal") dgtz.TriggerType = SYSTEM_TRIGGER_EXTERN_AND_NORMAL;
	  else {
	    Logger::instance().warning("DemonstratorConfiguration::ParseXML", "Invalid trigger type read from " + TString(filename));
	    return false;
	  }
	}

	if(name == "ioLevel") {
	  str_ = sub_pt.get<string>("<xmlattr>.type"); // CAEN_DGTZ_IOLevel_t
	  if(str_ == "TTL")      dgtz.IOLevel = 1L;
	  else if(str_ == "NIM") dgtz.IOLevel = 0L;
	  else {
	    Logger::instance().warning("DemonstratorConfiguration::ParseXML", "Invalid IO level read from " + TString(filename));
	    return false;
	  }
	}

	if(name == "maxNumEventsBLT") {
	  dgtz.MaxNumEventsBLT = sub_pt.get<uint32_t>("<xmlattr>.number");
	}

	if(name == "groupTriggerLogic") {
	  dgtz.GroupTriggerMajorityLevel = sub_pt.get<uint32_t>("<xmlattr>.majorityLevel");

	  str_ = sub_pt.get<string>("<xmlattr>.logic"); // CAEN_DGTZ_TrigerLogic_t
	  if(str_ == "or")            dgtz.GroupTriggerLogic = 0;
	  else if(str_ == "and")      dgtz.GroupTriggerLogic = 1;
	  else if(str_ == "majority") dgtz.GroupTriggerLogic = 2;
	  else {
	    Logger::instance().warning("DemonstratorConfiguration::ParseXML", "Invalid group trigger logic read from " + TString(filename));
	    return false;
	  }
	}

	if(name == "triggerCountVeto") {
	  dgtz.useTriggerCountVeto = sub_pt.get<bool>("<xmlattr>.enabled");
	  dgtz.triggerCountVetoWindow = sub_pt.get<uint32_t>("<xmlattr>.vetoWindow");
	}

      } // common block

      for(const auto& i : pt.get_child("config.groups")) {
	string name;
	ptree sub_pt;
	tie(name, sub_pt) = i;

	if(name != "group") continue;

	int groupNumber = sub_pt.get<int>("<xmlattr>.number");
	if(groupNumber > nGroupsPerDigitizer) continue;

	uint16_t gate = sub_pt.get<unsigned short>("<xmlattr>.coincidenceWindow");
	if(gate < 15 || gate > 1275) {
	  Logger::instance().warning("ConfigurationReader::ParseXML", "Invalid coincidenceWindow (group " + TString(Form("%d", groupNumber)) + "), must be within 15-1275ns.");
	  return false;
	}
	if((gate % 5) != 0) {
	  Logger::instance().warning("ConfigurationReader::ParseXML", "NOTE: coincidenceWindow (group " + TString(Form("%d", groupNumber)) + "), not a multiple of 5ns. Not fatal, but will be rounded!!");
	}

	uint8_t trigDelay = sub_pt.get<double>("<xmlattr>.triggerDelay");

	unsigned int logic_;
	str_ = sub_pt.get<string>("<xmlattr>.logic");
	if(str_ == "or")            logic_ = 0;
	else if(str_ == "and")      logic_ = 1;
	else if(str_ == "majority") logic_ = 2;
	else {
	  Logger::instance().warning("ConfigurationReader::ParseXML", "Invalid trigger logic (group " + TString(Form("%d", groupNumber)) + ") read from " + TString(filename));
	  return false;
	}

	// if the group number is negative, apply these settings to all groups
	if(groupNumber < 0) {
	  for (int j = 0; j < nGroupsPerDigitizer; j++) {
	    dgtz.groups[j].logic = logic_;
	    dgtz.groups[j].coincidenceWindow = gate;
	    dgtz.groups[j].triggerDelay = trigDelay;
	  }
	}
	else {
	  dgtz.groups[groupNumber].logic = logic_;
	  dgtz.groups[groupNumber].coincidenceWindow = gate;
	  dgtz.groups[groupNumber].triggerDelay = trigDelay;
	}
      } // groups block

      for(const auto& i : pt.get_child("config.channels")) {
	string name;
	ptree sub_pt;
	tie(name, sub_pt) = i;

	if(name != "channel") continue;

	int channelNumber = sub_pt.get<int>("<xmlattr>.number");
	if(channelNumber > nChannelsPerDigitizer) continue;

	ChannelConfiguration ch;

	ch.enable = sub_pt.get<bool>("<xmlattr>.channelEnable");

	ch.testPulseEnable  = sub_pt.get<bool>("<xmlattr>.testPulseEnable");
	ch.testPulsePattern = stoi(sub_pt.get<string>("<xmlattr>.testPulsePattern"), 0, 16);

	str_ = sub_pt.get<string>("<xmlattr>.testPulseSource"); // CAEN_DGTZ_SAMPulseSourceType_t
	if (str_ == "continuous")    ch.testPulseSource = 1;
	else if (str_ == "software") ch.testPulseSource = 0;
	else {
	  Logger::instance().warning("ConfigurationReader::ParseXML", "Invalid test pulse source (channel " + TString(Form("%d", channelNumber)) + ") read from " + TString(filename));
	  return false;
	}

	ch.triggerEnable    = sub_pt.get<bool>("<xmlattr>.triggerEnable");
	ch.triggerThreshold = sub_pt.get<double>("<xmlattr>.triggerThreshold");

	str_ = sub_pt.get<string>("<xmlattr>.triggerPolarity"); // CAEN_DGTZ_TriggerPolarity_t
	if (str_ == "risingEdge")       ch.triggerPolarity = 0L;
	else if (str_ == "fallingEdge") ch.triggerPolarity = 1L;
	else {
	  Logger::instance().warning("ConfigurationReader::ParseXML", "Invalid trigger polarity (channel " + TString(Form("%d", channelNumber)) + ") read from " + TString(filename));
	  return false;
	}

	ch.dcOffset = sub_pt.get<double>("<xmlattr>.dcOffset");

	ch.chargeStartCell = sub_pt.get<unsigned int>("<xmlattr>.chargeStartCell");
	ch.chargeLength    = sub_pt.get<unsigned short>("<xmlattr>.chargeLength");

	if(ch.chargeStartCell < 16 || (ch.chargeStartCell + ch.chargeLength) > dgtz.RecordLength) {
	  Logger::instance().warning("ConfigurationReader::ParseXML", "Invalid charge integration window (channel " + TString(Form("%d", channelNumber)) + ") read from " + TString(filename));
	  return false;
	}

	str_ = sub_pt.get<string>("<xmlattr>.enableChargeThreshold"); // CAEN_DGTZ_EnaDis_t
	if (str_ == "1")      ch.enableChargeThreshold = 1L;
	else if (str_ == "0") ch.enableChargeThreshold = 0L;
	else {
	  Logger::instance().warning("ConfigurationReader::ParseXML", "Invalid enableChargeThreshold (channel " + TString(Form("%d", channelNumber)) + ") read from " + TString(filename));
	  return false;
	}

	ch.chargeThreshold = sub_pt.get<float>("<xmlattr>.chargeThreshold");

	// if the channel number is negative, apply these settings to all channels
	if (channelNumber < 0) {
	  for (int j = 0; j < nChannelsPerDigitizer; j++) dgtz.channels[j] = ch;
	}

	else dgtz.channels[channelNumber] = ch;

      } // channels block

      cfg.digitizers[dgtz.boardIndex] = dgtz;

    } // boards block

    int numHVRead = 0;

    for(const auto& i : pt.get_child("config.highVoltage")) {
      string name;
      ptree sub_pt;
      tie(name, sub_pt) = i;

      for(unsigned int iVoltage = 0; iVoltage < nHVChannels; iVoltage++) {
	if(name == cfg.hvConfig.names[iVoltage]) {
	  cfg.hvConfig.voltages[iVoltage] = sub_pt.get<float>("<xmlattr>.voltage");
	  cfg.hvConfig.enabled[iVoltage] = sub_pt.get<bool>("<xmlattr>.enabled");

	  if(name.find("878") != string::npos) {
	    if(cfg.hvConfig.voltages[iVoltage] < 0 || cfg.hvConfig.voltages[iVoltage] > 1500.0) {
	      Logger::instance().warning("DemonstratorConfiguration::ParseXML", TString("Invalid voltage read for ") + TString(name) + " from " + TString(filename));
	      return false;
	    }
	  }
	  else if(name.find("ET") != string::npos) {
	    if(cfg.hvConfig.voltages[iVoltage] < 0 || cfg.hvConfig.voltages[iVoltage] > 1750.0) {
	      Logger::instance().warning("DemonstratorConfiguration::ParseXML", TString("Invalid voltage read for ") + TString(name) + " from " + TString(filename));
	      return false;
	    }
	  }
	  else if(name.find("7725") != string::npos) {
	    if(cfg.hvConfig.voltages[iVoltage] < 0 || cfg.hvConfig.voltages[iVoltage] > 1750.0) {
	      Logger::instance().warning("DemonstratorConfiguration::ParseXML", TString("Invalid voltage read for ") + TString(name) + " from " + TString(filename));
	      return false;
	    }
	  }
	  else {
	    Logger::instance().warning("DemonstratorConfiguration::ParseXML", TString("Unknown high voltage channel ") + TString(name) + TString(" read from ") + TString(name));
	    return false;
	  }

	  numHVRead++;
	}
      }

    } // high voltage block

    if(numHVRead != nHVChannels) {
      Logger::instance().warning("DemonstratorConfiguration::ParseXML", "Did not read all expected HV channels!");
      return false;
    }

    cfg.SetCurrentConfigFile(filename);

    // cfg is now completely valid, so swap cfg into the real one
    configuration = cfg;

    Logger::instance().info("ConfigurationReader::ParseXML", "Successfully read " + TString(filename));

    return true;
#else // NO_BOOST_INSTALL
    Logger::instance().warning("ConfigurationReader::ParseXML", "This function is not implemented without the BOOST libraries.");
    return false;
#endif
  }

} // namespace mdaq
