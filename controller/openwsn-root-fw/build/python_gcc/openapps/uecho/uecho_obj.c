/**
DO NOT EDIT DIRECTLY!!

This file was 'objectified' by SCons as a pre-processing
step for the building a Python extension module.

This was done on 2018-03-08 18:27:50.813155.
*/
#include "opendefs_obj.h"
#include "uecho_obj.h"
#include "openqueue_obj.h"
#include "openserial_obj.h"
#include "packetfunctions_obj.h"

//=========================== variables =======================================

// declaration of global variable _uecho_vars_ removed during objectification.

//=========================== prototypes ======================================

//=========================== public ==========================================

void uecho_init(OpenMote* self) {
   // clear local variables
   memset(&(self->uecho_vars),0,sizeof(uecho_vars_t));

   // register at UDP stack
   (self->uecho_vars).desc.port              = WKP_UDP_ECHO;
   (self->uecho_vars).desc.callbackReceive   = &uecho_receive;
   (self->uecho_vars).desc.callbackSendDone  = &uecho_sendDone;
 openudp_register(self, &(self->uecho_vars).desc);
}

void uecho_receive(OpenMote* self, OpenQueueEntry_t* request) {
   uint16_t          temp_l4_destination_port;
   OpenQueueEntry_t* reply;
   
   reply = openqueue_getFreePacketBuffer(self, COMPONENT_UECHO);
   if (reply==NULL) {
 openserial_printError(self, 
         COMPONENT_UECHO,
         ERR_NO_FREE_PACKET_BUFFER,
         (errorparameter_t)0,
         (errorparameter_t)0
      );
 openqueue_freePacketBuffer(self, request); //clear the request packet as well
      return;
   }
   
   reply->owner                         = COMPONENT_UECHO;
   
   // reply with the same OpenQueueEntry_t
   reply->creator                       = COMPONENT_UECHO;
   reply->l4_protocol                   = IANA_UDP;
   temp_l4_destination_port           = request->l4_destination_port;
   reply->l4_destination_port           = request->l4_sourcePortORicmpv6Type;
   reply->l4_sourcePortORicmpv6Type     = temp_l4_destination_port;
   reply->l3_destinationAdd.type        = ADDR_128B;
   
   // copy source to destination to echo.
   memcpy(&reply->l3_destinationAdd.addr_128b[0],&request->l3_sourceAdd.addr_128b[0],16);
   
 packetfunctions_reserveHeaderSize(self, reply,request->length);
   memcpy(&reply->payload[0],&request->payload[0],request->length);
 openqueue_freePacketBuffer(self, request);
   
   if (( openudp_send(self, reply))==E_FAIL) {
 openqueue_freePacketBuffer(self, reply);
   }
}

void uecho_sendDone(OpenMote* self, OpenQueueEntry_t* msg, owerror_t error) {
 openqueue_freePacketBuffer(self, msg);
}

bool uecho_debugPrint(OpenMote* self) {
   return FALSE;
}

//=========================== private =========================================
