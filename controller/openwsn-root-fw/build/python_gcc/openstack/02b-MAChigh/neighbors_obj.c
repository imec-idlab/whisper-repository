/**
DO NOT EDIT DIRECTLY!!

This file was 'objectified' by SCons as a pre-processing
step for the building a Python extension module.

This was done on 2018-03-08 18:26:36.093793.
*/
#include "opendefs_obj.h"
#include "neighbors_obj.h"
#include "openqueue_obj.h"
#include "packetfunctions_obj.h"
#include "idmanager_obj.h"
#include "openserial_obj.h"
#include "IEEE802154E_obj.h"
#include "openrandom_obj.h"
#include "msf_obj.h"

//=========================== variables =======================================

// declaration of global variable _neighbors_vars_ removed during objectification.

//=========================== prototypes ======================================

void registerNewNeighbor(OpenMote* self, 
        open_addr_t* neighborID,
        int8_t       rssi,
        asn_t*       asnTimestamp,
        bool         joinPrioPresent,
        uint8_t      joinPrio,
        bool         insecure
     );
bool isNeighbor(OpenMote* self, open_addr_t* neighbor);
void removeNeighbor(OpenMote* self, uint8_t neighborIndex);
bool isThisRowMatching(OpenMote* self, 
        open_addr_t* address,
        uint8_t      rowNumber
     );

//=========================== public ==========================================

/**
\brief Initializes this module.
*/
void neighbors_init(OpenMote* self) {
   
   // clear module variables
   memset(&(self->neighbors_vars),0,sizeof(neighbors_vars_t));
   // The .used fields get reset to FALSE by this memset.
   
}

//===== getters

/**
\brief Retrieve the number of neighbors this mote's currently knows of.

\returns The number of neighbors this mote's currently knows of.
*/
uint8_t neighbors_getNumNeighbors(OpenMote* self) {
   uint8_t i;
   uint8_t returnVal;
   
   returnVal=0;
   for (i=0;i<MAXNUMNEIGHBORS;i++) {
      if ((self->neighbors_vars).neighbors[i].used==TRUE) {
         returnVal++;
      }
   }
   return returnVal;
}

dagrank_t neighbors_getNeighborRank(OpenMote* self, uint8_t index) {
   return (self->neighbors_vars).neighbors[index].DAGrank;
}

int8_t neighbors_getRssi(OpenMote* self, uint8_t index){
   return (self->neighbors_vars).neighbors[index].rssi;
}

uint8_t neighbors_getNumTx(OpenMote* self, uint8_t index){
   return (self->neighbors_vars).neighbors[index].numTx;
}
/**
\brief Find neighbor to which to send KA.

This function iterates through the neighbor table and identifies the neighbor
we need to send a KA to, if any. This neighbor satisfies the following
conditions:
- it is one of our preferred parents
- we haven't heard it for over kaPeriod

\param[in] kaPeriod The maximum number of slots I'm allowed not to have heard
   it.

\returns A pointer to the neighbor's address, or NULL if no KA is needed.
*/
open_addr_t* neighbors_getKANeighbor(OpenMote* self, uint16_t kaPeriod) {
   uint8_t         i;
   uint16_t        timeSinceHeard;
   
   // policy is not to KA to non-preferred parents so go strait to check if Preferred Parent is aging
   if ( icmpv6rpl_getPreferredParentIndex(self, &i)) {      // we have a Parent
      if ((self->neighbors_vars).neighbors[i].used==1) {     // that resolves to a neighbor in use (should always)
         timeSinceHeard = ieee154e_asnDiff(self, &(self->neighbors_vars).neighbors[i].asn);
         if (timeSinceHeard>kaPeriod) {
            // this neighbor needs to be KA'ed to
            return &((self->neighbors_vars).neighbors[i].addr_64b);
         }
      }
   }
   return NULL;
}

