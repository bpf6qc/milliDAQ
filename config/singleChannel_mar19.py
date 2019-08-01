from Demonstrator import *

cfg = Demonstrator()

for dgtz in cfg.Digitizers:
        dgtz.IRQPolicy.use = False

        for iChannel, channel in enumerate(dgtz.channels):
                channel.triggerEnable = False


cfg.Digitizers[0].TriggerType.type = TriggerType.normal
cfg.Digitizers[0].GroupTriggerLogic.logic = GroupTriggerLogic.logicOr
cfg.Digitizers[0].channels[0].enable = True
cfg.Digitizers[0].channels[0].triggerEnable = True
cfg.Digitizers[0].channels[0].triggerThreshold = -0.010
cfg.Digitizers[0].channels[0].triggerPolarity = Channel.fallingEdge

cfg.Digitizers[1].TriggerType.type = TriggerType.none
