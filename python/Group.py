##########################################################################################
# Group.py
#
#	Defines a python class for individual V1743 groups (pairs) of channels.
#   Each V1743 digitizer will have 8 of these.
#
##########################################################################################

# Default: OR of both channels, 100ns coincidence window (gate primative), and trigger delay of 24
class Group:
	logicOr = 0
	logicAnd = 1

	def __init__(self):
		self.logic = self.logicOr
		self.coincidenceWindow = 100 # gate length in ns
		self.triggerDelay = 24       # bfrancis -- add units to this comment
