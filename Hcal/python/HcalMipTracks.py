#!/usr/bin/python

# we need the ldmx configuration package to construct the object
from LDMX.Framework import ldmxcfg

HcalMipTracks = ldmxcfg.Producer( "HcalMipTracks" , "ldmx::HcalMipTrackProducer" )

# name of collection that contains the hcal hits 
HcalMipTracks.parameters["HitCollectionName"] = "hcalDigis"
HcalMipTracks.parameters["HitPassName"] = "recon"

# Name of track collection to be added to event bus
HcalMipTracks.parameters["HcalMipTrackCollectionName"] = "hcalMipTracks"
 
# minimum PE for a single hit to be considered non-noise
HcalMipTracks.parameters["MinimumPE"] = 5.5

# maximum Energy of a group of hits to be considered a mip
HcalMipTracks.parameters["MaximumEnergy"] = 4000.0

# minimum number of clusters to be willing to be considered a plausible track
HcalMipTracks.parameters["MinimumNumClusters"] = 5