/**
\brief Find neighbor which should act as a Join Proxy during the join process.

This function iterates through the neighbor table and identifies the neighbor
with lowest join priority metric to send join traffic through. 

\returns A pointer to the neighbor's address, or NULL if no join proxy is found.
*/
open_addr_t* neighbors_getJoinProxy(OpenMote* self) {
   uint8_t i;
   uint8_t joinPrioMinimum;
   open_addr_t* joinProxy;

   joinPrioMinimum = 0xff;
   joinProxy = NULL;
   for (i=0;i<MAXNUMNEIGHBORS;i++) {
      if ((self->neighbors_vars).neighbors[i].used==TRUE && 
              (self->neighbors_vars).neighbors[i].stableNeighbor==TRUE &&
              (self->neighbors_vars).neighbors[i].joinPrio <= joinPrioMinimum) {
          joinProxy = &((self->neighbors_vars).neighbors[i].addr_64b);
          joinPrioMinimum = (self->neighbors_vars).neighbors[i].joinPrio;
      }
   }
   return joinProxy;
}

bool neighbors_getNeighborNoResource(OpenMote* self, uint8_t index){
    return (self->neighbors_vars).neighbors[index].f6PNORES;
}

bool neighbors_getNeighborIsInBlacklist(OpenMote* self, uint8_t index){
    return (self->neighbors_vars).neighbors[index].inBlacklist;
}

uint8_t neighbors_getSequenceNumber(OpenMote* self, open_addr_t* address){
    uint8_t i;
    for (i=0;i<MAXNUMNEIGHBORS;i++){
        if ( packetfunctions_sameAddress(self, address, &(self->neighbors_vars).neighbors[i].addr_64b)){
            break;
        }
    }
    //printf("I am node %d, my current sequence number is %d with neighbor %d\n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],(self->neighbors_vars).neighbors[i].sequenceNumber,address->addr_64b[7]);
    return (self->neighbors_vars).neighbors[i].sequenceNumber;

}

//===== interrogators

/**
\brief Indicate whether some neighbor is a stable neighbor

\param[in] address The address of the neighbor, a full 128-bit IPv6 addres.

\returns TRUE if that neighbor is stable, FALSE otherwise.
*/
bool neighbors_isStableNeighbor(OpenMote* self, open_addr_t* address) {
   uint8_t     i;
   open_addr_t temp_addr_64b;
   open_addr_t temp_prefix;
   bool        returnVal;
   
   // by default, not stable
   returnVal  = FALSE;
   
   // but neighbor's IPv6 address in prefix and EUI64
   switch (address->type) {
      case ADDR_128B:
 packetfunctions_ip128bToMac64b(self, address,&temp_prefix,&temp_addr_64b);
         break;
      default:
 openserial_printCritical(self, COMPONENT_NEIGHBORS,ERR_WRONG_ADDR_TYPE,
                               (errorparameter_t)address->type,
                               (errorparameter_t)0);
         return returnVal;
   }
   
   // iterate through neighbor table
   for (i=0;i<MAXNUMNEIGHBORS;i++) {
      if ( isThisRowMatching(self, &temp_addr_64b,i) && (self->neighbors_vars).neighbors[i].stableNeighbor==TRUE) {
         returnVal  = TRUE;
         break;
      }
   }
   
   return returnVal;
}

/**
\brief Indicate whether some neighbor is a stable neighbor

\param[in] index into the neighbor table.

\returns TRUE if that neighbor is in use and stable, FALSE otherwise.
*/
bool neighbors_isStableNeighborByIndex(OpenMote* self, uint8_t index) {
   return ((self->neighbors_vars).neighbors[index].stableNeighbor &&
           (self->neighbors_vars).neighbors[index].used);
}

