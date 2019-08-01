##########################################################################################
# V1743.py
#
#	Defines python classes for global V1743 board configuration parameters,
#   with arrays of Channel and Group configurations.
#   In the MilliQan-wide configuration, each digitizer will have one of these objects.
#
##########################################################################################

from Channel import Channel
from Group import Group

# Define how to communicate with this board
# Default: master board (linkNum/conetNode 0) with optical link
class Connection:
	usb = 0
	opticalLink = 1

	def __init__(self):
		self.linkNum = 0             # which number in the optical link chain
		self.conetNode = 0           # which CONET connector in the A3818 card
		self.vmeBaseAddress = 0      # 0 for optical link
		self.type = self.opticalLink

# Define the interrupt request policy
# Default: boards will be read if 2 events are ready to be read, or if 50ms passes while waiting
class IRQPolicy:
	rora = 0
	roak = 1

	def __init__(self):
		self.use = True
		self.level = 1         # always 1 for optical link
		self.status_id = 0     # meaningless for optical link
		self.nevents = 2       # number of events to wait for within timeout
		self.mode = self.rora  # board does not support ROAK
		self.timeout = 50      # time in ms

# Define whether to use the firmware charge integration mode
# Default: off
class ChargeMode:
	def __init__(self):
		self.use = False
		self.suppressBaseline = True # subtract the charge pedestal

# Define which corrections to apply to digitized waveforms (see V1743 docs)
# Default: all
class SAMCorrection:
	correctionDisabled = 0
	pedestalOnly = 1
	INL = 2
	allCorrections = 3

	def __init__(self):
		self.level = self.allCorrections

# Define the global trigger mode
# Default: external TRG-IN OR channel discriminators
class TriggerType:
	software = 0          # only on software signal
	normal = 1            # channel discriminators only
	auto = 2              # software OR channels OR external
	external = 3          # external TRG-IN only
	externalAndNormal = 4
	externalOrNormal = 5
	none = 6

	def __init__(self):
		self.type = self.normal

# Define the logic level of the front panel I/O
# Default: TTL
class IOLevel:
	nim = 0
	ttl = 1

	def __init__(self):
		self.type = self.ttl

# Define the trigger logic amongst all groups
# Default: majority 2 ("double coincidence")
class GroupTriggerLogic:
	logicOr = 0
	logicAnd = 1
	logicMajority = 2

	def __init__(self):
		self.logic = self.logicMajority
		self.majorityLevel = 2          # only used if logicMajority

# Define the suppression of the missed trigger counter within a given time after a recorded trigger
# Default: off
class TriggerCountVeto:
	def __init__(self):
		self.enabled = False
		self.vetoWindow = 125

##########################################################################################
# The configuration for the entire digitizer
# Default: Master clock source connected by optical link
#          1.6 GS/s
#          1024 samples
#          All other objects default
##########################################################################################

class V1743:
	numGroups = 8
	numChannels = 16

   	def __init__(self, index):
    		self.boardIndex = index # index of the board; should be ordered as the clock distribution order
		self.boardEnable = True # enable or disable the readout of the entire board

	    	self.Connection = Connection()
	    	self.Connection.conetNode = index

    		self.IRQPolicy = IRQPolicy()

	    	self.ChargeMode = ChargeMode()
    	
	    	self.SAMCorrection = SAMCorrection()
    		self.SAMFrequency = 1.6 # sampling rate in GS/s; must be 3.2, 1.6, 0.8, or 0.4
    	
	    	self.RecordLength = 1024 # number of samples digitized; must be N*16, for 4 <= N <= 256

	    	self.TriggerType = TriggerType()

    		self.IOLevel = IOLevel()

	    	self.MaxNumEventsBLT = 255 # max number of events to read in each block transfer over optical link

	    	self.GroupTriggerLogic = GroupTriggerLogic()

	    	self.TriggerCountVeto = TriggerCountVeto()

		self.groups = [Group() for _ in range(self.numGroups)]
		self.channels = [Channel() for _ in range(self.numChannels)]

	def IsValid(self):

		if not self.boardIndex >= 0:
			print 'V1743.py: boardIndex must be >= 0'
			return False

		if not self.Connection.linkNum >= 0:
			print 'V1743.py: Connection.linkNum must be >= 0'
			return False
		if not self.Connection.conetNode >= 0:
			print 'V1743.py: Connection.conetNode must be >= 0'
			return False
		if not self.Connection.vmeBaseAddress >= 0:
			print 'V1743.py: Connection.vmeBaseAddress must be >= 0'
			return False

		if self.IRQPolicy.use:
			if self.IRQPolicy.level != 1:
				print 'V1743.py: IRQPolicy.level must be 1'
				return False
			if self.IRQPolicy.nevents <= 0 or self.IRQPolicy.nevents > 7:
				print 'V1743.py: IRQPolicy.nevents must be between 0 and 7'
				return False
			if not self.IRQPolicy.mode == self.IRQPolicy.rora:
				print 'V1743.py: IRQPolicy.mode must be IRQPolicy.rora'
				return False
			if not self.IRQPolicy.timeout > 0:
				print 'V1743.py: IRQPolicy.timeout must be > 0'
				return False

		if self.SAMCorrection.level < SAMCorrection.correctionDisabled or self.SAMCorrection.level > SAMCorrection.allCorrections:
			print 'V1743.py: SAMCorrection.level must be a valid choice'
			return False

		if self.TriggerType.type < TriggerType.software or self.TriggerType.type > TriggerType.none:
			print 'V1743.py: TriggerType.type must be a valid choice'
			return False

		if self.IOLevel.type != IOLevel.nim and self.IOLevel.type != IOLevel.ttl:
			print 'V1743.py: IOLevel.type must be a valid choice'
			return False

		if self.GroupTriggerLogic.logic < GroupTriggerLogic.logicOr or self.GroupTriggerLogic.logic > GroupTriggerLogic.logicMajority:
			print 'V1743.py: GroupTriggerLogic.logic must be a valid choice'
			return False
		if self.GroupTriggerLogic.majorityLevel < 0 or self.GroupTriggerLogic.majorityLevel > 7:
			print 'V1743.py: GroupTriggerLogic.majorityLevel must be between 0 and 7'
			return False

		if self.TriggerCountVeto.enabled and self.TriggerCountVeto.vetoWindow < 0:
			print 'V1743.py: TriggerCountVeto.vetoWindow must be > 0'
			return False

		if len(self.groups) != self.numGroups:
			print 'V1743.py: there must be exactly ' + str(self.numGroups) + ' groups, and somehow there are ' + str(len(self.groups))
			return False

		if len(self.channels) != self.numChannels:
			print 'V1743.py: there must be exactly ' + str(self.numChannels) + ' channels, and somehow there are ' + str(len(self.channels))
			return False

		return True


