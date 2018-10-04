/**
DO NOT EDIT DIRECTLY!!

This file was 'objectified' by SCons as a pre-processing
step for the building a Python extension module.

This was done on 2018-03-08 18:27:49.425022.
*/
/**
\brief A CoAP resource which indicates the board its running on.
*/


#include "opendefs_obj.h"
#include "rrt_obj.h"
#include "sixtop_obj.h"
#include "idmanager_obj.h"
#include "openqueue_obj.h"
#include "neighbors_obj.h"
#include "packetfunctions_obj.h"
#include "leds_obj.h"
#include "openserial_obj.h"

//=========================== defines =========================================

const uint8_t rrt_path0[] = "rt";

//=========================== variables =======================================

// declaration of global variable _rrt_vars_ removed during objectification.

//=========================== prototypes ======================================

owerror_t rrt_receive(OpenMote* self, 
        OpenQueueEntry_t* msg,
        coap_header_iht*  coap_header,
        coap_option_iht*  coap_incomingOptions,
        coap_option_iht*  coap_outgoingOptions,
        uint8_t*          coap_outgoingOptionsLen
);
void rrt_sendDone(OpenMote* self, 
   OpenQueueEntry_t* msg,
   owerror_t error
);
void rrt_setGETRespMsg(OpenMote* self, 
   OpenQueueEntry_t* msg,
   uint8_t discovered   
);

void rrt_sendCoAPMsg(OpenMote* self, char actionMsg, uint8_t *ipv6mote);

//=========================== public ==========================================

/**
\brief Initialize this module.
*/
void rrt_init(OpenMote* self) {
   
   // do not run if DAGroot
   if( idmanager_getIsDAGroot(self)==TRUE) return; 
   
   // prepare the resource descriptor for the /rt path
   (self->rrt_vars).desc.path0len             = sizeof(rrt_path0)-1;
   (self->rrt_vars).desc.path0val             = (uint8_t*)(&rrt_path0);
   (self->rrt_vars).desc.path1len             = 0;
   (self->rrt_vars).desc.path1val             = NULL;
   (self->rrt_vars).desc.componentID          = COMPONENT_RRT;
   (self->rrt_vars).desc.securityContext      = NULL;
   (self->rrt_vars).desc.discoverable         = TRUE;
   (self->rrt_vars).desc.callbackRx           = &rrt_receive;
   (self->rrt_vars).desc.callbackSendDone     = &rrt_sendDone;

   (self->rrt_vars).discovered                = 0; //if this mote has been discovered by ringmaster
   
   // register with the CoAP module
 opencoap_register(self, &(self->rrt_vars).desc);
}

//=========================== private =========================================

