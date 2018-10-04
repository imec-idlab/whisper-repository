/**
DO NOT EDIT DIRECTLY!!

This file was 'objectified' by SCons as a pre-processing
step for the building a Python extension module.

This was done on 2018-03-08 19:29:25.019652.
*/
#include "opendefs_obj.h"
#include "sixtop_obj.h"
#include "openserial_obj.h"
#include "openqueue_obj.h"
#include "neighbors_obj.h"
#include "IEEE802154E_obj.h"
#include "iphc_obj.h"
#include "packetfunctions_obj.h"
#include "openrandom_obj.h"
#include "scheduler_obj.h"
#include "opentimers_obj.h"
#include "debugpins_obj.h"
#include "leds_obj.h"
#include "IEEE802154_obj.h"
#include "IEEE802154_security_obj.h"
#include "idmanager_obj.h"
#include "schedule_obj.h"
#include "whisper_obj.h"

//=========================== define ==========================================

// in seconds: sixtop maintaince is called every 30 seconds
#define MAINTENANCE_PERIOD        30

//=========================== variables =======================================

// declaration of global variable _sixtop_vars_ removed during objectification.

//=========================== prototypes ======================================

// send internal
owerror_t sixtop_send_internal(OpenMote* self, 
   OpenQueueEntry_t*    msg,
   bool                 payloadIEPresent
);

// timer interrupt callbacks
void sixtop_maintenance_timer_cb(OpenMote* self, opentimers_id_t id);
void sixtop_timeout_timer_cb(OpenMote* self, opentimers_id_t id);
void sixtop_sendingEb_timer_cb(OpenMote* self, opentimers_id_t id);

//=== EB/KA task

void timer_sixtop_sendEb_fired(OpenMote* self);
void timer_sixtop_management_fired(OpenMote* self);
void sixtop_sendEB(OpenMote* self);
void sixtop_sendKA(OpenMote* self);

//=== six2six task

void timer_sixtop_six2six_timeout_fired(OpenMote* self);
void sixtop_six2six_sendDone(OpenMote* self, 
   OpenQueueEntry_t*    msg,
   owerror_t            error
);
bool sixtop_processIEs(OpenMote* self, 
   OpenQueueEntry_t*    pkt,
   uint16_t*            lenIE
);
void sixtop_six2six_notifyReceive(OpenMote* self, 
   uint8_t              version, 
   uint8_t              type,
   uint8_t              code, 
   uint8_t              sfId,
   uint8_t              seqNum,
   uint8_t              ptr,
   uint8_t              length,
   OpenQueueEntry_t*    pkt
);

//=== helper functions

bool sixtop_addCells(OpenMote* self, 
   uint8_t              slotframeID,
   cellInfo_ht*         cellList,
   open_addr_t*         previousHop,
   uint8_t              cellOptions
);
bool sixtop_removeCells(OpenMote* self, 
   uint8_t              slotframeID,
   cellInfo_ht*         cellList,
   open_addr_t*         previousHop,
   uint8_t              cellOptions
);
bool sixtop_areAvailableCellsToBeScheduled(OpenMote* self, 
    uint8_t              frameID, 
    uint8_t              numOfCells, 
    cellInfo_ht*         cellList
);
bool sixtop_areAvailableCellsToBeRemoved(OpenMote* self,       
    uint8_t      frameID, 
    uint8_t      numOfCells, 
    cellInfo_ht* cellList,
    open_addr_t* neighbor,
    uint8_t      cellOptions
);

//=========================== public ==========================================

void sixtop_init(OpenMote* self) {
    
    (self->sixtop_vars).periodMaintenance  = 872 +( openrandom_get16b(self)&0xff);
    (self->sixtop_vars).busySendingKA      = FALSE;
    (self->sixtop_vars).busySendingEB      = FALSE;
    (self->sixtop_vars).dsn                = 0;
    (self->sixtop_vars).mgtTaskCounter     = 0;
    (self->sixtop_vars).kaPeriod           = MAXKAPERIOD;
    (self->sixtop_vars).ebPeriod           = EB_PORTION*( neighbors_getNumNeighbors(self)+1);
    (self->sixtop_vars).isResponseEnabled  = TRUE;
    (self->sixtop_vars).six2six_state      = SIX_STATE_IDLE;
    
    (self->sixtop_vars).ebSendingTimerId   = opentimers_create(self);
 opentimers_scheduleIn(self, 
        (self->sixtop_vars).ebSendingTimerId,
        (self->sixtop_vars).periodMaintenance,
        TIME_MS,
        TIMER_ONESHOT,
        sixtop_sendingEb_timer_cb
    );
    
    (self->sixtop_vars).maintenanceTimerId   = opentimers_create(self);
 opentimers_scheduleIn(self, 
        (self->sixtop_vars).maintenanceTimerId,
        (self->sixtop_vars).periodMaintenance,
        TIME_MS,
        TIMER_PERIODIC,
        sixtop_maintenance_timer_cb
    );
    
    (self->sixtop_vars).timeoutTimerId      = opentimers_create(self);
}

void sixtop_setKaPeriod(OpenMote* self, uint16_t kaPeriod) {
    if(kaPeriod > MAXKAPERIOD) {
        (self->sixtop_vars).kaPeriod = MAXKAPERIOD;
    } else {
        (self->sixtop_vars).kaPeriod = kaPeriod;
    } 
}

void sixtop_setEBPeriod(OpenMote* self, uint8_t ebPeriod) {
    if(ebPeriod != 0) {
        // convert parameter to miliseconds
        (self->sixtop_vars).ebPeriod = ebPeriod;
    }
}

void sixtop_setSFcallback(OpenMote* self, 
    sixtop_sf_getsfid           cb0,
    sixtop_sf_getmetadata       cb1, 
    sixtop_sf_translatemetadata cb2, 
    sixtop_sf_handle_callback   cb3
){
   (self->sixtop_vars).cb_sf_getsfid            = cb0;
   (self->sixtop_vars).cb_sf_getMetadata        = cb1;
   (self->sixtop_vars).cb_sf_translateMetadata  = cb2;
   (self->sixtop_vars).cb_sf_handleRCError      = cb3;
}

//======= scheduling

