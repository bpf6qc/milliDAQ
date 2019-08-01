#!/usr/bin/python

import glob
import os
import time
import socket
import sys
from ROOT import TFile

source = "/home/milliqan/data/"
destination = "milliqan@cms2.physics.ucsb.edu:/net/cms6/cms6r0/milliqan/UX5/"
cooloffDir = "/home/milliqan/transferred/"

logFile = "/home/milliqan/MilliDAQ_FileTransfers.log"
listOfFiles = "/home/milliqan/NewFilesAtUCSB.log"

def get_lock(process_name):
    # Without holding a reference to our socket somewhere it gets garbage
    # collected when the function exits
    get_lock._lock_socket = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)

    try:
        get_lock._lock_socket.bind('\0' + process_name)
        print 'Acquired lock for transferFiles.py.'
    except socket.error:
        print 'Lock already exists for transferFiles.py -- it\'s already running elsewhere!'
        sys.exit()

def FileIsGood(path):
	f = TFile.Open(path, "READ")
	if not f or f.IsZombie():
		os.system("echo '{0} is not ready to be transferred' >> {1}".format(path, logFile))
		return False
	f.Close()
	return True

#########################################

get_lock('transfer_files')

nTransferred = 0
mbytesTransferred = 0

os.system("echo '" + time.strftime("%c") + " Attempting to transfer data files to cms2.physics.ucsb.edu...' >> " + logFile)

for f in sorted(glob.glob(source + "*.root"), key=os.path.getmtime):
	if FileIsGood(f):
		nTransferred += 1
		sizeInMB = os.path.getsize(f) / 1024.0 / 1024.0
		mbytesTransferred += sizeInMB

		command = "rsync --bwlimit=20000 -zh " + f + " " + destination + " >> " + logFile
		os.system(command)

		os.system("echo '\t{0}:\t {1:.2f} MB' >> {2}".format(f, sizeInMB, logFile))
		os.system("echo {0} >> {1}".format(os.path.basename(f), listOfFiles))

		os.system("mv " + f + " " + cooloffDir);

os.system("echo 'Transferred {0:.2f} MB in {1} file(s).' >> {2}".format(mbytesTransferred, nTransferred, logFile))
