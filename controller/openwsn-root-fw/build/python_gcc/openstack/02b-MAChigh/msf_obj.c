/**
DO NOT EDIT DIRECTLY!!

This file was 'objectified' by SCons as a pre-processing
step for the building a Python extension module.

This was done on 2018-03-08 18:26:40.259995.
*/
#include "opendefs_obj.h"
#include "msf_obj.h"
#include "neighbors_obj.h"
#include "sixtop_obj.h"
#include "scheduler_obj.h"
#include "schedule_obj.h"
#include "openapps_obj.h"
#include "openrandom_obj.h"
#include "idmanager_obj.h"
#include "icmpv6rpl_obj.h"
#include "IEEE802154E_obj.h"

//=========================== definition =====================================

//=========================== variables =======================================

// declaration of global variable _msf_vars_ removed during objectification.

//=========================== prototypes ======================================

// sixtop callback 
uint16_t msf_getMetadata(OpenMote* self);
metadata_t msf_translateMetadata(OpenMote* self);
void msf_handleRCError(OpenMote* self, uint8_t code, open_addr_t* address);

void msf_timer_housekeeping_cb(OpenMote* self, opentimers_id_t id);
void msf_timer_housekeeping_task(OpenMote* self);

void msf_timer_waitretry_cb(OpenMote* self, opentimers_id_t id);

void msf_timer_clear_task(OpenMote* self);
// msf private
void msf_trigger6pAdd(OpenMote* self);
void msf_trigger6pDelete(OpenMote* self);
void msf_housekeeping(OpenMote* self);

//=========================== public ==========================================

void msf_init(OpenMote* self) {
    memset(&(self->msf_vars),0,sizeof(msf_vars_t));
    (self->msf_vars).numAppPacketsPerSlotFrame = 0;
 sixtop_setSFcallback(self, 
        (sixtop_sf_getsfid)msf_getsfid,
        (sixtop_sf_getmetadata)msf_getMetadata,
        (sixtop_sf_translatemetadata)msf_translateMetadata,
        (sixtop_sf_handle_callback)msf_handleRCError
    );
    (self->msf_vars).housekeepingTimerId = opentimers_create(self);
 opentimers_scheduleIn(self, 
        (self->msf_vars).housekeepingTimerId,
        872 +( openrandom_get16b(self)&0xff),
        TIME_MS,
        TIMER_ONESHOT,
        msf_timer_housekeeping_cb
    );
    (self->msf_vars).waitretryTimerId    = opentimers_create(self);
}

// called by schedule
void msf_updateCellsPassed(OpenMote* self, open_addr_t* neighbor){
  
    if ( icmpv6rpl_isPreferredParent(self, neighbor)==FALSE){
        return;
    }
  
    (self->msf_vars).numCellsPassed++;
    if ((self->msf_vars).numCellsPassed == MAX_NUMCELLS){
        if ((self->msf_vars).numCellsUsed > LIM_NUMCELLSUSED_HIGH){
 msf_trigger6pAdd(self);
        }
        if ((self->msf_vars).numCellsUsed < LIM_NUMCELLSUSED_LOW){
 msf_trigger6pDelete(self);
        }
        (self->msf_vars).numCellsPassed = 0;
        (self->msf_vars).numCellsUsed   = 0;
    }
}

void msf_updateCellsUsed(OpenMote* self, open_addr_t* neighbor){
  
    if ( icmpv6rpl_isPreferredParent(self, neighbor)==FALSE){
        return;
    }
    
    (self->msf_vars).numCellsUsed++;
}

void msf_trigger6pClear(OpenMote* self, open_addr_t* neighbor){
        
    if ( schedule_hasDedicatedCellToNeighbor(self, neighbor)>0){
 sixtop_request(self, 
            IANA_6TOP_CMD_CLEAR,                // code
            neighbor,                           // neighbor
            NUMCELLS_MSF,                       // number cells
            CELLOPTIONS_MSF,                    // cellOptions
            NULL,                               // celllist to add (not used)
            NULL,                               // celllist to delete (not used)
            IANA_6TISCH_SFID_MSF,               // sfid
            0,                                  // list command offset (not used)
            0                                   // list command maximum celllist (not used)
        );
    }
}
//=========================== callback =========================================

uint8_t msf_getsfid(OpenMote* self){
    return IANA_6TISCH_SFID_MSF;
}

uint16_t msf_getMetadata(OpenMote* self){
    return SCHEDULE_MINIMAL_6TISCH_DEFAULT_SLOTFRAME_HANDLE;
}

metadata_t msf_translateMetadata(OpenMote* self){
    return METADATA_TYPE_FRAMEID;
}