/**
\brief Indicate whether some neighbor is an insecure neighbor

\param[in] address The address of the neighbor, a 64-bit address.

\returns TRUE if that neighbor is insecure, FALSE otherwise.
*/
bool neighbors_isInsecureNeighbor(OpenMote* self, open_addr_t* address) {
   uint8_t     i;
   bool        returnVal;
   
   // if not found, insecure
   returnVal  = TRUE;
   
   switch (address->type) {
      case ADDR_64B:
         break;
      default:
 openserial_printCritical(self, COMPONENT_NEIGHBORS,ERR_WRONG_ADDR_TYPE,
                               (errorparameter_t)address->type,
                               (errorparameter_t)0);
         return returnVal;
   }
   
   // iterate through neighbor table
   for (i=0;i<MAXNUMNEIGHBORS;i++) {
      if ( isThisRowMatching(self, address,i)) {
         returnVal  = (self->neighbors_vars).neighbors[i].insecure;
         break;
      }
   }
   
   return returnVal;
}

/**
\brief Indicate whether some neighbor has a lower DAG rank that me.

\param[in] index The index of that neighbor in the neighbor table.

\returns TRUE if that neighbor is in use and has a higher DAG rank than me, FALSE otherwise.
*/
bool neighbors_isNeighborWithHigherDAGrank(OpenMote* self, uint8_t index) {
   bool    returnVal;
   
   if ((self->neighbors_vars).neighbors[index].used==TRUE &&
       (self->neighbors_vars).neighbors[index].DAGrank >= icmpv6rpl_getMyDAGrank(self)) { 
      returnVal = TRUE;
   } else {
      returnVal = FALSE;
   }
   
   return returnVal;
}

bool neighbors_reachedMinimalTransmission(OpenMote* self, uint8_t index){
    bool    returnVal;
    
    if (
        (self->neighbors_vars).neighbors[index].used  == TRUE &&
        (self->neighbors_vars).neighbors[index].numTx >  MINIMAL_NUM_TX
    ) {
        returnVal = TRUE;
    } else {
        returnVal = FALSE;
    }
    
    return returnVal;
}

//===== updating neighbor information

/**
\brief Indicate some (non-ACK) packet was received from a neighbor.

This function should be called for each received (non-ACK) packet so neighbor
statistics in the neighbor table can be updated.

The fields which are updated are:
- numRx
- rssi
- asn
- stableNeighbor
- switchStabilityCounter

\param[in] l2_src MAC source address of the packet, i.e. the neighbor who sent
   the packet just received.
\param[in] rssi   RSSI with which this packet was received.
\param[in] asnTs  ASN at which this packet was received.
\param[in] joinPrioPresent Whether a join priority was present in the received
   packet.
\param[in] joinPrio The join priority present in the packet, if any.
*/
void neighbors_indicateRx(OpenMote* self, open_addr_t* l2_src,
                          int8_t       rssi,
                          asn_t*       asnTs,
                          bool         joinPrioPresent,
                          uint8_t      joinPrio,
                          bool         insecure) {
   uint8_t i;
   bool    newNeighbor;
   
   // update existing neighbor
   newNeighbor = TRUE;
   for (i=0;i<MAXNUMNEIGHBORS;i++) {
      if ( isThisRowMatching(self, l2_src,i)) {
         
         // this is not a new neighbor
         newNeighbor = FALSE;
         
         // whether the neighbor is considered as secure or not
         (self->neighbors_vars).neighbors[i].insecure = insecure;

         // update numRx, rssi, asn
         (self->neighbors_vars).neighbors[i].numRx++;
         (self->neighbors_vars).neighbors[i].rssi=rssi;
         memcpy(&(self->neighbors_vars).neighbors[i].asn,asnTs,sizeof(asn_t));
         //update jp
         if (joinPrioPresent==TRUE){
            (self->neighbors_vars).neighbors[i].joinPrio=joinPrio;
         }
         
         // update stableNeighbor, switchStabilityCounter
         if ((self->neighbors_vars).neighbors[i].stableNeighbor==FALSE) {
            if ((self->neighbors_vars).neighbors[i].rssi>BADNEIGHBORMAXRSSI) {
               (self->neighbors_vars).neighbors[i].switchStabilityCounter++;
               if ((self->neighbors_vars).neighbors[i].switchStabilityCounter>=SWITCHSTABILITYTHRESHOLD) {
                  (self->neighbors_vars).neighbors[i].switchStabilityCounter=0;
                  (self->neighbors_vars).neighbors[i].stableNeighbor=TRUE;
               }
            } else {
               (self->neighbors_vars).neighbors[i].switchStabilityCounter=0;
            }
         } else if ((self->neighbors_vars).neighbors[i].stableNeighbor==TRUE) {
            if ((self->neighbors_vars).neighbors[i].rssi<GOODNEIGHBORMINRSSI) {
               (self->neighbors_vars).neighbors[i].switchStabilityCounter++;
               if ((self->neighbors_vars).neighbors[i].switchStabilityCounter>=SWITCHSTABILITYTHRESHOLD) {
                  (self->neighbors_vars).neighbors[i].switchStabilityCounter=0;
                   (self->neighbors_vars).neighbors[i].stableNeighbor=FALSE;
               }
            } else {
               (self->neighbors_vars).neighbors[i].switchStabilityCounter=0;
            }
         }
         
         // stop looping
         break;
      }
   }
   
   // register new neighbor
   if (newNeighbor==TRUE) {
 registerNewNeighbor(self, l2_src, rssi, asnTs, joinPrioPresent, joinPrio, insecure);
   }
}

