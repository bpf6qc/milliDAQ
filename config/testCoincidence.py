from Demonstrator import *

cfg = Demonstrator()

for dgtz in cfg.Digitizers:
        dgtz.IRQPolicy.use = False

        for iChannel, channel in enumerate(dgtz.channels):
                channel.triggerEnable = False

cfg.Digitizers[0].TriggerType.type = TriggerType.normal
cfg.Digitizers[0].GroupTriggerLogic.logic = GroupTriggerLogic.logicMajority
cfg.Digitizers[0].GroupTriggerLogic.majorityLevel = 2

cfg.Digitizers[0].channels[6].triggerEnable = True
cfg.Digitizers[0].channels[6].triggerThreshold = -0.200
cfg.Digitizers[0].channels[6].triggerPolarity = Channel.fallingEdge

cfg.Digitizers[0].channels[0].triggerEnable = True
cfg.Digitizers[0].channels[0].triggerThreshold = -0.200
cfg.Digitizers[0].channels[0].triggerPolarity = Channel.fallingEdge

cfg.Digitizers[1].TriggerType.type = TriggerType.none
