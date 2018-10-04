/**
DO NOT EDIT DIRECTLY!!

This file was 'objectified' by SCons as a pre-processing
step for the building a Python extension module.

This was done on 2018-03-08 18:26:42.322986.
*/
#include "opendefs_obj.h"
#include "schedule_obj.h"
#include "openserial_obj.h"
#include "openrandom_obj.h"
#include "packetfunctions_obj.h"
#include "sixtop_obj.h"
#include "idmanager_obj.h"
#include "msf_obj.h"
#include "IEEE802154E_obj.h"
#include "icmpv6rpl_obj.h"
#include "neighbors_obj.h"

//=========================== definition ======================================

//=========================== variables =======================================

// declaration of global variable _schedule_vars_ removed during objectification.

//=========================== prototypes ======================================

void schedule_resetEntry(OpenMote* self, scheduleEntry_t* pScheduleEntry);

//=========================== public ==========================================

//=== admin

/**
\brief Initialize this module.

\post Call this function before calling any other function in this module.
*/
void schedule_init(OpenMote* self) {
    slotOffset_t    start_slotOffset;
    slotOffset_t    running_slotOffset;
    open_addr_t     temp_neighbor;

    // reset local variables
    memset(&(self->schedule_vars),0,sizeof(schedule_vars_t));
    for (running_slotOffset=0;running_slotOffset<MAXACTIVESLOTS;running_slotOffset++) {
 schedule_resetEntry(self, &(self->schedule_vars).scheduleBuf[running_slotOffset]);
    }
    (self->schedule_vars).backoffExponenton   = MINBE-1;
    (self->schedule_vars).maxActiveSlots = MAXACTIVESLOTS;
    
    start_slotOffset = SCHEDULE_MINIMAL_6TISCH_SLOTOFFSET;
    if ( idmanager_getIsDAGroot(self)==TRUE) {
 schedule_startDAGroot(self);
    }
    
    // serial RX slot(s)
    start_slotOffset += SCHEDULE_MINIMAL_6TISCH_ACTIVE_CELLS;
    memset(&temp_neighbor,0,sizeof(temp_neighbor));
    for (running_slotOffset=start_slotOffset;running_slotOffset<start_slotOffset+NUMSERIALRX;running_slotOffset++) {
 schedule_addActiveSlot(self, 
            running_slotOffset,                    // slot offset
            CELLTYPE_SERIALRX,                     // type of slot
            FALSE,                                 // shared?
            0,                                     // channel offset
            &temp_neighbor                         // neighbor
        );
    }
}

/**
\brief Starting the DAGroot schedule propagation.
*/
void schedule_startDAGroot(OpenMote* self) {
   slotOffset_t    start_slotOffset;
   slotOffset_t    running_slotOffset;
   open_addr_t     temp_neighbor;
   
   start_slotOffset = SCHEDULE_MINIMAL_6TISCH_SLOTOFFSET;
   // set frame length, handle and number (default 1 by now)
   if ((self->schedule_vars).frameLength == 0) {
       // slotframe length is not set, set it to default length
 schedule_setFrameLength(self, SLOTFRAME_LENGTH);
   } else {
       // slotframe elgnth is set, nothing to do here
   }
 schedule_setFrameHandle(self, SCHEDULE_MINIMAL_6TISCH_DEFAULT_SLOTFRAME_HANDLE);
 schedule_setFrameNumber(self, SCHEDULE_MINIMAL_6TISCH_DEFAULT_SLOTFRAME_NUMBER);

   // shared TXRX anycast slot(s)
   memset(&temp_neighbor,0,sizeof(temp_neighbor));
   temp_neighbor.type             = ADDR_ANYCAST;
   for (running_slotOffset=start_slotOffset;running_slotOffset<start_slotOffset+SCHEDULE_MINIMAL_6TISCH_ACTIVE_CELLS;running_slotOffset++) {
 schedule_addActiveSlot(self, 
         running_slotOffset,                 // slot offset
         CELLTYPE_TXRX,                      // type of slot
         TRUE,                               // shared?
         SCHEDULE_MINIMAL_6TISCH_CHANNELOFFSET,    // channel offset
         &temp_neighbor                      // neighbor
      );
   }
}