/**
\brief Indicate some packet was sent to some neighbor.

This function should be called for each transmitted (non-ACK) packet so
neighbor statistics in the neighbor table can be updated.

The fields which are updated are:
- numTx
- numTxACK
- asn

\param[in] l2_dest MAC destination address of the packet, i.e. the neighbor
   who I just sent the packet to.
\param[in] numTxAttempts Number of transmission attempts to this neighbor.
\param[in] was_finally_acked TRUE iff the packet was ACK'ed by the neighbor
   on final transmission attempt.
\param[in] asnTs ASN of the last transmission attempt.
*/
void neighbors_indicateTx(OpenMote* self, 
    open_addr_t* l2_dest,
    uint8_t      numTxAttempts,
    bool         was_finally_acked,
    asn_t*       asnTs
) {
    uint8_t i;
    // don't run through this function if packet was sent to broadcast address
    if ( packetfunctions_isBroadcastMulticast(self, l2_dest)==TRUE) {
        return;
    }
    
    // loop through neighbor table
    for (i=0;i<MAXNUMNEIGHBORS;i++) {
        if ( isThisRowMatching(self, l2_dest,i)) {
            // handle roll-over case
            
            if ((self->neighbors_vars).neighbors[i].numTx>(0xff-numTxAttempts)) {
                (self->neighbors_vars).neighbors[i].numWraps++; //counting the number of times that tx wraps.
                (self->neighbors_vars).neighbors[i].numTx/=2;
                (self->neighbors_vars).neighbors[i].numTxACK/=2;
            }
            // update statistics
            (self->neighbors_vars).neighbors[i].numTx += numTxAttempts; 
            
            if (was_finally_acked==TRUE) {
                (self->neighbors_vars).neighbors[i].numTxACK++;
                memcpy(&(self->neighbors_vars).neighbors[i].asn,asnTs,sizeof(asn_t));
            }
            
            //printf("I am node %d, TX indication in neighbors \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);

            // numTx and numTxAck changed,, update my rank
 icmpv6rpl_updateMyDAGrankAndParentSelection(self);
            break;
        }
    }
}

void neighbors_updateSequenceNumber(OpenMote* self, open_addr_t* address){
    uint8_t i;
    for (i=0;i<MAXNUMNEIGHBORS;i++){
        if ( packetfunctions_sameAddress(self, address, &(self->neighbors_vars).neighbors[i].addr_64b)){
            (self->neighbors_vars).neighbors[i].sequenceNumber = ((self->neighbors_vars).neighbors[i].sequenceNumber+1) & 0xFF;
            // rollover from 0xff to 0x01
            if ((self->neighbors_vars).neighbors[i].sequenceNumber == 0){
                (self->neighbors_vars).neighbors[i].sequenceNumber = 1;
            }
            //printf("I am node %d, my new value of sequence number is %d with neighbor %d\n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],(self->neighbors_vars).neighbors[i].sequenceNumber,address->addr_64b[7]);
            break;

        }
    }

}

