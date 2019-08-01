# MilliDAQ

Core software to operate the CAEN V1743 for MilliQan, and analyze the resulting data.

Analysis recipe:
	1. git clone https://gitlab.cern.ch/MilliQan/MilliDAQ.git
	2. cd MilliDAQ/
	3. source test/lxplus.(c)sh # or test/fnallpc.(c)sh as appropriate
	4. make shared
	5. cd test/
	6. root -l Example.C

Recipe for digitizer operation (requires adminstrator rights):

	1. Install ROOT
	2. Install CAENVMElib, CAENComm (see caen/)
	3. Install CAENUpgrader (requires java 1.8+)
	4. Install CAENDigitizer
	5. Install the A2818 driver
	6. make
	7. sudo make install
	8. sudo systemctl start MilliDAQ.service

Acquisition begins immediately upon the service starting.

All actions are recorded in /var/log/MilliDAQ:
tail -f /var/log/MilliDAQ

To interact with digitizer: DAQCommand <command> <argument>
To list available commands: "DAQCommand"

To interact with the service:

systemctl status MilliDAQ.service
journalctl -u MilliDAQ.service

To force the rotation of /var/log/MilliDAQ:
sudo logrotate -vf /etc/logrotate.d/MilliDAQ
