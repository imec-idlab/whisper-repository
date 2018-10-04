/**
DO NOT EDIT DIRECTLY!!

This file was 'objectified' by SCons as a pre-processing
step for the building a Python extension module.

This was done on 2018-03-08 18:27:46.771718.
*/
/**
\brief A CoAP resource which allows an application to GET/SET the state of the
   error LED.
*/

#include "opendefs_obj.h"
#include "cleds_obj.h"
#include "opencoap_obj.h"
#include "packetfunctions_obj.h"
#include "leds_obj.h"
#include "openqueue_obj.h"

//=========================== variables =======================================

// declaration of global variable _cleds_vars_ removed during objectification.

const uint8_t cleds_path0[]       = "l";

//=========================== prototypes ======================================

owerror_t cleds_receive(OpenMote* self, OpenQueueEntry_t* msg,
        coap_header_iht*  coap_header,
        coap_option_iht*  coap_incomingOptions,
        coap_option_iht*  coap_outgoingOptions,
        uint8_t*          coap_outgoingOptionsLen);

void cleds_sendDone(OpenMote* self, 
   OpenQueueEntry_t* msg,
   owerror_t error
);

//=========================== public ==========================================

void cleds__init(OpenMote* self) {
   
   // prepare the resource descriptor for the /l path
   (self->cleds_vars).desc.path0len            = sizeof(cleds_path0)-1;
   (self->cleds_vars).desc.path0val            = (uint8_t*)(&cleds_path0);
   (self->cleds_vars).desc.path1len            = 0;
   (self->cleds_vars).desc.path1val            = NULL;
   (self->cleds_vars).desc.componentID         = COMPONENT_CLEDS;
   (self->cleds_vars).desc.securityContext     = NULL;
   (self->cleds_vars).desc.discoverable        = TRUE;
   (self->cleds_vars).desc.callbackRx          = &cleds_receive;
   (self->cleds_vars).desc.callbackSendDone    = &cleds_sendDone;
   
   // register with the CoAP module
 opencoap_register(self, &(self->cleds_vars).desc);
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
owerror_t cleds_receive(OpenMote* self, OpenQueueEntry_t* msg,
        coap_header_iht*  coap_header,
        coap_option_iht*  coap_incomingOptions,
        coap_option_iht*  coap_outgoingOptions,
        uint8_t*          coap_outgoingOptionsLen) {
   owerror_t outcome;
   
   switch (coap_header->Code) {
      case COAP_CODE_REQ_GET:
         // reset packet payload
         msg->payload                     = &(msg->packet[127]);
         msg->length                      = 0;
         
         // add CoAP payload
 packetfunctions_reserveHeaderSize(self, msg,1);

         if ( leds_error_isOn(self)==1) {
            msg->payload[0]               = '1';
         } else {
            msg->payload[0]               = '0';
         }
            
         // set the CoAP header
         coap_header->Code                = COAP_CODE_RESP_CONTENT;
         
         outcome                          = E_SUCCESS;
         break;
      
      case COAP_CODE_REQ_PUT:
      
         // change the LED's state
         if (msg->payload[0]=='1') {
 leds_error_on(self);
         } else if (msg->payload[0]=='2') {
 leds_error_toggle(self);
         } else {
 leds_error_off(self);
         }
         
         // reset packet payload
         msg->payload                     = &(msg->packet[127]);
         msg->length                      = 0;
         
         // set the CoAP header
         coap_header->Code                = COAP_CODE_RESP_CHANGED;
         
         outcome                          = E_SUCCESS;
         break;
         
      default:
         outcome                          = E_FAIL;
         break;
   }
   
   return outcome;
}

/**
\brief The stack indicates that the packet was sent.

\param[in] msg The CoAP message just sent.
\param[in] error The outcome of sending it.
*/
void cleds_sendDone(OpenMote* self, OpenQueueEntry_t* msg, owerror_t error) {
 openqueue_freePacketBuffer(self, msg);
}
