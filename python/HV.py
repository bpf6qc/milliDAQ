##########################################################################################
# HV.py
#
#	Defines a python class for the high voltage configuration.
#
##########################################################################################

# Default: nominal bias voltages in all PMTs
class HV:
	def __init__(self):

		# [Volts]
		self.voltages = {
			  "878box2"   : 1450.0,
  			"L3ML_7725" : 1600.0,
  			"L3BR_7725" : 1600.0,
  			"ETbox"     : 1600.0,
  			"878box1"   : 1450.0,
  			"L3TL_878"  : 1450.0,
  			"Slab0_878" : 1450.0,
  			"ShL1L_878" : 1450.0,
  			"ShL1R_878" : 1450.0,
  			"ShL1T_878" : 1450.0,
  			"ShL2L_878" : 1450.0,
  			"ShL2T_878" : 1450.0,
  			"ShL3L_878" : 1450.0,
		}

		self.enabled = {
			  "878box2"   : True,
  			"L3ML_7725" : True,
  			"L3BR_7725" : True,
  			"ETbox"     : True,
  			"878box1"   : True,
  			"L3TL_878"  : True,
  			"Slab0_878" : True,
  			"ShL1L_878" : True,
  			"ShL1R_878" : True,
  			"ShL1T_878" : True,
  			"ShL2L_878" : True,
  			"ShL2T_878" : True,
  			"ShL3L_878" : True,
		}

	def IsValid(self):

		for name in self.voltages:
			if 'ET' in name:
				if self.voltages[name] < 0 or self.voltages[name] > 1750.0:
					print 'ET voltage must be between 0 and 1750 volts!'
					return False
			if '7725' in name:
				if self.voltages[name] < 0 or self.voltages[name] > 1750.0:
					print 'R7725 voltage must be between 0 and 1750 volts!'
					return False
			if '878' in name:
				if self.voltages[name] < 0 or self.voltages[name] > 1500.0:
					print 'R878 voltage must be between 0 and 1500 volts!'
					return False

      		if not len(self.voltages) == 13 and len(self.enabled) == 13:
			print 'There must be 13 entries in HighVoltage.voltages and HighVoltage.enabled!!!'
			return False

		return True
