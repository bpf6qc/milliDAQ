#!/usr/bin/python

import sys
import os
import stat
import subprocess
import datetime

fifoFile = '/tmp/MilliDAQ_FIFO'
logFile = '/var/log/MilliDAQ.log'
commandHistoryFile = '/home/milliqan/MilliDAQ_commandHistory.log'

commands = [
    'status',
    'quit',
    'pause',
    'unpause',
    'start',
    'stop',
    'restart',
    'print',
    'reconfigure',
    'dqm',
    'rotate',
    'peek',
    'timedStart',
    'nEventsStart',
    'interactive_configure',
    'interactive_timed',
    'interactive_nevents',
    'interactive_togglewaves',
]

commandsWithArgs = {
    'print',
    'reconfigure',
    'timedStart',
    'nEventsStart',
    'interactive_configure',
    'interactive_timed',
    'interactive_nevents',
}

printCommands = [
    'configuration',
    'board',
    'rates',
    'status',
]

def PrintUsage():
    print
    print 'Usage: command (argument)'
    print
    print 'Commands available:'
    print '    quit                   -- terminate the MilliDAQ process'
    print '    pause                  -- pause then run'
    print '    unpause                -- unpause a paused run'
    print '    start                  -- start the run'
    print '    timedStart <nSeconds>  -- start the run, but automatically stop after <nSeconds> seconds'
    print '    nEventsStart <nEvents> -- start the run, but automatically stop after recording <nEvents> events'
    print '    restart                -- stop, then start the run'
    print '    stop                   -- stop the run'
    print '    print [configuration | board | rates | status]'
    print '          print configure  -- print the current V1743Configuration parameters: trigger mode, thresholds, etc'
    print '          print board      -- print information about the V1743 board: connection, firmware versions, etc'
    print '          print rates       -- print DQM information: trigger rates, missed triggers, etc'
    print '          print status     -- print DAQ information: state, configuration path, etc'
    print '    reconfigure <file.xml> -- stop the run, read/apply a new configuration file, then start'
    print '    dqm                    -- force an update of DQM plots and rates'
    print '    rotate                 -- force the current output file to be saved and move on to a new file'
    print '    peek                   -- save the last 10 events to a different file'
    print 'InteractiveDAQ only commands:'
    print '    interactive_configure <file.xml> -- read/apply a new configuration while not running'
    print '    interactive_timed <nsec>         -- start a timed run lasting <nsec> seconds'
    print '    interactive_nevents <nevents>    -- start a run recording <nevents> events'
    print '    interactive_togglewaves          -- toggle the saving of waveforms on/off (not yet implemented)'
    print

nArgs = len(sys.argv) - 1

# require a command
if not nArgs > 0:
    PrintUsage()
    exit()

# require the FIFO to exist
if not stat.S_ISFIFO(os.stat(fifoFile).st_mode):
    print
    print 'Error: ' + fifoFile + ' does not exist! Cannot send commands to MilliDAQ!'
    print
    exit()

# require only allowed commands
if not sys.argv[1] in commands:
    PrintUsage()
    exit()

# if the command expects an argument, require an argument of the correct type
if sys.argv[1] in commandsWithArgs:
    if not nArgs > 1:
        print 'Expected argument for command: ' + sys.argv[1]
        exit()
    if sys.argv[1] == 'print' and sys.argv[2] not in printCommands:
        PrintUsage()
        exit()
    # if 'reconfigure', get the absolute path in case a relative path has been given
    if sys.argv[1] == 'reconfigure':
        if not sys.argv[2].endswith(".py") and not sys.argv[2].endswith(".xml"):
            print 'Only python or XML files are supported for configurations!'
            exit()
        sys.argv[2] = os.path.abspath(sys.argv[2])
        # in case of python configuration file, test now that it's a valid one
        if sys.argv[2].endswith(".py"):
	    cfgModule = __import__(os.path.basename(sys.argv[2]).split('.')[0])
            if not cfgModule.cfg:
                print 'No cfg object found in', sys.argv[2], '!'
                exit()
            if not cfgModule.cfg.IsValid():
                print 'Invalid configuration object in', sys.arv[2], '!'
                exit()

message = sys.argv[1] + ' ' + sys.argv[2] if nArgs > 1 else sys.argv[1]

# send these to the FIFO
os.system('echo ' + message + ' >> ' + fifoFile)

# record this in the history log
os.system('echo ' + datetime.datetime.now().strftime("%c") + ' ' + sys.argv[0] + ' ' + message + ' >> ' + commandHistoryFile)

if not os.path.isfile(logFile):
    print 'Command sent, but log file ' + logFile + ' doesn\'t exist to watch.'
    exit()

# hold on "tail -f" until the user quits watching the log file
print 'Displaying log file, CTRL+C to quit...'

os.system('tail -f ' + logFile)
