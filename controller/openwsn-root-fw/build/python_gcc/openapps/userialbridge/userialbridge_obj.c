/**
DO NOT EDIT DIRECTLY!!

This file was 'objectified' by SCons as a pre-processing
step for the building a Python extension module.

This was done on 2018-03-08 18:27:53.957893.
*/
#include "opendefs_obj.h"
#include "userialbridge_obj.h"
#include "openqueue_obj.h"
#include "opentimers_obj.h"
#include "openserial_obj.h"
#include "packetfunctions_obj.h"
#include "scheduler_obj.h"
#include "IEEE802154E_obj.h"
#include "idmanager_obj.h"

//=========================== variables =======================================

// declaration of global variable _userialbridge_vars_ removed during objectification.

static const uint8_t userialbridge_dst_addr[]   = {
    0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
}; 

//=========================== prototypes ======================================

void userialbridge_task_cb(OpenMote* self);

//=========================== public ==========================================

void userialbridge_init(OpenMote* self) {
    
    // clear local variables
    memset(&(self->userialbridge_vars),0,sizeof(userialbridge_vars_t));
    
    // register with openserial
    (self->userialbridge_vars).openserial_rsvp.cmdId = SERFRAME_PC2MOTE_TRIGGERUSERIALBRIDGE;
    (self->userialbridge_vars).openserial_rsvp.cb    = userialbridge_triggerData;
    (self->userialbridge_vars).openserial_rsvp.next  = NULL;
 openserial_register(self, &(self->userialbridge_vars).openserial_rsvp);

    // register at UDP stack
    (self->userialbridge_vars).desc.port              = WKP_UDP_SERIALBRIDGE;
    (self->userialbridge_vars).desc.callbackReceive   = NULL;
    (self->userialbridge_vars).desc.callbackSendDone  = &userialbridge_sendDone;
 openudp_register(self, &(self->userialbridge_vars).desc);
}

void userialbridge_sendDone(OpenMote* self, OpenQueueEntry_t* msg, owerror_t error) {
 openqueue_freePacketBuffer(self, msg);
}

//=========================== private =========================================

void userialbridge_triggerData(OpenMote* self) {
    
    // store payload to send
    (self->userialbridge_vars).txbufLen = openserial_getInputBuffer(self, 
        &(self->userialbridge_vars).txbuf[0],
        USERIALBRIDGE_MAXPAYLEN
    );
    
    // push task
 scheduler_push_task(self, userialbridge_task_cb,TASKPRIO_COAP);
}

void userialbridge_task_cb(OpenMote* self) {
    OpenQueueEntry_t*    pkt;
    
    // don't run if not synch
    if ( ieee154e_isSynch(self) == FALSE) return;
    
    // if you get here, send a packet
    
    // get a free packet buffer
    pkt = openqueue_getFreePacketBuffer(self, COMPONENT_USERIALBRIDGE);
    if (pkt==NULL) {
 openserial_printError(self, 
            COMPONENT_USERIALBRIDGE,
            ERR_NO_FREE_PACKET_BUFFER,
            (errorparameter_t)0,
            (errorparameter_t)0
        );
        return;
    }
    
    pkt->owner                         = COMPONENT_USERIALBRIDGE;
    pkt->creator                       = COMPONENT_USERIALBRIDGE;
    pkt->l4_protocol                   = IANA_UDP;
    pkt->l4_destination_port           = WKP_UDP_SERIALBRIDGE;
    pkt->l4_sourcePortORicmpv6Type     = WKP_UDP_SERIALBRIDGE;
    pkt->l3_destinationAdd.type        = ADDR_128B;
    memcpy(&pkt->l3_destinationAdd.addr_128b[0],userialbridge_dst_addr,16);
    
 packetfunctions_reserveHeaderSize(self, pkt,(self->userialbridge_vars).txbufLen);
    memcpy(&pkt->payload[0],&(self->userialbridge_vars).txbuf[0],(self->userialbridge_vars).txbufLen);
    
    if (( openudp_send(self, pkt))==E_FAIL) {
 openqueue_freePacketBuffer(self, pkt);
    }
}