void neighbors_resetSequenceNumber(OpenMote* self, open_addr_t* address){
    uint8_t i;
    for (i=0;i<MAXNUMNEIGHBORS;i++){
        if ( packetfunctions_sameAddress(self, address, &(self->neighbors_vars).neighbors[i].addr_64b)){
            (self->neighbors_vars).neighbors[i].sequenceNumber = 0;
            //printf("I am node %d, reseting sequence number is %d with neighbor %d\n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],(self->neighbors_vars).neighbors[i].sequenceNumber,address->addr_64b[7]);

            break;
        }
    }
}

//===== write addresses

/**
\brief Write the 64-bit address of some neighbor to some location.
// Returns false if neighbor not in use or address type is not 64bits
*/
bool neighbors_getNeighborEui64(OpenMote* self, open_addr_t* address, uint8_t addr_type, uint8_t index){
   bool ReturnVal = FALSE;
   switch(addr_type) {
      case ADDR_64B:
         memcpy(&(address->addr_64b),&((self->neighbors_vars).neighbors[index].addr_64b.addr_64b),LENGTH_ADDR64b);
         address->type=ADDR_64B;
         ReturnVal=(self->neighbors_vars).neighbors[index].used;
         break;
      default:
 openserial_printCritical(self, COMPONENT_NEIGHBORS,ERR_WRONG_ADDR_TYPE,
                               (errorparameter_t)addr_type,
                               (errorparameter_t)1);
         break; 
   }
   return ReturnVal;
}
// ==== update backoff
void neighbors_updateBackoff(OpenMote* self, open_addr_t* address){
    uint8_t i;
    for (i=0;i<MAXNUMNEIGHBORS;i++){
        if ( packetfunctions_sameAddress(self, address, &(self->neighbors_vars).neighbors[i].addr_64b)){
            // increase the backoffExponent
            if ((self->neighbors_vars).neighbors[i].backoffExponenton<MAXBE) {
                (self->neighbors_vars).neighbors[i].backoffExponenton++;
            }
            // set the backoff to a random value in [0..2^BE]
            (self->neighbors_vars).neighbors[i].backoff = openrandom_get16b(self)%(1<<(self->neighbors_vars).neighbors[i].backoffExponenton);
            break;
        }
    }
}
void neighbors_decreaseBackoff(OpenMote* self, open_addr_t* address){
    uint8_t i;
    for (i=0;i<MAXNUMNEIGHBORS;i++){
        if ( packetfunctions_sameAddress(self, address, &(self->neighbors_vars).neighbors[i].addr_64b)){
            if ((self->neighbors_vars).neighbors[i].backoff>0) {
                (self->neighbors_vars).neighbors[i].backoff--;
            }
            break;
        }
    }
}
bool neighbors_backoffHitZero(OpenMote* self, open_addr_t* address){
    uint8_t i;
    bool returnVal;
    
    returnVal = FALSE;
    for (i=0;i<MAXNUMNEIGHBORS;i++){
        if ( packetfunctions_sameAddress(self, address, &(self->neighbors_vars).neighbors[i].addr_64b)){
            returnVal = ((self->neighbors_vars).neighbors[i].backoff==0);
            break;
        }
    }
    return returnVal;
}

void neighbors_resetBackoff(OpenMote* self, open_addr_t* address){
    uint8_t i;

    for (i=0;i<MAXNUMNEIGHBORS;i++){
        if ( packetfunctions_sameAddress(self, address, &(self->neighbors_vars).neighbors[i].addr_64b)){
            (self->neighbors_vars).neighbors[i].backoffExponenton     = MINBE-1;
            (self->neighbors_vars).neighbors[i].backoff               = 0;
            break;
        }
    }
}

//===== setters