owerror_t sixtop_request(OpenMote* self, 
    uint8_t      code, 
    open_addr_t* neighbor, 
    uint8_t      numCells, 
    uint8_t      cellOptions, 
    cellInfo_ht* celllist_toBeAdded,
    cellInfo_ht* celllist_toBeDeleted,
    uint8_t      sfid,
    uint16_t     listingOffset,
    uint16_t     listingMaxNumCells
    ){
    OpenQueueEntry_t* pkt;
    uint8_t           i;
    uint8_t           len;
    uint16_t          length_groupid_type;
    uint8_t           sequenceNumber;
    owerror_t         outcome;
   
    //printf("I am node %d, sixtop_request!! \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);

    // filter parameters: handler, status and neighbor
    if(
        (self->sixtop_vars).six2six_state != SIX_STATE_IDLE   ||
        neighbor                  == NULL
    ){
    	//printf("I am node %d, I want some cells but I am busy \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
        // neighbor can't be none or previous transcation doesn't finishe yet
        return E_FAIL;
    }

    // get a free packet buffer
    pkt = openqueue_getFreePacketBuffer(self, COMPONENT_SIXTOP_RES);
    if (pkt==NULL) {
 openserial_printError(self, 
            COMPONENT_SIXTOP_RES,
            ERR_NO_FREE_PACKET_BUFFER,
            (errorparameter_t)0,
            (errorparameter_t)0
        );
        return E_FAIL;
    }

    //printf("%d \n",code);
    //printf("I am node %d, I want send REQUEST type %d \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],code);

    // take ownership
    pkt->creator = COMPONENT_SIXTOP_RES;
    pkt->owner   = COMPONENT_SIXTOP_RES;
   
    memcpy(&(pkt->l2_nextORpreviousHop),neighbor,sizeof(open_addr_t));
    if (celllist_toBeDeleted != NULL){
        memcpy((self->sixtop_vars).celllist_toDelete,celllist_toBeDeleted,CELLLIST_MAX_LEN*sizeof(cellInfo_ht));
    }
    (self->sixtop_vars).cellOptions = cellOptions;
    
    len  = 0;
    if (
        code == IANA_6TOP_CMD_ADD      || 
        code == IANA_6TOP_CMD_DELETE   || 
        code == IANA_6TOP_CMD_RELOCATE
    ){
        // append 6p celllists
        if (code == IANA_6TOP_CMD_ADD || code == IANA_6TOP_CMD_RELOCATE){
            for(i=0;i<CELLLIST_MAX_LEN;i++) {
                if(celllist_toBeAdded[i].isUsed){
 packetfunctions_reserveHeaderSize(self, pkt,4); 
                    pkt->payload[0] = (uint8_t)(celllist_toBeAdded[i].slotoffset         & 0x00FF);
                    pkt->payload[1] = (uint8_t)((celllist_toBeAdded[i].slotoffset        & 0xFF00)>>8);
                    pkt->payload[2] = (uint8_t)(celllist_toBeAdded[i].channeloffset      & 0x00FF);
                    pkt->payload[3] = (uint8_t)((celllist_toBeAdded[i].channeloffset     & 0xFF00)>>8);
                    len += 4;
                }
            }
        }
        if (code == IANA_6TOP_CMD_DELETE || code == IANA_6TOP_CMD_RELOCATE){
            for(i=0;i<CELLLIST_MAX_LEN;i++) {
                if(celllist_toBeDeleted[i].isUsed){
 packetfunctions_reserveHeaderSize(self, pkt,4); 
                    pkt->payload[0] = (uint8_t)(celllist_toBeDeleted[i].slotoffset       & 0x00FF);
                    pkt->payload[1] = (uint8_t)((celllist_toBeDeleted[i].slotoffset      & 0xFF00)>>8);
                    pkt->payload[2] = (uint8_t)(celllist_toBeDeleted[i].channeloffset    & 0x00FF);
                    pkt->payload[3] = (uint8_t)((celllist_toBeDeleted[i].channeloffset   & 0xFF00)>>8);
                    len += 4;
                }
            }
        }
        // append 6p numberCells
 packetfunctions_reserveHeaderSize(self, pkt,sizeof(uint8_t));
        *((uint8_t*)(pkt->payload)) = numCells;
        len += 1;
    }
    
    if (code == IANA_6TOP_CMD_LIST){
        // append 6p max number of cells
 packetfunctions_reserveHeaderSize(self, pkt,sizeof(uint16_t));
        *((uint8_t*)(pkt->payload))   = (uint8_t)(listingMaxNumCells & 0x00FF);
        *((uint8_t*)(pkt->payload+1)) = (uint8_t)(listingMaxNumCells & 0xFF00)>>8;
        len +=2;
        // append 6p listing offset
 packetfunctions_reserveHeaderSize(self, pkt,sizeof(uint16_t));
        *((uint8_t*)(pkt->payload))   = (uint8_t)(listingOffset & 0x00FF);
        *((uint8_t*)(pkt->payload+1)) = (uint8_t)(listingOffset & 0xFF00)>>8;
        len += 2;
        // append 6p Reserved field
 packetfunctions_reserveHeaderSize(self, pkt,sizeof(uint8_t));
        *((uint8_t*)(pkt->payload)) = 0;
        len += 1;
    }
    
    if (code != IANA_6TOP_CMD_CLEAR){
        // append 6p celloptions
 packetfunctions_reserveHeaderSize(self, pkt,sizeof(uint8_t));
        *((uint8_t*)(pkt->payload)) = cellOptions;
        len+=1;
    } else {
        // record the neighbor in case no response  for clear
        memcpy(&(self->sixtop_vars).neighborToClearCells,neighbor,sizeof(open_addr_t));
    }
    
    // append 6p metadata
 packetfunctions_reserveHeaderSize(self, pkt,sizeof(uint16_t));
    pkt->payload[0] = (uint8_t)( (self->sixtop_vars).cb_sf_getMetadata() & 0x00FF);
    pkt->payload[1] = (uint8_t)(((self->sixtop_vars).cb_sf_getMetadata() & 0xFF00)>>8);
    len += 2;
   
//    // append 6p Seqnum and schedule Generation
// packetfunctions_reserveHeaderSize(self, pkt,sizeof(uint8_t));
//    sequenceNumber              = neighbors_getSequenceNumber(self, neighbor);
//    *((uint8_t*)(pkt->payload)) = sequenceNumber;
//    len += 1;


    if (sfid == 5){
        // append 6p Seqnum and schedule Generation
 packetfunctions_reserveHeaderSize(self, pkt,sizeof(uint8_t));
        sequenceNumber              = 0;
    	sfid=0;
    	pkt->isSixtopFake=TRUE;
    	//printf("I am node %d, sending 6P REQUEST to %d with FAKE option \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],neighbor->addr_64b[7]);

    	if (neighbor->addr_64b[7] == whisper_getTargetParentNew(self)){
    		//this is the first 6P message

            //printf("A I am node %d, sending FAKE 6P REQUEST to %d with seq num %d and fake source address %d \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],neighbor->addr_64b[7],sequenceNumber, whisper_getFakeSource(self));
            //sequenceNumber = sequenceNumber+1;
    	    *((uint8_t*)(pkt->payload)) = sequenceNumber;
    	    len += 1;

    	}if (neighbor->addr_64b[7] == whisper_getTargetChildren(self)){
    		//this is the first 6P message
    		if (code != IANA_6TOP_CMD_CLEAR){
    			sequenceNumber = sequenceNumber+1;
    		}else{
    			//we use a wrong seq num on purpose
    			//sequenceNumber = sequenceNumber+2;

    			//this is the second clear message
    			sequenceNumber = sequenceNumber+2;

    		}
    		//printf("B I am node %d, sending FAKE 6P REQUEST to %d with seq num %d and fake source address %d\n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],neighbor->addr_64b[7],sequenceNumber, whisper_getFakeSource(self));
            //printf("I am node %d, sending FAKE 6P REQUEST to %d with incremented seq num %d and fake source address %d\n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],neighbor->addr_64b[7],sequenceNumber, whisper_getFakeSource(self));
    	    *((uint8_t*)(pkt->payload)) = sequenceNumber;
    	    len += 1;

    	}if (neighbor->addr_64b[7] == whisper_getTargetParentOld(self)){
    		//this is the first 6P message, clearing
    		sequenceNumber = sequenceNumber+1;
            //printf("A I am node %d, sending FAKE 6P REQUEST to %d with seq num %d and fake source address %d \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],neighbor->addr_64b[7],sequenceNumber, whisper_getFakeSource(self));
            //sequenceNumber = sequenceNumber+1;
    	    *((uint8_t*)(pkt->payload)) = sequenceNumber;
    	    len += 1;


    	}
    }else{
    	pkt->isSixtopFake=FALSE;
    	//printf("I am node %d, sending 6P REQUEST to %d withOut FAKE option \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],neighbor->addr_64b[7]);
        // append 6p Seqnum and schedule Generation
 packetfunctions_reserveHeaderSize(self, pkt,sizeof(uint8_t));
        sequenceNumber              = neighbors_getSequenceNumber(self, neighbor);
        *((uint8_t*)(pkt->payload)) = sequenceNumber;
        len += 1;
    }





    
    // append 6p sfid
 packetfunctions_reserveHeaderSize(self, pkt,sizeof(uint8_t));
    *((uint8_t*)(pkt->payload)) = sfid;
    len += 1;
      
    // append 6p code
 packetfunctions_reserveHeaderSize(self, pkt,sizeof(uint8_t));
    *((uint8_t*)(pkt->payload)) = code;
    // record the code to determine the action after 6p senddone
    pkt->l2_sixtop_command      = code;
    len += 1;
    
    // append 6p version, T(type) and  R(reserved)
 packetfunctions_reserveHeaderSize(self, pkt,sizeof(uint8_t));
    *((uint8_t*)(pkt->payload)) = IANA_6TOP_6P_VERSION | IANA_6TOP_TYPE_REQUEST;
    len += 1;
    
    // append 6p subtype id
 packetfunctions_reserveHeaderSize(self, pkt,sizeof(uint8_t));
    *((uint8_t*)(pkt->payload)) = IANA_6TOP_SUBIE_ID;
    len += 1;
   
    // append IETF IE header (length_groupid_type)
 packetfunctions_reserveHeaderSize(self, pkt, sizeof(uint16_t));
    length_groupid_type  = len;
    length_groupid_type |= (IANA_IETF_IE_GROUP_ID  | IANA_IETF_IE_TYPE); 
    pkt->payload[0]      = length_groupid_type        & 0xFF;
    pkt->payload[1]      = (length_groupid_type >> 8) & 0xFF;
   
    // indicate IEs present
    pkt->l2_payloadIEpresent = TRUE;
    // record this packet as sixtop request message
    pkt->l2_sixtop_messageType    = SIXTOP_CELL_REQUEST;
    
    // send packet
    outcome = sixtop_send(self, pkt);
    
    if (outcome == E_SUCCESS){
        //update states
        switch(code){
        case IANA_6TOP_CMD_ADD:
            if (pkt->isSixtopFake==FALSE){
            	(self->sixtop_vars).six2six_state = SIX_STATE_WAIT_ADDREQUEST_SENDDONE;
            	//printf("I am node %d, enqueued ADD to %d, state SIX_STATE_WAIT_ADDREQUEST_SENDDONE %d\n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],neighbor->addr_64b[7],(self->sixtop_vars).six2six_state);
            }else{
            	(self->sixtop_vars).six2six_state = SIX_STATE_IDLE;
            	//printf("I am node %d, enqueued FAKE ADD to %d, comming back state SIX_STATE_IDLE %d \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],neighbor->addr_64b[7],(self->sixtop_vars).six2six_state);

            }

            break;
        case IANA_6TOP_CMD_DELETE:
            (self->sixtop_vars).six2six_state = SIX_STATE_WAIT_DELETEREQUEST_SENDDONE;
            break;
        case IANA_6TOP_CMD_RELOCATE:
            (self->sixtop_vars).six2six_state = SIX_STATE_WAIT_RELOCATEREQUEST_SENDDONE;
            break;
        case IANA_6TOP_CMD_COUNT:
            (self->sixtop_vars).six2six_state = SIX_STATE_WAIT_COUNTREQUEST_SENDDONE;
            break;
        case IANA_6TOP_CMD_LIST:
            (self->sixtop_vars).six2six_state = SIX_STATE_WAIT_LISTREQUEST_SENDDONE;
            break;
        case IANA_6TOP_CMD_CLEAR:
            if (pkt->isSixtopFake==FALSE){
            	(self->sixtop_vars).six2six_state = SIX_STATE_WAIT_CLEARREQUEST_SENDDONE;
            	//printf("I am node %d, enqueued CLEAR to %d, state SIX_STATE_WAIT_ADDREQUEST_SENDDONE %d\n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],neighbor->addr_64b[7],(self->sixtop_vars).six2six_state);
            }else{
            	(self->sixtop_vars).six2six_state = SIX_STATE_IDLE;
            	//printf("I am node %d, enqueued FAKE CLEAR to %d, comming back state SIX_STATE_IDLE %d \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],neighbor->addr_64b[7],(self->sixtop_vars).six2six_state);

            }
            //(self->sixtop_vars).six2six_state = SIX_STATE_WAIT_CLEARREQUEST_SENDDONE;
            break;
        }
    } else {
 openqueue_freePacketBuffer(self, pkt);
    }
    return outcome;
}

//======= from upper layer

owerror_t sixtop_send(OpenMote* self, OpenQueueEntry_t *msg) {

    open_addr_t addressToWrite;
    
    if (
 idmanager_getIsDAGroot(self) == FALSE               &&
        (
            msg->creator != COMPONENT_SIXTOP_RES        &&
            msg->creator != COMPONENT_CJOIN             &&
            (
 icmpv6rpl_getPreferredParentEui64(self, &addressToWrite) == FALSE      ||
                (
 icmpv6rpl_getPreferredParentEui64(self, &addressToWrite)           &&
 schedule_hasDedicatedCellToNeighbor(self, &addressToWrite)== FALSE
                )
            )
        )
    ){
        return E_FAIL;
    }

    // set metadata
    msg->owner        = COMPONENT_SIXTOP;
    msg->l2_frameType = IEEE154_TYPE_DATA;

    // set l2-security attributes
    msg->l2_securityLevel   = IEEE802154_security_getSecurityLevel(self, msg);
    msg->l2_keyIdMode       = IEEE802154_SECURITY_KEYIDMODE; 
    msg->l2_keyIndex        = IEEE802154_security_getDataKeyIndex(self);

    if (msg->l2_payloadIEpresent == FALSE) {
        return sixtop_send_internal(self, 
            msg,
            FALSE
        );
    } else {
        return sixtop_send_internal(self, 
            msg,
            TRUE
        );
    }
}

//======= from lower layer

void task_sixtopNotifSendDone(OpenMote* self) {
    OpenQueueEntry_t* msg;

    //printf("I am node %d, Send DONE!\n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);

    // get recently-sent packet from openqueue
    msg = openqueue_sixtopGetSentPacket(self);
    if (msg==NULL) {
 openserial_printCritical(self, 
            COMPONENT_SIXTOP,
            ERR_NO_SENT_PACKET,
            (errorparameter_t)0,
            (errorparameter_t)0
        );
        return;
    }

    // take ownership
    msg->owner = COMPONENT_SIXTOP;
    
    // update neighbor statistics
    if (msg->l2_sendDoneError==E_SUCCESS) {
 neighbors_indicateTx(self, 
            &(msg->l2_nextORpreviousHop),
            msg->l2_numTxAttempts,
            TRUE,
            &msg->l2_asn
        );
    } else {
 neighbors_indicateTx(self, 
             &(msg->l2_nextORpreviousHop),
             msg->l2_numTxAttempts,
             FALSE,
             &msg->l2_asn
        );
    }
    
    // send the packet to where it belongs
    switch (msg->creator) {
        case COMPONENT_SIXTOP:
            if (msg->l2_frameType==IEEE154_TYPE_BEACON) {
                // this is a EB
                
                // not busy sending EB anymore
                (self->sixtop_vars).busySendingEB = FALSE;
            } else {
                // this is a KA
                
                // not busy sending KA anymore
                (self->sixtop_vars).busySendingKA = FALSE;
            }
            // discard packets
 openqueue_freePacketBuffer(self, msg);
            break;
        case COMPONENT_SIXTOP_RES:
        	//printf("I am NODE %d, 6P packet send done accepted \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
 sixtop_six2six_sendDone(self, msg,msg->l2_sendDoneError);
            break;
        default:
            // send the rest up the stack
 iphc_sendDone(self, msg,msg->l2_sendDoneError);
            break;
    }
}

