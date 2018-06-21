/**
 * @file HitLog.h
 * @brief Class implementation of hit log to search and reconstruct.
 * @author Tom Eichlersmith, University of Minnesota
 */

#ifndef PLAYTEST_HITLOG_H
#define PLAYTEST_HITLOG_H

//Standard Libraries
#include <iostream> //Checks to std::cout for development purposes
#include <vector> //Vector of hits per layer
#include <map> //std::map for storage tree
#include <queue> //std::queue for search cone and layer list
#include <utility> //std::pair for storage tree
#include <iterator> //std::next and std::prev for search through map
#include <cmath> //floor and ceil for stripbounds calculations

//LDMX Framework
#include "Event/HcalHit.h" //Get Hcal Specific information from hit


namespace ldmx {

    //* type that will be used to reference hits
    typedef const ldmx::HcalHit* HitPtr;

    /**
     * @class HitLog
     * @brief Stores HitPtrs in a std::map for easy searching and track reconstruction.
     */
    class HitLog {
        public:
            /**
             * Default Constructor.
             */
            HitLog();

            /**
             * Preferred Constructor.
             */
            HitLog( const float minPE , const int conedepth , const int coneangle , const int trackwidth ,
                    const float origin , const float lowside , const float upside , const int nstrips , const int nlayers );

            /**
             * Add hit to log
             */
            void AddHit( HitPtr hit );

            /**
             * Attempt to reconstruct a track from a seed layer.
             *
             * @param seedlayer layer index for seed
             * @param track plausible track - should be empty
             * @return true if a track was found
             */
            bool TrackSearch( int seedlayer , std::vector< HitPtr > &track );


        private:
            
            std::map< int , HitPtr > log_; //* map that will be used to store the hits

            std::vector< bool > layercheck_; //* vector of layers to see if searched or not

            std::queue< std::pair< int , int > > cone_; //* search cone in keys around seed
            std::queue< int > layerlist_; //* list of layers to go through after partial track is begun
            
            int nlayers_; //* number of layers in detector
            int nstrips_; //* number of strips per layer

            float minPE_; //* Minimum number of PEs to not be considered noise

            int conedepth_; //* depth of search cone around seed in layers
            int coneangle_; //* angular opening of cone around seed in strips across the first layer

            int trackwidth_; //* width of extended track to search in number of strips

            float origin_; //* center of Ecal in strips
            float lowside_; //* low side of Ecal in strips
            float upside_; //* upper side of Ecal in strips

            /**
             * Function to generate key for a given hit.
             * Relies on layermod_.
             *
             * @param hit pointer to HcalHit instance that a key is needed for
             * @return integer key value
             */
            int KeyGen( HitPtr hit ) const;

            /**
             * Corrects for strip numbers outside of real range.
             */
            void CorrectStrip( int &strip) const;
            
            /**
             * Finds a seed strip given a seed layer.
             * Looks through entire log if nothing is found in given layer.
             *
             */
            bool FindSeed( int &seedlayer , int &seedstrip ) const;
            
            /**
             * Constructs search cone around seed and list of layers that aren't in cone or seed.
             *
             */
            void SetSearchCone( const int seedlayer , const int seedstrip );
            
            /**
             * Begins partial track by searching through cone around seed.
             */
            bool BeginPartialTrack( std::vector< HitPtr > &track ) const;

            /**
             * Search for next mip given layer and partial track.
             * Will add to track if found a mip hit.
             * Assumes track has AT LEAST two hits in it.
             */
            bool SearchLayer( const int layer , std::vector< HitPtr > &track ) const;

            /**
             * Check if plausible track is acceptable.
             */
            bool isAcceptableTrack( const std::vector< HitPtr > track ) const;

            /**
             * Function to search a specific range of a HitLog for a hit.
             * Returns a pair of nullptrs if failed to find an isolated hit.
             *
             * @param lowkey lower bound key
             * @param upkey upper bound key
             * @param track partial track to add hit to if found
             * @return true if successfully found a hit in the given key range 
             */
            bool SearchByKey( const int lowkey , const int upkey , std::vector< HitPtr > &track ) const;

    };

}

#endif /* PLAYTEST_HITLOG_H */
