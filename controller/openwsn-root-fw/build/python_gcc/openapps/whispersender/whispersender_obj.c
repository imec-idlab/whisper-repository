/**
DO NOT EDIT DIRECTLY!!

This file was 'objectified' by SCons as a pre-processing
step for the building a Python extension module.

This was done on 2018-03-08 18:27:58.753358.
*/
#include "opendefs_obj.h"
#include "whispersender_obj.h"
#include "openqueue_obj.h"
#include "openserial_obj.h"
#include "packetfunctions_obj.h"
#include "scheduler_obj.h"
#include "IEEE802154E_obj.h"
#include "idmanager_obj.h"
#include "opencoap_obj.h"

//=========================== variables =======================================

whispersender_vars_t whispersender_vars;

static const uint8_t whispersender_payload[]    = "whispersender";
static const uint8_t whispersender_dst_addr[]   = {
   0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
}; 

const uint8_t whispersender_path0[] = "p";

//=========================== prototypes ======================================

void whispersender_timer_cb(OpenMote* self, opentimers_id_t id);
void whispersender_task_cb(OpenMote* self);
owerror_t whispersender_receiveCoap(OpenMote* self, 
      OpenQueueEntry_t* msg,
      coap_header_iht* coap_header,
      coap_option_iht* coap_options
   );
void whispersender_receiveUdp(OpenMote* self, OpenQueueEntry_t* pkt);
void whispersender_sendDoneCoap(OpenMote* self, OpenQueueEntry_t* msg, owerror_t error);

void whispersender_sendDoneUdp(OpenMote* self, OpenQueueEntry_t* msg, owerror_t error);
//=========================== public ==========================================

void whispersender_init(OpenMote* self) {

    // do not run if DAGroot
    if( idmanager_getIsDAGroot(self)==TRUE) return; 
    // do not run if not whisper node
    if( idmanager_getMyID(self, ADDR_64B)->addr_64b[7] != 0x36) return;	//only mote 4 can send

 leds_debug_toggle(self);
    // clear local variables
    memset(&whispersender_vars,0,sizeof(whispersender_vars_t));


    whispersender_vars.descoap.path0len             = sizeof(whispersender_path0)-1;
    whispersender_vars.descoap.path0val             = (uint8_t*)(&whispersender_path0);
    whispersender_vars.descoap.path1len             = 0;
    whispersender_vars.descoap.path1val             = NULL;
    whispersender_vars.descoap.componentID          = COMPONENT_WHISPERSENDER;
    whispersender_vars.descoap.discoverable         = TRUE;
    whispersender_vars.descoap.callbackRx           = &whispersender_receiveCoap;
    whispersender_vars.descoap.callbackSendDone     = &whispersender_sendDoneCoap;

    whispersender_vars.timerIdWhispersender = opentimers_create(self);
    whispersender_vars.counter=0;

    // register with the CoAP module
 opencoap_register(self, &whispersender_vars.descoap);


    whispersender_vars.period = WHISPERSENDER_PERIOD_MS;

        // register at UDP stack
    whispersender_vars.descudp.port              	= WKP_UDP_WHISPER;
    whispersender_vars.descudp.callbackReceive      = &whispersender_receiveUdp;
    whispersender_vars.descudp.callbackSendDone     = &whispersender_sendDoneUdp;
 openudp_register(self, &whispersender_vars.descudp);

/*    // start periodic timer*/
/*    whispersender_sender_vars.timerId = opentimers_create(self);*/
/* opentimers_scheduleIn(self, */
/*        whispersender_sender_vars.timerId,*/
/*        WHISPER_SENDER_PERIOD_MS,*/
/*        TIME_MS,*/
/*        TIMER_PERIODIC,*/
/*        whispersender_sender_timer_cb*/
/*    );*/
}

void whispersender_receiveUdp(OpenMote* self, OpenQueueEntry_t* pkt) {
   
 openqueue_freePacketBuffer(self, pkt);
   
 openserial_printError(self, 
      COMPONENT_UINJECT,
      ERR_RCVD_ECHO_REPLY,
      (errorparameter_t)0,
      (errorparameter_t)0
   );
}