/**
\brief Trigger this module to print status information, over serial.

debugPrint_* functions are used by the openserial module to continuously print
status information about several modules in the OpenWSN stack.

\returns TRUE if this function printed something, FALSE otherwise.
*/
bool debugPrint_schedule(OpenMote* self) {
   debugScheduleEntry_t temp;
   
   // increment the row just printed
   (self->schedule_vars).debugPrintRow         = ((self->schedule_vars).debugPrintRow+1)%(self->schedule_vars).maxActiveSlots;
   
   // gather status data
   temp.row                            = (self->schedule_vars).debugPrintRow;
   temp.slotOffset                     = \
      (self->schedule_vars).scheduleBuf[(self->schedule_vars).debugPrintRow].slotOffset;
   temp.type                           = \
      (self->schedule_vars).scheduleBuf[(self->schedule_vars).debugPrintRow].type;
   temp.shared                         = \
      (self->schedule_vars).scheduleBuf[(self->schedule_vars).debugPrintRow].shared;
   temp.channelOffset                  = \
      (self->schedule_vars).scheduleBuf[(self->schedule_vars).debugPrintRow].channelOffset;
   memcpy(
      &temp.neighbor,
      &(self->schedule_vars).scheduleBuf[(self->schedule_vars).debugPrintRow].neighbor,
      sizeof(open_addr_t)
   );
   temp.numRx                          = \
      (self->schedule_vars).scheduleBuf[(self->schedule_vars).debugPrintRow].numRx;
   temp.numTx                          = \
      (self->schedule_vars).scheduleBuf[(self->schedule_vars).debugPrintRow].numTx;
   temp.numTxACK                       = \
      (self->schedule_vars).scheduleBuf[(self->schedule_vars).debugPrintRow].numTxACK;
   memcpy(
      &temp.lastUsedAsn,
      &(self->schedule_vars).scheduleBuf[(self->schedule_vars).debugPrintRow].lastUsedAsn,
      sizeof(asn_t)
   );
   
   // send status data over serial port
 openserial_printStatus(self, 
      STATUS_SCHEDULE,
      (uint8_t*)&temp,
      sizeof(debugScheduleEntry_t)
   );
   
   return TRUE;
}

/**
\brief Trigger this module to print status information, over serial.

debugPrint_* functions are used by the openserial module to continuously print
status information about several modules in the OpenWSN stack.

\returns TRUE if this function printed something, FALSE otherwise.
*/
bool debugPrint_backoff(OpenMote* self) {
   uint8_t temp[2];
   
   // gather status data
   temp[0] = (self->schedule_vars).backoffExponenton;
   temp[1] = (self->schedule_vars).backoff;
   
   // send status data over serial port
 openserial_printStatus(self, 
      STATUS_BACKOFF,
      (uint8_t*)&temp,
      sizeof(temp)
   );
   
   return TRUE;
}

//=== from 6top (writing the schedule)

/**
\brief Set frame length.

\param newFrameLength The new frame length.
*/
void schedule_setFrameLength(OpenMote* self, frameLength_t newFrameLength) {
   
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();
   
   (self->schedule_vars).frameLength = newFrameLength;
   if (newFrameLength <= MAXACTIVESLOTS) {
      (self->schedule_vars).maxActiveSlots = newFrameLength;
   }
   ENABLE_INTERRUPTS();
}

/**
\brief Set frame handle.

\param frameHandle The new frame handle.
*/
void schedule_setFrameHandle(OpenMote* self, uint8_t frameHandle) {
   
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();
   
   (self->schedule_vars).frameHandle = frameHandle;
   
   ENABLE_INTERRUPTS();
}