void task_sixtopNotifReceive(OpenMote* self) {
    OpenQueueEntry_t* msg;
    uint16_t          lenIE;
    // get received packet from openqueue
    msg = openqueue_sixtopGetReceivedPacket(self);
    if (msg==NULL) {
 openserial_printCritical(self, 
            COMPONENT_SIXTOP,
            ERR_NO_RECEIVED_PACKET,
            (errorparameter_t)0,
            (errorparameter_t)0
        );
        return;
    }

   
    // take ownership
    msg->owner = COMPONENT_SIXTOP;
   
    // update neighbor statistics
 neighbors_indicateRx(self, 
        &(msg->l2_nextORpreviousHop),
        msg->l1_rssi,
        &msg->l2_asn,
        msg->l2_joinPriorityPresent,
        msg->l2_joinPriority,
        msg->l2_securityLevel == IEEE154_ASH_SLF_TYPE_NOSEC ? TRUE : FALSE
    );
    
    // process the header IEs
    lenIE=0;
    if(
        msg->l2_frameType              == IEEE154_TYPE_DATA  &&
        msg->l2_payloadIEpresent       == TRUE               &&
 sixtop_processIEs(self, msg, &lenIE) == FALSE
    ) {
        // free the packet's RAM memory
 openqueue_freePacketBuffer(self, msg);
        //log error
        return;
    }
   
    // toss the header IEs
 packetfunctions_tossHeader(self, msg,lenIE);
   
    // reset it to avoid race conditions with this var.
    msg->l2_joinPriorityPresent = FALSE; 
   
    // send the packet up the stack, if it qualifies
    switch (msg->l2_frameType) {
    case IEEE154_TYPE_BEACON:
    case IEEE154_TYPE_DATA:
    case IEEE154_TYPE_CMD:
        if (msg->length>0) {
            if (msg->l2_frameType == IEEE154_TYPE_BEACON){
                // I have one byte frequence field, no useful for upper layer
                // free up the RAM
 openqueue_freePacketBuffer(self, msg);
                break;
            }
            // send to upper layer
 iphc_receive(self, msg);
        } else {
            // free up the RAM
 openqueue_freePacketBuffer(self, msg);
        }
        break;
    case IEEE154_TYPE_ACK:
    default:
        // free the packet's RAM memory
 openqueue_freePacketBuffer(self, msg);
        // log the error
 openserial_printError(self, 
            COMPONENT_SIXTOP,
            ERR_MSG_UNKNOWN_TYPE,
            (errorparameter_t)msg->l2_frameType,
            (errorparameter_t)0
        );
        break;
    }
}

//======= debugging

/**
\brief Trigger this module to print status information, over serial.

debugPrint_* functions are used by the openserial module to continuously print
status information about several modules in the OpenWSN stack.

\returns TRUE if this function printed something, FALSE otherwise.
*/
bool debugPrint_myDAGrank(OpenMote* self) {
    uint16_t output;
    
    output = 0;
    output = icmpv6rpl_getMyDAGrank(self);
 openserial_printStatus(self, STATUS_DAGRANK,(uint8_t*)&output,sizeof(uint16_t));
    return TRUE;
}

/**
\brief Trigger this module to print status information, over serial.

debugPrint_* functions are used by the openserial module to continuously print
status information about several modules in the OpenWSN stack.

\returns TRUE if this function printed something, FALSE otherwise.
*/
bool debugPrint_kaPeriod(OpenMote* self) {
    uint16_t output;
    
    output = (self->sixtop_vars).kaPeriod;
 openserial_printStatus(self, 
        STATUS_KAPERIOD,
        (uint8_t*)&output,
        sizeof(output)
    );
    return TRUE;
}

void sixtop_setIsResponseEnabled(OpenMote* self, bool isEnabled){
    (self->sixtop_vars).isResponseEnabled = isEnabled;
}

//=========================== private =========================================

