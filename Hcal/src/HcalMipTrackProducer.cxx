/**
 * @file HcalMipTrackProducer.cxx
 * @brief Implementation file for HcalMipTrackProducer class.
 * @author Tom Eichlersmith, University of Minnesota
 */

#include "Hcal/HcalMipTrackProducer.h"

namespace ldmx {

    void HcalMipTrackProducer::configure(const ldmx::ParameterSet& ps) {

        hcalHitCollName_ = ps.getString( "HcalHitCollectionName" );

        hcalHitPassName_ = ps.getString( "HcalHitPassName" );

        maxTrackCount_ = 100;

        hcalMipTracks_ = new TClonesArray( "ldmx::HcalMipTrack" , maxTrackCount_ );

        hcalMipTracksCollName_ = ps.getString( "HcalMipTrackCollectionName" );

        minPE_ = ps.getDouble( "MinimumPE" );

        maxEnergy_ = ps.getDouble( "MaximumEnergy" );

        fracTotalClusters_ = ps.getDouble( "FractionTotalClusters" );

        if ( fracTotalClusters_ < 0.0 or fracTotalClusters_ > 1.0 ) {
            std::cerr << "[ HcalMipTrackProducer::configure ] : FractionTotalClusters is out of viable range!\n";
            std::cerr << "                                      Must be set within [0,1]\n";
            fracTotalClusters_ = 0.2;
        }
        
        fracClustersLeft_ = ps.getDouble( "FractionClustersLeft" );
        
        if ( fracClustersLeft_ < 0.0 or fracClustersLeft_ > 1.0 ) {
            std::cerr << "[ HcalMipTrackProducer::configure ] : FractionClustersLeft is out of viable range!\n";
            std::cerr << "                                      Must be set within [0,1]\n";
            fracClustersLeft_ = 0.8;
        }

        meanTime_produce_ = 0.0;
        meanNumTouchLogs_ = 0.0;

        return;
    }

    void HcalMipTrackProducer::produce(ldmx::Event& event) {
        
        numTouchLogs_ = 0;
        std::clock_t start_produce;
        start_produce = std::clock();

        //Clear event containers
        hcalHitLog_.clear();
        clusterLog_.clear();

        const TClonesArray *rawhits = event.getCollection( hcalHitCollName_ , hcalHitPassName_ );

        //go through raw hits and ignore noise hits
        int nhits = rawhits->GetEntriesFast();
        for ( int iH = 0; iH < nhits; iH++ ) {
            numTouchLogs_++;
            HcalHit* chit = dynamic_cast<HcalHit*>(rawhits->At( iH ));

            if ( isNotNoise( chit ) ) {
                int section = chit->getSection();
                int layer = chit->getLayer();
                int strip = chit->getStrip();

                unsigned int key = section*1000*100 + layer*100 + strip;

                hcalHitLog_[ key ] = chit;
            } //if not noise hit

        } //iterate through rawhits (iH)
        
        clusterHits();
        
        std::vector< unsigned int > track_mipids;
        int trackcnt = 0;
        int nclustersintracks = 0;
        while ( findSeed( false ) and trackcnt < maxTrackCount_ ) {
            
            if ( buildTrack( track_mipids ) ) {
                //able to build track from seed (add to collection) 
                HcalMipTrack *track = (HcalMipTrack *)(hcalMipTracks_->ConstructedAt( trackcnt ));
                for ( unsigned int mipid : track_mipids ) {
                    numTouchLogs_++;
                    MipCluster* cmip = &clusterLog_[ mipid ];
                     
                    //add HcalHits to track
                    for ( int i = 0; i < cmip->getNumHits(); i++ ) {
                        track->addHit( cmip->getHcalHit( i ) );
                    }//iterate through hits in cluster
                    
                    //add real point to track
                    std::vector<double> point, errors;
                    cmip->getPoint( point , errors );
                    track->addPoint( point , errors );
                    
                    //erase mipid from log
                    clusterLog_.erase( mipid );

                } //add clusters with mipids to track
                
                nclustersintracks += track_mipids.size();
                trackcnt++;

            } else {
                //unable to build track, mark as bad seed     
                clusterLog_.at( seedID_ ).wasBadSeed();

            } //if able to build track

        } //repeat track construction until no more viable seeds
        
        //store collection in event bus
        event.add( hcalMipTracksCollName_ , hcalMipTracks_ ); 
        
        numTracksPerEvent_[ trackcnt ] ++;
        
        if ( meanClustersPerTrack_.count( trackcnt ) < 1 )
            meanClustersPerTrack_[ trackcnt ] = 0.0;
        
        double meanclusters = 0.0;
        if ( trackcnt > 0 )
            meanclusters = (double)(nclustersintracks)/(double)(trackcnt);
        double prev_mean = meanClustersPerTrack_.at(trackcnt);
        int nevents = numTracksPerEvent_.at( trackcnt );
        meanClustersPerTrack_[ trackcnt ] = ((double)(nevents-1)/(double)(nevents))*prev_mean +
            meanclusters/nevents;
        
        int ievent = event.getEventHeader()->getEventNumber();
        double time_produce = (std::clock() - start_produce)/(double)(CLOCKS_PER_SEC / 1000 ); //ms
        meanTime_produce_ = ((double)(ievent)/(double)(ievent+1))*meanTime_produce_ +
            time_produce/(double)(ievent+1);
        meanNumTouchLogs_ = ((double)(ievent)/(double)(ievent+1))*meanNumTouchLogs_ +
            numTouchLogs_/(double)(ievent+1);

        return;
    }