void neighbors_setNeighborRank(OpenMote* self, uint8_t index, dagrank_t rank) {
   (self->neighbors_vars).neighbors[index].DAGrank=rank;

}

void neighbors_setNeighborNoResource(OpenMote* self, open_addr_t* address){
   uint8_t i;
   
   // loop through neighbor table
   for (i=0;i<MAXNUMNEIGHBORS;i++) {
      if ( isThisRowMatching(self, address,i)) {
          (self->neighbors_vars).neighbors[i].f6PNORES = TRUE;
 icmpv6rpl_updateMyDAGrankAndParentSelection(self);
          break;
      }
   }
}

void neighbors_setPreferredParent(OpenMote* self, uint8_t index, bool isPreferred){
    (self->neighbors_vars).neighbors[index].parentPreference = isPreferred;
}

//===== managing routing info

/**
\brief return the link cost to a neighbor, expressed as a rank increase from this neighbor to this node

This really belongs to icmpv6rpl but it would require a much more complex interface to the neighbor table
*/

uint16_t neighbors_getLinkMetric(OpenMote* self, uint8_t index) {
    uint16_t  rankIncrease;
    uint32_t  rankIncreaseIntermediary; // stores intermediary results of rankIncrease calculation

//    if ( idmanager_getMyID(self, ADDR_64B)->addr_64b[7]==4){
//        //printf("I am NODE %d, getting rank increase \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
//    }

    // we assume that this neighbor has already been checked for being in use         
    // calculate link cost to this neighbor
    if ((self->neighbors_vars).neighbors[index].numTxACK==0) {

//        if ( idmanager_getMyID(self, ADDR_64B)->addr_64b[7]==4){
//            printf("I am NODE %d, NO ACKs yet \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
//        }

        if ((self->neighbors_vars).neighbors[index].numTx > DEFAULTLINKCOST){
            if ((self->neighbors_vars).neighbors[index].numTx < MINIMAL_NUM_TX){
                rankIncrease = (3*(self->neighbors_vars).neighbors[index].numTx-2)*MINHOPRANKINCREASE;
            } else {
                rankIncrease = 65535;
            }
        } else {
            rankIncrease = (3*DEFAULTLINKCOST-2)*MINHOPRANKINCREASE;
        }
    } else {

        //6TiSCH minimal draft using OF0 for rank computation: ((3*numTx/numTxAck)-2)*minHopRankIncrease
        // numTx is on 8 bits, so scaling up 10 bits won't lead to saturation
        // but this <<10 followed by >>10 does not provide any benefit either. Result is the same.
        rankIncreaseIntermediary = (((uint32_t)(self->neighbors_vars).neighbors[index].numTx) << 10);
        rankIncreaseIntermediary = (3*rankIncreaseIntermediary * MINHOPRANKINCREASE) / ((uint32_t)(self->neighbors_vars).neighbors[index].numTxACK);
        rankIncreaseIntermediary = rankIncreaseIntermediary - ((uint32_t)(2 * MINHOPRANKINCREASE)<<10);
        // this could still overflow for numTx large and numTxAck small, Casting to 16 bits will yiel the least significant bits
        if (rankIncreaseIntermediary >= (65536<<10)) {
            rankIncrease = 65535;
        } else {
            rankIncrease = (uint16_t)(rankIncreaseIntermediary >> 10);
        }
        
        if (
            rankIncrease>(3*DEFAULTLINKCOST-2)*MINHOPRANKINCREASE &&
            (self->neighbors_vars).neighbors[index].numTx > MINIMAL_NUM_TX
        ){
            // PDR too low, put the neighbor in blacklist
            (self->neighbors_vars).neighbors[index].inBlacklist = TRUE;
        }
//        if ( idmanager_getMyID(self, ADDR_64B)->addr_64b[7]==4){
//            printf("I am NODE %d, I have some ACKs, rank increase is %d \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],rankIncrease);
//        }
    }
    return rankIncrease;
}

//===== maintenance

