/**
DO NOT EDIT DIRECTLY!!

This file was 'objectified' by SCons as a pre-processing
step for the building a Python extension module.

This was done on 2018-03-08 18:25:56.365852.
*/
#ifndef __SCHEDULE_H
#define __SCHEDULE_H

/**
\addtogroup MAChigh
\{
\addtogroup Schedule
\{
*/

#include "Python.h"

#include "opendefs_obj.h"

//=========================== define ==========================================

/**
\brief The length of the superframe, in slots.

The superframe reappears over time and can be arbitrarily long.
*/
#define SLOTFRAME_LENGTH    23 //should be 101

//draft-ietf-6tisch-minimal-06
#define SCHEDULE_MINIMAL_6TISCH_ACTIVE_CELLS                      1
#define SCHEDULE_MINIMAL_6TISCH_SLOTOFFSET                        0
#define SCHEDULE_MINIMAL_6TISCH_CHANNELOFFSET                     0
#define SCHEDULE_MINIMAL_6TISCH_DEFAULT_SLOTFRAME_HANDLE          0 //id of slotframe
#define SCHEDULE_MINIMAL_6TISCH_DEFAULT_SLOTFRAME_NUMBER          1 //1 slotframe by default.

#define NUMSERIALRX          9

/*
  NUMSLOTSOFF is the max number of cells that the mote can add into schedule, 
  besides 6TISCH_ACTIVE_CELLS and NUMSERIALRX Cell. Initially those cells are 
  off. The value of NUMSLOTSOFF can be changed but the value should satisfy:
 
        MAXACTIVESLOTS < SLOTFRAME_LENGTH 
        
  This would make sure number of slots are available (SLOTFRAME_LENGTH-MAXACTIVESLOTS) 
  for serial port to transmit data to dagroot.
*/

#define NUMSLOTSOFF          13

/**
\brief Maximum number of active slots in a superframe.

Note that this is merely used to allocate RAM memory for the schedule. The
schedule is represented, in RAM, by a table. There is one row per active slot
in that table; a slot is "active" when it is not of type CELLTYPE_OFF.

Set this number to the exact number of active slots you are planning on having
in your schedule, so not to waste RAM.
*/
#define MAXACTIVESLOTS       (SCHEDULE_MINIMAL_6TISCH_ACTIVE_CELLS+NUMSERIALRX+NUMSLOTSOFF)

/**
\brief Minimum backoff exponent.

Backoff is used only in slots that are marked as shared in the schedule. When
not shared, the mote assumes that schedule is collision-free, and therefore
does not use any backoff mechanism when a transmission fails.
*/
#define MINBE                2

/**
\brief Maximum backoff exponent.

See MINBE for an explanation of backoff.
*/
#define MAXBE                4

/**
\brief a threshold used for triggering the maintaining process.uint: percent
*/
#define RELOCATE_PDRTHRES  50 // 50 means 50%

typedef enum{
    CELLOPTIONS_TX              = 1<<0,
    CELLOPTIONS_RX              = 1<<1,
    CELLOPTIONS_SHARED          = 1<<2,
    CELLOPTIONS_TIMEKEPPING     = 1<<3,
    CELLOPTIONS_PRIORITY        = 1<<4
}linkOptions_t;

//=========================== typedef =========================================

typedef uint8_t    channelOffset_t;
typedef uint16_t   slotOffset_t;
typedef uint16_t   frameLength_t;

typedef enum {
   CELLTYPE_OFF              = 0,
   CELLTYPE_TX               = 1,
   CELLTYPE_RX               = 2,
   CELLTYPE_TXRX             = 3,
   CELLTYPE_SERIALRX         = 4,
   CELLTYPE_MORESERIALRX     = 5
} cellType_t;

typedef struct {
   slotOffset_t    slotOffset;
   cellType_t      type;
   bool            shared;
   uint8_t         channelOffset;
   open_addr_t     neighbor;
   uint8_t         numRx;
   uint8_t         numTx;
   uint8_t         numTxACK;
   asn_t           lastUsedAsn;
   void*           next;
} scheduleEntry_t;

