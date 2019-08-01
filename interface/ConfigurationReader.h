#ifndef CONFIGURATION_READER_H
#define CONFIGURATION_READER_H

#ifndef NO_PYTHON_API_INSTALL
#include <python2.7/Python.h>
#endif

#ifndef NO_BOOST_INSTALL
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/lexical_cast.hpp>
#endif

#include <iostream>

#include "DemonstratorConfiguration.h"

namespace mdaq {
  class ConfigurationReader {
  public:
    ConfigurationReader() {};
    virtual ~ConfigurationReader() {};

    const bool ParsePython(mdaq::DemonstratorConfiguration &configuration, const std::string filename);
    const bool ParseXML(mdaq::DemonstratorConfiguration &configuration, const std::string filename);

#ifndef NO_PYTHON_API_INSTALL
  private:
    template<typename T> T getParameter(PyObject * obj, const char * name);

    const bool callMethod(PyObject * obj, const std::string name);

    const bool IsError(PyObject * obj) {
      if(!obj) {
	PyErr_Print();
	return true;
      }
      return false;
    };

    PyObject * pName;   // name of the python module
    PyObject * pModule; // python module
    PyObject * pDict;   // module dictionary

    PyObject * pConfig; // cfg

    PyObject * pDigitizers; // cfg.Digitizers
    PyObject * pDigitizer;  // cfg.Digitizers[i]

    PyObject * pHighVoltage; // cfg.HighVoltage
    PyObject * pVoltageDict; // cfg.HighVoltage.voltages
    PyObject * pEnabledDict; // cfg.HighVoltage.enabled

    // all class objects stored in cfg.Digitizers[i]
    PyObject * pConnection;
    PyObject * pIRQPolicy;
    PyObject * pChargeMode;
    PyObject * pSAMCorrection;
    PyObject * pTriggerType;
    PyObject * pIOLevel;
    PyObject * pGroupTriggerLogic;
    PyObject * pTriggerCountVeto;

    PyObject * pGroups; // cfg.Digitizers[i].Groups
    PyObject * pGroup;  // cfg.Digitizers[i].Groups[j]

    PyObject * pChannels; // cfg.Digitizers[i].Channels
    PyObject * pChannel;  // cfg.Digitizers[i].Channels[j]

    PyObject * pValue; // individual values to be read from objects
#endif // NO_PYTHON_API_INSTALL
  };
}

#endif
