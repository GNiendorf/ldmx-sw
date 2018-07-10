#include "EventDisplay/EventObjects.h"   

namespace ldmx {

    EventObjects::EventObjects() {

        hexReadout_ = new EcalHexReadout();
        drawer_ = new EveShapeDrawer();

        Initialize();
    }

    void EventObjects::Initialize() {
        
        ecalHits_ = new TEveElementList("ECAL RecHits");
        hcalHits_ = new TEveElementList("HCAL RecHits");
        recoilTrackerHits_ = new TEveElementList("Recoil Sim Hits");
        ecalClusters_ = new TEveElementList("ECAL Clusters");
        hcalTracks_ = new TEveElementList("HCAL Tracks");
        ecalSimParticles_ = new TEveElementList("ECAL SP Sim Particles");
        hits_ = new TEveElementList("Reco Hits");
        recoObjs_ = new TEveElementList("Reco Objects");

    }

    static bool compEcalHits(const EcalHit* a, const EcalHit* b) {
        return a->getEnergy() > b->getEnergy();
    }

    static bool compHcalHits(const HcalHit* a, const HcalHit* b) {
        return a->getPE() > b->getPE();
    }

    static bool compSimsP(const SimTrackerHit* a, const SimTrackerHit* b) {

        std::vector<double> paVec = a->getMomentum();
        std::vector<double> pbVec = b->getMomentum();

        double pa2 = pow(paVec[0],2)+pow(paVec[1],2)+pow(paVec[2],2);
        double pb2 = pow(pbVec[0],2)+pow(pbVec[1],2)+pow(pbVec[2],2);

        return pa2 > pb2;
    }

    static bool compSims(const SimTrackerHit* a, const SimTrackerHit* b) { 
        if (a->getSimParticle() == b->getSimParticle()) {
            return compSimsP(a,b);
        } else {
            return a->getSimParticle() < b->getSimParticle();
        }
    }

    void EventObjects::SetSimThresh(double simThresh) {

        simThresh_ = simThresh;
        TEveElement* spHits = 0;
        spHits = hits_->FindChild("ECAL SP Sim Particles");
        TEveElement::List_i sim;

        for (sim = spHits->BeginChildren(); sim != spHits->EndChildren(); sim++) {
            
            TEveElement* el = *sim;
            SimParticle* sp = (ldmx::SimParticle*)el->GetSourceObject();
            std::vector<double> pVec = sp->getMomentum();
            double p = pow(pow(pVec[0],2) + pow(pVec[1],2) + pow(pVec[2],2),0.5);
            if (p < simThresh_) { 
                el->SetRnrSelf(kFALSE); 
            } else {
                el->SetRnrSelf(kTRUE);
            }
        }
    }

    void EventObjects::ColorClusters() {

        TEveElement* clusters = recoObjs_->FindChild("ECAL Clusters");
        if (clusters == 0) { 
            std::cout << "No clusters to color!" << std::endl;
            return; 
        }

        int theColor = 0;
        TEveElement::List_i cluster;
        for (cluster = clusters->BeginChildren(); cluster != clusters->EndChildren(); cluster++) {
            
            TEveElement* el = *cluster;
            TEveElement::List_i hit;
            Int_t color = 0;
            if (!el->IsPickable()) {
                color = 19;
            } else if (theColor < 9) {
                color = colors_[theColor];
                theColor++;
            } else {
                Int_t ci = 200*r_.Rndm();
                color = ci;
            }

            for (hit = el->BeginChildren(); hit != el->EndChildren(); hit++) { 
                TEveElement* elChild = *hit;
                elChild->SetMainColor(color);
            }
        }
    }

    void EventObjects::drawECALHits(TClonesArray* hits) {

        ldmx::EcalHit* hit;
        TEveRGBAPalette* palette = new TEveRGBAPalette(0,500.0);

        std::vector<EcalHit*> hitVec;
        for (TIter next(hits); hit = (ldmx::EcalHit*)next();) {
            hitVec.push_back(hit);
        }

        std::sort(hitVec.begin(), hitVec.end(), compEcalHits);

        for (int i = 0; i < hitVec.size(); i++) {
            double energy = hitVec[i]->getEnergy();
            if (energy == 0) { continue; }

            unsigned int hitID = hitVec[i]->getID();
            unsigned int cellID = hitID>>15;
            unsigned int moduleID = (hitID<<17)>>29;
            int layer = hitVec[i]->getLayer();
            unsigned int combinedID = 10*cellID + moduleID;
            std::pair<double, double> xyPos = hexReadout_->getCellCenterAbsolute(combinedID);

            TString digiName;
            digiName.Form("%1.5g MeV", energy);

            const UChar_t* rgb = palette->ColorFromValue(energy);
            TColor* aColor = new TColor();
            Int_t color = aColor->GetColor((Int_t)rgb[0], (Int_t)rgb[1], (Int_t)rgb[2]);

            TEveBox *ecalDigiHit = drawer_->drawBox(xyPos.first, xyPos.second, layerZPos[layer]+ecal_front_z-1.5, 3, 3, layerZPos[layer]+ecal_front_z+1.5, 0, color, 0, digiName);
            ecalHits_->AddElement(ecalDigiHit);
        }

        ecalHits_->SetPickableRecursively(1);

        hits_->AddElement(ecalHits_);
    }