/**
\brief Called when a CoAP message is received for this resource.

\param[in] msg          The received message. CoAP header and options already
   parsed.
\param[in] coap_header  The CoAP header contained in the message.
\param[in] coap_options The CoAP options contained in the message.

\return Whether the response is prepared successfully.
*/
owerror_t rrt_receive(OpenMote* self, 
        OpenQueueEntry_t* msg,
        coap_header_iht*  coap_header,
        coap_option_iht*  coap_incomingOptions,
        coap_option_iht*  coap_outgoingOptions,
        uint8_t*          coap_outgoingOptionsLen
) {
   owerror_t outcome;
   uint8_t mssgRecvd;
   uint8_t moteToSendTo[16];
   uint8_t actionToFwd;

   switch (coap_header->Code) {
      case COAP_CODE_REQ_GET:
         
         //=== reset packet payload (we will reuse this packetBuffer)
         msg->payload                     = &(msg->packet[127]);
         msg->length                      = 0;
         
         //=== prepare  CoAP response
 rrt_setGETRespMsg(self, msg, (self->rrt_vars).discovered);
         
         // set the CoAP header
         coap_header->Code                = COAP_CODE_RESP_CONTENT;
         
         outcome                          = E_SUCCESS;
         break;
      case COAP_CODE_REQ_PUT:
      case COAP_CODE_REQ_POST:
         mssgRecvd = msg->payload[0];
         
         if (mssgRecvd == 'C') {
            (self->rrt_vars).discovered = 1;
         } else if (mssgRecvd == 'B' && (self->rrt_vars).discovered == 1) {
            //blink mote
 leds_error_toggle(self);

            //send packet back saying it did action B - blink
 rrt_sendCoAPMsg(self, 'B', NULL); //NULL for ringmaster
         } else if (mssgRecvd == 'F' && (self->rrt_vars).discovered == 1) { //format - FB[ipv6]
            actionToFwd = msg->payload[1];
            memcpy(&moteToSendTo, &msg->payload[2], 16);
 rrt_sendCoAPMsg(self, actionToFwd, moteToSendTo);
         }

         // reset packet payload
         msg->payload                     = &(msg->packet[127]);
         msg->length                      = 0;

         //set the CoAP header
         coap_header->Code                = COAP_CODE_RESP_CONTENT;

         outcome                          = E_SUCCESS;
         
         break;
      case COAP_CODE_REQ_DELETE:
         msg->payload                     = &(msg->packet[127]);
         msg->length                      = 0;
         
         //unregister the current mote as 'discovered' by ringmaster
         (self->rrt_vars).discovered = 0; 
         
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

void rrt_setGETRespMsg(OpenMote* self, OpenQueueEntry_t* msg, uint8_t registered) {
         if (registered == 0) {
 packetfunctions_reserveHeaderSize(self, msg,11);
             msg->payload[0]  =  'r';
             msg->payload[1]  =  'e';
             msg->payload[2]  =  'g';
             msg->payload[3]  =  'i';
             msg->payload[4]  =  's';
             msg->payload[5]  =  't';
             msg->payload[6]  =  'e';
             msg->payload[7]  =  'r';
             msg->payload[8]  =  'i';
             msg->payload[9]  =  'n';
             msg->payload[10] = 'g';

 rrt_sendCoAPMsg(self, 'D', NULL); //'D' stands for discovery, 0 for ringmaster

         } else {
 packetfunctions_reserveHeaderSize(self, msg,10);
             msg->payload[0] = 'r';
             msg->payload[1] = 'e';
             msg->payload[2] = 'g';
             msg->payload[3] = 'i';
             msg->payload[4] = 's';
             msg->payload[5] = 't';
             msg->payload[6] = 'e';
             msg->payload[7] = 'r';
             msg->payload[8] = 'e';
             msg->payload[9] = 'd';
         }
}

/**
 * if mote is 0, then send to the ringmater, defined by ipAddr_ringmaster
**/
void rrt_sendCoAPMsg(OpenMote* self, char actionMsg, uint8_t *ipv6mote) {
      OpenQueueEntry_t* pkt;
      owerror_t outcome;
      coap_option_iht options[2];
      uint8_t medType;


      pkt = openqueue_getFreePacketBuffer(self, COMPONENT_RRT);
      if (pkt == NULL) {
 openserial_printError(self, COMPONENT_RRT,ERR_BUSY_SENDING,
                                (errorparameter_t)0,
                                (errorparameter_t)0);
 openqueue_freePacketBuffer(self, pkt);
          return;
      }

      pkt->creator   = COMPONENT_RRT;
      pkt->owner      = COMPONENT_RRT;
      pkt->l4_protocol  = IANA_UDP;

 packetfunctions_reserveHeaderSize(self, pkt, 1);
      pkt->payload[0] = actionMsg;

      // location-path option
      options[0].type = COAP_OPTION_NUM_URIPATH;
      options[0].length = sizeof(rrt_path0) - 1;
      options[0].pValue = (uint8_t *) rrt_path0;
      
       // content-type option
      medType = COAP_MEDTYPE_APPOCTETSTREAM;
      options[1].type = COAP_OPTION_NUM_CONTENTFORMAT;
      options[1].length = 1;
      options[1].pValue = &medType;

      //metada
      pkt->l4_destination_port   = WKP_UDP_RINGMASTER; 
      pkt->l3_destinationAdd.type = ADDR_128B;
      // set destination address here
      if (!ipv6mote) {  //if mote ptr is NULL, then send to ringmaster
        memcpy(&pkt->l3_destinationAdd.addr_128b[0], &ipAddr_ringmaster, 16);
      } else {
        memcpy(&pkt->l3_destinationAdd.addr_128b[0], &ipv6mote[0], 16);
      }

      //send
      outcome = opencoap_send(self, 
              pkt,
              COAP_TYPE_NON,
              COAP_CODE_REQ_PUT,
              1, // token len
              options,
              2, // options len
              &(self->rrt_vars).desc
              );
      

      if (outcome == E_FAIL) {
 openqueue_freePacketBuffer(self, pkt);
      }
}

/**
\brief The stack indicates that the packet was sent.

\param[in] msg The CoAP message just sent.
\param[in] error The outcome of sending it.
*/
void rrt_sendDone(OpenMote* self, OpenQueueEntry_t* msg, owerror_t error) {
 openqueue_freePacketBuffer(self, msg);
}
