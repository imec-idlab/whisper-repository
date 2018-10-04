/**
DO NOT EDIT DIRECTLY!!

This file was 'objectified' by SCons as a pre-processing
step for the building a Python extension module.

This was done on 2018-03-08 19:14:20.068990.
*/
/**
\brief A CoAP resource which indicates the board its running on.
*/

#include "opendefs_obj.h"
#include "whisper_obj.h"
#include "opencoap_obj.h"
#include "openqueue_obj.h"
#include "packetfunctions_obj.h"
#include "openserial_obj.h"
#include "openrandom_obj.h"
#include "board_obj.h"
#include "idmanager_obj.h"
#include "msf_obj.h"
#include "neighbors_obj.h"
#include "schedule_obj.h"
#include "IEEE802154E_obj.h"
#include "scheduler_obj.h"
#include "sixtop_obj.h"

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

void whisper_timer_cb(OpenMote* self, opentimers_id_t id);
void whisper_task_cb(OpenMote* self);

void whisper_timer_cb2(OpenMote* self, opentimers_id_t id);
void whisper_task_cb2(OpenMote* self);

void whisper_timer_cb3(OpenMote* self, opentimers_id_t id);
void whisper_task_cb3(OpenMote* self);

void whisper_timer_cb4(OpenMote* self, opentimers_id_t id);
void whisper_task_cb4(OpenMote* self);

void whisper_task_remote(OpenMote* self, uint8_t* buf, uint8_t bufLen);

//bool whisper_candidateAddCellList(cellInfo_ht* cellList,uint8_t requiredCells);
//=========================== prototypes ======================================

owerror_t whisper_receive(OpenMote* self, 
   OpenQueueEntry_t* msg,
   coap_header_iht*  coap_header,
   coap_option_iht*  coap_options
);
void whisper_sendDone(OpenMote* self, 
   OpenQueueEntry_t* msg,
   owerror_t error
);

//=========================== public ==========================================


/**
\brief Initialize this module.
*/
void whisper_init(OpenMote* self) {




   // do not run if DAGroot
   //if( idmanager_getIsDAGroot(self)==TRUE) return;
   // do not run if not whisper node
   //if( idmanager_getMyID(self, ADDR_64B)->addr_64b[7] != 0x6b) return;
   if( idmanager_getMyID(self, ADDR_64B)->addr_64b[7] != 0x01) return;

 leds_debug_toggle(self);

   // start ONESHOT timer
//   whisper_vars.timerIdSixp = 0;
//   whisper_vars.timerIdSixp2 = 2;
//   whisper_vars.timerIdSixp3 = 3;
//   whisper_vars.timerIdRpl = 1;

   // prepare the resource descriptor for the /w path

 whisper_setState(self, 0);
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
  
   whisper_vars.timerIdWhisper = opentimers_create(self);

   // register with the CoAP module
 opencoap_register(self, &whisper_vars.desc);
}

void whisper_setSentDIOs(OpenMote* self, uint8_t i){
	numSentDios=i;
}

uint8_t whisper_getSentDIOs(OpenMote* self){
	return numSentDios;
}

void whisper_setState(OpenMote* self, uint8_t i){
	state=i;
}

uint8_t whisper_getState(OpenMote* self){
	return state;
}

uint8_t whisper_getTargetChildren(OpenMote* self){
	return targetChildren;
}

void whisper_setTargetChildren(OpenMote* self, uint8_t i){
	targetChildren=i;
}

uint8_t whisper_getTargetParentNew(OpenMote* self){
	return targetParentNew;
}

void whisper_setTargetParentNew(OpenMote* self, uint8_t i){
	targetParentNew=i;
}

uint8_t whisper_getTargetParentOld(OpenMote* self){
	return targetParentOld;
}

void whisper_setTargetParentOld(OpenMote* self, uint8_t i){
	targetParentOld=i;
}

uint8_t whisper_getFakeSource(OpenMote* self){
	return fakeSource;
}

void whisper_setFakeSource(OpenMote* self, uint8_t i){
	fakeSource=i;
}

