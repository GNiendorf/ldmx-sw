/**
 * @file TargetENProcessFilter.h
 * @brief Class defining a UserActionPlugin that biases Geant4 to only process 
 *        events which involve an electronuclear reaction in the target
 * @author Omar Moreno, SLAC National Accelerator Laboratory
 */

#ifndef BIASING_TARGETENPROCESSFILTER_H_
#define BIASING_TARGETENPROCESSFILTER_H_

//------------//
//   Geant4   //
//------------//
#include "G4RunManager.hh"

//----------//
//   LDMX   //
//----------//
#include "SimPlugins/UserActionPlugin.h"
#include "Biasing/BiasingMessenger.h"
#include "Biasing/TargetENProcessFilterMessenger.h"

namespace ldmx {

    class TargetENProcessFilterMessenger; 

    class TargetENProcessFilter : public UserActionPlugin {

        public:

            /**
             * Class constructor.
             */
            TargetENProcessFilter();

            /**
             * Class destructor.
             */
            ~TargetENProcessFilter();

            /**
             * Get the name of the plugin.
             * @return The name of the plugin.
             */
            virtual std::string getName() {
                return "TargetENProcessFilter";
            }

            /**
             * Get whether this plugin implements the event action.
             * @return True if the plugin implements the event action.
             */
            virtual bool hasEventAction() { 
                return true;
            }

            /**
             * Get whether this plugin implements the stepping action.
             * @return True to indicate this plugin implements the stepping action.
             */
            bool hasSteppingAction() {
                return true;
            }

            /**
             * Get whether this plugin implements the stacking aciton.
             * @return True to indicate this plugin implements the stacking action.
             */
            bool hasStackingAction() { 
                return true;
            }

            /**
             * Implementmthe stepping action which performs the target volume biasing.
             * @param step The Geant4 step.
             */
            void stepping(const G4Step* step);

            /**
             * End of event action.
             */
            virtual void endEvent(const G4Event*);

            /** 
             * @param volume Set the volume that the filter will be applied to. 
             */
            void setVolume(std::string volumeName) { volumeName_ = volumeName; }; 

            /**
             * Set the energy threshold that the recoil electron must exceed.
             */
            void setRecoilEnergyThreshold(double recoilEnergyThreshold) { 
                recoilEnergyThreshold_ = recoilEnergyThreshold; 
            }


        private:

            /** Messenger used to pass arguments to this class. */
            TargetENProcessFilterMessenger* messenger_{nullptr}; 

            /** The volume name of the LDMX target. */
            G4String volumeName_{"target_PV"};

            /** Flag indicating if the reaction of intereset occurred. */
            bool reactionOccurred_{false};

            /** Energy that the recoil electron must not surpass. */
            double recoilEnergyThreshold_{1500}; 

    };
}

#endif // BIASING_TARGETPROCESSFILTER_H__