    void EventObjects::drawHCALHits(TClonesArray* hits) {

        ldmx::HcalID detID;
        ldmx::HcalHit* hit;

        TEveRGBAPalette* palette = new TEveRGBAPalette(0,100.0);

        std::vector<HcalHit*> hitVec;
        for (TIter next(hits); hit = (ldmx::HcalHit*)next();) {
            hitVec.push_back(hit);
        }

        std::sort(hitVec.begin(), hitVec.end(), compHcalHits);

        for (int i = 0; i < hitVec.size(); i++) {
            int pe = hitVec[i]->getPE();
            if (pe == 0) { continue; }

            bool isNoise = hitVec[i]->getZ() == 0;

            detID.setRawValue(hitVec[i]->getID());
            detID.unpack();

            const UChar_t* rgb = palette->ColorFromValue(pe);
            TColor* aColor = new TColor();
            Int_t color = aColor->GetColor((Int_t)rgb[0], (Int_t)rgb[1], (Int_t)rgb[2]);

            int bar = hitVec[i]->getStrip();
            int layer = hitVec[i]->getLayer();
            int section = hitVec[i]->getSection();
            TString digiName;
            digiName.Form("%d PEs, Section %d, Layer %d, Bar %d, Z %1.5g", pe, section, layer, bar, hitVec[i]->getZ());

            TEveBox* hcalDigiHit = 0;
            //TEveBox* hcalDigiHit2 = 0;
            //hcalDigiHit2 = drawer_->drawBox(hitVec[i]->getX(), hitVec[i]->getY(), hitVec[i]->getZ()-15, 30, 30, hitVec[i]->getZ()+15, 0, color, 0, digiName);
            if (section == 0) {
                if (layer % 2 == 0) {
                    //horizontal
                    hcalDigiHit = drawer_->drawBox(hitVec[i]->getX(), bar_width*(0.5+bar)-hcal_y_width/2, (layer-1)*hcal_layer_thick+hcal_front_z+abso_thick, 150, bar_width, (layer-1)*hcal_layer_thick+hcal_front_z+scint_thick+abso_thick, 0, color, 0, digiName);
                } else {
                    //vertical, once alternating bar orientation in the back is implemented, uncomment
                    //hcalDigiHit = drawer_->drawBox((bar_width*(0.5-bar)-hcal_y_width/2), hitVec[i]->getY(), (layer-1)*hcal_layer_thick+hcal_front_z+abso_thick, bar_width, 150, (layer-1)*hcal_layer_thick+hcal_front_z+scint_thick+abso_thick, 0, color, 0, digiName);
                    hcalDigiHit = drawer_->drawBox(hitVec[i]->getX(), bar_width*(0.5+bar)-hcal_y_width/2, (layer-1)*hcal_layer_thick+hcal_front_z+abso_thick, 150, bar_width, (layer-1)*hcal_layer_thick+hcal_front_z+scint_thick+abso_thick, 0, color, 0, digiName);
                }
            } else if (section == 1) {hcalDigiHit = drawer_->drawBox(hitVec[i]->getX(), hcal_ecal_xy/2+hcal_layer_thick*layer, ecal_front_z+bar*bar_width, 150, scint_thick, ecal_front_z+(bar+1)*bar_width, 0, color, 0, digiName);
            } else if (section == 3) {hcalDigiHit = drawer_->drawBox((hcal_ecal_xy/2+hcal_layer_thick*layer), hitVec[i]->getY(), ecal_front_z+bar*bar_width, scint_thick, 150, ecal_front_z+(bar+1)*bar_width, 0, color, 0, digiName);
            } else if (section == 2) {hcalDigiHit = drawer_->drawBox(hitVec[i]->getX(), -(hcal_ecal_xy/2+hcal_layer_thick*layer), ecal_front_z+bar*bar_width, 150, scint_thick, ecal_front_z+(bar+1)*bar_width, 0, color, 0, digiName);
            } else if (section == 4) {hcalDigiHit = drawer_->drawBox(-(hcal_ecal_xy/2+hcal_layer_thick*layer), hitVec[i]->getY(), ecal_front_z+bar*bar_width, scint_thick, 150, ecal_front_z+(bar+1)*bar_width, 0, color, 0, digiName);
            }

            if (hcalDigiHit != 0) {
                if (isNoise) { hcalDigiHit->SetRnrSelf(0); }
                hcalHits_->AddElement(hcalDigiHit);
            }
            //if (hcalDigiHit2 != 0) {
            //    hcalHits_->AddElement(hcalDigiHit2);
            //}

        }

        hcalHits_->SetPickableRecursively(1);
        hits_->AddElement(hcalHits_);

    }

