#!/usr/bin/python

from LDMX.Framework import ldmxcfg

hcalVeto = ldmxcfg.Producer("HcalVeto","ldmx::HcalVetoProcessor")

hcalVeto.parameters["totalPEThreshold_"] = 8
hcalVeto.parameters["MaxXY_"] = 1.55
hcalVeto.parameters["MaxZ_"] = 3.5