/**
\brief Set frame number.

\param frameNumber The new frame number.
*/
void schedule_setFrameNumber(OpenMote* self, uint8_t frameNumber) {
   
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();
   
   (self->schedule_vars).frameNumber = frameNumber;
   
   ENABLE_INTERRUPTS();
}

/**
\brief Get the information of a specific slot.

\param slotOffset
\param neighbor
\param info
*/
void schedule_getSlotInfo(OpenMote* self, 
   slotOffset_t         slotOffset,
   open_addr_t*         neighbor,
   slotinfo_element_t*  info
){
   
   scheduleEntry_t* slotContainer;
  
   // find an empty schedule entry container
   slotContainer = &(self->schedule_vars).scheduleBuf[0];
   while (slotContainer<=&(self->schedule_vars).scheduleBuf[(self->schedule_vars).maxActiveSlots-1]) {
       //check that this entry for that neighbour and timeslot is not already scheduled.
       if ( packetfunctions_sameAddress(self, neighbor,&(slotContainer->neighbor))&& (slotContainer->slotOffset==slotOffset)){
               //it exists so this is an update.
               info->link_type                 = slotContainer->type;
               info->shared                    =slotContainer->shared;
               info->channelOffset             = slotContainer->channelOffset;
               return; //as this is an update. No need to re-insert as it is in the same position on the list.
        }
        slotContainer++;
   }
   //return cell type off.
   info->link_type                 = CELLTYPE_OFF;
   info->shared                    = FALSE;
   info->channelOffset             = 0;//set to zero if not set.                          
}

/**
\brief Add a new active slot into the schedule.

\param slotOffset       The slotoffset of the new slot
\param type             The type of the cell
\param shared           Whether this cell is shared (TRUE) or not (FALSE).
\param channelOffset    The channelOffset of the new slot
\param neighbor         The neighbor associated with this cell (all 0's if
   none)
*/
owerror_t schedule_addActiveSlot(OpenMote* self, 
      slotOffset_t    slotOffset,
      cellType_t      type,
      bool            shared,
      channelOffset_t channelOffset,
      open_addr_t*    neighbor
   ) {
   uint8_t asn[5];
   scheduleEntry_t* slotContainer;
   scheduleEntry_t* previousSlotWalker;
   scheduleEntry_t* nextSlotWalker;
   
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();
   
   // find an empty schedule entry container
   slotContainer = &(self->schedule_vars).scheduleBuf[0];
   while (
         slotContainer->type!=CELLTYPE_OFF &&
         slotContainer<=&(self->schedule_vars).scheduleBuf[(self->schedule_vars).maxActiveSlots-1]
      ) {
      slotContainer++;
   }
   
   // abort it schedule overflow
   if (slotContainer>&(self->schedule_vars).scheduleBuf[(self->schedule_vars).maxActiveSlots-1]) {
      ENABLE_INTERRUPTS();
 openserial_printError(self, 
         COMPONENT_SCHEDULE,ERR_SCHEDULE_OVERFLOWN,
         (errorparameter_t)0,
         (errorparameter_t)0
      );
      return E_FAIL;
   }
   
   // fill that schedule entry with parameters passed
   slotContainer->slotOffset                = slotOffset;
   slotContainer->type                      = type;
   slotContainer->shared                    = shared;
   slotContainer->channelOffset             = channelOffset;
   memcpy(&slotContainer->neighbor,neighbor,sizeof(open_addr_t));
   
   // fill that schedule entry with current asn
 ieee154e_getAsn(self, &(asn[0]));
   slotContainer->lastUsedAsn.bytes0and1 = 256*asn[1]+asn[0];
   slotContainer->lastUsedAsn.bytes2and3 = 256*asn[3]+asn[2];
   slotContainer->lastUsedAsn.byte4      = asn[4];
   
   // insert in circular list
   if ((self->schedule_vars).currentScheduleEntry==NULL) {
      // this is the first active slot added
      
      // the next slot of this slot is this slot
      slotContainer->next                   = slotContainer;
      
      // current slot points to this slot
      (self->schedule_vars).currentScheduleEntry    = slotContainer;
   } else  {
      // this is NOT the first active slot added
      
      // find position in schedule
      previousSlotWalker                    = (self->schedule_vars).currentScheduleEntry;
      while (1) {
         nextSlotWalker                     = previousSlotWalker->next;
         if (
               (
                     (previousSlotWalker->slotOffset <  slotContainer->slotOffset) &&
                     (slotContainer->slotOffset <  nextSlotWalker->slotOffset)
               )
               ||
               (
                     (previousSlotWalker->slotOffset <  slotContainer->slotOffset) &&
                     (nextSlotWalker->slotOffset <= previousSlotWalker->slotOffset)
               )
               ||
               (
                     (slotContainer->slotOffset <  nextSlotWalker->slotOffset) &&
                     (nextSlotWalker->slotOffset <= previousSlotWalker->slotOffset)
               )
         ) {
            break;
         }
         if (previousSlotWalker->slotOffset == slotContainer->slotOffset) {
            // slot is already in schedule
 openserial_printError(self, 
               COMPONENT_SCHEDULE,ERR_SCHEDULE_ADDDUPLICATESLOT,
               (errorparameter_t)slotContainer->slotOffset,
               (errorparameter_t)0
            );
            // reset the entry
            slotContainer->slotOffset                = 0;
            slotContainer->type                      = CELLTYPE_OFF;
            slotContainer->shared                    = FALSE;
            slotContainer->channelOffset             = 0;
            memset(&slotContainer->neighbor,0,sizeof(open_addr_t));
            ENABLE_INTERRUPTS();
            return E_FAIL;
         }
         previousSlotWalker                 = nextSlotWalker;
      }
      // insert between previousSlotWalker and nextSlotWalker
      previousSlotWalker->next              = slotContainer;
      slotContainer->next                   = nextSlotWalker;
   }
   
   ENABLE_INTERRUPTS();
   return E_SUCCESS;
}

