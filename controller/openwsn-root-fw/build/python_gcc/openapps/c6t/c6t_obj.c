/**
DO NOT EDIT DIRECTLY!!

This file was 'objectified' by SCons as a pre-processing
step for the building a Python extension module.

This was done on 2018-03-08 18:27:41.716180.
*/
/**
\brief CoAP 6top application.

\author Xavi Vilajosana <xvilajosana@eecs.berkeley.edu>, February 2013.
\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, July 2014
*/

#include "opendefs_obj.h"
#include "c6t_obj.h"
#include "sixtop_obj.h"
#include "idmanager_obj.h"
#include "openqueue_obj.h"
#include "neighbors_obj.h"
#include "msf_obj.h"

//=========================== defines =========================================

const uint8_t c6t_path0[] = "6t";

//=========================== variables =======================================

// declaration of global variable _c6t_vars_ removed during objectification.

//=========================== prototypes ======================================

owerror_t c6t_receive(OpenMote* self, OpenQueueEntry_t* msg,
        coap_header_iht*  coap_header,
        coap_option_iht*  coap_incomingOptions,
        coap_option_iht*  coap_outgoingOptions,
        uint8_t*          coap_outgoingOptionsLen
);
void c6t_sendDone(OpenMote* self, 
   OpenQueueEntry_t* msg,
   owerror_t         error
);

//=========================== public ==========================================

void c6t_init(OpenMote* self) {
   if( idmanager_getIsDAGroot(self)==TRUE) return; 
   
   // prepare the resource descriptor for the /6t path
   (self->c6t_vars).desc.path0len            = sizeof(c6t_path0)-1;
   (self->c6t_vars).desc.path0val            = (uint8_t*)(&c6t_path0);
   (self->c6t_vars).desc.path1len            = 0;
   (self->c6t_vars).desc.path1val            = NULL;
   (self->c6t_vars).desc.componentID         = COMPONENT_C6T;
   (self->c6t_vars).desc.securityContext     = NULL;
   (self->c6t_vars).desc.discoverable        = TRUE;
   (self->c6t_vars).desc.callbackRx          = &c6t_receive;
   (self->c6t_vars).desc.callbackSendDone    = &c6t_sendDone;
   
 opencoap_register(self, &(self->c6t_vars).desc);
}

//=========================== private =========================================

/**
\brief Receives a command and a list of items to be used by the command.

\param[in] msg          The received message. CoAP header and options already
   parsed.
\param[in] coap_header  The CoAP header contained in the message.
\param[in] coap_options The CoAP options contained in the message.

\return Whether the response is prepared successfully.
*/
owerror_t c6t_receive(OpenMote* self, OpenQueueEntry_t* msg,
        coap_header_iht*  coap_header,
        coap_option_iht*  coap_incomingOptions,
        coap_option_iht*  coap_outgoingOptions,
        uint8_t*          coap_outgoingOptionsLen
) {
   owerror_t            outcome;
   open_addr_t          neighbor;
   bool                 foundNeighbor;
   cellInfo_ht          celllist_add[CELLLIST_MAX_LEN];
   cellInfo_ht          celllist_delete[CELLLIST_MAX_LEN];
   
   switch (coap_header->Code) {
      
      case COAP_CODE_REQ_PUT:
         // add a slot
         
         // reset packet payload
         msg->payload                  = &(msg->packet[127]);
         msg->length                   = 0;
         
         // get preferred parent
         foundNeighbor = icmpv6rpl_getPreferredParentEui64(self, &neighbor);
         if (foundNeighbor==FALSE) {
            outcome                    = E_FAIL;
            coap_header->Code          = COAP_CODE_RESP_PRECONDFAILED;
            break;
         }
         
         if ( msf_candidateAddCellList(self, celllist_add,1)==FALSE){
            // set the CoAP header
            outcome                       = E_FAIL;
            coap_header->Code             = COAP_CODE_RESP_CHANGED;
            break;
         }
         // call sixtop
         outcome = sixtop_request(self, 
            IANA_6TOP_CMD_ADD,                  // code
            &neighbor,                          // neighbor
            1,                                  // number cells
            CELLOPTIONS_TX,                     // cellOptions
            celllist_add,                       // celllist to add
            NULL,                               // celllist to delete (not used)
 msf_getsfid(self),                      // sfid
            0,                                  // list command offset (not used)
            0                                   // list command maximum celllist (not used)
         );
         
         // set the CoAP header
         coap_header->Code             = COAP_CODE_RESP_CHANGED;

         break;
      
      case COAP_CODE_REQ_DELETE:
         // delete a slot
         
         // reset packet payload
         msg->payload                  = &(msg->packet[127]);
         msg->length                   = 0;
         
         // get preferred parent
         foundNeighbor = icmpv6rpl_getPreferredParentEui64(self, &neighbor);
         if (foundNeighbor==FALSE) {
            outcome                    = E_FAIL;
            coap_header->Code          = COAP_CODE_RESP_PRECONDFAILED;
            break;
         }
         
         // call sixtop
         if ( msf_candidateRemoveCellList(self, celllist_delete,&neighbor,1)==FALSE){
            // set the CoAP header
            outcome                       = E_FAIL;
            coap_header->Code             = COAP_CODE_RESP_CHANGED;
            break;
         }
         // call sixtop
         outcome = sixtop_request(self, 
            IANA_6TOP_CMD_ADD,                  // code
            &neighbor,                          // neighbor
            1,                                  // number cells
            CELLOPTIONS_TX,                     // cellOptions
            celllist_add,                       // celllist to add
            NULL,                               // celllist to delete (not used)
 msf_getsfid(self),                      // sfid
            0,                                  // list command offset (not used)
            0                                   // list command maximum celllist (not used)
         );
         
         // set the CoAP header
         coap_header->Code             = COAP_CODE_RESP_CHANGED;
         break;
         
      default:
         outcome = E_FAIL;
         break;
   }
   
   return outcome;
}

void c6t_sendDone(OpenMote* self, OpenQueueEntry_t* msg, owerror_t error) {
 openqueue_freePacketBuffer(self, msg);
}
