##########################################################################################
# Channel.py
#
#	Defines a python class for individual V1743 channel configurations.
#   Each V1743 digitizer will have 16 of these objects.
#
##########################################################################################

# Default: enabled in readout and trigger; fallingEdge at -10 mV
class Channel:
	risingEdge = 0
	fallingEdge = 1

	softwarePulseSource = 0
	continuousPulseSource = 1

	def __init__(self):
		self.enable = True # readout is enabled, written to disk

		# Each channel has a pulse generator for testing purposes
		# If enabled, a waveform will be injected with a 16-bit pattern
		# Each bit of this pattern, sequentially, determines whether 64 samples will be high or low/zero
		# For example: 0xF001 gives a pulse of 4*64 samples high, (12*64 - 1)  samples low, then 1*64 samples high
		# The 'source' determines when the pulse is injected; either continuously at a fixed frequency, or
		# in response to a software command
		self.testPulseEnable = False
		self.testPulsePattern = 0x0
		self.testPulseSource = self.continuousPulseSource

		# Trigger definition
		self.triggerEnable = True               # this channel is allowed to contribute to its group's trigger
		self.triggerThreshold = -0.010          # threshold in volts
		self.triggerPolarity = self.fallingEdge # polarity

		# DC offset in volts
		self.dcOffset = 0

		# Charge mode parameters
		self.chargeStartCell = 0           # which sample the charge integration begins
		self.chargeLength = 0              # length in samples of the charge integration window
		self.enableChargeThreshold = False # only trigger when charge is over a threshold
		self.chargeThreshold = 0           # the charge threshold in pC