/**
\brief Remove an active slot from the schedule.

\param slotOffset       The slotoffset of the slot to remove.
\param neighbor         The neighbor associated with this cell (all 0's if
   none)
*/
owerror_t schedule_removeActiveSlot(OpenMote* self, slotOffset_t slotOffset, open_addr_t* neighbor) {
   scheduleEntry_t* slotContainer;
   scheduleEntry_t* previousSlotWalker;
   
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();
   
   // find the schedule entry
   slotContainer = &(self->schedule_vars).scheduleBuf[0];
   while (slotContainer<=&(self->schedule_vars).scheduleBuf[(self->schedule_vars).maxActiveSlots-1]) {
      if (
            slotContainer->slotOffset==slotOffset
            &&
 packetfunctions_sameAddress(self, neighbor,&(slotContainer->neighbor))
            ){
         break;
      }
      slotContainer++;
   }
   
   // abort it could not find
   if (slotContainer>&(self->schedule_vars).scheduleBuf[(self->schedule_vars).maxActiveSlots-1]) {
      ENABLE_INTERRUPTS();
 openserial_printCritical(self, 
         COMPONENT_SCHEDULE,ERR_FREEING_ERROR,
         (errorparameter_t)0,
         (errorparameter_t)0
      );
      //printf("I am node %d, removing slot. COULD NOT FIND \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
      return E_FAIL;
   }
   
   // remove from linked list
   if (slotContainer->next==slotContainer) {
      // this is the last active slot
      
      // the next slot of this slot is NULL
      slotContainer->next                   = NULL;
      
      // current slot points to this slot
      (self->schedule_vars).currentScheduleEntry    = NULL;
   } else  {
      // this is NOT the last active slot
      
      // find the previous in the schedule
      previousSlotWalker                    = (self->schedule_vars).currentScheduleEntry;
      
      while (1) {
         if (previousSlotWalker->next==slotContainer){
            break;
         }
         previousSlotWalker                 = previousSlotWalker->next;
      }
      
      // remove this element from the linked list, i.e. have the previous slot
      // "jump" to slotContainer's next
      previousSlotWalker->next              = slotContainer->next;
      
      // update current slot if points to slot I just removed
      if ((self->schedule_vars).currentScheduleEntry==slotContainer) {
         (self->schedule_vars).currentScheduleEntry = slotContainer->next;
      }
   }
   
   // reset removed schedule entry
 schedule_resetEntry(self, slotContainer);
   
   ENABLE_INTERRUPTS();
   
   return E_SUCCESS;
}