void whisper_trigger_nextStep(OpenMote* self);

//
////sets the
//uint8_t whisper_getSeqNumOfNeigh(uint8_t n){
//	return sequenceNumber[n-1];
//}
//
//void whisper_setFakeSource(OpenMote* self, uint8_t n,uint8_t i){
//	sequenceNumber[n-1]=i;
//}
//=========================== private =========================================

/**
\brief Called when a CoAP message is received for this resource.

\param[in] msg          The received message. CoAP header and options already
   parsed.
\param[in] coap_header  The CoAP header contained in the message.
\param[in] coap_options The CoAP options contained in the message.

\return Whether the response is prepared successfully.
*/
owerror_t whisper_receive(OpenMote* self, 
      OpenQueueEntry_t* msg,
      coap_header_iht* coap_header,
      coap_option_iht* coap_options
   ) {
   uint8_t          i;
   owerror_t outcome;

   if ( msf_candidateAddCellList(self, celllist_add,1)==FALSE){
      // set the CoAP header
      outcome                       = E_FAIL;
      coap_header->Code             = COAP_CODE_RESP_CHANGED;
   }
 openrandom_get16b(self);
   open_addr_t      NeighborAddress;

/*   uint8_t whisper_sixp_neigh128[] = {*/
/*   	0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \*/
/*   	0x14, 0x15, 0x92, 0xcc, 0x00, 0x00, 0x00, 0x03*/
/*   };*/
   //whisper_newParent128[15]=fakedio_getTargetChildren();
   //memcpy(&(whisper_newParent128[15]),(self->icmpv6rpl_vars).newParent,1);
   //memcpy(&(self->icmpv6rpl_vars).dioFakeDestination.addr_128b[0],whisper_newParent128,sizeof(whisper_newParent128));

   switch (coap_header->Code) {
      case COAP_CODE_REQ_PUT:
        
		if (state==0){	//0 IDLE
			//printf("I am WHISPER node %d, received a COAP message to change parent of %d from %d to %d \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],msg->payload[1],msg->payload[2],msg->payload[3]);
				//sending FAKE DIO
			//if ( (msg->payload[0]==120) && (msg->payload[1]==4) && (msg->payload[2]==2) && (msg->payload[3]==3) ) { 	// ord('x')=120 , works only with node 4 changing from 2 to 3
			if ( msg->payload[0]==120) { 	// ord('x')=120 , works only with node 4 changing from 2 to 3

 whisper_setTargetChildren(self, msg->payload[1]);
 whisper_setTargetParentOld(self, msg->payload[2]);
 whisper_setTargetParentNew(self, msg->payload[3]);

				//printf("I am WISHPER node %d, First, adding cells between %d and its new parent %d  \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7], whisper_getTargetChildren(self), whisper_getTargetParentNew(self));
				//printf("I am WISHPER node %d, Sending DIO to 3. My rank is %d \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7], icmpv6rpl_getMyDAGrank(self));
				//first add cells in new parent
				//only used for set the roles, the dio is trigger over openvisualizer


//				 for (i=0;i<MAXNUMNEIGHBORS;i++) {
//					if ( neighbors_getNeighborEui64(self, &NeighborAddress, ADDR_64B, i)) { // this neighbor entry is in use
//						if (NeighborAddress.addr_64b[7] == whisper_getTargetParentNew(self)){
//							//printf("Node %d is my neighbor I am going to send him a Fake 6P message\n",NeighborAddress.addr_64b[7]);
// whisper_setFakeSource(self,  whisper_getTargetChildren(self));
//							//printf("with source address  %d\n", whisper_getFakeSource(self));
//							 // call sixtop
// leds_debug_toggle(self);
//							 outcome = sixtop_request(self, 
//								IANA_6TOP_CMD_ADD,                  // code
//								&NeighborAddress,                   // neighbor
//								1,                                  // number cells
//								CELLOPTIONS_MSF,                    // cellOptions
//								celllist_add,                       // celllist to add
//								NULL,                               // celllist to delete (not used)
//								5,                      		    // sfid 5, means whsiper
//								0,                                  // list command offset (not used)
//								0                                   // list command maximum celllist (not used)
//							 );
//							 i=100;
// whisper_setState(self, 1);//1 Sending first 6P add
//						}
//					}
//				 }
//				// whisper_setState(self, 1);
// whisper_trigger_nextStep(self);

			}else{
				//skipping 6P
//				if ( msg->payload[0]==122) {
// whisper_setState(self, 2);
// whisper_trigger_nextStep(self);
//				}

				//printf("I am WISHPER node %d, end operation (bad Coap request: format -> x,source,oldParent,newParent)\n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
			    outcome                       = E_FAIL;
			    coap_header->Code             = COAP_CODE_RESP_CHANGED;
 whisper_setState(self, 0);	//back to IDLE
				break;
			}
		}
//		else{
//			printf("I am WISHPER node %d, received COAP message, not idle. State = %d \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],state);
//		}
 whisper_setState(self, 0);
		//printf("Done COAP\n");
        //=== reset packet payload (we will reuse this packetBuffer)
        msg->payload                     = &(msg->packet[127]);
        msg->length                      = 0;
                 
        // set the CoAP header
        coap_header->Code                = COAP_CODE_RESP_CONTENT;
         
        outcome                          = E_SUCCESS;
        break;
      default:
    	 //printf("I am WISHPER node %d, end operation (no cells in first 6P message)\n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
         // return an error message
         outcome = E_FAIL;
   }
   
   return outcome;
}


