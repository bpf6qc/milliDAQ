##########################################################################################
# Demonstrator.py
#
#   Defines a python class for the demonstrator-wide configuration.
#   Contains a list of digitizer configurations and very high-level parameters.
#
##########################################################################################

from HV import *
from V1743 import *

class Demonstrator:

	def __init__(self):
		self.numV1743 = 2 # number of digitizers

		self.TFileCompressionEnabled = False

		self.HLTEnabled = False
		self.HLTMaxAmplitude = 0.0
		self.HLTNumRequired = 1

		self.CosmicPrescaleEnabled = False
		self.CosmicPrescaleMaxAmplitude = 0.0
		self.CosmicPrescaleNumRequired = 1
		self.CosmicPrescalePS = 1

		self.Digitizers = [V1743(idx) for idx in range(self.numV1743)]

		self.HighVoltage = HV()

	def IsValid(self):

		usedIndices = []

		if self.numV1743 > 1:
			for digitizer in self.Digitizers[1:]:
				if not digitizer.ChargeMode.use == self.Digitizers[0].ChargeMode.use:
					print 'Demonstrator.py: useChargeMode must be the same for all digitizers!'
					return False

		for digitizer in self.Digitizers:
			if not digitizer.IsValid():
				return False
			if digitizer.boardIndex in usedIndices:
				print 'Demonstrator.py: board index ' + digitizer.boardIndex + ' is used twice!'
				return False
			else:
				usedIndices.append(digitizer.boardIndex)

		if not self.CosmicPrescalePS >= 1:
			print 'Demonstrator.py: CosmicPrescalePS must be >= 1'
			return False

		if not self.HLTNumRequired >= 0:
			print 'Demonstrator.py: HLTNumRequired must be >= 0'
			return False

		if not self.CosmicPrescaleNumRequired >= 0:
			print 'Demonstrator.py: CosmicPrescaleNumRequired must be >= 0'
			return False

		if not self.HighVoltage.IsValid():
			return False

		return True