    void EventObjects::drawRecoilHits(TClonesArray* hits) {

        ldmx::SimTrackerHit* hit;
        for (TIter next(hits); hit = (ldmx::SimTrackerHit*)next();) {

            std::vector<float> xyzPos = hit->getPosition();

            if ((xyzPos[2] > 4 && xyzPos[2] < 5) || (xyzPos[2] > 19 && xyzPos[2] < 20) || (xyzPos[2] > 34 && xyzPos[2] < 35) || (xyzPos[2] > 49 && xyzPos[2] < 50)) {

                TEveBox *recoilHit = drawer_->drawBox(xyzPos[0], 0, xyzPos[2], 1, stereo_strip_length, xyzPos[2]+recoil_sensor_thick, 0, kRed+1, 0, "Recoil Hit");
                recoilTrackerHits_->AddElement(recoilHit);

            } else if ((xyzPos[2] > 10 && xyzPos[2] < 11) || (xyzPos[2] > 40 && xyzPos[2] < 41)) {

                TVector3 rotPos = {xyzPos[0], xyzPos[1], xyzPos[2]};
                rotPos.RotateZ(-stereo_angle);

                TEveBox *recoilHit = drawer_->drawBox(rotPos[0], 0, xyzPos[2], 1, stereo_strip_length, xyzPos[2]+recoil_sensor_thick, stereo_angle, kRed+1, 0, "Recoil Hit");
                recoilTrackerHits_->AddElement(recoilHit);

            } else if ((xyzPos[2] > 25 && xyzPos[2] < 26) || (xyzPos[2] > 55 && xyzPos[2] < 56)) {

                TVector3 rotPos = {xyzPos[0], xyzPos[1], xyzPos[2]};
                rotPos.RotateZ(stereo_angle);

                TEveBox *recoilHit = drawer_->drawBox(rotPos[0], 0, xyzPos[2], 1, stereo_strip_length, xyzPos[2]+recoil_sensor_thick, -stereo_angle, kRed+1, 0, "Recoil Hit");
                recoilTrackerHits_->AddElement(recoilHit);

            } else if (xyzPos[2] > 65) {
                if (fabs(xyzPos[1]) > 1.0) { // dead region

                    if (xyzPos[1] > 0) {

                        TEveBox *recoilHit = drawer_->drawBox(xyzPos[0], mono_strip_length/2+1, xyzPos[2], 1, mono_strip_length, xyzPos[2]+recoil_sensor_thick, 0, kRed, 0, "Recoil Hit");
                        recoilTrackerHits_->AddElement(recoilHit);
                    } else {

                        TEveBox *recoilHit = drawer_->drawBox(xyzPos[0], -mono_strip_length/2-1, xyzPos[2], 1, mono_strip_length, xyzPos[2]+recoil_sensor_thick, 0, kRed, 0, "Recoil Hit");
                        recoilTrackerHits_->AddElement(recoilHit);
                    }
                }
            }
        }

        hits_->AddElement(recoilTrackerHits_);
    }