bool schedule_isSlotOffsetAvailable(OpenMote* self, uint16_t slotOffset){
   
   scheduleEntry_t* scheduleWalker;
   
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();
   
   if (slotOffset>=(self->schedule_vars).frameLength){
      ENABLE_INTERRUPTS();
      return FALSE;
   }
   
   scheduleWalker = (self->schedule_vars).currentScheduleEntry;
   do {
      if(slotOffset == scheduleWalker->slotOffset){
          ENABLE_INTERRUPTS();
          return FALSE;
      }
      scheduleWalker = scheduleWalker->next;
   }while(scheduleWalker!=(self->schedule_vars).currentScheduleEntry);
   
   ENABLE_INTERRUPTS();
   
   return TRUE;
}

void schedule_removeAllCells(OpenMote* self, 
    uint8_t        slotframeID,
    open_addr_t*   previousHop
    ){
    uint8_t i;
    //printf("I am node %d, Removig ALL cells!! \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
    // remove all entries in schedule with previousHop address
    for(i=0;i<MAXACTIVESLOTS;i++){
        if ( packetfunctions_sameAddress(self, &((self->schedule_vars).scheduleBuf[i].neighbor),previousHop)){
 schedule_removeActiveSlot(self, 
              (self->schedule_vars).scheduleBuf[i].slotOffset,
              previousHop
           );
           //printf("I am node %d, Removig cell %d \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],(self->schedule_vars).scheduleBuf[i].slotOffset);
        }
    }
}

uint8_t schedule_getNumberOfFreeEntries(OpenMote* self){
   uint8_t i; 
   uint8_t counter;
   
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();
   
   counter = 0;
   for(i=0;i<MAXACTIVESLOTS;i++) {
      if((self->schedule_vars).scheduleBuf[i].type == CELLTYPE_OFF){
         counter++;
      }
   }
   
   ENABLE_INTERRUPTS();
   
   return counter;
}

uint8_t schedule_getNumberOfDedicatedCells(OpenMote* self, open_addr_t* neighbor){
   uint8_t i; 
   uint8_t counter;
   
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();
   
   counter = 0;
   for(i=0;i<MAXACTIVESLOTS;i++) {
      if(
 packetfunctions_sameAddress(self, &(self->schedule_vars).scheduleBuf[i].neighbor, neighbor) == TRUE
      ){
         counter++;
      }
   }
   
   ENABLE_INTERRUPTS();
   
   return counter;
}

open_addr_t* schedule_getNonParentNeighborWithDedicatedCells(OpenMote* self, open_addr_t* neighbor){
   uint8_t i; 
   
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();

   for(i=0;i<MAXACTIVESLOTS;i++) {
      if(
         (self->schedule_vars).scheduleBuf[i].neighbor.type == ADDR_64B &&
 packetfunctions_sameAddress(self, &(self->schedule_vars).scheduleBuf[i].neighbor, neighbor) == FALSE
      ){
         return &(self->schedule_vars).scheduleBuf[i].neighbor;
      }
   }
   
   ENABLE_INTERRUPTS();
   
   return NULL;
}

