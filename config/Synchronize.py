from Demonstrator import *

cfg = Demonstrator()

for dgtz in cfg.Digitizers:
        dgtz.IRQPolicy.use = False
#        dgtz.SAMFrequency = 3.2

        for iChannel, channel in enumerate(dgtz.channels):
                channel.enable = False
                channel.triggerEnable = False

cfg.Digitizers[0].TriggerType.type = TriggerType.normal
cfg.Digitizers[0].GroupTriggerLogic.logic = GroupTriggerLogic.logicOr
cfg.Digitizers[0].channels[1].enable = True
cfg.Digitizers[0].channels[1].triggerEnable = True
cfg.Digitizers[0].channels[1].triggerThreshold = 0.050
cfg.Digitizers[0].channels[1].triggerPolarity = Channel.risingEdge
cfg.Digitizers[0].channels[1].testPulseEnable = False
cfg.Digitizers[0].channels[1].testPulsePattern = 0xFF00
cfg.Digitizers[0].MaxNumEventsBLT = 255

cfg.Digitizers[1].TriggerType.type = TriggerType.normal
cfg.Digitizers[1].GroupTriggerLogic.logic = GroupTriggerLogic.logicOr
cfg.Digitizers[1].channels[1].enable = True
cfg.Digitizers[1].channels[1].triggerEnable = False
cfg.Digitizers[1].channels[1].triggerThreshold = 0.050
cfg.Digitizers[1].channels[1].triggerPolarity = Channel.risingEdge
cfg.Digitizers[1].channels[1].testPulseEnable = False
cfg.Digitizers[1].channels[1].testPulsePattern = 0xFF00
cfg.Digitizers[1].MaxNumEventsBLT = 255