void whisper_trigger_nextStep(OpenMote* self){

	//bool          res=0;
	open_addr_t      NeighborAddress;
	uint8_t          i;
	cellInfo_ht   celllist_del[1];

	//printf("I am WHISPER node %d, next STEP \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
	switch ( whisper_getState(self)){
	case 1: //another 6P add
		//printf("I am WHISPER node %d, next STEP is send second 6P add timer id %d \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],whisper_vars.timerIdWhisper);
 leds_debug_toggle(self);
		 //whisper_vars.timerIdSixp = opentimers_create(self);
 opentimers_scheduleIn(self, 
			   whisper_vars.timerIdWhisper,
			   10000,
			   TIME_MS,
			   TIMER_ONESHOT,
			   whisper_timer_cb
		   );
		   break;
	case 2:	//send RPL fake DIOs
 leds_debug_toggle(self);
		//whisper_vars.timerIdRpl = opentimers_create(self);
		//printf("I am WHISPER node %d, NEXT step starting RPL timer id %d \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7], whisper_vars.timerIdWhisper);

 opentimers_scheduleIn(self, 
			   whisper_vars.timerIdWhisper,
			   10000,
			   TIME_MS,
			   TIMER_ONESHOT,
			   whisper_timer_cb2
		   );
		  break;
	case 3: //first 6P clear
		//whisper_vars.timerIdSixp2 = opentimers_create(self);
		//printf("I am WHISPER node %d, next STEP clear 1 time id %d \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],whisper_vars.timerIdWhisper);
 leds_debug_toggle(self);

		//no Clearing for the moment
 whisper_setState(self, 0);
// opentimers_scheduleIn(self, 
//			   whisper_vars.timerIdWhisper,
//			   10000,
//			   TIME_MS,
//			   TIMER_ONESHOT,
//			   whisper_timer_cb3
//		   );


		   break;
	case 4: //another 6P clear
		//whisper_vars.timerIdSixp3 = opentimers_create(self);
	     //printf("I am WHISPER node %d, next STEP clear 2 timer id %d\n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],whisper_vars.timerIdWhisper);

 opentimers_scheduleIn(self, 
			   whisper_vars.timerIdWhisper,
			   10000,
			   TIME_MS,
			   TIMER_ONESHOT,
			   whisper_timer_cb4
		   );
		   break;
	case 5:

		//printf("I am WHISPER node %d, FINISHED, but first I add 1 cell to fake-ack!! \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
		for (i=0;i<MAXNUMNEIGHBORS;i++) {
					if ( neighbors_getNeighborEui64(self, &NeighborAddress, ADDR_64B, i)) { // this neighbor entry is in use
						if (NeighborAddress.addr_64b[7] == whisper_getTargetParentOld(self)){
							//printf("Node %d is my neighbor I am going to enable a fake cell \n",NeighborAddress.addr_64b[7]);
							break;
						}
					}
		}

		memset(celllist_del,0,101*sizeof(cellInfo_ht));
		celllist_del[0].slotoffset       = 5;
		celllist_del[0].channeloffset    = 0x0A;
		celllist_del[0].isUsed           = TRUE;

//        res= sixtop_addCells(self, 
//            0,
//			celllist_del,
//            &NeighborAddress,
//			CELLOPTIONS_MSF
//        );

 schedule_addActiveSlot(self, 
        		celllist_del[0].slotoffset,
			CELLTYPE_TXRX,
            TRUE,
			celllist_del[0].channeloffset,
            &NeighborAddress
        );

        for (i=0;i<MAXNUMNEIGHBORS;i++) {

        }


//        if (res){
//        	printf("I am WHISPER node %d, Adding fake cell ok \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
//        }else{
//        	printf("I am WHISPER node %d, Adding fake cell NOK \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
//        }
 whisper_setState(self, 6);
		// whisper_setState(self, 0); //0 IDLE
		break;
	default:
		break;
		//
		//printf("ERROR\n");
	}

}