    void EventObjects::drawECALClusters(TClonesArray* clusters) {

        TEveRGBAPalette* palette = new TEveRGBAPalette(0,4000.0);

        int iC = 0;
        EcalCluster* cluster;
        for (TIter next(clusters); cluster = (ldmx::EcalCluster*)next();) {

            TString clusterName;
            clusterName.Form("ECAL Cluster %d", iC);

            TEveElement* ecalCluster = new TEveElementList(clusterName);

            double energy = cluster->getEnergy();
            std::vector<unsigned int> clusterHitIDs = cluster->getHitIDs();

            int numHits = clusterHitIDs.size();

            for (int iHit = 0; iHit < numHits; iHit++) {
                unsigned int cellID = clusterHitIDs[iHit]>>15;
                unsigned int moduleID = (clusterHitIDs[iHit]<<17)>>29;
                int layer = (clusterHitIDs[iHit]<<20)>>24;
                unsigned int combinedID = 10*cellID + moduleID;

                std::pair<double, double> xyPos = hexReadout_->getCellCenterAbsolute(combinedID);
    
                const UChar_t* rgb = palette->ColorFromValue(energy);
                TColor* aColor = new TColor();
                Int_t color = aColor->GetColor((Int_t)rgb[0], (Int_t)rgb[1], (Int_t)rgb[2]);
    
                TEveBox *ecalDigiHit = drawer_->drawBox(xyPos.first, xyPos.second, layerZPos[layer]+ecal_front_z-1.5, 3, 3, layerZPos[layer]+ecal_front_z+1.5, 0, color, 0, "RecHit");
                ecalCluster->AddElement(ecalDigiHit);

                if (numHits < 2) { 
                    ecalCluster->SetPickableRecursively(0);
                } else {
                    ecalCluster->SetPickableRecursively(1);
                }
            }
            ecalClusters_->AddElement(ecalCluster);
            iC++;
        }

        ecalClusters_->SetPickable(1);
        recoObjs_->AddElement(ecalClusters_);
    }

    void EventObjects::drawHCALTracks(TClonesArray* tracks) {
        
        const float undefinedpos = 100000.0;

        int iT = 0;
        HcalTrack* track;
        for (TIter next(tracks); track = (ldmx::HcalTrack*)next();) {
            
            //starting and ending points of track
            std::vector<float> start( 3 , 0.0 ), end( 3 , 0.0 );
            
            //Iterate through hits in the track
            TRefArray hitlist = track->getTrack();
            for (int iHit = 0; iHit < hitlist.GetEntriesFast(); iHit++) {
                
                const ldmx::HcalHit* chit = (const ldmx::HcalHit*)(hitlist.At(iHit));
                //hit information
                int layer = chit->getLayer();
                int strip = chit->getStrip();
                std::vector<float> currpos( 3 , undefinedpos+1 ); //current position of chit
                
                //Lifting position from HcalHit (which is currently calculated from simhit)
                currpos[0] = chit->getX();
                currpos[1] = chit->getY();
                currpos[2] = chit->getZ();
                
                //uncomment below to lift position from layer,strip information
                /*
                switch ( chit->getSection() ) {
                    case HcalSection::BACK : 
                        if ( layer % 2 == 0 ) {
                            //horizontal
                            currpos[1] = bar_width*(0.5+bar)-hcal_y_width/2;
                        } else {
                            //vertical
                            currpos[0] = bar_width*(0.5+bar)-hcal_x_width/2;
                        }
                        currpos[2] = (layer-1)*hcal_layer_thick+hcal_front_z+scint_thick;
                        break;
                    case HcalSection::TOP : 
                        currpos[1] = hcal_ecal_xy/2+hcal_layer_thick*layer;
                        currpos[2] = ecal_front_z+bar*bar_width;
                        break;
                    case HcalSection::BOTTOM : 
                        currpos[1] = -(hcal_ecal_xy/2+hcal_layer_thick*layer);
                        currpos[2] = ecal_front_z+bar*bar_width;
                        break;
                    case HcalSection::LEFT : 
                        currpos[0] = (hcal_ecal_xy/2+hcal_layer_thick*layer);
                        currpos[2] = ecal_front_z+bar*bar_width;
                        break;
                    case HcalSection::RIGHT : 
                        currpos[0] = -(hcal_ecal_xy/2+hcal_layer_thick*layer);
                        currpos[2] = ecal_front_z+bar*bar_width;
                        break;
                    default : 
                        std::cout << "[EventObjects::drawHCALTracks] HcalSection undefined, skipping hit" << std::endl;
                        break;
                }
                */

                //Only update start/end position if currpos is earlier/later than start/end
                // checking if coordinates are defined as well
                if ( currpos[2] < start[2] and currpos[2] > 0.0 ) {
                    if ( currpos[0] < undefinedpos ) {
                        start[0] = currpos[0];
                    }

                    if ( currpos[1] < undefinedpos ) {
                        start[1] = currpos[1];
                    }

                    start[2] = currpos[2];
                }

                if ( currpos[2] > end[2] and currpos[2] < undefinedpos ) {
                    if ( currpos[0] < undefinedpos ) {
                        end[0] = currpos[0];
                    }

                    if ( currpos[1] < undefinedpos ) {
                        end[1] = currpos[1];
                    }

                    end[2] = currpos[2];
                }
                
            } //iterate through hits in track (iHit)
            
            //Declare new display element to represent current track
            TString trackName;
            trackName.Form("HCAL Track %d", iT);
            
            double r = pow(pow((end[0]-start[0]),2) + pow((end[1]-start[1]),2) + pow((end[2]-start[2]),2),0.5);
            
            TEveArrow* trackray = new TEveArrow( end[0]-start[0] , end[1]-start[1] , end[2]-start[2] , start[0] , start[1] , start[2] );
            trackray->SetElementName(trackName);
            trackray->SetMainColor(kBlack);
            trackray->SetTubeR(60*0.02/r);
            trackray->SetConeL(100*0.02/r);
            trackray->SetConeR(150*0.02/r);
            trackray->SetPickable(kTRUE);

            hcalTracks_->AddElement(trackray);
            iT++;
        } //iterate through tracks in collection (track, iT)

        //hcalTracks_->SetPickable(1);
        recoObjs_->AddElement(hcalTracks_);
        
    }
    
