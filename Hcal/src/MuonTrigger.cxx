/**
 * @file MuonTrigger.cxx
 * @brief Implementation file for MuonTrigger producer
 * @author Tom Eichlersmith, University of Minnesota
 */

#include "Hcal/MuonTrigger.h"

namespace ldmx {

    void MuonTrigger::configure(const ldmx::ParameterSet& ps) {
        
        hcalHitCollName_ = ps.getString( "HcalHitCollectionName" );
        hcalHitPassName_ = ps.getString( "HcalHitPassName" );

        triggerObjectName_ = ps.getString( "TriggerObjectName" );

        minConsecLayersHit_[0] = ps.getInteger( "MinConsecutiveLayersHitBackHcal" );
        minConsecStripsHit_[0] = ps.getInteger( "MinConsecutiveStripsHitBackHcal" );

        minConsecLayersHit_[1] = ps.getInteger( "MinConsecutiveLayersHitTopHcal" );
        minConsecStripsHit_[1] = ps.getInteger( "MinConsecutiveStripsHitTopHcal" );

        minConsecLayersHit_[2] = ps.getInteger( "MinConsecutiveLayersHitBottomHcal" );
        minConsecStripsHit_[2] = ps.getInteger( "MinConsecutiveStripsHitBottomHcal" );

        minConsecLayersHit_[3] = ps.getInteger( "MinConsecutiveLayersHitLeftHcal" );
        minConsecStripsHit_[3] = ps.getInteger( "MinConsecutiveStripsHitLeftHcal" );

        minConsecLayersHit_[4] = ps.getInteger( "MinConsecutiveLayersHitRightHcal" );
        minConsecStripsHit_[4] = ps.getInteger( "MinConsecutiveStripsHitRightHcal" );

        return;
    }

    void MuonTrigger::produce(ldmx::Event& event) {
        
        //get hcal hits
        const TClonesArray *hcalHits = event.getCollection( hcalHitCollName_ , hcalHitPassName_ );

        //sort layers hit and strips hit into sections
        std::set<int> layersHit[5], stripsHit[5];
        int nHits = hcalHits->GetEntriesFast();
        for ( int iH = 0; iH < nHits; iH++ ) {
            const HcalHit *chit = (const HcalHit *)(hcalHits->At(iH));
            int section = chit->getSection();
            if ( section >= 0 and section < 5 ) {
                layersHit[ section ].insert( chit->getLayer() );
                stripsHit[ section ].insert( chit->getStrip() );
            } else {
                std::cerr << "WARNING [ MuonTrigger::produce ] : Unknown HcalSection!" << std::endl;
            }
        } //add each hit information to set

        bool pass = false;
        int consecLayersHit[5], consecStripsHit[5];
        for ( int s = 0; s < 5; s++ ) {
            
            consecLayersHit[s] = consecCount( layersHit[s] );
            consecStripsHit[s] = consecCount( stripsHit[s] );

            if ( consecLayersHit[s] > minConsecLayersHit_[s] and
                 consecStripsHit[s] > minConsecStripsHit_[s]
               ) {
                pass = true;
            } //if any section passes
            
        } //check each section
        
        //build result object
        result_.set( triggerObjectName_ , pass , 20 );
        for ( int s = 0; s < 5; s++ ) {
            result_.setAlgoVar( 4*s   , minConsecLayersHit_[s] );
            result_.setAlgoVar( 4*s+1 , minConsecStripsHit_[s] );
            result_.setAlgoVar( 4*s+2 , consecLayersHit[s] );
            result_.setAlgoVar( 4*s+3 , consecStripsHit[s] );
        }

        event.addToCollection( "Trigger" , result_ );

        return;
    }
    
    int MuonTrigger::consecCount( const std::set<int> &numbers ) const {
        
        int maxConsec(-1);
        
        if ( !numbers.empty() ) {
            
            int consec(0);
            int prev = *numbers.cbegin();
            for ( int curr : numbers ) {
                   
                if ( curr - prev > 1 ) {
                    if ( consec > maxConsec ) //improve upon max
                        maxConsec = consec;
    
                    consec = 0; //reset counter
                } //check if not consecutive anymore
     
                consec++; //count this current number

                prev = curr; //update prev
            } //go through ordered numbers
    
            if ( consec > maxConsec )
                maxConsec = consec;
    
        } //numbers is non-empty

        return maxConsec;

    }   
}

DECLARE_PRODUCER_NS(ldmx, MuonTrigger);