    bool HcalMipTrackProducer::isNotNoise( HcalHit* hit ) const {
        return ( !hit->getNoise() and hit->getPE() > minPE_ );
    }

    bool HcalMipTrackProducer::isMip( const MipCluster* cluster ) const {
        return ( cluster->getEnergy() < maxEnergy_ );
    }

    void HcalMipTrackProducer::clusterHits() {
        
        // cluster hits in same layer
        std::map< unsigned int , HcalHit* >::iterator itH;
        std::map< unsigned int , HcalHit* >::iterator prev_itH = hcalHitLog_.begin();
        MipCluster *current_cluster = new MipCluster();
        for ( itH = hcalHitLog_.begin(); itH != hcalHitLog_.end(); ++itH ) {
            numTouchLogs_++;
            //itH and prev_itH will both point to hcalHitLog_.begin() on first loop
            unsigned int keydif = itH->first - prev_itH->first;

            if ( keydif > 1 ) {
                //current hit is in different cluster
                // add previous cluster to log and clear temporary cluster
                current_cluster->setUID( prev_itH->first );
                current_cluster->set(); //calculate real space point
                if ( isMip( current_cluster ) ) {
                    clusterLog_[ current_cluster->getUID() ] = *current_cluster;
                }//check if mip
                delete current_cluster;
                current_cluster = new MipCluster();
            }//check if separate cluster

            current_cluster->addHit( itH->second );

            prev_itH = itH;

        }//iterate through sorted hcalHitLog (itH)

        //clean up at end of hit log
        if ( current_cluster ) {
            current_cluster->setUID( prev_itH->first );
            current_cluster->set();
            if ( isMip( current_cluster ) ) {
                clusterLog_[ current_cluster->getUID() ] = *current_cluster;
            }
            delete current_cluster;
            current_cluster = nullptr;
        }
        
        //set the total number of clusters
        minNumClusters_ = static_cast<int>( clusterLog_.size()*fracTotalClusters_ );

        return; 
    }

    bool HcalMipTrackProducer::findSeed( const bool useMedian ) {
        
        seedPoint_.clear();
        seedErrors_.clear();
        seedID_ = 0;

        if ( clusterLog_.size() > minNumClusters_ ) {

            std::map< const double , unsigned int > zpos_id;
            
            std::vector< double > point , errors;
            for ( auto keyclust : clusterLog_ ) {
                numTouchLogs_++;
                if ( (keyclust.second).isGoodSeed() ) {
                    (keyclust.second).getPoint( point , errors );
                    zpos_id[ point[2] ] = keyclust.first;
                } //if not been a seed before
            } //go through clusterLog_
            
            std::map< const double , unsigned int >::iterator seed_it = zpos_id.begin();
            
            if ( seed_it != zpos_id.end() ) {
            
                if ( useMedian ) {
                    //move seed_it to rough media
                    for ( int i = 0; i < zpos_id.size()/2; i++ )
                        ++seed_it;
                }
    
                seedID_ = seed_it->second;
                clusterLog_.at( seedID_ ).getPoint( seedPoint_ , seedErrors_ );
                numTouchLogs_++;

            } //zpos_id is not empty

        } //enough clusters in log
        
        return (!seedPoint_.empty());
    }
 
