/**
 * @file HcalMipTriggerResult.h
 * @brief Header file for HcalMipTriggerResult class.
 * @author Tom Eichlersmith, University of Minnesota
 */

#ifndef EVENT_HCALMIPTRIGGERRESULT_H
#define EVENT_HCALMIPTRIGGERRESULT_H

//STL
#include <vector> //storage of vectors

//LDMX Framework
#include "Event/TriggerResult.h" //Base class

namespace ldmx {
    
    /**
     * @class HcalMipTriggerResult
     * @brief Storage class for result from HcalMipTriggerProducer.
     */
    class HcalMipTriggerResult : public TriggerResult {
        public:
            /**
             * Default Constructor
             */
            HcalMipTriggerResult(); 

            /**
             * Destructor
             */
            ~HcalMipTriggerResult(); 

            /**
             * Print a description of this object.
             */
            void Print(Option_t *option = "") const;

            /**
             * Reset the TriggerResult object.
             */
            void Clear(Option_t *option = "");

            /**
             * Copy this object.
             * @param o The target object.
             */
            void Copy(TObject& o) const;

            /**
             * Add a track to the vector of tracks.
             */
            void addTrack( const std::vector< unsigned int > &track );

            /**
             * Get number of tracks in this result.
             */
            int getNumTracks() const; 

            /**
             * Get the whole vector of tracks.
             */
            std::vector< std::vector< unsigned int > > getTrackVec() const; 
            
        private:

            /** The list of tracks found (may be empty) */
            std::vector< std::vector< unsigned int > > trackVec_;

            /**
             * ROOT Class Definition
             */
            //ClassDef( HcalMipTriggerResult , 1 );

    };
}

#endif /* EVENT_HCALMIPTRIGGERRESULT_H */