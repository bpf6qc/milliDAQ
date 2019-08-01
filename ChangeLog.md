Change Log
==========
----------

Oct. 26, 2017:
--------------------

* Create this change log.

* New commands for DAQCommand:
  - `DAQCommand print status` -- prints for the user a brief summary of program conditions such as state (running, paused, stopped, etc), configuration, run duration, and more. This should more easily inform the operator as to the current state of the DAQ.
     - To avoid confusion between "info" and "status", `DAQCommand print info` has been **changed to** `DAQCommand print rates`.
  - `DAQCommand startTimed <nSeconds>` -- issued when stopped, this is the same as "start" but will automatically issue "stop" after `nSeconds` seconds have elapsed.
  - `DAQCommand startNevents <nEvents>` -- same as `startTimed` but with a target number of events. 

* Log of commands:
  - Commands issued with DAQCommand will now be logged in `~milliqan/MilliDAQ_commandHistory.log`

* Relative paths for `DAQCommand reconfigure`:
  - Operators can now give either the absolute path or a relative path to a configuration file.
  - Example: `DAQCommand config/DoubleCoincidence.xml` works as you'd expect in your current working directory.

* Default configurations:
  - MilliDAQ will now attempt to read the most recent run's configuration from `/var/log/MilliDAQ_RunList.log` when it first starts. If this fails, it will read `TripleCoincidence.xml` as a default. If even this fails, the program will exit.

* Fixed a bug that occasionally arose when issuing `DAQCommand quit` or stopping `MilliDAQ.service`.

* Changes to transferFiles.py:
  - The script now acquires a lock so that it's not possible to run twice simultaneously (at least by the same user).
  - The oldest data files are now transferred first.
  - Data files are no longer automatically deleted once transferred. Instead they are moved into a "cooloff" directory (`~milliqan/transferred/`). 
     - The operator is now responsible for periodically checking that transferred files are truly present and okay at UCSB. The script `~milliqan/checkTransfers.sh` is to be used as user `milliqan` before any files are actually deleted.

* Clean up DAQCommand script (now python).

* OSX support for compiling shared libraries.

Mar. 19, 2018:
--------------------

* Dual-board readout!

* Add python configs

* Add dcOffset to trigger thresholds

* Cleaned up mdaq::DAQ access specifiers

* Added HV class, but haven't implemented any calls to it yet

Mar. 21, 2018:
--------------------

* Now track TDC rollovers in V1743Event, and provide simple getters to get the rollover-corrected TTTs and TDCs. Also include more time info like TTT/TDC clock tick lengths and nanoseconds/sample

* GetWaveform functions in V1743Event now return an x-axis in units of [ns] instead of samples.

* Removed a rare bug where event(s) from the previous run would be stored as the the first event(s) of the next run.

* Fixed a bug that allowed an invalid configuration to be applied to
the digitizer, resulting in a crash

* Allowed DAQCommand to error-check python configuration files as an
extra precaution.

* Finalize the CAENSY5527 class for the final HV cabling layout

* Cleaned up much of the indendtation for readability