    bool HcalMipTrackProducer::buildTrack( std::vector< unsigned int > &track_mipids ) {

        track_mipids.clear();
        
        //iterate through all pairs of points
        HcalMipTrack best_track;
        std::map< unsigned int , MipCluster >::iterator itEnd, itC; //iterators for map
        for ( itEnd = clusterLog_.begin(); itEnd != clusterLog_.end(); ++itEnd ) {
            
            if ( itEnd->first != seedID_ ) {
                
                //construct track in cylinder
                std::vector< double > endpoint, enderrors;
                (itEnd->second).getPoint( endpoint , enderrors );
                
                //calculate line properties 
                //  (direction, negative direction, smudging factor)
                std::vector< double > direction( 3 , 0.0 ), negdirection( 3 , 0.0 );
                std::vector< double > linesmudge( seedErrors_ );
                for ( unsigned int iC = 0; iC < 3; iC++ ) {
                    direction[ iC ] = endpoint[iC] - seedPoint_[iC];
                    negdirection[ iC ] = -1*direction[iC];
    
                    //determine line smudging
                    if ( enderrors[iC] < linesmudge[iC] ) {
                        linesmudge[iC] = enderrors[iC];
                    }
                } //space coordinates (iC)
                
                std::vector< unsigned int > ctrack_mipids; //ids of mip clusters in track
                //iterate through all clusters to see if they are in track
                for ( itC = clusterLog_.begin(); itC != clusterLog_.end(); ++itC ) {
                    numTouchLogs_++;
                    std::vector< double > point, errors;
                    (itC->second).getPoint( point , errors );
                    
                    //construct hit box
                    // could add a fudge factor controlled by user
                    std::vector< double > maxBox( 3 , 0.0 ), minBox( 3 , 0.0 );
                    for ( unsigned int iC = 0; iC < 3; iC++ ) {
                        maxBox[iC] = point[iC] + errors[iC] + linesmudge[iC];
                        minBox[iC] = point[iC] - errors[iC] - linesmudge[iC];
                    }
                    
                    //see if ray hits box on either side of origin along line
                    if ( rayHitBox( seedPoint_ , direction    , minBox , maxBox ) or
                         rayHitBox( seedPoint_ , negdirection , minBox , maxBox ) ) {
                        ctrack_mipids.push_back( itC->first );
                    } 
    
                } //iterate through all clusters to see if they are in track (itC)
                
                //check if current track is acceptable and an improvement
                if ( isAcceptableTrack( ctrack_mipids ) and 
                     ctrack_mipids.size() > track_mipids.size() ) {
                    track_mipids = ctrack_mipids;
                }//ctrack is a acceptable and includes more clusters than other track
            
            }//make sure origin and end aren't the same

        } //go through remaining hits as end point (itEnd)
        
        return (!track_mipids.empty());
    }
   
    bool HcalMipTrackProducer::rayHitBox( const std::vector<double> origin , const std::vector<double> dir , 
                    const std::vector<double> minBox , const std::vector<double> maxBox ) const {
        
        bool originInside = true;
        bool originBetween[3];

        //Determine planes that are on the "front" of the box w.r.t. the origin of the ray
        std::vector<double> candidatePlane( 3 , 0.0 );
        for ( unsigned int iC = 0; iC < 3; iC++ ) {
            
            if ( origin.at(iC) < minBox.at(iC) ) {
                originBetween[iC] = false;
                candidatePlane[iC] = minBox.at(iC);
                originInside = false;
            } else if ( origin.at(iC) > maxBox.at(iC) ) {
                originBetween[iC] = false;
                candidatePlane[iC] = maxBox.at(iC);
                originInside = false;
            } else {
                originBetween[iC] = true;
            } //where origin is w.r.t. box
                
        } //iterate through coordinates (iC)
        
        //Origin Inside Box ==> Ray Intersects Box
        if ( originInside ) {
            return true;
        }

        //Calculate maximum T distances to candidatePlanes
        std::vector<double> maxT( 3 , 0.0 );
        for ( unsigned int iC = 0; iC < 3; iC++ ) {
            
            if ( !originBetween[iC] and dir.at(iC) != 0.0 ) {
                maxT[ iC ] = ( candidatePlane[iC] - origin.at(iC) ) / dir.at(iC);        
            } else {
                maxT[ iC ] = -1.0;
            }

        } //iterate through coordinates (iC)

        //Get largest of maxTs for the final choice of intersection
        unsigned int iMax = 0;
        for ( unsigned int iC = 0; iC < 3; iC++ ) {
            if ( maxT[ iMax ] < maxT[ iC ] ) {
                iMax = iC;
            }
        } //iterate through coordinates (iC)
        
        //Check if final candidate is inside box
        if ( maxT[ iMax ] < 0.0 ) {
            return false;
        }

        for ( unsigned int iC = 0; iC < 3; iC++ ) {
            
            if ( iMax != iC ) {
                double coordinate = origin.at(iC) + maxT[iC]*dir.at(iC);
                if ( coordinate < minBox.at(iC) or coordinate > maxBox.at(iC) ) {
                    //coordinate outside box
                    return false;
                }
            } //if coordinate is not maximum T plane
        } //iterate through coordinates (iC)

        return true;
    }

    bool HcalMipTrackProducer::isAcceptableTrack( const std::vector< unsigned int > &track_mipids ) const {
        return ( track_mipids.size() > fracClustersLeft_*clusterLog_.size() );
    }

    void HcalMipTrackProducer::mergeTracks() {
        
        //Get End points of each track
        //Compare slopes
        //compare starts to ends

        return;
    }
}

DECLARE_PRODUCER_NS( ldmx , HcalMipTrackProducer );