owerror_t whispersender_receiveCoap(OpenMote* self, 
	      OpenQueueEntry_t* msg,
	      coap_header_iht* coap_header,
	      coap_option_iht* coap_options) {
   

   uint8_t          i;
   owerror_t outcome=0;


   switch (coap_header->Code) {
      case COAP_CODE_REQ_PUT:

    	  // leds_debug_toggle(self);

			//printf("I am TARGET node %d, received a COAP message to start sending packets to the root \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
				//sending FAKE DIO
			//if ( (msg->payload[0]==120) && (msg->payload[1]==4) && (msg->payload[2]==2) && (msg->payload[3]==3) ) { 	// ord('x')=120 , works only with node 4 changing from 2 to 3
			if ( msg->payload[0]==120) {
				//printf("I am TARGET node %d, starting sending packets to the root \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
 opentimers_scheduleIn(self, 
					   whispersender_vars.timerIdWhispersender,
					   whispersender_vars.period,
					   TIME_MS,
					   TIMER_PERIODIC,
					   whispersender_timer_cb
				   );

			}else{

				//printf("I am TARGET node %d, end operation (bad Coap request: format -> x)\n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
			    outcome                       = E_FAIL;
			    coap_header->Code             = COAP_CODE_RESP_CHANGED;
					//back to IDLE
				break;
			}
			//printf("Done COAP\n");
	        //=== reset packet payload (we will reuse this packetBuffer)
	        msg->payload                     = &(msg->packet[127]);
	        msg->length                      = 0;

	        // set the CoAP header
	        coap_header->Code                = COAP_CODE_RESP_CONTENT;

	        outcome                          = E_SUCCESS;
	        break;

	      default:
	    	 //printf("I am TARGET node %d, end operation (no cells in first 6P message)\n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
	         // return an error message
	         outcome = E_FAIL;
	   }


   return outcome;

   
// openserial_printError(self, 
//      COMPONENT_whispersender,
//      ERR_RCVD_ECHO_REPLY,
//      (errorparameter_t)0,
//      (errorparameter_t)0
//   );
}

//=========================== private =========================================

/**
\note timer fired, but we don't want to execute task in ISR mode instead, push
   task to scheduler with CoAP priority, and let scheduler take care of it.
*/
void whispersender_timer_cb(OpenMote* self, opentimers_id_t id){
   
 scheduler_push_task(self, whispersender_task_cb,TASKPRIO_COAP);
}

void whispersender_task_cb(OpenMote* self) {

	

   OpenQueueEntry_t*    pkt;
   uint8_t              asnArray[5];

 leds_sync_toggle(self);
   //printf("I am TARGET node %d, Timer expired. Sending another packet\n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);

   // don't run if not synch
   if ( ieee154e_isSynch(self) == FALSE){
 opentimers_destroy(self, whispersender_vars.timerIdWhispersender);
	   return;
   }

   // don't run on dagroot
   if ( idmanager_getIsDAGroot(self)) {
 opentimers_destroy(self, whispersender_vars.timerIdWhispersender);
      return;
   }

   // if you get here, send a packet. You should be only be node 4

   // get a free packet buffer
   pkt = openqueue_getFreePacketBuffer(self, COMPONENT_WHISPERSENDER);
   if (pkt==NULL) {
 openserial_printError(self, 
    		  COMPONENT_UINJECT,
         ERR_NO_FREE_PACKET_BUFFER,
         (errorparameter_t)0,
         (errorparameter_t)0
      );
      return;
   }

   pkt->owner                         = COMPONENT_WHISPERSENDER;
   pkt->creator                       = COMPONENT_WHISPERSENDER;
   pkt->l4_protocol                   = IANA_UDP;
   pkt->l4_destination_port           = WKP_UDP_WHISPER;
   pkt->l4_sourcePortORicmpv6Type     = WKP_UDP_WHISPER;
   pkt->l3_destinationAdd.type        = ADDR_128B;
   memcpy(&pkt->l3_destinationAdd.addr_128b[0],whispersender_dst_addr,16);

   // add payload
 packetfunctions_reserveHeaderSize(self, pkt,sizeof(whispersender_payload)-1);
   memcpy(&pkt->payload[0],whispersender_payload,sizeof(whispersender_payload)-1);

 packetfunctions_reserveHeaderSize(self, pkt,sizeof(uint16_t));
   pkt->payload[1] = (uint8_t)((whispersender_vars.counter & 0xff00)>>8);
   pkt->payload[0] = (uint8_t)(whispersender_vars.counter & 0x00ff);
   whispersender_vars.counter++;

 packetfunctions_reserveHeaderSize(self, pkt,sizeof(asn_t));
 ieee154e_getAsn(self, asnArray);
   pkt->payload[0] = asnArray[0];
   pkt->payload[1] = asnArray[1];
   pkt->payload[2] = asnArray[2];
   pkt->payload[3] = asnArray[3];
   pkt->payload[4] = asnArray[4];

 packetfunctions_reserveHeaderSize(self, pkt,sizeof(uint8_t));
   pkt->payload[0] = idmanager_getMyID(self, ADDR_64B)->addr_64b[7];

   //printf("I am TARGET node %d, enqueueing DATAW packet with counter %d \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],whispersender_vars.counter);
 leds_debug_toggle(self);
   if (( openudp_send(self, pkt))==E_FAIL) {
 openqueue_freePacketBuffer(self, pkt);
   }

}

void whispersender_sendDoneCoap(OpenMote* self, OpenQueueEntry_t* msg, owerror_t error) {

   //printf("I am TARGET node %d, Send done DATAW packet with counter %d \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],whispersender_vars.counter);

 openqueue_freePacketBuffer(self, msg);
}

void whispersender_sendDoneUdp(OpenMote* self, OpenQueueEntry_t* msg, owerror_t error) {

   //printf("I am TARGET node %d, Send done DATAW packet with counter %d \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],whispersender_vars.counter);

 openqueue_freePacketBuffer(self, msg);
}


