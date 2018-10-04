/**
\brief A CoAP resource which indicates the board its running on.
*/

#include "opendefs.h"
#include "whisper.h"
#include "opencoap.h"
#include "openqueue.h"
#include "packetfunctions.h"
#include "openserial.h"
#include "openrandom.h"
#include "board.h"
#include "idmanager.h"
#include "msf.h"
#include "neighbors.h"
#include "schedule.h"
#include "IEEE802154E.h"
#include "scheduler.h"
#include "sixtop.h"

//=========================== defines =========================================

const uint8_t whisper_path0[] = "w";

//=========================== variables =======================================


uint8_t targetParentNew;
uint8_t targetParentOld;
uint8_t targetChildren;
uint8_t state;
uint8_t fakeSource;
uint8_t numSentDios;
cellInfo_ht          celllist_add[CELLLIST_MAX_LEN];

whisper_vars_t whisper_vars;

void whisper_timer_cb(opentimers_id_t id);
void whisper_task_cb(void);

void whisper_timer_cb2(opentimers_id_t id);
void whisper_task_cb2(void);

void whisper_timer_cb3(opentimers_id_t id);
void whisper_task_cb3(void);

void whisper_timer_cb4(opentimers_id_t id);
void whisper_task_cb4(void);

void whisper_task_remote(uint8_t* buf, uint8_t bufLen);

//bool whisper_candidateAddCellList(cellInfo_ht* cellList,uint8_t requiredCells);
//=========================== prototypes ======================================

owerror_t     whisper_receive(
   OpenQueueEntry_t* msg,
   coap_header_iht*  coap_header,
   coap_option_iht*  coap_options
);
void          whisper_sendDone(
   OpenQueueEntry_t* msg,
   owerror_t error
);

//=========================== public ==========================================


/**
\brief Initialize this module.
*/
void whisper_init() {

   //this will be run in the root

   leds_debug_toggle();

   // start ONESHOT timer

   // prepare the resource descriptor for the /w path

   whisper_setState(0);
   targetParentOld=0;
   targetParentNew=0;
   targetChildren=0;
   fakeSource=0;
   numSentDios=0;

   whisper_vars.desc.path0len             = sizeof(whisper_path0)-1;
   whisper_vars.desc.path0val             = (uint8_t*)(&whisper_path0);
   whisper_vars.desc.path1len             = 0;
   whisper_vars.desc.path1val             = NULL;
   whisper_vars.desc.componentID          = COMPONENT_WHISPER;
   whisper_vars.desc.discoverable         = TRUE;
   whisper_vars.desc.callbackRx           = &whisper_receive;
   whisper_vars.desc.callbackSendDone     = &whisper_sendDone;
  
   whisper_vars.timerIdWhisper = opentimers_create();

   // register with the CoAP module
   opencoap_register(&whisper_vars.desc);
}

void whisper_setSentDIOs(uint8_t i){
	numSentDios=i;
}

uint8_t whisper_getSentDIOs(){
	return numSentDios;
}

void whisper_setState(uint8_t i){
	state=i;
}

uint8_t whisper_getState(){
	return state;
}

uint8_t whisper_getTargetChildren(){
	return targetChildren;
}

void whisper_setTargetChildren(uint8_t i){
	targetChildren=i;
}

uint8_t whisper_getTargetParentNew(){
	return targetParentNew;
}

void whisper_setTargetParentNew(uint8_t i){
	targetParentNew=i;
}

uint8_t whisper_getTargetParentOld(){
	return targetParentOld;
}

void whisper_setTargetParentOld(uint8_t i){
	targetParentOld=i;
}

uint8_t whisper_getFakeSource(){
	return fakeSource;
}

void whisper_setFakeSource(uint8_t i){
	fakeSource=i;
}

void whisper_trigger_nextStep(void);

//=========================== private =========================================

/**
\brief Called when a CoAP message is received for this resource.

\param[in] msg          The received message. CoAP header and options already
   parsed.
\param[in] coap_header  The CoAP header contained in the message.
\param[in] coap_options The CoAP options contained in the message.

\return Whether the response is prepared successfully.
*/


//not used, the root receives the primitives from Serial
owerror_t whisper_receive(
      OpenQueueEntry_t* msg,
      coap_header_iht* coap_header,
      coap_option_iht* coap_options
   ) {
   uint8_t          i;
   owerror_t outcome;

   if (msf_candidateAddCellList(celllist_add,1)==FALSE){
      // set the CoAP header
      outcome                       = E_FAIL;
      coap_header->Code             = COAP_CODE_RESP_CHANGED;
   }
   openrandom_get16b();
   open_addr_t      NeighborAddress;


   switch (coap_header->Code) {
      case COAP_CODE_REQ_PUT:

         
        outcome                          = E_SUCCESS;
        break;
      default:
         // return an error message
         outcome = E_FAIL;
   }
   
   return outcome;
}


/**
\note timer fired, but we don't want to execute task in ISR mode instead, push
   task to scheduler with CoAP priority, and let scheduler take care of it.
*/

void whisper_task_remote(uint8_t* buf, uint8_t bufLen){

	//will be run only in the root
	
	//define targets
	if (buf[1]==85){
		whisper_setTargetChildren(0x36);
		whisper_setTargetParentOld(0x55);
		whisper_setTargetParentNew(0xac);
		leds_debug_toggle();

	}else{
		whisper_setTargetChildren(0x36);
		whisper_setTargetParentOld(0xac);
		whisper_setTargetParentNew(0x55);
		leds_debug_toggle();
	}

	if (whisper_getTargetParentOld()==0x55){
		leds_sync_toggle();
	}else{
		leds_error_toggle();

	}

	 uint16_t		  rank;



	  rank=((3*DEFAULTLINKCOST-2)*MINHOPRANKINCREASE)-1;
          icmpv6rpl_sendWhisperDIO(whisper_getTargetChildren(),whisper_getTargetParentOld(),rank);
	  //icmpv6rpl_sendWhisperDIO(whisper_getTargetChildren(),whisper_getTargetParentOld(),2*rank);
	  whisper_setState(0);

	return;

}


/**
\brief The stack indicates that the packet was sent.

\param[in] msg The CoAP message just sent.
\param[in] error The outcome of sending it.
*/
void whisper_sendDone(OpenQueueEntry_t* msg, owerror_t error) {


   openqueue_freePacketBuffer(msg);
}