/**
\brief Transfer packet to MAC.

This function adds a IEEE802.15.4 header to the packet and leaves it the 
OpenQueue buffer. The very last thing it does is assigning this packet to the 
virtual component COMPONENT_SIXTOP_TO_IEEE802154E. Whenever it gets a change,
IEEE802154E will handle the packet.

\param[in] msg The packet to the transmitted
\param[in] payloadIEPresent Indicates wheter an Information Element is present in the
   packet.

\returns E_SUCCESS iff successful.
*/
owerror_t sixtop_send_internal(OpenMote* self, 
    OpenQueueEntry_t* msg, 
    bool    payloadIEPresent) {


	if (msg->isDioFake==TRUE){
		if ( idmanager_getIsDAGroot(self)==TRUE) {
			msg->l2_nextORpreviousHop.addr_64b[7]=2;
			printf("I am whisperroot node %d, sending sixtop fake message \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
//	      if ( idmanager_getIsDAGroot(self)==TRUE) {
//			if ( whisper_getTargetParentOld(self)==0x55){
//				msg->l2_nextORpreviousHop.addr_64b[7]=0x61;
//			}else{
//				msg->l2_nextORpreviousHop.addr_64b[7]=0x2b;
//			}
	      }
	}



    // assign a number of retries
    if (
 packetfunctions_isBroadcastMulticast(self, &(msg->l2_nextORpreviousHop))==TRUE
    ) {
        msg->l2_retriesLeft = 1;
    } else {
        msg->l2_retriesLeft = TXRETRIES + 1;
    }
    // record this packet's dsn (for matching the ACK)
    msg->l2_dsn = (self->sixtop_vars).dsn++;
    // this is a new packet which I never attempted to send
    msg->l2_numTxAttempts = 0;
    // transmit with the default TX power
    msg->l1_txPower = TX_POWER;
    // add a IEEE802.15.4 header
 ieee802154_prependHeader(self, 
        msg,
        msg->l2_frameType,
        payloadIEPresent,
        msg->l2_dsn,
        &(msg->l2_nextORpreviousHop)
    );
    // change owner to IEEE802154E fetches it from queue
    msg->owner  = COMPONENT_SIXTOP_TO_IEEE802154E;
    return E_SUCCESS;
}

// timer interrupt callbacks
void sixtop_sendingEb_timer_cb(OpenMote* self, opentimers_id_t id){
 scheduler_push_task(self, timer_sixtop_sendEb_fired,TASKPRIO_SIXTOP);
    // update the period
    (self->sixtop_vars).periodMaintenance  = 872 +( openrandom_get16b(self)&0xff);
 opentimers_scheduleIn(self, 
        (self->sixtop_vars).ebSendingTimerId,
        (self->sixtop_vars).periodMaintenance,
        TIME_MS,
        TIMER_ONESHOT,
        sixtop_sendingEb_timer_cb
    );
}

void sixtop_maintenance_timer_cb(OpenMote* self, opentimers_id_t id) {
 scheduler_push_task(self, timer_sixtop_management_fired,TASKPRIO_SIXTOP);
}

void sixtop_timeout_timer_cb(OpenMote* self, opentimers_id_t id) {
 scheduler_push_task(self, timer_sixtop_six2six_timeout_fired,TASKPRIO_SIXTOP_TIMEOUT);
}

//======= EB/KA task

void timer_sixtop_sendEb_fired(OpenMote* self){
    
    uint16_t newPeriod;
    // current period 
    newPeriod = EB_PORTION*( neighbors_getNumNeighbors(self)+1);
    if (
        (self->sixtop_vars).ebPeriod  < newPeriod &&
        (self->sixtop_vars).ebCounter > newPeriod
    ){
        (self->sixtop_vars).ebCounter = 0;
        (self->sixtop_vars).ebPeriod  = newPeriod;
 sixtop_sendEB(self);
    } else {
        (self->sixtop_vars).ebPeriod  = newPeriod;
        (self->sixtop_vars).ebCounter = ((self->sixtop_vars).ebCounter+1)%(self->sixtop_vars).ebPeriod;
        switch ((self->sixtop_vars).ebCounter) {
        case 0:
 sixtop_sendEB(self);
            break;
        default:
            break;
        }
    }
}

/**
\brief Timer handlers which triggers MAC management task.

This function is called in task context by the scheduler after the RES timer
has fired. This timer is set to fire every second, on average.

The body of this function executes one of the MAC management task.
*/
void timer_sixtop_management_fired(OpenMote* self) {
   
    (self->sixtop_vars).mgtTaskCounter = ((self->sixtop_vars).mgtTaskCounter+1)%MAINTENANCE_PERIOD;
   
    switch ((self->sixtop_vars).mgtTaskCounter) {
        case 0:
            // called every MAINTENANCE_PERIOD seconds
 neighbors_removeOld(self);
            break;
        default:
            // called every second, except once every MAINTENANCE_PERIOD seconds
 sixtop_sendKA(self);
            break;
    }
}

/**
\brief Send an EB.

This is one of the MAC management tasks. This function inlines in the
timers_res_fired() function, but is declared as a separate function for better
readability of the code.
*/
port_INLINE void sixtop_sendEB(OpenMote* self) {
    OpenQueueEntry_t* eb;
    uint8_t     i;
    uint8_t     eb_len;
    uint16_t    temp16b;
    open_addr_t addressToWrite;
    
    memset(&addressToWrite,0,sizeof(open_addr_t));
    if (
        ( ieee154e_isSynch(self)==FALSE)                     ||
        ( IEEE802154_security_isConfigured(self)==FALSE)     ||
        ( icmpv6rpl_getMyDAGrank(self)==DEFAULTDAGRANK)      ||
 icmpv6rpl_daoSent(self)==FALSE
    ) {
        // I'm not sync'ed, or did not join, or did not acquire a DAGrank or did not send out a DAO
        // before starting to advertize the network, we need to make sure that we are reachable downwards,
        // thus, the condition if DAO was sent
        
        // delete packets genereted by this module (EB and KA) from openqueue
 openqueue_removeAllCreatedBy(self, COMPONENT_SIXTOP);
        
        // I'm not busy sending an EB or KA
        (self->sixtop_vars).busySendingEB = FALSE;
        (self->sixtop_vars).busySendingKA = FALSE;
        
        // stop here
        return;
    }

    if (
 idmanager_getIsDAGroot(self) == FALSE &&
        (
 icmpv6rpl_getPreferredParentEui64(self, &addressToWrite) == FALSE ||
            (
 icmpv6rpl_getPreferredParentEui64(self, &addressToWrite) &&
 schedule_hasDedicatedCellToNeighbor(self, &addressToWrite) == FALSE
            )
        )
    ){
        // delete packets genereted by this module (EB and KA) from openqueue
 openqueue_removeAllCreatedBy(self, COMPONENT_SIXTOP);
        
        // I'm not busy sending an EB or KA
        (self->sixtop_vars).busySendingEB = FALSE;
        (self->sixtop_vars).busySendingKA = FALSE;
        
        return;
    }
   
    if ((self->sixtop_vars).busySendingEB==TRUE) {
        // don't continue if I'm still sending a previous EB
        return;
    }
    
    // if I get here, I will send an EB
    
    // get a free packet buffer
    eb = openqueue_getFreePacketBuffer(self, COMPONENT_SIXTOP);
    if (eb==NULL) {
 openserial_printError(self, 
            COMPONENT_SIXTOP,
            ERR_NO_FREE_PACKET_BUFFER,
            (errorparameter_t)0,
            (errorparameter_t)0
        );
        return;
    }
    
    // declare ownership over that packet
    eb->creator = COMPONENT_SIXTOP;
    eb->owner   = COMPONENT_SIXTOP;
    
    // in case we none default number of shared cells defined in minimal configuration
    if (ebIEsBytestream[EB_SLOTFRAME_NUMLINK_OFFSET]>1){
        for (i=ebIEsBytestream[EB_SLOTFRAME_NUMLINK_OFFSET]-1;i>0;i--){
 packetfunctions_reserveHeaderSize(self, eb,5);
            eb->payload[0]   = i;    // slot offset
            eb->payload[1]   = 0x00;
            eb->payload[2]   = 0x00; // channel offset
            eb->payload[3]   = 0x00;
            eb->payload[4]   = 0x0F; // link options
        }
    }
    
    // reserve space for EB IEs
 packetfunctions_reserveHeaderSize(self, eb,EB_IE_LEN);
    for (i=0;i<EB_IE_LEN;i++){
        eb->payload[i]   = ebIEsBytestream[i];
    }
    
    if (ebIEsBytestream[EB_SLOTFRAME_NUMLINK_OFFSET]>1){
        // reconstruct the MLME IE header since length changed 
        eb_len = EB_IE_LEN-2+5*(ebIEsBytestream[EB_SLOTFRAME_NUMLINK_OFFSET]-1);
        temp16b = eb_len | IEEE802154E_PAYLOAD_DESC_GROUP_ID_MLME | IEEE802154E_PAYLOAD_DESC_TYPE_MLME;
        eb->payload[0] = (uint8_t)(temp16b & 0x00ff);
        eb->payload[1] = (uint8_t)((temp16b & 0xff00)>>8);
    }
    
    // Keep a pointer to where the ASN will be
    // Note: the actual value of the current ASN and JP will be written by the
    //    IEEE802.15.4e when transmitting
    eb->l2_ASNpayload               = &eb->payload[EB_ASN0_OFFSET]; 
    
    // some l2 information about this packet
    eb->l2_frameType                     = IEEE154_TYPE_BEACON;
    eb->l2_nextORpreviousHop.type        = ADDR_16B;
    eb->l2_nextORpreviousHop.addr_16b[0] = 0xff;
    eb->l2_nextORpreviousHop.addr_16b[1] = 0xff;
    
    //I has an IE in my payload
    eb->l2_payloadIEpresent = TRUE;

    // set l2-security attributes
    eb->l2_securityLevel   = IEEE802154_SECURITY_LEVEL_BEACON;
    eb->l2_keyIdMode       = IEEE802154_SECURITY_KEYIDMODE;
    eb->l2_keyIndex        = IEEE802154_security_getBeaconKeyIndex(self);
    
    // put in queue for MAC to handle
 sixtop_send_internal(self, eb,eb->l2_payloadIEpresent);
    
    // I'm now busy sending an EB
    (self->sixtop_vars).busySendingEB = TRUE;
}

/**
\brief Send an keep-alive message, if necessary.

This is one of the MAC management tasks. This function inlines in the
timers_res_fired() function, but is declared as a separate function for better
readability of the code.
*/
port_INLINE void sixtop_sendKA(OpenMote* self) {
    OpenQueueEntry_t* kaPkt;
    open_addr_t*      kaNeighAddr;
    


    if ( ieee154e_isSynch(self)==FALSE) {
        // I'm not sync'ed
        
        // delete packets genereted by this module (EB and KA) from openqueue
 openqueue_removeAllCreatedBy(self, COMPONENT_SIXTOP);
        
        // I'm not busy sending an EB or KA
        (self->sixtop_vars).busySendingEB = FALSE;
        (self->sixtop_vars).busySendingKA = FALSE;
      
        // stop here
        return;
    }

    if ((self->sixtop_vars).busySendingKA==TRUE) {
        // don't proceed if I'm still sending a KA
        return;
    }

    kaNeighAddr = neighbors_getKANeighbor(self, (self->sixtop_vars).kaPeriod);
    if (kaNeighAddr==NULL) {
        // don't proceed if I have no neighbor I need to send a KA to
        return;
    }
    
    if ( schedule_hasDedicatedCellToNeighbor(self, kaNeighAddr) == FALSE){
        // delete packets genereted by this module (EB and KA) from openqueue
 openqueue_removeAllCreatedBy(self, COMPONENT_SIXTOP);
        
        // I'm not busy sending an EB or KA
        (self->sixtop_vars).busySendingEB = FALSE;
        (self->sixtop_vars).busySendingKA = FALSE;
        
        return;
    }

    //printf("I am node %d, I will send a Keep Alive\n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
    // if I get here, I will send a KA

    // get a free packet buffer
    kaPkt = openqueue_getFreePacketBuffer(self, COMPONENT_SIXTOP);
    if (kaPkt==NULL) {
 openserial_printError(self, 
            COMPONENT_SIXTOP,
            ERR_NO_FREE_PACKET_BUFFER,
            (errorparameter_t)1,
            (errorparameter_t)0
        );
        return;
   }
   
    // declare ownership over that packet
    kaPkt->creator = COMPONENT_SIXTOP;
    kaPkt->owner   = COMPONENT_SIXTOP;
    
    // some l2 information about this packet
    kaPkt->l2_frameType = IEEE154_TYPE_DATA;
    memcpy(&(kaPkt->l2_nextORpreviousHop),kaNeighAddr,sizeof(open_addr_t));
    
    // set l2-security attributes
    kaPkt->l2_securityLevel   = IEEE802154_SECURITY_LEVEL; // do not exchange KAs with 
    kaPkt->l2_keyIdMode       = IEEE802154_SECURITY_KEYIDMODE;
    kaPkt->l2_keyIndex        = IEEE802154_security_getDataKeyIndex(self);

    kaPkt->isSixtopFake=FALSE;

    // put in queue for MAC to handle
 sixtop_send_internal(self, kaPkt,FALSE);
    
    // I'm now busy sending a KA
    (self->sixtop_vars).busySendingKA = TRUE;

#ifdef OPENSIM
 debugpins_ka_set(self);
 debugpins_ka_clr(self);
#endif
}

//======= six2six task

void timer_sixtop_six2six_timeout_fired(OpenMote* self) {
  
    if ((self->sixtop_vars).six2six_state == SIX_STATE_WAIT_CLEARRESPONSE){
        // no response for the 6p clear, just clear locally
 schedule_removeAllCells(self, 
            (self->sixtop_vars).cb_sf_getMetadata(),
            &(self->sixtop_vars).neighborToClearCells
        );
 neighbors_resetSequenceNumber(self, &(self->sixtop_vars).neighborToClearCells);
        memset(&(self->sixtop_vars).neighborToClearCells,0,sizeof(open_addr_t));
    }
    // timeout timer fired, reset the state of sixtop to idle
    (self->sixtop_vars).six2six_state = SIX_STATE_IDLE;
 opentimers_cancel(self, (self->sixtop_vars).timeoutTimerId);
}

void sixtop_six2six_sendDone(OpenMote* self, OpenQueueEntry_t* msg, owerror_t error){

	//printf("I am node %d, SENDDONE \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);

    msg->owner = COMPONENT_SIXTOP_RES;

    // if this is a request send done
    if (msg->l2_sixtop_messageType == SIXTOP_CELL_REQUEST){
        if(error == E_FAIL) {
            // reset handler and state if the request is failed to send out
            (self->sixtop_vars).six2six_state = SIX_STATE_IDLE;
            //printf("I am NODE %d, REQUEST SEND DONE FAIL \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
        } else {
        	//printf("I am NODE %d, 6P REQUEST SEND DONE sent successfully \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
        	if (msg->isSixtopFake){
 whisper_trigger_nextStep(self);
        		//printf("I am NODE %d, FAKE 6P REQUEST SEND DONE sent successfully. NEXT STEP \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
//        		if ( whisper_getState(self)==1){
//        			printf("I am NODE %d, 6P FAKE send done sent successfully \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
// whisper_trigger_nextStep(self);
//        		}else{
//        			printf("I am NODE %d, second 6P FAKE send done sent successfully \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
//        		}
        	}
            // the packet has been sent out successfully
            switch ((self->sixtop_vars).six2six_state) {
            case SIX_STATE_WAIT_ADDREQUEST_SENDDONE:
                (self->sixtop_vars).six2six_state = SIX_STATE_WAIT_ADDRESPONSE;
                break;
            case SIX_STATE_WAIT_DELETEREQUEST_SENDDONE:
                (self->sixtop_vars).six2six_state = SIX_STATE_WAIT_DELETERESPONSE;
                break;
            case SIX_STATE_WAIT_RELOCATEREQUEST_SENDDONE:
                (self->sixtop_vars).six2six_state = SIX_STATE_WAIT_RELOCATERESPONSE;
                break;
            case SIX_STATE_WAIT_LISTREQUEST_SENDDONE:
                (self->sixtop_vars).six2six_state = SIX_STATE_WAIT_LISTRESPONSE;
                break;
            case SIX_STATE_WAIT_COUNTREQUEST_SENDDONE:
                (self->sixtop_vars).six2six_state = SIX_STATE_WAIT_COUNTRESPONSE;
                break;
            case SIX_STATE_WAIT_CLEARREQUEST_SENDDONE:
                (self->sixtop_vars).six2six_state = SIX_STATE_WAIT_CLEARRESPONSE;
                break;
            default:
                // should never happen
                break;
            }
            // start timeout timer if I am waiting for a response
 opentimers_scheduleIn(self, 
                (self->sixtop_vars).timeoutTimerId,
                SIX2SIX_TIMEOUT_MS,
                TIME_MS,
                TIMER_ONESHOT,
                sixtop_timeout_timer_cb
            );
        }
    }
    
    // if this is a response send done
    if (msg->l2_sixtop_messageType == SIXTOP_CELL_RESPONSE){
        if(error == E_SUCCESS) {
        	//printf("I am node %d, sendDone ACK to 6P RESPONSE packet!!!!!!!!!!!!!!!!!!!!!!!!!! \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
 neighbors_updateSequenceNumber(self, &(msg->l2_nextORpreviousHop));
            // in case a response is sent out, check the return code
            if (msg->l2_sixtop_returnCode == IANA_6TOP_RC_SUCCESS){
                if (msg->l2_sixtop_command == IANA_6TOP_CMD_ADD){
                	//printf("I am node %d, adding cells  from neigh ACK to ADD RESPONSE\n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
                	//printf("I am node %d, adding cells  METADATA is %d\n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],msg->l2_sixtop_frameID);
 sixtop_addCells(self, 
                        msg->l2_sixtop_frameID,
                        msg->l2_sixtop_celllist_add,
                        &(msg->l2_nextORpreviousHop),
                        msg->l2_sixtop_cellOptions
                    );
                }
                
                if (msg->l2_sixtop_command == IANA_6TOP_CMD_DELETE){
 sixtop_removeCells(self, 
                      msg->l2_sixtop_frameID,
                        msg->l2_sixtop_celllist_delete,
                      &(msg->l2_nextORpreviousHop),
                        msg->l2_sixtop_cellOptions
                    );
                }
              
                if ( msg->l2_sixtop_command == IANA_6TOP_CMD_RELOCATE){
 sixtop_removeCells(self, 
                        msg->l2_sixtop_frameID,
                        msg->l2_sixtop_celllist_delete,
                        &(msg->l2_nextORpreviousHop),
                        msg->l2_sixtop_cellOptions
                    );
 sixtop_addCells(self, 
                        msg->l2_sixtop_frameID,
                        msg->l2_sixtop_celllist_add,
                        &(msg->l2_nextORpreviousHop),
                        msg->l2_sixtop_cellOptions
                    );
                }
                
                if ( msg->l2_sixtop_command == IANA_6TOP_CMD_CLEAR){
                	//printf("I am NODE %d, receiving RESPONSE send done. DELETING ALL CELLS in my schedule\n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
 schedule_removeAllCells(self, 
                        msg->l2_sixtop_frameID, 
                        &(msg->l2_nextORpreviousHop)
                    );
                    // neighbors_resetSequenceNumber(self, &(msg->l2_nextORpreviousHop));
                }
            } else {
            	//printf("I am node %d, sendDone ACK to 6P RESPONSE packet is FAIL! \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
                // the return code doesn't end up with SUCCESS
                // The return code will be processed on request side.
            }
        } else {
            // doesn't receive the ACK of response packet from request side after maximum retries.
        }
    }
    // free the buffer
 openqueue_freePacketBuffer(self, msg);
}

port_INLINE bool sixtop_processIEs(OpenMote* self, OpenQueueEntry_t* pkt, uint16_t * lenIE) {
    uint8_t ptr;
    uint8_t temp_8b;
    uint8_t subtypeid,code,sfid,version,type,seqNum;
    uint16_t temp_16b,len,headerlen;
   
    ptr         = 0;
    headerlen   = 0;
  
    //candidate IE header  if type ==0 header IE if type==1 payload IE
    temp_8b     = *((uint8_t*)(pkt->payload)+ptr);
    ptr++;
    temp_16b    = temp_8b + ((*((uint8_t*)(pkt->payload)+ptr))<<8);
    ptr++;
    *lenIE += 2;
    // check ietf ie group id, type
    if ((temp_16b & IEEE802154E_DESC_LEN_PAYLOAD_ID_TYPE_MASK) != (IANA_IETF_IE_GROUP_ID | IANA_IETF_IE_TYPE)){
        // wrong IE ID or type, record and drop the packet
 openserial_printError(self, COMPONENT_SIXTOP,ERR_UNSUPPORTED_FORMAT,0,0);
        return FALSE;
    }
    len = temp_16b & IEEE802154E_DESC_LEN_PAYLOAD_IE_MASK;
    *lenIE += len;
  
    // check 6p subtype Id
    subtypeid  = *((uint8_t*)(pkt->payload)+ptr);
    ptr += 1;
    if (subtypeid != IANA_6TOP_SUBIE_ID){
        // wrong subtypeID, record and drop the packet
 openserial_printError(self, COMPONENT_SIXTOP,ERR_UNSUPPORTED_FORMAT,1,0);
        return FALSE;
    }
    headerlen += 1;
    
    // check 6p version
    temp_8b = *((uint8_t*)(pkt->payload)+ptr);
    ptr += 1;
    // 6p doesn't define type 3
    if (temp_8b>>IANA_6TOP_TYPE_SHIFT == 3){
        // wrong type, record and drop the packet
 openserial_printError(self, COMPONENT_SIXTOP,ERR_UNSUPPORTED_FORMAT,2,0);
        return FALSE;
    }
    version    = temp_8b &  IANA_6TOP_VESION_MASK;
    type       = temp_8b >> IANA_6TOP_TYPE_SHIFT;
    headerlen += 1;
    
    // get 6p code
    code       = *((uint8_t*)(pkt->payload)+ptr);
    ptr       += 1;
    headerlen += 1;
    // get 6p sfid
    sfid       = *((uint8_t*)(pkt->payload)+ptr);
    ptr       += 1;
    headerlen += 1;
    // get 6p seqNum and GEN
    seqNum     =  *((uint8_t*)(pkt->payload)+ptr) & 0xff;
    ptr       += 1;
    headerlen += 1;
    
    // give six2six to process
 sixtop_six2six_notifyReceive(self, version,type,code,sfid,seqNum,ptr,len-headerlen,pkt);
    *lenIE     = len+2;
    return TRUE;
}

void sixtop_six2six_notifyReceive(OpenMote* self, 
    uint8_t           version, 
    uint8_t           type,
    uint8_t           code, 
    uint8_t           sfId,
    uint8_t           seqNum,
    uint8_t           ptr,
    uint8_t           length,
    OpenQueueEntry_t* pkt
){
    uint8_t           returnCode;
    uint16_t          metadata;
    uint8_t           cellOptions;
    uint8_t           cellOptions_transformed;
    uint16_t          offset;
    uint16_t          length_groupid_type;
    uint16_t          startingOffset;
    uint8_t           maxNumCells;
    uint16_t          i;
    uint16_t          slotoffset;
    uint16_t          channeloffset;
    uint16_t          numCells;
    uint16_t          temp16;
    OpenQueueEntry_t* response_pkt;
    uint8_t           pktLen            = length;
    uint8_t           response_pktLen   = 0;
    cellInfo_ht       celllist_list[CELLLIST_MAX_LEN];
    


    if (type == SIXTOP_CELL_REQUEST){
        // if this is a 6p request message
    	//printf("I am node %d, received REQUEST type %d \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],code);
        //printf("I am NODE %d, Received a 6P Request from %d \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],pkt->l2_nextORpreviousHop->addr_64b[7]);

//    	uint8_t          i;
//		open_addr_t      NeighborAddress;
//		for (i=0;i<MAXNUMNEIGHBORS;i++) {
//			if ( neighbors_getNeighborEui64(self, &NeighborAddress, ADDR_64B, i)) { // this neighbor entry is in use
//				if ( packetfunctions_sameAddress(self, &NeighborAddress,&(pkt->l2_nextORpreviousHop))){
//						//copying l3 64b address to l2
//						//printf("I am node %d, receiving a 6P REQUEST from nieghbor %d \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],NeighborAddress.addr_64b[7]);
//
//					   // packetfunctions_ip128bToMac64b(self, &(NeighborAddress.addr_64b),&temp_src_prefix,&temp_src_mac64b);
//					   //printf("IEEE My fake MAC source address is %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x \n", NeighborAddress.addr_64b[0],NeighborAddress.addr_64b[1],NeighborAddress.addr_64b[2],NeighborAddress.addr_64b[3],NeighborAddress.addr_64b[4],NeighborAddress.addr_64b[5],NeighborAddress.addr_64b[6],NeighborAddress.addr_64b[7]);
//					   break;
//				}
//			}
//		}

        // get a free packet buffer
        response_pkt = openqueue_getFreePacketBuffer(self, COMPONENT_SIXTOP_RES);
        if (response_pkt==NULL) {
 openserial_printError(self, 
                COMPONENT_SIXTOP_RES,
                ERR_NO_FREE_PACKET_BUFFER,
                (errorparameter_t)0,
                (errorparameter_t)0
            );
            return;
        }
   
        // take ownership
        response_pkt->creator = COMPONENT_SIXTOP_RES;
        response_pkt->owner   = COMPONENT_SIXTOP_RES;
        
        memcpy(&(response_pkt->l2_nextORpreviousHop),
               &(pkt->l2_nextORpreviousHop),
               sizeof(open_addr_t)
        );
    
        // the follow while loop only execute once
        do{
            // version check
            if (version != IANA_6TOP_6P_VERSION){
                returnCode = IANA_6TOP_RC_VER_ERR;
                break;
            }
            // sfid check
            if (sfId != (self->sixtop_vars).cb_sf_getsfid()){
                returnCode = IANA_6TOP_RC_SFID_ERR;
                break;
            }

            //printf("I am node %d, checking seq num. My value is %d, the incoming seqNum is %d\n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7], neighbors_getSequenceNumber(self, &(pkt->l2_nextORpreviousHop)),seqNum);
//            if (code == IANA_6TOP_CMD_CLEAR){
//                printf("I am node %d, However this is command clear, so it's ok\n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
//
//            }

            // sequenceNumber check
            if (seqNum != neighbors_getSequenceNumber(self, &(pkt->l2_nextORpreviousHop)) && code != IANA_6TOP_CMD_CLEAR){
                //printf("I am node %d, Received a 6P packet with seq num %d \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],seqNum);
                //printf("I am node %d, I was expecting seq num %d. Sending ERROR \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7], neighbors_getSequenceNumber(self, &(pkt->l2_nextORpreviousHop)));
                returnCode = IANA_6TOP_RC_SEQNUM_ERR;
                break;
            }
            // previous 6p transcation check
            if ((self->sixtop_vars).six2six_state != SIX_STATE_IDLE){
                returnCode = IANA_6TOP_RC_RESET;
                break;
            }
            // metadata meaning check
            if ((self->sixtop_vars).cb_sf_translateMetadata()!=METADATA_TYPE_FRAMEID){
 openserial_printError(self, 
                    COMPONENT_SIXTOP,
                    ERR_UNSUPPORTED_METADATA,
                    (self->sixtop_vars).cb_sf_translateMetadata(),
                    0
                );
                returnCode = IANA_6TOP_RC_ERROR;
                break;
            }
            
            // commands check
            
            // get metadata, metadata indicates frame id 
            metadata  = *((uint8_t*)(pkt->payload)+ptr);
            metadata |= *((uint8_t*)(pkt->payload)+ptr+1)<<8;
            ptr      += 2;
            pktLen   -= 2;
            
            // clear command
            if (code == IANA_6TOP_CMD_CLEAR){
                // the cells will be removed when the repsonse sendone successfully
                // don't clear cells here
                returnCode = IANA_6TOP_RC_SUCCESS;

                //printf("I am node %d, RECEIVED CLEAR 6P message, rc = OK! \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);

                break;
            }
            
            cellOptions  = *((uint8_t*)(pkt->payload)+ptr);
            ptr         += 1;
            pktLen      -= 1;
            
            // list command
            if (code == IANA_6TOP_CMD_LIST){
                ptr += 1; // skip the one byte reserved field
                offset  = *((uint8_t*)(pkt->payload)+ptr);
                offset |= *((uint8_t*)(pkt->payload)+ptr+1)<<8;
                ptr += 2;
                maxNumCells  = *((uint8_t*)(pkt->payload)+ptr);
                maxNumCells |= *((uint8_t*)(pkt->payload)+ptr+1)<<8;
                ptr += 2;
                
                returnCode = IANA_6TOP_RC_SUCCESS;
                startingOffset = offset;
                if ((cellOptions & (CELLOPTIONS_TX | CELLOPTIONS_RX)) != (CELLOPTIONS_TX | CELLOPTIONS_RX)){
                    cellOptions_transformed = cellOptions ^ (CELLOPTIONS_TX | CELLOPTIONS_RX);
                } else {
                    cellOptions_transformed = cellOptions;
                }
                for(i=0; i<maxNumCells; i++) {
                    if (
 schedule_getOneCellAfterOffset(self, 
                            metadata,
                            startingOffset,
                            &(pkt->l2_nextORpreviousHop),
                            cellOptions_transformed,
                            &slotoffset,
                            &channeloffset)
                    ){
                        // found one cell after slot offset+i
 packetfunctions_reserveHeaderSize(self, response_pkt,4); 
                        response_pkt->payload[0] = slotoffset     & 0x00FF;
                        response_pkt->payload[1] = (slotoffset    & 0xFF00)>>8;
                        response_pkt->payload[2] = channeloffset  & 0x00FF;
                        response_pkt->payload[3] = (channeloffset & 0xFF00)>>8;
                        response_pktLen         += 4;
                        startingOffset           = slotoffset+1;
                    } else {
                        // no more cell after offset
                        returnCode = IANA_6TOP_RC_EOL;
                        break;
                    }
                }
                if (
 schedule_getOneCellAfterOffset(self, 
                        metadata,
                        startingOffset,
                        &(pkt->l2_nextORpreviousHop),
                        cellOptions_transformed,
                        &slotoffset,
                        &channeloffset) == FALSE
                ){
                    returnCode = IANA_6TOP_RC_EOL;
                }
                
                break;
            }
            
            // count command
            if (code == IANA_6TOP_CMD_COUNT){
                numCells = 0;
                startingOffset = 0;
                if ((cellOptions & (CELLOPTIONS_TX | CELLOPTIONS_RX)) != (CELLOPTIONS_TX | CELLOPTIONS_RX)){
                    cellOptions_transformed = cellOptions ^ (CELLOPTIONS_TX | CELLOPTIONS_RX);
                } else {
                    cellOptions_transformed = cellOptions;
                }
                for(i=0; i< schedule_getFrameLength(self); i++) {
                    if (
 schedule_getOneCellAfterOffset(self, 
                            metadata,
                            startingOffset,
                            &(pkt->l2_nextORpreviousHop),
                            cellOptions_transformed,
                            &slotoffset,
                            &channeloffset)
                    ){
                        // found one cell after slot i
                        numCells++;
                        startingOffset = slotoffset+1;
                    }
                }
                returnCode = IANA_6TOP_RC_SUCCESS;
 packetfunctions_reserveHeaderSize(self, response_pkt,sizeof(uint16_t));
                response_pkt->payload[0] =  numCells & 0x00FF;
                response_pkt->payload[1] = (numCells & 0xFF00)>>8;
                response_pktLen         += 2;
                break;
            }
            
            numCells = *((uint8_t*)(pkt->payload)+ptr);
            ptr     += 1;
            pktLen  -= 1;
            
            // add command
            if (code == IANA_6TOP_CMD_ADD){
                if ( schedule_getNumberOfFreeEntries(self) < numCells){
                    returnCode = IANA_6TOP_RC_NORES;
                    break;
                }
                // retrieve cell list
                i = 0;
                memset(response_pkt->l2_sixtop_celllist_add,0,sizeof(response_pkt->l2_sixtop_celllist_add));
                while(pktLen>0){
                    response_pkt->l2_sixtop_celllist_add[i].slotoffset     =  *((uint8_t*)(pkt->payload)+ptr);
                    response_pkt->l2_sixtop_celllist_add[i].slotoffset    |= (*((uint8_t*)(pkt->payload)+ptr+1))<<8;
                    response_pkt->l2_sixtop_celllist_add[i].channeloffset  =  *((uint8_t*)(pkt->payload)+ptr+2);
                    response_pkt->l2_sixtop_celllist_add[i].channeloffset |= (*((uint8_t*)(pkt->payload)+ptr+3))<<8;
                    response_pkt->l2_sixtop_celllist_add[i].isUsed         = TRUE;
                    ptr    += 4;
                    pktLen -= 4;
                    i++;
                }
                if ( sixtop_areAvailableCellsToBeScheduled(self, metadata,numCells,response_pkt->l2_sixtop_celllist_add)){

                    for(i=0;i<CELLLIST_MAX_LEN;i++) {
                        if(response_pkt->l2_sixtop_celllist_add[i].isUsed){
 packetfunctions_reserveHeaderSize(self, response_pkt,4); 
                            response_pkt->payload[0] = (uint8_t)(response_pkt->l2_sixtop_celllist_add[i].slotoffset         & 0x00FF);
                            response_pkt->payload[1] = (uint8_t)((response_pkt->l2_sixtop_celllist_add[i].slotoffset        & 0xFF00)>>8);
                            response_pkt->payload[2] = (uint8_t)(response_pkt->l2_sixtop_celllist_add[i].channeloffset      & 0x00FF);
                            response_pkt->payload[3] = (uint8_t)((response_pkt->l2_sixtop_celllist_add[i].channeloffset     & 0xFF00)>>8);
                            response_pktLen += 4;
                        }
                    }
                }
                returnCode = IANA_6TOP_RC_SUCCESS;
                break;
            }
            
            // delete command
            if (code == IANA_6TOP_CMD_DELETE){
                i = 0;
                memset(response_pkt->l2_sixtop_celllist_delete,0,sizeof(response_pkt->l2_sixtop_celllist_delete));
                while(pktLen>0){
                    response_pkt->l2_sixtop_celllist_delete[i].slotoffset     =  *((uint8_t*)(pkt->payload)+ptr);
                    response_pkt->l2_sixtop_celllist_delete[i].slotoffset    |= (*((uint8_t*)(pkt->payload)+ptr+1))<<8;
                    response_pkt->l2_sixtop_celllist_delete[i].channeloffset  =  *((uint8_t*)(pkt->payload)+ptr+2);
                    response_pkt->l2_sixtop_celllist_delete[i].channeloffset |= (*((uint8_t*)(pkt->payload)+ptr+3))<<8;
                    response_pkt->l2_sixtop_celllist_delete[i].isUsed         = TRUE;
                    ptr    += 4;
                    pktLen -= 4;
                    i++;
                }
                if ((cellOptions & (CELLOPTIONS_TX | CELLOPTIONS_RX)) != (CELLOPTIONS_TX | CELLOPTIONS_RX)){
                    cellOptions_transformed = cellOptions ^ (CELLOPTIONS_TX | CELLOPTIONS_RX);
                } else {
                    cellOptions_transformed = cellOptions;
                }
                if ( sixtop_areAvailableCellsToBeRemoved(self, metadata,numCells,response_pkt->l2_sixtop_celllist_delete,&(pkt->l2_nextORpreviousHop),cellOptions_transformed)){
                    returnCode = IANA_6TOP_RC_SUCCESS;
                    for(i=0;i<CELLLIST_MAX_LEN;i++) {
                        if(response_pkt->l2_sixtop_celllist_delete[i].isUsed){
 packetfunctions_reserveHeaderSize(self, response_pkt,4); 
                            response_pkt->payload[0] = (uint8_t)(response_pkt->l2_sixtop_celllist_delete[i].slotoffset         & 0x00FF);
                            response_pkt->payload[1] = (uint8_t)((response_pkt->l2_sixtop_celllist_delete[i].slotoffset        & 0xFF00)>>8);
                            response_pkt->payload[2] = (uint8_t)(response_pkt->l2_sixtop_celllist_delete[i].channeloffset      & 0x00FF);
                            response_pkt->payload[3] = (uint8_t)((response_pkt->l2_sixtop_celllist_delete[i].channeloffset     & 0xFF00)>>8);
                            response_pktLen += 4;
                        }
                    }
                } else {
                    returnCode = IANA_6TOP_RC_CELLLIST_ERR;
                }
                    break;
            }
            
            // relocate command
            if (code == IANA_6TOP_CMD_RELOCATE){
                // retrieve cell list to be relocated
                i = 0;
                memset(response_pkt->l2_sixtop_celllist_delete,0,sizeof(response_pkt->l2_sixtop_celllist_delete));
                temp16 = numCells;
                while(temp16>0){
                    response_pkt->l2_sixtop_celllist_delete[i].slotoffset     =  *((uint8_t*)(pkt->payload)+ptr);
                    response_pkt->l2_sixtop_celllist_delete[i].slotoffset    |= (*((uint8_t*)(pkt->payload)+ptr+1))<<8;
                    response_pkt->l2_sixtop_celllist_delete[i].channeloffset  =  *((uint8_t*)(pkt->payload)+ptr+2);
                    response_pkt->l2_sixtop_celllist_delete[i].channeloffset |= (*((uint8_t*)(pkt->payload)+ptr+3))<<8;
                    response_pkt->l2_sixtop_celllist_delete[i].isUsed         = TRUE;
                    ptr    += 4;
                    pktLen -= 4;
                    temp16--;
                    i++;
                }
                if ((cellOptions & (CELLOPTIONS_TX | CELLOPTIONS_RX)) != (CELLOPTIONS_TX | CELLOPTIONS_RX)){
                    cellOptions_transformed = cellOptions ^ (CELLOPTIONS_TX | CELLOPTIONS_RX);
                } else {
                    cellOptions_transformed = cellOptions;
                }
                if ( sixtop_areAvailableCellsToBeRemoved(self, metadata,numCells,response_pkt->l2_sixtop_celllist_delete,&(pkt->l2_nextORpreviousHop),cellOptions_transformed)==FALSE){
                    returnCode = IANA_6TOP_RC_CELLLIST_ERR;
                    break;
                }
                // retrieve cell list to be relocated
                i = 0;
                memset(response_pkt->l2_sixtop_celllist_add,0,sizeof(response_pkt->l2_sixtop_celllist_add));
                while(pktLen>0){
                    response_pkt->l2_sixtop_celllist_add[i].slotoffset     =  *((uint8_t*)(pkt->payload)+ptr);
                    response_pkt->l2_sixtop_celllist_add[i].slotoffset    |= (*((uint8_t*)(pkt->payload)+ptr+1))<<8;
                    response_pkt->l2_sixtop_celllist_add[i].channeloffset  =  *((uint8_t*)(pkt->payload)+ptr+2);
                    response_pkt->l2_sixtop_celllist_add[i].channeloffset |= (*((uint8_t*)(pkt->payload)+ptr+3))<<8;
                    response_pkt->l2_sixtop_celllist_add[i].isUsed         = TRUE;
                    ptr    += 4;
                    pktLen -= 4;
                    i++;
                }
                if ( sixtop_areAvailableCellsToBeScheduled(self, metadata,numCells,response_pkt->l2_sixtop_celllist_add)){
                    for(i=0;i<CELLLIST_MAX_LEN;i++) {
                        if(response_pkt->l2_sixtop_celllist_add[i].isUsed){
 packetfunctions_reserveHeaderSize(self, response_pkt,4); 
                            response_pkt->payload[0] = (uint8_t)(response_pkt->l2_sixtop_celllist_add[i].slotoffset         & 0x00FF);
                            response_pkt->payload[1] = (uint8_t)((response_pkt->l2_sixtop_celllist_add[i].slotoffset        & 0xFF00)>>8);
                            response_pkt->payload[2] = (uint8_t)(response_pkt->l2_sixtop_celllist_add[i].channeloffset      & 0x00FF);
                            response_pkt->payload[3] = (uint8_t)((response_pkt->l2_sixtop_celllist_add[i].channeloffset     & 0xFF00)>>8);
                            response_pktLen += 4;
                        }
                    }
                }
                returnCode = IANA_6TOP_RC_SUCCESS;
                break;
            }
        } while(0);
        
        // record code, returnCode, frameID and cellOptions. They will be used when 6p repsonse senddone
        response_pkt->l2_sixtop_command     = code;
        response_pkt->l2_sixtop_returnCode  = returnCode;
        response_pkt->l2_sixtop_frameID     = metadata;
        // revert tx and rx link option bits
        if ((cellOptions & (CELLOPTIONS_TX | CELLOPTIONS_RX)) != (CELLOPTIONS_TX | CELLOPTIONS_RX)){
            response_pkt->l2_sixtop_cellOptions = cellOptions ^ (CELLOPTIONS_TX | CELLOPTIONS_RX);
        } else {
            response_pkt->l2_sixtop_cellOptions = cellOptions;
        }
        
        // append 6p Seqnum
 packetfunctions_reserveHeaderSize(self, response_pkt,sizeof(uint8_t));
        *((uint8_t*)(response_pkt->payload)) = seqNum;
        response_pktLen += 1;
        
        // append 6p sfid
 packetfunctions_reserveHeaderSize(self, response_pkt,sizeof(uint8_t));
        *((uint8_t*)(response_pkt->payload)) = (self->sixtop_vars).cb_sf_getsfid();
        response_pktLen += 1;
          
        // append 6p code
 packetfunctions_reserveHeaderSize(self, response_pkt,sizeof(uint8_t));
        *((uint8_t*)(response_pkt->payload)) = returnCode;
        response_pktLen += 1;
        
        // append 6p version, T(type) and  R(reserved)
 packetfunctions_reserveHeaderSize(self, response_pkt,sizeof(uint8_t));
        *((uint8_t*)(response_pkt->payload)) = IANA_6TOP_6P_VERSION | IANA_6TOP_TYPE_RESPONSE;
        response_pktLen += 1;
        
        // append 6p subtype id
 packetfunctions_reserveHeaderSize(self, response_pkt,sizeof(uint8_t));
        *((uint8_t*)(response_pkt->payload)) = IANA_6TOP_SUBIE_ID;
        response_pktLen += 1;
       
        // append IETF IE header (length_groupid_type)
 packetfunctions_reserveHeaderSize(self, response_pkt, sizeof(uint16_t));
        length_groupid_type  = response_pktLen;
        length_groupid_type |= (IANA_IETF_IE_GROUP_ID  | IANA_IETF_IE_TYPE); 
        response_pkt->payload[0]      = length_groupid_type        & 0xFF;
        response_pkt->payload[1]      = (length_groupid_type >> 8) & 0xFF;
       
        // indicate IEs present
        response_pkt->l2_payloadIEpresent = TRUE;
        // record this packet as sixtop request message
        response_pkt->l2_sixtop_messageType    = SIXTOP_CELL_RESPONSE;

        if ((self->sixtop_vars).isResponseEnabled){
            // send packet
        	//printf("I am NODE %d, Enqueueing response type \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],returnCode);
 sixtop_send(self, response_pkt);
        } else {
 openqueue_freePacketBuffer(self, response_pkt);
        }
    }
      
    if (type == SIXTOP_CELL_RESPONSE) {

    	//printf("I am node %d, received REQUEST type %d \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],code);
        //printf("I am NODE %d, Received a 6P Response from %d  \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],pkt->l2_nextORpreviousHop->addr_64b[7]);
//    	uint8_t          i;
//		open_addr_t      NeighborAddress;
//		for (i=0;i<MAXNUMNEIGHBORS;i++) {
//			if ( neighbors_getNeighborEui64(self, &NeighborAddress, ADDR_64B, i)) { // this neighbor entry is in use
//				if ( packetfunctions_sameAddress(self, &NeighborAddress,&(pkt->l2_nextORpreviousHop))){
//						//copying l3 64b address to l2
//						//printf("I am node %d, receiving a 6P RESPONSE from nieghbor %d \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],NeighborAddress.addr_64b[7]);
//
//					   // packetfunctions_ip128bToMac64b(self, &(NeighborAddress.addr_64b),&temp_src_prefix,&temp_src_mac64b);
//					   //printf("IEEE My fake MAC source address is %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x \n", NeighborAddress.addr_64b[0],NeighborAddress.addr_64b[1],NeighborAddress.addr_64b[2],NeighborAddress.addr_64b[3],NeighborAddress.addr_64b[4],NeighborAddress.addr_64b[5],NeighborAddress.addr_64b[6],NeighborAddress.addr_64b[7]);
//					   break;
//				}
//			}
//		}

        // this is a 6p response message
 neighbors_updateSequenceNumber(self, &(pkt->l2_nextORpreviousHop));
        
        // if the code is SUCCESS
        if (code == IANA_6TOP_RC_SUCCESS || code == IANA_6TOP_RC_EOL){
            switch((self->sixtop_vars).six2six_state){
            case SIX_STATE_WAIT_ADDRESPONSE:
                i = 0;
                memset(pkt->l2_sixtop_celllist_add,0,sizeof(pkt->l2_sixtop_celllist_add));
                while(pktLen>0){
                    pkt->l2_sixtop_celllist_add[i].slotoffset     =  *((uint8_t*)(pkt->payload)+ptr);
                    pkt->l2_sixtop_celllist_add[i].slotoffset    |= (*((uint8_t*)(pkt->payload)+ptr+1))<<8;
                    pkt->l2_sixtop_celllist_add[i].channeloffset  =  *((uint8_t*)(pkt->payload)+ptr+2);
                    pkt->l2_sixtop_celllist_add[i].channeloffset |= (*((uint8_t*)(pkt->payload)+ptr+3))<<8;
                    pkt->l2_sixtop_celllist_add[i].isUsed         = TRUE;
                    ptr    += 4;
                    pktLen -= 4;
                    i++;
                }
 sixtop_addCells(self, 
                    (self->sixtop_vars).cb_sf_getMetadata(),     // frame id 
                    pkt->l2_sixtop_celllist_add,  // celllist to be added
                    &(pkt->l2_nextORpreviousHop), // neighbor that cells to be added to
                    (self->sixtop_vars).cellOptions       // cell options
                );
                break;
            case SIX_STATE_WAIT_DELETERESPONSE:
 sixtop_removeCells(self, 
                    (self->sixtop_vars).cb_sf_getMetadata(),
                    (self->sixtop_vars).celllist_toDelete,
                    &(pkt->l2_nextORpreviousHop),
                    (self->sixtop_vars).cellOptions
                );
                break;
            case SIX_STATE_WAIT_RELOCATERESPONSE:
                i = 0;
                memset(pkt->l2_sixtop_celllist_add,0,sizeof(pkt->l2_sixtop_celllist_add));
                while(pktLen>0){
                    pkt->l2_sixtop_celllist_add[i].slotoffset     =  *((uint8_t*)(pkt->payload)+ptr);
                    pkt->l2_sixtop_celllist_add[i].slotoffset    |= (*((uint8_t*)(pkt->payload)+ptr+1))<<8;
                    pkt->l2_sixtop_celllist_add[i].channeloffset  =  *((uint8_t*)(pkt->payload)+ptr+2);
                    pkt->l2_sixtop_celllist_add[i].channeloffset |= (*((uint8_t*)(pkt->payload)+ptr+3))<<8;
                    pkt->l2_sixtop_celllist_add[i].isUsed         = TRUE;
                    ptr    += 4;
                    pktLen -= 4;
                    i++;
                }
 sixtop_removeCells(self, 
                    (self->sixtop_vars).cb_sf_getMetadata(),
                    (self->sixtop_vars).celllist_toDelete,
                    &(pkt->l2_nextORpreviousHop),
                    (self->sixtop_vars).cellOptions
                );
 sixtop_addCells(self, 
                    (self->sixtop_vars).cb_sf_getMetadata(),     // frame id 
                    pkt->l2_sixtop_celllist_add,  // celllist to be added
                    &(pkt->l2_nextORpreviousHop), // neighbor that cells to be added to
                    (self->sixtop_vars).cellOptions       // cell options
                );
                break;
            case SIX_STATE_WAIT_COUNTRESPONSE:
                numCells  = *((uint8_t*)(pkt->payload)+ptr);
                numCells |= (*((uint8_t*)(pkt->payload)+ptr+1))<<8;
                ptr += 2;
 openserial_printInfo(self, 
                    COMPONENT_SIXTOP,
                    ERR_SIXTOP_COUNT,
                    (errorparameter_t)numCells,
                    (errorparameter_t)(self->sixtop_vars).six2six_state
                );
                break;
            case SIX_STATE_WAIT_LISTRESPONSE:
                i = 0;
                memset(celllist_list,0,CELLLIST_MAX_LEN*sizeof(cellInfo_ht));
                while(pktLen>0){
                    celllist_list[i].slotoffset     =  *((uint8_t*)(pkt->payload)+ptr);
                    celllist_list[i].slotoffset    |= (*((uint8_t*)(pkt->payload)+ptr+1))<<8;
                    celllist_list[i].channeloffset  =  *((uint8_t*)(pkt->payload)+ptr+2);
                    celllist_list[i].channeloffset |= (*((uint8_t*)(pkt->payload)+ptr+3))<<8;
                    celllist_list[i].isUsed         = TRUE;
                    ptr    += 4;
                    pktLen -= 4;
                    i++;
                }
                // print out first two cells in the list
 openserial_printInfo(self, 
                    COMPONENT_SIXTOP,
                    ERR_SIXTOP_LIST,
                    (errorparameter_t)celllist_list[0].slotoffset,
                    (errorparameter_t)celllist_list[1].slotoffset
                );
                break;
            case SIX_STATE_WAIT_CLEARRESPONSE:
 schedule_removeAllCells(self, 
                    (self->sixtop_vars).cb_sf_getMetadata(),
                    &(pkt->l2_nextORpreviousHop)
                );
 neighbors_resetSequenceNumber(self, &(pkt->l2_nextORpreviousHop));
                break;
            default:
                // should neven happen
            	//printf("I am node %d, received 6P RESPONSE packet but I was not expecting it!!!!!!!!!!!!!!!!!!!!!!!!!! \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);

            	if( ( idmanager_getMyID(self, ADDR_64B)->addr_64b[7] == 0x99) && ( whisper_getState(self)==6) ){
                	//printf("I am node %d, Removig dedicated cell with old parent \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
 whisper_setState(self, 0);
//                    if ( schedule_removeActiveSlot(self, 5,&(pkt->l2_nextORpreviousHop))){
//                    	printf("I am node %d, Removig dedicated cell with old parent. SUCCESS \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
//                    }else{
//                    	printf("I am node %d, Removig dedicated cell with old parent ERROR \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
//                    }
            	}

                break;
            }
        } else {
            (self->sixtop_vars).cb_sf_handleRCError(code, &(pkt->l2_nextORpreviousHop));
        }
 openserial_printInfo(self, 
            COMPONENT_SIXTOP,
            ERR_SIXTOP_RETURNCODE,
            (errorparameter_t)code,
            (errorparameter_t)(self->sixtop_vars).six2six_state
        );
        memset(&(self->sixtop_vars).neighborToClearCells,0,sizeof(open_addr_t));
        (self->sixtop_vars).six2six_state   = SIX_STATE_IDLE;
 opentimers_cancel(self, (self->sixtop_vars).timeoutTimerId);
    }
}

//======= helper functions

bool sixtop_addCells(OpenMote* self, 
    uint8_t      slotframeID,
    cellInfo_ht* cellList,
    open_addr_t* previousHop,
    uint8_t      cellOptions
){
    uint8_t     i;
    bool        isShared;
    open_addr_t temp_neighbor;
    cellType_t  type;
    bool        hasCellsAdded;
   
    // translate cellOptions to cell type 
    if (cellOptions == CELLOPTIONS_TX){
        type     = CELLTYPE_TX;
        isShared = FALSE;
    }
    if (cellOptions == CELLOPTIONS_RX){
        type     = CELLTYPE_RX;  
        isShared = FALSE;
    }
    if (cellOptions == (CELLOPTIONS_TX | CELLOPTIONS_RX | CELLOPTIONS_SHARED)){
        type     = CELLTYPE_TXRX;  
        isShared = TRUE;
    }
   
    memcpy(&temp_neighbor,previousHop,sizeof(open_addr_t));

    hasCellsAdded = FALSE;
    // add cells to schedule
    for(i = 0;i<CELLLIST_MAX_LEN;i++){
        if (cellList[i].isUsed){
            hasCellsAdded = TRUE;
			//printf("I am node %d, adding CELL at offset  %d \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],cellList[i].slotoffset);



/*            //6b - 61*/
/*            if( ( idmanager_getMyID(self, ADDR_64B)->addr_64b[7] == 0x6b) && (temp_neighbor.addr_64b[7]==0x61) ){*/
/* schedule_addActiveSlot(self, */
/*                    11,*/
/*                    type,*/
/*                    isShared,*/
/*					0x0A,*/
/*                    &temp_neighbor*/
/*                );*/
/*            }*/

/*            if( ( idmanager_getMyID(self, ADDR_64B)->addr_64b[7] == 0x61) && (temp_neighbor.addr_64b[7]==0x6b) ){*/
/* schedule_addActiveSlot(self, */
/*                    11,*/
/*                    type,*/
/*                    isShared,*/
/*					0x0A,*/
/*                    &temp_neighbor*/
/*                );*/
/*            }*/


/*            //6b - 2b*/
/*            if( ( idmanager_getMyID(self, ADDR_64B)->addr_64b[7] == 0x6b) && (temp_neighbor.addr_64b[7]==0x2b) ){*/
/* schedule_addActiveSlot(self, */
/*                    12,*/
/*                    type,*/
/*                    isShared,*/
/*					0x0A,*/
/*                    &temp_neighbor*/
/*                );*/
/*            }*/

/*            if( ( idmanager_getMyID(self, ADDR_64B)->addr_64b[7] == 0x2b) && (temp_neighbor.addr_64b[7]==0x6b) ){*/
/* schedule_addActiveSlot(self, */
/*                    12,*/
/*                    type,*/
/*                    isShared,*/
/*					0x0A,*/
/*                    &temp_neighbor*/
/*                );*/
/*            }*/


/*            //61 - 67*/
/*            if( ( idmanager_getMyID(self, ADDR_64B)->addr_64b[7] == 0x61) && (temp_neighbor.addr_64b[7]==0x67) ){*/
/* schedule_addActiveSlot(self, */
/*                    21,*/
/*                    type,*/
/*                    isShared,*/
/*					0x0A,*/
/*                    &temp_neighbor*/
/*                );*/
/*            }*/

/*            if( ( idmanager_getMyID(self, ADDR_64B)->addr_64b[7] == 0x67) && (temp_neighbor.addr_64b[7]==0x61) ){*/
/* schedule_addActiveSlot(self, */
/*                    21,*/
/*                    type,*/
/*                    isShared,*/
/*					0x0A,*/
/*                    &temp_neighbor*/
/*                );*/
/*            }*/



/*            //2b - 07*/
/*            if( ( idmanager_getMyID(self, ADDR_64B)->addr_64b[7] == 0x2b) && (temp_neighbor.addr_64b[7]==0x07) ){*/
/* schedule_addActiveSlot(self, */
/*                    22,*/
/*                    type,*/
/*                    isShared,*/
/*					0x0A,*/
/*                    &temp_neighbor*/
/*                );*/
/*            }*/

/*            if( ( idmanager_getMyID(self, ADDR_64B)->addr_64b[7] == 0x07) && (temp_neighbor.addr_64b[7]==0x2b) ){*/
/* schedule_addActiveSlot(self, */
/*                    22,*/
/*                    type,*/
/*                    isShared,*/
/*					0x0A,*/
/*                    &temp_neighbor*/
/*                );*/
/*            }*/

/*            // 67 - 55*/
/*            if( ( idmanager_getMyID(self, ADDR_64B)->addr_64b[7] == 0x67) && (temp_neighbor.addr_64b[7]==0x55) ){*/
/* schedule_addActiveSlot(self, */
/*                    18,*/
/*                    type,*/
/*                    isShared,*/
/*					0x0A,*/
/*                    &temp_neighbor*/
/*                );*/
/*            }*/

/*            if( ( idmanager_getMyID(self, ADDR_64B)->addr_64b[7] == 0x55) && (temp_neighbor.addr_64b[7]==0x67) ){*/
/* schedule_addActiveSlot(self, */
/*                    18,*/
/*                    type,*/
/*                    isShared,*/
/*					0x0A,*/
/*                    &temp_neighbor*/
/*                );*/
/*            }*/

/*            // 07 - ac*/
/*            if( ( idmanager_getMyID(self, ADDR_64B)->addr_64b[7] == 0x07) && (temp_neighbor.addr_64b[7]==0xac) ){*/
/* schedule_addActiveSlot(self, */
/*                    19,*/
/*                    type,*/
/*                    isShared,*/
/*					0x0A,*/
/*                    &temp_neighbor*/
/*                );*/
/*            }*/

/*            if( ( idmanager_getMyID(self, ADDR_64B)->addr_64b[7] == 0xac) && (temp_neighbor.addr_64b[7]==0x07) ){*/
/* schedule_addActiveSlot(self, */
/*                    19,*/
/*                    type,*/
/*                    isShared,*/
/*					0x0A,*/
/*                    &temp_neighbor*/
/*                );*/
/*            }*/

/*            // 55 - 36*/
/*            if( ( idmanager_getMyID(self, ADDR_64B)->addr_64b[7] == 0x55) && (temp_neighbor.addr_64b[7]==0x36) ){*/
/* schedule_addActiveSlot(self, */
/*                    13,*/
/*                    type,*/
/*                    isShared,*/
/*					0x0A,*/
/*                    &temp_neighbor*/
/*                );*/
/*            }*/

/*            if( ( idmanager_getMyID(self, ADDR_64B)->addr_64b[7] == 0x36) && (temp_neighbor.addr_64b[7]==0x55) ){*/
/* schedule_addActiveSlot(self, */
/*                    13,*/
/*                    type,*/
/*                    isShared,*/
/*					0x0A,*/
/*                    &temp_neighbor*/
/*                );*/
/*            }*/

/*            // ac - 36*/
/*            if( ( idmanager_getMyID(self, ADDR_64B)->addr_64b[7] == 0xac) && (temp_neighbor.addr_64b[7]==0x36) ){*/
/* schedule_addActiveSlot(self, */
/*                    14,*/
/*                    type,*/
/*                    isShared,*/
/*					0x0A,*/
/*                    &temp_neighbor*/
/*                );*/
/*            }*/

/*            if( ( idmanager_getMyID(self, ADDR_64B)->addr_64b[7] == 0x36) && (temp_neighbor.addr_64b[7]==0xac) ){*/
/* schedule_addActiveSlot(self, */
/*                    14,*/
/*                    type,*/
/*                    isShared,*/
/*					0x0A,*/
/*                    &temp_neighbor*/
/*                );*/
/*            }*/
 schedule_addActiveSlot(self, 
                cellList[i].slotoffset,
                type,
                isShared,
                cellList[i].channeloffset,
                &temp_neighbor
            );
         }
    }
   
    return hasCellsAdded;
}

bool sixtop_removeCells(OpenMote* self, 
      uint8_t      slotframeID,
      cellInfo_ht* cellList,
      open_addr_t* previousHop,
    uint8_t      cellOptions
   ){
    uint8_t     i;
    open_addr_t temp_neighbor;
    bool        hasCellsRemoved;
    
    memcpy(&temp_neighbor,previousHop,sizeof(open_addr_t));
    
    hasCellsRemoved = FALSE;
    // delete cells from schedule
    for(i=0;i<CELLLIST_MAX_LEN;i++){
        if (cellList[i].isUsed){
            hasCellsRemoved = TRUE;
 schedule_removeActiveSlot(self, 
                cellList[i].slotoffset,
                &temp_neighbor
            );
        }
    }
    
    return hasCellsRemoved;
}

bool sixtop_areAvailableCellsToBeScheduled(OpenMote* self, 
      uint8_t      frameID, 
      uint8_t      numOfCells, 
      cellInfo_ht* cellList
){
    uint8_t i;
    uint8_t numbOfavailableCells;
    bool    available;
    
    i          = 0;
    numbOfavailableCells = 0;
    available  = FALSE;
  
    if(numOfCells == 0 || numOfCells>CELLLIST_MAX_LEN){
        // log wrong parameter error TODO
    
        available = FALSE;
    } else {
        do {
            if( schedule_isSlotOffsetAvailable(self, cellList[i].slotoffset) == TRUE){
    			//printf("I am node %d, CELL at offset  %d is available \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],cellList[i].slotoffset);

                numbOfavailableCells++;
            } else {
                // mark the cell
                cellList[i].isUsed = FALSE;
            }
            i++;
        }while(i<CELLLIST_MAX_LEN && numbOfavailableCells!=numOfCells);
        
        if(numbOfavailableCells>0){
            // there are more than one cell can be added.
            // the rest cells in the list will not be used
            while(i<CELLLIST_MAX_LEN){
                cellList[i].isUsed = FALSE;
                i++;
            }
            available = TRUE;
        } else {
            // No cell in the list is able to be added
            available = FALSE;
        }
    }
    return available;
}

bool sixtop_areAvailableCellsToBeRemoved(OpenMote* self,       
    uint8_t      frameID, 
    uint8_t      numOfCells, 
    cellInfo_ht* cellList,
    open_addr_t* neighbor,
    uint8_t      cellOptions
){
    uint8_t              i;
    uint8_t              numOfavailableCells;
    bool                 available;
    slotinfo_element_t   info;
    cellType_t           type;
    open_addr_t          anycastAddr;
    
    i          = 0;
    numOfavailableCells = 0;
    available           = TRUE;
    
    // translate cellOptions to cell type 
    if (cellOptions == CELLOPTIONS_TX){
        type = CELLTYPE_TX;
    }
    if (cellOptions == CELLOPTIONS_RX){
        type = CELLTYPE_RX;
    }
    if (cellOptions == (CELLOPTIONS_TX | CELLOPTIONS_RX | CELLOPTIONS_SHARED)){
        type = CELLTYPE_TXRX;
        memset(&anycastAddr,0,sizeof(open_addr_t));
        anycastAddr.type = ADDR_ANYCAST;
    }
    
    if(numOfCells == 0 || numOfCells>CELLLIST_MAX_LEN){
        // log wrong parameter error TODO
        available = FALSE;
    } else {
        do {
            if (cellList[i].isUsed){
                memset(&info,0,sizeof(slotinfo_element_t));
                if (type==CELLTYPE_TXRX){
 schedule_getSlotInfo(self, cellList[i].slotoffset,&anycastAddr,&info);
                } else {
 schedule_getSlotInfo(self, cellList[i].slotoffset,neighbor,&info);
                }
                if(info.link_type != type){
                    available = FALSE;
                    break;
                } else {
                    numOfavailableCells++;
                }
            }
            i++;
        }while(i<CELLLIST_MAX_LEN && numOfavailableCells<numOfCells);
        
        if(numOfavailableCells==numOfCells && available == TRUE){
            //the rest link will not be scheduled, mark them as off type
            while(i<CELLLIST_MAX_LEN){
                cellList[i].isUsed = FALSE;
                i++;
            }
        } else {
            // local schedule can't satisfy the bandwidth of cell request
            available = FALSE;
        }
   }
   return available;
}