BEGIN_PACK
typedef struct {
   uint8_t         row;
   slotOffset_t    slotOffset;
   uint8_t         type;
   bool            shared;
   uint8_t         channelOffset;
   open_addr_t     neighbor;
   uint8_t         numRx;
   uint8_t         numTx;
   uint8_t         numTxACK;
   asn_t           lastUsedAsn;
} debugScheduleEntry_t;
END_PACK

typedef struct {
  uint8_t          address[LENGTH_ADDR64b];
  cellType_t       link_type;
  bool             shared;
  slotOffset_t     slotOffset;
  channelOffset_t  channelOffset;
}slotinfo_element_t;

//=========================== variables =======================================

typedef struct {
   scheduleEntry_t  scheduleBuf[MAXACTIVESLOTS];
   scheduleEntry_t* currentScheduleEntry;
   frameLength_t    frameLength;
   frameLength_t    maxActiveSlots;
   uint8_t          frameHandle;
   uint8_t          frameNumber;
   uint8_t          backoffExponenton;
   uint8_t          backoff;
   uint8_t          debugPrintRow;
} schedule_vars_t;

#include "openwsnmodule_obj.h"
typedef struct OpenMote OpenMote;

//=========================== prototypes ======================================

// admin
void schedule_init(OpenMote* self);
void schedule_startDAGroot(OpenMote* self);
bool debugPrint_schedule(OpenMote* self);
bool debugPrint_backoff(OpenMote* self);

// from 6top
void schedule_setFrameLength(OpenMote* self, frameLength_t newFrameLength);
void schedule_setFrameHandle(OpenMote* self, uint8_t frameHandle);
void schedule_setFrameNumber(OpenMote* self, uint8_t frameNumber);
owerror_t schedule_addActiveSlot(OpenMote* self, 
   slotOffset_t         slotOffset,
   cellType_t           type,
   bool                 shared,
   uint8_t              channelOffset,
   open_addr_t*         neighbor
);

void schedule_getSlotInfo(OpenMote* self, 
   slotOffset_t         slotOffset,                      
   open_addr_t*         neighbor,
   slotinfo_element_t*  info
);
owerror_t schedule_removeActiveSlot(OpenMote* self, 
   slotOffset_t         slotOffset,
   open_addr_t*         neighbor
);
bool schedule_isSlotOffsetAvailable(OpenMote* self, uint16_t slotOffset);
void schedule_removeAllCells(OpenMote* self, 
   uint8_t        slotframeID,
   open_addr_t*   previousHop
);
uint8_t schedule_getNumberOfFreeEntries(OpenMote* self);
uint8_t schedule_getNumberOfDedicatedCells(OpenMote* self, open_addr_t* neighbor);
bool schedule_isNumTxWrapped(OpenMote* self, open_addr_t* neighbor);
bool schedule_getCellsToBeRelocated(OpenMote* self, open_addr_t* neighbor, cellInfo_ht* celllist);
bool schedule_hasDedicatedCellToNeighbor(OpenMote* self, open_addr_t* neighbor);
open_addr_t* schedule_getNonParentNeighborWithDedicatedCells(OpenMote* self, open_addr_t* neighbor);

// from IEEE802154E
void schedule_syncSlotOffset(OpenMote* self, slotOffset_t targetSlotOffset);
void schedule_advanceSlot(OpenMote* self);
slotOffset_t schedule_getNextActiveSlotOffset(OpenMote* self);
frameLength_t schedule_getFrameLength(OpenMote* self);
cellType_t schedule_getType(OpenMote* self);
void schedule_getNeighbor(OpenMote* self, open_addr_t* addrToWrite);
channelOffset_t schedule_getChannelOffset(OpenMote* self);
bool schedule_getOkToSend(OpenMote* self);
void schedule_resetBackoff(OpenMote* self);
void schedule_indicateRx(OpenMote* self, asn_t*   asnTimestamp);
void schedule_indicateTx(OpenMote* self, 
                        asn_t*    asnTimestamp,
                        bool      succesfullTx
                   );
// from sixtop
bool schedule_getOneCellAfterOffset(OpenMote* self, 
    uint8_t metadata,
    uint8_t offset,
    open_addr_t* neighbor, 
    uint8_t cellOptions, 
    uint16_t* slotoffset, 
    uint16_t* channeloffset
);
/**
\}
\}
*/
          
#endif