void neighbors_removeOld(OpenMote* self) {
    uint8_t    i, j;
    bool       haveParent;
    PORT_TIMER_WIDTH timeSinceHeard;
    open_addr_t addressToWrite;

    if (
 icmpv6rpl_getPreferredParentEui64(self, &addressToWrite) == FALSE      ||
        (
 icmpv6rpl_getPreferredParentEui64(self, &addressToWrite)           &&
 schedule_hasDedicatedCellToNeighbor(self, &addressToWrite)== FALSE
        )
    ) {
        return;
    }
    


    // remove old neighbor
    for (i=0;i<MAXNUMNEIGHBORS;i++) {
        if ((self->neighbors_vars).neighbors[i].used==1) {
            timeSinceHeard = ieee154e_asnDiff(self, &(self->neighbors_vars).neighbors[i].asn);
            if (timeSinceHeard>DESYNCTIMEOUT) {
 msf_trigger6pClear(self, &(self->neighbors_vars).neighbors[i].addr_64b);
                haveParent = icmpv6rpl_getPreferredParentIndex(self, &j);
                if (haveParent && (i==j)) { // this is our preferred parent, carefully!
 icmpv6rpl_killPreferredParent(self);
                    //printf("I am node %d, Kill preferent parent. Updating! \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
 icmpv6rpl_updateMyDAGrankAndParentSelection(self);
                }
                // keep the NORES neighbor in the table
                if (
                    (self->neighbors_vars).neighbors[i].f6PNORES    == FALSE &&
                    (self->neighbors_vars).neighbors[i].inBlacklist == FALSE
                ){
 removeNeighbor(self, i);
                }
            }
        }
    }
}

//===== debug

/**
\brief Triggers this module to print status information, over serial.

debugPrint_* functions are used by the openserial module to continuously print
status information about several modules in the OpenWSN stack.

\returns TRUE if this function printed something, FALSE otherwise.
*/
bool debugPrint_neighbors(OpenMote* self) {
    debugNeighborEntry_t temp;
    (self->neighbors_vars).debugRow=((self->neighbors_vars).debugRow+1)%MAXNUMNEIGHBORS;
    temp.row=(self->neighbors_vars).debugRow;
    temp.neighborEntry=(self->neighbors_vars).neighbors[(self->neighbors_vars).debugRow];
 openserial_printStatus(self, STATUS_NEIGHBORS,(uint8_t*)&temp,sizeof(debugNeighborEntry_t));
    return TRUE;
}

//=========================== private =========================================

void registerNewNeighbor(OpenMote* self, open_addr_t* address,
                         int8_t       rssi,
                         asn_t*       asnTimestamp,
                         bool         joinPrioPresent,
                         uint8_t      joinPrio,
                         bool         insecure) {
   uint8_t  i;
   // filter errors
   if (address->type!=ADDR_64B) {
 openserial_printCritical(self, COMPONENT_NEIGHBORS,ERR_WRONG_ADDR_TYPE,
                            (errorparameter_t)address->type,
                            (errorparameter_t)2);
      return;
   }
   // add this neighbor
   if ( isNeighbor(self, address)==FALSE) {
      i=0;
      while(i<MAXNUMNEIGHBORS) {
         if ((self->neighbors_vars).neighbors[i].used==FALSE) {
            if (rssi < BADNEIGHBORMAXRSSI){
                break;
            }
            // add this neighbor
            (self->neighbors_vars).neighbors[i].used                   = TRUE;
            (self->neighbors_vars).neighbors[i].insecure               = insecure;
            // (self->neighbors_vars).neighbors[i].stableNeighbor         = FALSE;
            // Note: all new neighbors are consider stable
            (self->neighbors_vars).neighbors[i].stableNeighbor         = TRUE;
            (self->neighbors_vars).neighbors[i].switchStabilityCounter = 0;
            memcpy(&(self->neighbors_vars).neighbors[i].addr_64b,address,sizeof(open_addr_t));
            (self->neighbors_vars).neighbors[i].DAGrank                = DEFAULTDAGRANK;
            // since we don't have a DAG rank at this point, no need to call for routing table update
            (self->neighbors_vars).neighbors[i].rssi                   = rssi;
            (self->neighbors_vars).neighbors[i].numRx                  = 1;
            (self->neighbors_vars).neighbors[i].numTx                  = 0;
            (self->neighbors_vars).neighbors[i].numTxACK               = 0;
            memcpy(&(self->neighbors_vars).neighbors[i].asn,asnTimestamp,sizeof(asn_t));
            (self->neighbors_vars).neighbors[i].backoffExponenton      = MINBE-1;;
            (self->neighbors_vars).neighbors[i].backoff                = 0;
            //update jp
            if (joinPrioPresent==TRUE){
               (self->neighbors_vars).neighbors[i].joinPrio=joinPrio;
            }
            
            break;
         }
         i++;
      }
      if (i==MAXNUMNEIGHBORS) {
 openserial_printError(self, COMPONENT_NEIGHBORS,ERR_NEIGHBORS_FULL,
                               (errorparameter_t)MAXNUMNEIGHBORS,
                               (errorparameter_t)0);
         return;
      }
   }
}

