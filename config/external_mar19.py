from Demonstrator import *

cfg = Demonstrator()

for dgtz in cfg.Digitizers:
        dgtz.IRQPolicy.use = False

        for iChannel, channel in enumerate(dgtz.channels):
                channel.triggerEnable = False

cfg.Digitizers[0].TriggerType.type = TriggerType.external
cfg.Digitizers[1].TriggerType.type = TriggerType.external