    void EventObjects::drawECALSimParticles(TClonesArray* ecalSimParticles) {

        ldmx::SimTrackerHit* ecalSPP;
        std::vector<SimTrackerHit*> simVec;
        std::vector<SimTrackerHit*> filteredSimVec;
        for (TIter next(ecalSimParticles); ecalSPP = (ldmx::SimTrackerHit*)next();) {
            simVec.push_back(ecalSPP);
        }

        std::sort(simVec.begin(), simVec.end(), compSims);
           
        SimParticle* lastP = 0; // sometimes multiple SP hits from same particle
        for (int j = 0; j < simVec.size(); j++) {
            SimParticle* sP = simVec[j]->getSimParticle();
            if (sP == lastP) continue;
            lastP = sP;
            filteredSimVec.push_back(simVec[j]);
        }

        std::sort(filteredSimVec.begin(), filteredSimVec.end(), compSimsP);

        for (int j = 0; j < filteredSimVec.size(); j++) {

            SimParticle* sP = filteredSimVec[j]->getSimParticle();

            std::vector<double> pVec = filteredSimVec[j]->getMomentum();
            std::vector<float> rVec = filteredSimVec[j]->getPosition();
            double p = pow(pow(pVec[0],2)+pow(pVec[1],2)+pow(pVec[2],2),0.5);

            double E = sP->getEnergy();

            std::vector<double> simStart = sP->getVertex();
            std::vector<double> simEnd = sP->getEndPoint();
            double rCheck = pow(pow(simEnd[0],2)+pow(simEnd[1],2)+pow(simEnd[2],2),0.5);

            double scale = 1;
            double largest = 0;
            if (abs(simEnd[0]) > 3500.0) {
                 scale = 500.0/abs(simEnd[0]-simStart[0]);
                 largest = simEnd[0];
            }
            if (abs(simEnd[1]) > 3500.0 && abs(simEnd[1]) > largest) {
                 scale = 500.0/abs(simEnd[1]-simStart[1]);
                 largest = simEnd[1];
            }
            if (abs(simEnd[2]) > 3500.0 && abs(simEnd[2]) > 3500) {
                 scale = 500.0/abs(simEnd[2]-simStart[2]);
            }

            double r = pow(pow(scale*(simEnd[0]-simStart[0]),2) + pow(scale*(simEnd[1]-simStart[1]),2) + pow(scale*(simEnd[2]-simStart[2]),2),0.5);
            signed int pdgID = sP->getPdgID();

            TEveArrow* simArr = new TEveArrow(scale*(simEnd[0]-simStart[0]),scale*(simEnd[1]-simStart[1]),scale*(simEnd[2]-simStart[2]),simStart[0],simStart[1],simStart[2]);

            simArr->SetSourceObject(sP);
            simArr->SetMainColor(kBlack);
            simArr->SetTubeR(60*0.02/r);
            simArr->SetConeL(100*0.02/r);
            simArr->SetConeR(150*0.02/r);
            simArr->SetPickable(kTRUE);
            if (p < simThresh_) { simArr->SetRnrSelf(kFALSE); }

            TString name;
            name.Form("PDG = %d, p = %1.5g MeV/c", pdgID, p);
            simArr->SetElementName(name);
            ecalSimParticles_->AddElement(simArr);
        }

        hits_->AddElement(ecalSimParticles_);
    }
}