bool schedule_isNumTxWrapped(OpenMote* self, open_addr_t* neighbor){
    uint8_t i;
    
    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();
    
    for(i=0;i<MAXACTIVESLOTS;i++) {
        if(
 packetfunctions_sameAddress(self, &(self->schedule_vars).scheduleBuf[i].neighbor, neighbor) == TRUE
        ){
            if ((self->schedule_vars).scheduleBuf[i].numTx<0xFF/2){
                ENABLE_INTERRUPTS();
                return FALSE;
            }
        }
    }
    
    ENABLE_INTERRUPTS();
    return TRUE;
}
bool schedule_getCellsToBeRelocated(OpenMote* self, open_addr_t* neighbor, cellInfo_ht* celllist){
    uint8_t     i;
    
    uint16_t    highestPDR;
    uint16_t    cellPDR;
    
    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();
    
    highestPDR = 0;
    // found the cell with higest PDR
    for(i=0;i<MAXACTIVESLOTS;i++) {
        if(
 packetfunctions_sameAddress(self, &(self->schedule_vars).scheduleBuf[i].neighbor, neighbor) == TRUE
        ){
            if ((self->schedule_vars).scheduleBuf[i].numTx>0){
                cellPDR = 100*(self->schedule_vars).scheduleBuf[i].numTxACK/(self->schedule_vars).scheduleBuf[i].numTx;
                if (cellPDR > highestPDR){
                    highestPDR = cellPDR;
                }
            }
        }
    }
    
    if (highestPDR==0){
        // no cell to relocate
        return FALSE;
    }
    
    for(i=0;i<MAXACTIVESLOTS;i++) {
        if(
 packetfunctions_sameAddress(self, &(self->schedule_vars).scheduleBuf[i].neighbor, neighbor) == TRUE
        ){
            if ((self->schedule_vars).scheduleBuf[i].numTx>0){
                cellPDR = 100*(self->schedule_vars).scheduleBuf[i].numTxACK/(self->schedule_vars).scheduleBuf[i].numTx;
                if (highestPDR-cellPDR > RELOCATE_PDRTHRES){
                    celllist->isUsed            = TRUE;
                    celllist->slotoffset        = (self->schedule_vars).scheduleBuf[i].slotOffset;
                    celllist->channeloffset     = (self->schedule_vars).scheduleBuf[i].channelOffset;
                    return TRUE;
                }
            }
        }
    }
    
    ENABLE_INTERRUPTS();
    
    return FALSE;
}

bool schedule_hasDedicatedCellToNeighbor(OpenMote* self, open_addr_t* neighbor){
    uint8_t i;
    
    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();
    
    for(i=0;i<MAXACTIVESLOTS;i++) {
        if(
            (self->schedule_vars).scheduleBuf[i].neighbor.type == ADDR_64B &&
 packetfunctions_sameAddress(self, neighbor,&(self->schedule_vars).scheduleBuf[i].neighbor)
        ){
            return TRUE;
        }
    }
    
    ENABLE_INTERRUPTS();
    return FALSE;
}

//=== from IEEE802154E: reading the schedule and updating statistics

void schedule_syncSlotOffset(OpenMote* self, slotOffset_t targetSlotOffset) {
   
    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();
    
    while ((self->schedule_vars).currentScheduleEntry->slotOffset!=targetSlotOffset) {
 schedule_advanceSlot(self);
    }
    
    ENABLE_INTERRUPTS();
}

/**
\brief advance to next active slot
*/
void schedule_advanceSlot(OpenMote* self) {

    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();
    (self->schedule_vars).currentScheduleEntry = (self->schedule_vars).currentScheduleEntry->next;

    ENABLE_INTERRUPTS();
}

/**
\brief return slotOffset of next active slot
*/
slotOffset_t schedule_getNextActiveSlotOffset(OpenMote* self) {
    slotOffset_t res;   
    
    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();
    
    res = ((scheduleEntry_t*)((self->schedule_vars).currentScheduleEntry->next))->slotOffset;
    
    ENABLE_INTERRUPTS();
    
    return res;
}

/**
\brief Get the frame length.

\returns The frame length.
*/
frameLength_t schedule_getFrameLength(OpenMote* self) {
    frameLength_t returnVal;
    
    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();
    
    returnVal = (self->schedule_vars).frameLength;
    
    ENABLE_INTERRUPTS();
    
    return returnVal;
}

