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


   // do not run if DAGroot
   if(idmanager_getIsDAGroot()==TRUE) return; 
   // do not run if not whisper node
   if(idmanager_getMyID(ADDR_64B)->addr_64b[7] != 0x07) return;			//this will be run only in the Whisper node

   leds_debug_toggle();

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
        
		if (state==0){	//0 IDLE

			//sending FAKE DIO

			if ( msg->payload[0]==120) { 	// ord('x')=120 , works only with node 4 changing from 2 to 3

				whisper_setTargetChildren(msg->payload[1]);
				whisper_setTargetParentOld(msg->payload[2]);
				whisper_setTargetParentNew(msg->payload[3]);

				//first add cells in new parent
				 for (i=0;i<MAXNUMNEIGHBORS;i++) {
					if (neighbors_getNeighborEui64(&NeighborAddress, ADDR_64B, i)) { // this neighbor entry is in use
						if (NeighborAddress.addr_64b[7] == whisper_getTargetParentNew()){
							whisper_setFakeSource(whisper_getTargetChildren());
							 // call sixtop
							leds_debug_toggle();
							 outcome = sixtop_request(
								IANA_6TOP_CMD_ADD,                  // code
								&NeighborAddress,                   // neighbor
								1,                                  // number cells
								CELLOPTIONS_MSF,                    // cellOptions
								celllist_add,                       // celllist to add
								NULL,                               // celllist to delete (not used)
								5,                      		    // sfid 5, means whsiper
								0,                                  // list command offset (not used)
								0                                   // list command maximum celllist (not used)
							 );
							 i=100;
							 whisper_setState(1);//1 Sending first 6P add
						}
					}
				 }
				whisper_trigger_nextStep();

			}else{
				//skipping 6P
				if ( msg->payload[0]==122) {
					whisper_setState(2);
					whisper_trigger_nextStep();
				}

			    outcome                       = E_FAIL;
			    coap_header->Code             = COAP_CODE_RESP_CHANGED;
				whisper_setState(0);	//back to IDLE
				break;
			}
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


void whisper_trigger_nextStep(void){

	open_addr_t      NeighborAddress;
	uint8_t          i;
	cellInfo_ht   celllist_del[1];


	switch (whisper_getState()){
	case 1: //another 6P add

		leds_debug_toggle();

		   opentimers_scheduleIn(
			   whisper_vars.timerIdWhisper,
			   10000,
			   TIME_MS,
			   TIMER_ONESHOT,
			   whisper_timer_cb
		   );
		   break;
	case 2:	//send RPL fake DIOs
		leds_debug_toggle();

		   opentimers_scheduleIn(
			   whisper_vars.timerIdWhisper,
			   10000,
			   TIME_MS,
			   TIMER_ONESHOT,
			   whisper_timer_cb2
		   );
		  break;
	case 3: //first 6P clear

		leds_debug_toggle();
		//no Clearing for the moment
		whisper_setState(0);

		   break;
	case 4: //another 6P clear



		   opentimers_scheduleIn(
			   whisper_vars.timerIdWhisper,
			   10000,
			   TIME_MS,
			   TIMER_ONESHOT,
			   whisper_timer_cb4
		   );
		   break;
	case 5:


		for (i=0;i<MAXNUMNEIGHBORS;i++) {
					if (neighbors_getNeighborEui64(&NeighborAddress, ADDR_64B, i)) { // this neighbor entry is in use
						if (NeighborAddress.addr_64b[7] == whisper_getTargetParentOld()){
							break;
						}
					}
		}

		memset(celllist_del,0,101*sizeof(cellInfo_ht));
		celllist_del[0].slotoffset       = 5;
		celllist_del[0].channeloffset    = 0x0A;
		celllist_del[0].isUsed           = TRUE;


		schedule_addActiveSlot(
				celllist_del[0].slotoffset,
				CELLTYPE_TXRX,
		    TRUE,
				celllist_del[0].channeloffset,
		    &NeighborAddress
		);

		for (i=0;i<MAXNUMNEIGHBORS;i++) {

		}

		whisper_setState(6);

		break;
	default:
		break;

	}

}

/**
\note timer fired, but we don't want to execute task in ISR mode instead, push
   task to scheduler with CoAP priority, and let scheduler take care of it.
*/
void whisper_timer_cb(opentimers_id_t id){

   scheduler_push_task(whisper_task_cb,TASKPRIO_COAP);
}

void whisper_timer_cb2(opentimers_id_t id){

   scheduler_push_task(whisper_task_cb2,TASKPRIO_COAP);
}

void whisper_timer_cb3(opentimers_id_t id){

   scheduler_push_task(whisper_task_cb3,TASKPRIO_COAP);
}

void whisper_timer_cb4(opentimers_id_t id){

   scheduler_push_task(whisper_task_cb4,TASKPRIO_COAP);
}

void whisper_task_cb() {
	   // will be run only in Whisper node

      if (state==1){

    	  whisper_trigger_nextStep();
    	 open_addr_t      NeighborAddress;
    	 uint8_t          i;

		 for (i=0;i<MAXNUMNEIGHBORS;i++) {
			if (neighbors_getNeighborEui64(&NeighborAddress, ADDR_64B, i)) { // this neighbor entry is in use
				if (NeighborAddress.addr_64b[7] == whisper_getTargetChildren()){

					 // call sixtop
					whisper_setFakeSource(whisper_getTargetParentNew());

					 sixtop_request(
						IANA_6TOP_CMD_ADD,                  // code
						&NeighborAddress,                   // neighbor
						1,                                  // number cells
						CELLOPTIONS_MSF,                    // cellOptions
						celllist_add,                       // celllist to add
						NULL,                               // celllist to delete (not used)
						5,                      		    // sfid 5, means whsiper
						0,                                  // list command offset (not used)
						0                                   // list command maximum celllist (not used)
					 );
					 break;
				}
			}
		 }
      }else{

			return;
      }
      whisper_setState(2);//state 2 Sending second 6P add

	  return;

}

void whisper_task_cb2() {

	 uint16_t		  rank;

	  if (state==2){

		  rank=((3*DEFAULTLINKCOST-2)*MINHOPRANKINCREASE)-1;
		  icmpv6rpl_sendWhisperDIO(whisper_getTargetChildren(),whisper_getTargetParentOld(),rank);
		  //icmpv6rpl_sendWhisperDIO(whisper_getTargetChildren(),whisper_getTargetParentOld(),2*rank);
	  }else{

		  return;
	  }
	  whisper_setState(0);	//3 state Sending DIOs

	  return;

}

void whisper_task_cb3() {
	   // will be run only in Whisper node

      if (state==3){

    	 open_addr_t      NeighborAddress;
    	 uint8_t          i;


		 for (i=0;i<MAXNUMNEIGHBORS;i++) {
			if (neighbors_getNeighborEui64(&NeighborAddress, ADDR_64B, i)) { // this neighbor entry is in use
				if (NeighborAddress.addr_64b[7] == whisper_getTargetChildren()){

					 // call sixtop
					whisper_setFakeSource(whisper_getTargetParentOld());

			        sixtop_request(
			            IANA_6TOP_CMD_CLEAR,                // code
						&NeighborAddress,                           // neighbor
			            NUMCELLS_MSF,                       // number cells
			            CELLOPTIONS_MSF,                    // cellOptions
			            NULL,                               // celllist to add (not used)
			            NULL,                               // celllist to delete (not used)
			            5,               // sfid
			            0,                                  // list command offset (not used)
			            0                                   // list command maximum celllist (not used)
			        );

					 break;
				}
			}
		 }
      }else{

			return;
      }
      whisper_setState(4);	//4 Sending first 6P clear

	  return;

}

void whisper_task_cb4() {
      // will be run only in Whisper node
      if (state==4){


    	 open_addr_t      NeighborAddress;
    	 uint8_t          i;

		 for (i=0;i<MAXNUMNEIGHBORS;i++) {
			if (neighbors_getNeighborEui64(&NeighborAddress, ADDR_64B, i)) { // this neighbor entry is in use
				if (NeighborAddress.addr_64b[7] == whisper_getTargetParentOld()){

					 // call sixtop

					whisper_setFakeSource(whisper_getTargetChildren());

				    sixtop_request(
				        IANA_6TOP_CMD_CLEAR,   // code
						&NeighborAddress,              // neighbor
				        NUMCELLS_MSF,           // number cells
				        CELLOPTIONS_MSF,        // cellOptions
				        NULL,                   // celllist to add (not used)
						NULL,        // celllist to delete
				        5,   // sfid
				        0,                      // list command offset (not used)
				        0                       // list command maximum celllist (not used)
				    );
					 break;
				}
			}
		 }
      }else{
			return;
      }

      whisper_setState(5);
	  opentimers_destroy(whisper_vars.timerIdWhisper);

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