bool isNeighbor(OpenMote* self, open_addr_t* neighbor) {
   uint8_t i=0;
   for (i=0;i<MAXNUMNEIGHBORS;i++) {
      if ( isThisRowMatching(self, neighbor,i)) {
         return TRUE;
      }
   }
   return FALSE;
}

void removeNeighbor(OpenMote* self, uint8_t neighborIndex) {
   (self->neighbors_vars).neighbors[neighborIndex].used                      = FALSE;
   (self->neighbors_vars).neighbors[neighborIndex].parentPreference          = 0;
   (self->neighbors_vars).neighbors[neighborIndex].stableNeighbor            = FALSE;
   (self->neighbors_vars).neighbors[neighborIndex].switchStabilityCounter    = 0;
   //(self->neighbors_vars).neighbors[neighborIndex].addr_16b.type           = ADDR_NONE; // to save RAM
   (self->neighbors_vars).neighbors[neighborIndex].addr_64b.type             = ADDR_NONE;
   //(self->neighbors_vars).neighbors[neighborIndex].addr_128b.type          = ADDR_NONE; // to save RAM
   (self->neighbors_vars).neighbors[neighborIndex].DAGrank                   = DEFAULTDAGRANK;
   (self->neighbors_vars).neighbors[neighborIndex].rssi                      = 0;
   (self->neighbors_vars).neighbors[neighborIndex].numRx                     = 0;
   (self->neighbors_vars).neighbors[neighborIndex].numTx                     = 0;
   (self->neighbors_vars).neighbors[neighborIndex].numTxACK                  = 0;
   (self->neighbors_vars).neighbors[neighborIndex].asn.bytes0and1            = 0;
   (self->neighbors_vars).neighbors[neighborIndex].asn.bytes2and3            = 0;
   (self->neighbors_vars).neighbors[neighborIndex].asn.byte4                 = 0;
   (self->neighbors_vars).neighbors[neighborIndex].f6PNORES                  = FALSE;
   (self->neighbors_vars).neighbors[neighborIndex].sequenceNumber            = 0;
   (self->neighbors_vars).neighbors[neighborIndex].backoffExponenton         = MINBE-1;;
   (self->neighbors_vars).neighbors[neighborIndex].backoff                   = 0;
}

//=========================== helpers =========================================

bool isThisRowMatching(OpenMote* self, open_addr_t* address, uint8_t rowNumber) {
   switch (address->type) {
      case ADDR_64B:
         return (self->neighbors_vars).neighbors[rowNumber].used &&
 packetfunctions_sameAddress(self, address,&(self->neighbors_vars).neighbors[rowNumber].addr_64b);
      default:
 openserial_printCritical(self, COMPONENT_NEIGHBORS,ERR_WRONG_ADDR_TYPE,
                               (errorparameter_t)address->type,
                               (errorparameter_t)3);
         return FALSE;
   }
}