void msf_handleRCError(OpenMote* self, uint8_t code, open_addr_t* address){
    uint16_t waitDuration;
    
    if (
        code==IANA_6TOP_RC_RESET        ||
        code==IANA_6TOP_RC_LOCKED       ||
        code==IANA_6TOP_RC_BUSY
    ){
        // waitretry
        (self->msf_vars).waitretry = TRUE;
        waitDuration = WAITDURATION_MIN+ openrandom_get16b(self)%WAITDURATION_RANDOM_RANGE;
 opentimers_scheduleIn(self, 
            (self->msf_vars).waitretryTimerId,
            waitDuration,
            TIME_MS,
            TIMER_ONESHOT,
            msf_timer_waitretry_cb
        );
    }
    
    if (
        code==IANA_6TOP_RC_ERROR        ||
        code==IANA_6TOP_RC_VER_ERR      ||
        code==IANA_6TOP_RC_SFID_ERR
    ){
        // quarantine
    }
    
    if (
        code==IANA_6TOP_RC_SEQNUM_ERR   ||
        code==IANA_6TOP_RC_CELLLIST_ERR
    ){
        // clear
 scheduler_push_task(self, msf_timer_clear_task,TASKPRIO_MSF);
    }
    
    if (code==IANA_6TOP_RC_NORES){
        // mark neighbor f6NORES
 neighbors_setNeighborNoResource(self, address);
    }
}

void msf_timer_waitretry_cb(OpenMote* self, opentimers_id_t id){
    (self->msf_vars).waitretry = FALSE;
}

void msf_timer_housekeeping_cb(OpenMote* self, opentimers_id_t id){
 scheduler_push_task(self, msf_timer_housekeeping_task,TASKPRIO_MSF);
    // update the period
 opentimers_scheduleIn(self, 
        (self->msf_vars).housekeepingTimerId,
        872 +( openrandom_get16b(self)&0xff),
        TIME_MS,
        TIMER_ONESHOT,
        msf_timer_housekeeping_cb
    );
}
                        
void msf_timer_housekeeping_task(OpenMote* self){
    (self->msf_vars).housekeepingTimerCounter = ((self->msf_vars).housekeepingTimerCounter+1)%HOUSEKEEPING_PERIOD;
    switch ((self->msf_vars).housekeepingTimerCounter) {
    case 0:
 msf_housekeeping(self);
        break;
    default:
        break;
    }
}

void msf_timer_clear_task(OpenMote* self){
    open_addr_t    neighbor;
    bool           foundNeighbor;
    
    // get preferred parent
    foundNeighbor = icmpv6rpl_getPreferredParentEui64(self, &neighbor);
    if (foundNeighbor==FALSE) {
        return;
    }
    
 sixtop_request(self, 
        IANA_6TOP_CMD_CLEAR,                // code
        &neighbor,                          // neighbor
        NUMCELLS_MSF,                       // number cells
        CELLOPTIONS_MSF,                    // cellOptions
        NULL,                               // celllist to add (not used)
        NULL,                               // celllist to delete (not used)
        IANA_6TISCH_SFID_MSF,               // sfid
        0,                                  // list command offset (not used)
        0                                   // list command maximum celllist (not used)
    );
}

//=========================== private =========================================

void msf_trigger6pAdd(OpenMote* self){
    open_addr_t    neighbor;
    bool           foundNeighbor;
    cellInfo_ht    celllist_add[CELLLIST_MAX_LEN];
    
    if ( ieee154e_isSynch(self)==FALSE) {
        return;
    }
    
    if ((self->msf_vars).waitretry){
        return;
    }
    
    // get preferred parent
    foundNeighbor = icmpv6rpl_getPreferredParentEui64(self, &neighbor);
    if (foundNeighbor==FALSE) {
        return;
    }
    
    if ( msf_candidateAddCellList(self, celllist_add,NUMCELLS_MSF)==FALSE){
        // failed to get cell list to add
        return;
    }
    
 sixtop_request(self, 
        IANA_6TOP_CMD_ADD,                  // code
        &neighbor,                          // neighbor
        NUMCELLS_MSF,                       // number cells
        CELLOPTIONS_MSF,                    // cellOptions
        celllist_add,                       // celllist to add
        NULL,                               // celllist to delete (not used)
        IANA_6TISCH_SFID_MSF,               // sfid
        0,                                  // list command offset (not used)
        0                                   // list command maximum celllist (not used)
    );
}

