#include "opendefs.h"
#include "whispersender.h"
#include "openqueue.h"
#include "openserial.h"
#include "packetfunctions.h"
#include "scheduler.h"
#include "IEEE802154E.h"
#include "idmanager.h"
#include "opencoap.h"

//=========================== variables =======================================

whispersender_vars_t whispersender_vars;

static const uint8_t whispersender_payload[]    = "whispersender";
static const uint8_t whispersender_dst_addr[]   = {
   0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
}; 

const uint8_t whispersender_path0[] = "p";

//=========================== prototypes ======================================

void whispersender_timer_cb(opentimers_id_t id);
void whispersender_task_cb(void);
owerror_t whispersender_receiveCoap(
      OpenQueueEntry_t* msg,
      coap_header_iht* coap_header,
      coap_option_iht* coap_options
   );
void whispersender_receiveUdp(OpenQueueEntry_t* pkt);
void whispersender_sendDoneCoap(OpenQueueEntry_t* msg, owerror_t error);

void whispersender_sendDoneUdp(OpenQueueEntry_t* msg, owerror_t error);
//=========================== public ==========================================

void whispersender_init() {

    // do not run if DAGroot
    if(idmanager_getIsDAGroot()==TRUE) return; 
    // do not run if not whisper node
    if(idmanager_getMyID(ADDR_64B)->addr_64b[7] != 0x36) return;	//only mote 9 can send, Test 1

    leds_debug_toggle();
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

    whispersender_vars.timerIdWhispersender = opentimers_create();
    whispersender_vars.counter=0;

    // register with the CoAP module
    opencoap_register(&whispersender_vars.descoap);


    whispersender_vars.period = WHISPERSENDER_PERIOD_MS;

        // register at UDP stack
    whispersender_vars.descudp.port              	= WKP_UDP_WHISPER;
    whispersender_vars.descudp.callbackReceive      = &whispersender_receiveUdp;
    whispersender_vars.descudp.callbackSendDone     = &whispersender_sendDoneUdp;
    openudp_register(&whispersender_vars.descudp);
}

void whispersender_receiveUdp(OpenQueueEntry_t* pkt) {
   
   openqueue_freePacketBuffer(pkt);
   
   openserial_printError(
      COMPONENT_UINJECT,
      ERR_RCVD_ECHO_REPLY,
      (errorparameter_t)0,
      (errorparameter_t)0
   );
}


owerror_t whispersender_receiveCoap(
	      OpenQueueEntry_t* msg,
	      coap_header_iht* coap_header,
	      coap_option_iht* coap_options) {
   

   uint8_t          i;
   owerror_t outcome=0;


   switch (coap_header->Code) {
      case COAP_CODE_REQ_PUT:

			if ( msg->payload[0]==120) {
				   opentimers_scheduleIn(
					   whispersender_vars.timerIdWhispersender,
					   whispersender_vars.period,
					   TIME_MS,
					   TIMER_PERIODIC,
					   whispersender_timer_cb
				   );

			}else{

			    outcome                       = E_FAIL;
			    coap_header->Code             = COAP_CODE_RESP_CHANGED;
					//back to IDLE
				break;
			}

	        //=== reset packet payload (we will reuse this packetBuffer)
	        msg->payload                     = &(msg->packet[127]);
	        msg->length                      = 0;

	        // set the CoAP header
	        coap_header->Code                = COAP_CODE_RESP_CONTENT;

	        outcome                          = E_SUCCESS;
	        break;

	      default:

	         // return an error message
	         outcome = E_FAIL;
	   }


   return outcome;

}

//=========================== private =========================================

/**
\note timer fired, but we don't want to execute task in ISR mode instead, push
   task to scheduler with CoAP priority, and let scheduler take care of it.
*/
void whispersender_timer_cb(opentimers_id_t id){
   
   scheduler_push_task(whispersender_task_cb,TASKPRIO_COAP);
}

void whispersender_task_cb() {

	

   OpenQueueEntry_t*    pkt;
   uint8_t              asnArray[5];

   leds_sync_toggle();

   // don't run if not synch
   if (ieee154e_isSynch() == FALSE){
	   opentimers_destroy(whispersender_vars.timerIdWhispersender);
	   return;
   }

   // don't run on dagroot
   if (idmanager_getIsDAGroot()) {
      opentimers_destroy(whispersender_vars.timerIdWhispersender);
      return;
   }

   // if you get here, send a packet. You should be only be node 4

   // get a free packet buffer
   pkt = openqueue_getFreePacketBuffer(COMPONENT_WHISPERSENDER);
   if (pkt==NULL) {
      openserial_printError(
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
   packetfunctions_reserveHeaderSize(pkt,sizeof(whispersender_payload)-1);
   memcpy(&pkt->payload[0],whispersender_payload,sizeof(whispersender_payload)-1);

   packetfunctions_reserveHeaderSize(pkt,sizeof(uint16_t));
   pkt->payload[1] = (uint8_t)((whispersender_vars.counter & 0xff00)>>8);
   pkt->payload[0] = (uint8_t)(whispersender_vars.counter & 0x00ff);
   whispersender_vars.counter++;

   packetfunctions_reserveHeaderSize(pkt,sizeof(asn_t));
   ieee154e_getAsn(asnArray);
   pkt->payload[0] = asnArray[0];
   pkt->payload[1] = asnArray[1];
   pkt->payload[2] = asnArray[2];
   pkt->payload[3] = asnArray[3];
   pkt->payload[4] = asnArray[4];

   packetfunctions_reserveHeaderSize(pkt,sizeof(uint8_t));
   pkt->payload[0] = idmanager_getMyID(ADDR_64B)->addr_64b[7];

   leds_debug_toggle();
   if ((openudp_send(pkt))==E_FAIL) {
      openqueue_freePacketBuffer(pkt);
   }

}

void whispersender_sendDoneCoap(OpenQueueEntry_t* msg, owerror_t error) {


   openqueue_freePacketBuffer(msg);
}

void whispersender_sendDoneUdp(OpenQueueEntry_t* msg, owerror_t error) {


   openqueue_freePacketBuffer(msg);
}