/**
\note timer fired, but we don't want to execute task in ISR mode instead, push
   task to scheduler with CoAP priority, and let scheduler take care of it.
*/
void whisper_timer_cb(OpenMote* self, opentimers_id_t id){

 scheduler_push_task(self, whisper_task_cb,TASKPRIO_COAP);
}

void whisper_timer_cb2(OpenMote* self, opentimers_id_t id){

 scheduler_push_task(self, whisper_task_cb2,TASKPRIO_COAP);
}

void whisper_timer_cb3(OpenMote* self, opentimers_id_t id){

 scheduler_push_task(self, whisper_task_cb3,TASKPRIO_COAP);
}

void whisper_timer_cb4(OpenMote* self, opentimers_id_t id){

 scheduler_push_task(self, whisper_task_cb4,TASKPRIO_COAP);
}

void whisper_task_cb(OpenMote* self) {
	   // run only in Whisper node

      //printf("I am WHISPER node %d, timer has expired!! Sending second 6P message \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);

      if (state==1){

 whisper_trigger_nextStep(self);
    	 open_addr_t      NeighborAddress;
    	 uint8_t          i;

//    	  if ( msf_candidateAddCellList(self, celllist_add,1)==FALSE){
//
//    	      printf("I am WHISPER node %d, end operation (no cells in second 6P message)\n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
// whisper_setState(self, 0);
//
// opentimers_destroy(self, whisper_vars.timerIdWhisper);
//    		  return;
//    	  }
    	 //printf("I am WHISPER node %d, I have some cells \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
		 for (i=0;i<MAXNUMNEIGHBORS;i++) {
			if ( neighbors_getNeighborEui64(self, &NeighborAddress, ADDR_64B, i)) { // this neighbor entry is in use
				if (NeighborAddress.addr_64b[7] == whisper_getTargetChildren(self)){
			//		printf("Node %d is my neighbor I am going to send him a Fake 6P message\n",NeighborAddress.addr_64b[7]);
					 // call sixtop
 whisper_setFakeSource(self,  whisper_getTargetParentNew(self));
				//	printf("with source address  %d\n", whisper_getFakeSource(self));
 sixtop_request(self, 
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
//				else{
//					printf("I am WISHPER node %d, node %d is not my my target children %d \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],NeighborAddress.addr_64b[7], whisper_getTargetChildren(self));
//				}
			}
		 }
      }else{
			//printf("I am WISHPER node %d, wrong state in second 6P message. State = %d \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],state);
			return;
      }
 whisper_setState(self, 2);//2 Sending second 6P add
      //printf("I am WISHPER node %d, end operation\n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
      // whisper_trigger_nextStep(self);

	  // opentimers_destroy(self, whisper_vars.timerIdSixp);
	  return;

}

void whisper_task_cb2(OpenMote* self) {

	 //printf("I am WHISPER node %d, RPL timer has expired!! Doing some RPL tasks \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
	 uint16_t		  rank;

	  if (state==2){

		  rank=((3*DEFAULTLINKCOST-2)*MINHOPRANKINCREASE)-1;
 icmpv6rpl_sendWhisperDIO(self, whisper_getTargetChildren(self), whisper_getTargetParentOld(self),rank);
		//  printf("Sending 2nd DIO\n");
 icmpv6rpl_sendWhisperDIO(self, whisper_getTargetChildren(self), whisper_getTargetParentOld(self),2*rank);
	  }else{
		 // printf("I am WISHPER node %d, wrong state in sending RPL DIOs. State = %d \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],state);
		  return;
	  }
 whisper_setState(self, 0);	//3 Sending DIOs

	  //printf("I am WHISPER node %d, preparing next step, send frist 6p clear \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
	  // opentimers_destroy(self, whisper_vars.timerIdRpl);
	  // whisper_trigger_nextStep(self);
	  // whisper_trigger_nextStep(self);

	  return;

}

void whisper_task_cb3(OpenMote* self) {
	   // run only in Whisper node

      //printf("I am WHISPER node %d, timer has expired!! Sending first 6P clear message \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);

      if (state==3){
    	  // whisper_setState(self, 4);
    	  // whisper_trigger_nextStep(self);
    	 open_addr_t      NeighborAddress;
    	 uint8_t          i;

//    	  if ( msf_candidateAddCellList(self, celllist_add,1)==FALSE){
//
//    	      printf("I am WISHPER node %d, end operation (no cells in second 6P message)\n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
// whisper_setState(self, 0);
//
// opentimers_destroy(self, whisper_vars.timerIdSixp);
//    		  return;
//    	  }
    	 //printf("I am WHISPER node %d, I have some cells \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
		 for (i=0;i<MAXNUMNEIGHBORS;i++) {
			if ( neighbors_getNeighborEui64(self, &NeighborAddress, ADDR_64B, i)) { // this neighbor entry is in use
				if (NeighborAddress.addr_64b[7] == whisper_getTargetChildren(self)){
			//		printf("Node %d is my neighbor I am going to send him a Fake 6P message\n",NeighborAddress.addr_64b[7]);
					 // call sixtop
 whisper_setFakeSource(self,  whisper_getTargetParentOld(self));
				//	printf("with source address  %d\n", whisper_getFakeSource(self));
 sixtop_request(self, 
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
			//printf("I am WISHPER node %d, wrong state in first 6P clear. State = %d \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],state);
			return;
      }
 whisper_setState(self, 4);	//4 Sending first 6P clear
      //printf("I am WISHPER node %d, prepare next step, last 6p clear\n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
      // whisper_trigger_nextStep(self);

	  // opentimers_destroy(self, whisper_vars.timerIdSixp2);
	  return;

}

void whisper_task_cb4(OpenMote* self) {
	   // run only in Whisper node

      //printf("I am WHISPER node %d, timer has expired!! Sending second 6P clear message \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
      //bool          res=0;
      if (state==4){
    	//  printf("I am WHISPER node %d, DONE! \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);

    	 open_addr_t      NeighborAddress;
    	 uint8_t          i;

//    	  if ( msf_candidateAddCellList(self, celllist_add,1)==FALSE){
//
//    	      printf("I am WISHPER node %d, end operation (no cells in second 6P message)\n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
// whisper_setState(self, 0);
//
// opentimers_destroy(self, whisper_vars.timerIdSixp);
//    		  return;
//    	  }
    	 //printf("I am WHISPER node %d, I have some cells \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);
		 for (i=0;i<MAXNUMNEIGHBORS;i++) {
			if ( neighbors_getNeighborEui64(self, &NeighborAddress, ADDR_64B, i)) { // this neighbor entry is in use
				if (NeighborAddress.addr_64b[7] == whisper_getTargetParentOld(self)){
					//printf("Node %d is my neighbor I am going to send him a Fake 6P message\n",NeighborAddress.addr_64b[7]);
					 // call sixtop




 whisper_setFakeSource(self,  whisper_getTargetChildren(self));
					//printf("with source address  %d\n", whisper_getFakeSource(self));
// sixtop_request(self, 
//				            IANA_6TOP_CMD_CLEAR,                // code
//							&NeighborAddress,                           // neighbor
//				            NUMCELLS_MSF,                       // number cells
//				            CELLOPTIONS_MSF,                    // cellOptions
//				            NULL,                               // celllist to add (not used)
//				            NULL,                               // celllist to delete (not used)
//				            5,               // sfid
//				            0,                                  // list command offset (not used)
//				            0                                   // list command maximum celllist (not used)
//				        );
 sixtop_request(self, 
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
			//printf("I am WISHPER node %d, wrong state in second 6P clear message. State = %d \n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7],state);
			return;
      }

      //printf("I am WISHPER node %d, end TOTAL operation\n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);

 whisper_setState(self, 5);
 opentimers_destroy(self, whisper_vars.timerIdWhisper);

	  return;

}


void whisper_task_remote(OpenMote* self, uint8_t* buf, uint8_t bufLen){

	printf("I am WISHPER node %d, sending FAKE REMOTE DIO\n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);


	if (buf[1]==85){
 whisper_setTargetChildren(self, 0x36);
 whisper_setTargetParentOld(self, 0x55);
 whisper_setTargetParentNew(self, 0xac);
 leds_debug_toggle(self);

	}else{
 whisper_setTargetChildren(self, 0x36);
 whisper_setTargetParentOld(self, 0xac);
 whisper_setTargetParentNew(self, 0x55);
 leds_debug_toggle(self);
 leds_error_toggle(self);


	}
	//printf("I am WISHPER node %d, sending FAKE REMOTE DIO\n", idmanager_getMyID(self, ADDR_64B)->addr_64b[7]);

	 uint16_t		  rank;



	  rank=((3*DEFAULTLINKCOST-2)*MINHOPRANKINCREASE)-1;
 icmpv6rpl_sendWhisperDIO(self, whisper_getTargetChildren(self), whisper_getTargetParentOld(self),rank);

 icmpv6rpl_sendWhisperDIO(self, whisper_getTargetChildren(self), whisper_getTargetParentOld(self),2*rank);

 whisper_setState(self, 0);	//3 Sending DIOs

	return;

}


/**
\brief The stack indicates that the packet was sent.

\param[in] msg The CoAP message just sent.
\param[in] error The outcome of sending it.
*/
void whisper_sendDone(OpenMote* self, OpenQueueEntry_t* msg, owerror_t error) {


 openqueue_freePacketBuffer(self, msg);
}

//bool whisper_candidateAddCellList(
//      cellInfo_ht* cellList,
//      uint8_t      requiredCells
//   ){
////    uint8_t i;
////    frameLength_t slotoffset;
////    uint8_t numCandCells;
// schedule_isSlotOffsetAvailable(self, 5);
//    //memset(cellList,0,CELLLIST_MAX_LEN*sizeof(cellInfo_ht));
////    numCandCells=0;
////    for(i=0;i<CELLLIST_MAX_LEN;i++){
////        //slotoffset = openrandom_get16b(self)% schedule_getFrameLength(self);
////        if( schedule_isSlotOffsetAvailable(self, slotoffset)==TRUE){
////            cellList[numCandCells].slotoffset       = slotoffset;
////            cellList[numCandCells].channeloffset    = openrandom_get16b(self)&0x0F;
////            cellList[numCandCells].isUsed           = TRUE;
////            numCandCells++;
////        }
////    }
////
////    if (numCandCells<requiredCells || requiredCells==0) {
////        return FALSE;
////    } else {
////        return TRUE;
////    }
//    return TRUE;
//}