void msf_trigger6pDelete(OpenMote* self){
    open_addr_t    neighbor;
    bool           foundNeighbor;
    cellInfo_ht    celllist_delete[CELLLIST_MAX_LEN];
    
    if ( ieee154e_isSynch(self)==FALSE) {
        return;
    }
    
    if ((self->msf_vars).waitretry){
        return;
    }
    
    // get preferred parent
    foundNeighbor = icmpv6rpl_getPreferredParentEui64(self, &neighbor);
    if (foundNeighbor==FALSE) {
        return;
    }
    
    if ( schedule_getNumberOfDedicatedCells(self, &neighbor)<=1){
        // at least one dedicated cell presents
        return;
    }
    
    if ( msf_candidateRemoveCellList(self, celllist_delete,&neighbor,NUMCELLS_MSF)==FALSE){
        // failed to get cell list to delete
        return;
    }
 sixtop_request(self, 
        IANA_6TOP_CMD_DELETE,   // code
        &neighbor,              // neighbor
        NUMCELLS_MSF,           // number cells
        CELLOPTIONS_MSF,        // cellOptions
        NULL,                   // celllist to add (not used)
        celllist_delete,        // celllist to delete
        IANA_6TISCH_SFID_MSF,   // sfid
        0,                      // list command offset (not used)
        0                       // list command maximum celllist (not used)
    );
}

void msf_appPktPeriod(OpenMote* self, uint8_t numAppPacketsPerSlotFrame){
    (self->msf_vars).numAppPacketsPerSlotFrame = numAppPacketsPerSlotFrame;
}

bool msf_candidateAddCellList(OpenMote* self, 
      cellInfo_ht* cellList,
      uint8_t      requiredCells
   ){
    uint8_t i;
    frameLength_t slotoffset;
    uint8_t numCandCells;
    
    memset(cellList,0,CELLLIST_MAX_LEN*sizeof(cellInfo_ht));
    numCandCells=0;
    for(i=0;i<CELLLIST_MAX_LEN;i++){
        slotoffset = openrandom_get16b(self)% schedule_getFrameLength(self);
        if( schedule_isSlotOffsetAvailable(self, slotoffset)==TRUE){
            cellList[numCandCells].slotoffset       = slotoffset;
            cellList[numCandCells].channeloffset    = openrandom_get16b(self)&0x0F;
            cellList[numCandCells].isUsed           = TRUE;
            numCandCells++;
        }
    }
   
    if (numCandCells<requiredCells || requiredCells==0) {
        return FALSE;
    } else {
        return TRUE;
    }
}

bool msf_candidateRemoveCellList(OpenMote* self, 
      cellInfo_ht* cellList,
      open_addr_t* neighbor,
      uint8_t      requiredCells
   ){
   uint8_t              i;
   uint8_t              numCandCells;
   slotinfo_element_t   info;
   
   memset(cellList,0,CELLLIST_MAX_LEN*sizeof(cellInfo_ht));
   numCandCells    = 0;
   for(i=0;i< schedule_getFrameLength(self);i++){
 schedule_getSlotInfo(self, i,neighbor,&info);
      if(info.link_type == CELLTYPE_TX){
         cellList[numCandCells].slotoffset       = i;
         cellList[numCandCells].channeloffset    = info.channelOffset;
         cellList[numCandCells].isUsed           = TRUE;
         numCandCells++;
         if (numCandCells==CELLLIST_MAX_LEN){
            break;
         }
      }
   }
   
   if(numCandCells<requiredCells){
      return FALSE;
   }else{
      return TRUE;
   }
}

void msf_housekeeping(OpenMote* self){
    
    open_addr_t    parentNeighbor;
    bool           foundNeighbor;
    cellInfo_ht    celllist_add[CELLLIST_MAX_LEN];
    cellInfo_ht    celllist_delete[CELLLIST_MAX_LEN];
    
    if ( ieee154e_isSynch(self)==FALSE) {
        return;
    }
    
    foundNeighbor = icmpv6rpl_getPreferredParentEui64(self, &parentNeighbor);
    if (foundNeighbor==FALSE) {
        return;
    }
    if ( schedule_getNumberOfDedicatedCells(self, &parentNeighbor)==0){
 msf_trigger6pAdd(self);
        return;
    }
    
    if ((self->msf_vars).waitretry){
        return;
    }
    
    if ( schedule_isNumTxWrapped(self, &parentNeighbor)==FALSE){
        return;
    }
    
    memset(celllist_delete, 0, CELLLIST_MAX_LEN*sizeof(cellInfo_ht));
    if ( schedule_getCellsToBeRelocated(self, &parentNeighbor, celllist_delete)){
        if ( msf_candidateAddCellList(self, celllist_add,NUMCELLS_MSF)==FALSE){
            // failed to get cell list to add
            return;
        }
 sixtop_request(self, 
            IANA_6TOP_CMD_RELOCATE,   // code
            &parentNeighbor,          // neighbor
            NUMCELLS_MSF,             // number cells
            CELLOPTIONS_MSF,          // cellOptions
            celllist_add,             // celllist to add
            celllist_delete,          // celllist to delete
            IANA_6TISCH_SFID_MSF,     // sfid
            0,                        // list command offset (not used)
            0                         // list command maximum celllist (not used)
        );
    }
}