/**
\brief Get the type of the current schedule entry.

\returns The type of the current schedule entry.
*/
cellType_t schedule_getType(OpenMote* self) {
   cellType_t returnVal;
   
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();
   
   returnVal = (self->schedule_vars).currentScheduleEntry->type;
   
   ENABLE_INTERRUPTS();
   
   return returnVal;
}

/**
\brief Get the neighbor associated wit the current schedule entry.

\returns The neighbor associated wit the current schedule entry.
*/
void schedule_getNeighbor(OpenMote* self, open_addr_t* addrToWrite) {
   
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();
   
   memcpy(addrToWrite,&((self->schedule_vars).currentScheduleEntry->neighbor),sizeof(open_addr_t));
   
   ENABLE_INTERRUPTS();
}

/**
\brief Get the channel offset of the current schedule entry.

\returns The channel offset of the current schedule entry.
*/
channelOffset_t schedule_getChannelOffset(OpenMote* self) {
   channelOffset_t returnVal;
   
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();
   
   returnVal = (self->schedule_vars).currentScheduleEntry->channelOffset;
   
   ENABLE_INTERRUPTS();
   
   return returnVal;
}

/**
\brief Check whether I can send on this slot.

This function is called at the beginning of every TX slot.
If the slot is *not* a shared slot, it always return TRUE.
If the slot is a shared slot, it decrements the backoff counter and returns 
TRUE only if it hits 0.

Note that the backoff counter is global, not per slot.

\returns TRUE if it is OK to send on this slot, FALSE otherwise.
*/
bool schedule_getOkToSend(OpenMote* self) {
    bool returnVal;
    
    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();
    
    if ((self->schedule_vars).currentScheduleEntry->shared==FALSE) {
        // non-shared slot: backoff does not apply
        
        returnVal = TRUE;
    } else {
        // shared slot: check backoff before answering
      
        if ((self->schedule_vars).currentScheduleEntry->neighbor.type == ADDR_ANYCAST){
            // this is a minimal cell
            if ((self->schedule_vars).backoff>0) {
                (self->schedule_vars).backoff--;
            }
            
            // only return TRUE if backoff hit 0
            if ((self->schedule_vars).backoff==0) {
                returnVal = TRUE;
            } else {
                returnVal = FALSE;
            }
        } else {
            // this is a dedicated cell
 neighbors_decreaseBackoff(self, &(self->schedule_vars).currentScheduleEntry->neighbor);
            
            returnVal = neighbors_backoffHitZero(self, &(self->schedule_vars).currentScheduleEntry->neighbor);
        }
    }
    
    ENABLE_INTERRUPTS();
    
    return returnVal;
}

/**
\brief Reset the backoff and backoffExponent.
*/
void schedule_resetBackoff(OpenMote* self) {
   
    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();
    
    // reset backoffExponent
    (self->schedule_vars).backoffExponenton   = MINBE-1;
    // reset backoff
    (self->schedule_vars).backoff             = 0;
    
    ENABLE_INTERRUPTS();
}

/**
\brief Indicate the reception of a packet.
*/
void schedule_indicateRx(OpenMote* self, asn_t* asnTimestamp) {
   
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();
   
   // increment usage statistics
   (self->schedule_vars).currentScheduleEntry->numRx++;

   // update last used timestamp
   memcpy(&((self->schedule_vars).currentScheduleEntry->lastUsedAsn), asnTimestamp, sizeof(asn_t));
   
   ENABLE_INTERRUPTS();
}

/**
\brief Indicate the transmission of a packet.
*/
void schedule_indicateTx(OpenMote* self, asn_t* asnTimestamp, bool succesfullTx) {
   
    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();
    
    // increment usage statistics
    if ((self->schedule_vars).currentScheduleEntry->numTx==0xFF) {
        (self->schedule_vars).currentScheduleEntry->numTx/=2;
        (self->schedule_vars).currentScheduleEntry->numTxACK/=2;
    }
    (self->schedule_vars).currentScheduleEntry->numTx++;
    if (succesfullTx==TRUE) {
        (self->schedule_vars).currentScheduleEntry->numTxACK++;
    }

    // update last used timestamp
    memcpy(&(self->schedule_vars).currentScheduleEntry->lastUsedAsn, asnTimestamp, sizeof(asn_t));

    // update this backoff parameters for shared slots
    if ((self->schedule_vars).currentScheduleEntry->shared==TRUE) {
        if (succesfullTx==TRUE) {
            if ((self->schedule_vars).currentScheduleEntry->neighbor.type==ADDR_ANYCAST){
                // reset backoffExponent
                (self->schedule_vars).backoffExponenton     = MINBE-1;
                // reset backoff
                (self->schedule_vars).backoff               = 0;
            } else {
 neighbors_resetBackoff(self, &(self->schedule_vars).currentScheduleEntry->neighbor);
            }
        } else {
            if ((self->schedule_vars).currentScheduleEntry->neighbor.type==ADDR_ANYCAST){
                // increase the backoffExponent
                if ((self->schedule_vars).backoffExponenton<MAXBE) {
                    (self->schedule_vars).backoffExponenton++;
                }
                // set the backoff to a random value in [0..2^BE]
                (self->schedule_vars).backoff       = openrandom_get16b(self)%(1<<(self->schedule_vars).backoffExponenton);
            } else {
 neighbors_updateBackoff(self, &(self->schedule_vars).currentScheduleEntry->neighbor);
            }
        }
    }
    
    ENABLE_INTERRUPTS();
}

bool schedule_getOneCellAfterOffset(OpenMote* self, uint8_t metadata,uint8_t offset,open_addr_t* neighbor, uint8_t cellOptions, uint16_t* slotoffset, uint16_t* channeloffset){
    bool returnVal;
    scheduleEntry_t* scheduleWalker;
    cellType_t type;
    INTERRUPT_DECLARATION();
    DISABLE_INTERRUPTS();

    // translate cellOptions to cell type 
    if (cellOptions == CELLOPTIONS_TX){
        type = CELLTYPE_TX;
    }
    if (cellOptions == CELLOPTIONS_RX){
        type = CELLTYPE_RX;
    }
    if (cellOptions == (CELLOPTIONS_TX | CELLOPTIONS_RX | CELLOPTIONS_SHARED)){
        type = CELLTYPE_TXRX;
    }
    
    returnVal      = FALSE;
    scheduleWalker = &(self->schedule_vars).scheduleBuf[0]; // fisrt entry record slotoffset 0
    do {
        if(type == scheduleWalker->type && scheduleWalker->slotOffset >= offset){
            *slotoffset    = scheduleWalker->slotOffset;
            *channeloffset = scheduleWalker->channelOffset;
            returnVal      = TRUE;
            break;
        }
        scheduleWalker = scheduleWalker->next;
   }while(scheduleWalker!=&(self->schedule_vars).scheduleBuf[0]);
   
    ENABLE_INTERRUPTS();
   
   return returnVal;
}

//=========================== private =========================================

/**
\pre This function assumes interrupts are already disabled.
*/
void schedule_resetEntry(OpenMote* self, scheduleEntry_t* e) {
   e->slotOffset             = 0;
   e->type                   = CELLTYPE_OFF;
   e->shared                 = FALSE;
   e->channelOffset          = 0;

   e->neighbor.type          = ADDR_NONE;
   memset(&e->neighbor.addr_64b[0], 0x00, sizeof(e->neighbor.addr_64b));

   e->numRx                  = 0;
   e->numTx                  = 0;
   e->numTxACK               = 0;
   e->lastUsedAsn.bytes0and1 = 0;
   e->lastUsedAsn.bytes2and3 = 0;
   e->lastUsedAsn.byte4      = 0;
   
   e->next                   = NULL;
}
