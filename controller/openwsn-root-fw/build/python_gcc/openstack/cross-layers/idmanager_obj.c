/**
DO NOT EDIT DIRECTLY!!

This file was 'objectified' by SCons as a pre-processing
step for the building a Python extension module.

This was done on 2018-03-08 18:27:27.288239.
*/
#include "opendefs_obj.h"
#include "idmanager_obj.h"
#include "eui64_obj.h"
#include "packetfunctions_obj.h"
#include "openserial_obj.h"
#include "neighbors_obj.h"
#include "schedule_obj.h"
#include "IEEE802154_security_obj.h"

//=========================== variables =======================================

// declaration of global variable _idmanager_vars_ removed during objectification.

//=========================== prototypes ======================================

//=========================== public ==========================================

void idmanager_init(OpenMote* self) {
   
   // reset local variables
   memset(&(self->idmanager_vars), 0, sizeof(idmanager_vars_t));
   // this is used to not wakeup in non-activeslot
   (self->idmanager_vars).slotSkip             = FALSE;
   
   // isDAGroot
#ifdef DAGROOT
   (self->idmanager_vars).isDAGroot            = TRUE;
#else
   (self->idmanager_vars).isDAGroot            = FALSE;
#endif
   
   // myPANID
   (self->idmanager_vars).myPANID.type         = ADDR_PANID;
#ifdef PANID_DEFINED 
   (self->idmanager_vars).myPANID.panid[0]     = PANID_DEFINED & 0x00ff;
   (self->idmanager_vars).myPANID.panid[1]     =(PANID_DEFINED & 0xff00)>>8;
#else
   (self->idmanager_vars).myPANID.panid[0]     = 0xca;
   (self->idmanager_vars).myPANID.panid[1]     = 0xfe;
#endif
   // myPrefix
   (self->idmanager_vars).myPrefix.type        = ADDR_PREFIX;
#ifdef DAGROOT
   (self->idmanager_vars).myPrefix.prefix[0]   = 0xbb;
   (self->idmanager_vars).myPrefix.prefix[1]   = 0xbb;
   (self->idmanager_vars).myPrefix.prefix[2]   = 0x00;
   (self->idmanager_vars).myPrefix.prefix[3]   = 0x00;
   (self->idmanager_vars).myPrefix.prefix[4]   = 0x00;
   (self->idmanager_vars).myPrefix.prefix[5]   = 0x00;
   (self->idmanager_vars).myPrefix.prefix[6]   = 0x00;
   (self->idmanager_vars).myPrefix.prefix[7]   = 0x00;
#else
   // set prefix to link-local
   (self->idmanager_vars).myPrefix.prefix[0]   = 0xfe;
   (self->idmanager_vars).myPrefix.prefix[1]   = 0x80;
   (self->idmanager_vars).myPrefix.prefix[2]   = 0x00;
   (self->idmanager_vars).myPrefix.prefix[3]   = 0x00;
   (self->idmanager_vars).myPrefix.prefix[4]   = 0x00;
   (self->idmanager_vars).myPrefix.prefix[5]   = 0x00;
   (self->idmanager_vars).myPrefix.prefix[6]   = 0x00;
   (self->idmanager_vars).myPrefix.prefix[7]   = 0x00;
#endif
   
   // my64bID
   (self->idmanager_vars).my64bID.type         = ADDR_64B;
 eui64_get(self, (self->idmanager_vars).my64bID.addr_64b);
   
   // my16bID
 packetfunctions_mac64bToMac16b(self, &(self->idmanager_vars).my64bID,&(self->idmanager_vars).my16bID);
}

bool idmanager_getIsDAGroot(OpenMote* self) {
   bool res;
   INTERRUPT_DECLARATION();
   
   DISABLE_INTERRUPTS();
   res=(self->idmanager_vars).isDAGroot;
   ENABLE_INTERRUPTS();
   return res;
}

void idmanager_setIsDAGroot(OpenMote* self, bool newRole) {
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();
   (self->idmanager_vars).isDAGroot = newRole;
 icmpv6rpl_updateMyDAGrankAndParentSelection(self);
 schedule_startDAGroot(self);
   ENABLE_INTERRUPTS();
}

bool idmanager_getIsSlotSkip(OpenMote* self) {
   bool res;
   INTERRUPT_DECLARATION();
   
   DISABLE_INTERRUPTS();
   res=(self->idmanager_vars).slotSkip;
   ENABLE_INTERRUPTS();
   return res;
}

open_addr_t* idmanager_getMyID(OpenMote* self, uint8_t type) {
   open_addr_t* res;
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();
   switch (type) {
     case ADDR_16B:
        res= &(self->idmanager_vars).my16bID;
        break;
     case ADDR_64B:
        res= &(self->idmanager_vars).my64bID;
        break;
     case ADDR_PANID:
        res= &(self->idmanager_vars).myPANID;
        break;
     case ADDR_PREFIX:
        res= &(self->idmanager_vars).myPrefix;
        break;
     case ADDR_128B:
        // you don't ask for my full address, rather for prefix, then 64b
     default:
 openserial_printCritical(self, COMPONENT_IDMANAGER,ERR_WRONG_ADDR_TYPE,
              (errorparameter_t)type,
              (errorparameter_t)0);
        res= NULL;
        break;
   }
   ENABLE_INTERRUPTS();
   return res;
}

owerror_t idmanager_setMyID(OpenMote* self, open_addr_t* newID) {
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();
   switch (newID->type) {
     case ADDR_16B:
        memcpy(&(self->idmanager_vars).my16bID,newID,sizeof(open_addr_t));
        break;
     case ADDR_64B:
        memcpy(&(self->idmanager_vars).my64bID,newID,sizeof(open_addr_t));
        break;
     case ADDR_PANID:
        memcpy(&(self->idmanager_vars).myPANID,newID,sizeof(open_addr_t));
        break;
     case ADDR_PREFIX:
        memcpy(&(self->idmanager_vars).myPrefix,newID,sizeof(open_addr_t));
        break;
     case ADDR_128B:
        //don't set 128b, but rather prefix and 64b
     default:
 openserial_printCritical(self, COMPONENT_IDMANAGER,ERR_WRONG_ADDR_TYPE,
              (errorparameter_t)newID->type,
              (errorparameter_t)1);
        ENABLE_INTERRUPTS();
        return E_FAIL;
   }
   ENABLE_INTERRUPTS();
   return E_SUCCESS;
}

bool idmanager_isMyAddress(OpenMote* self, open_addr_t* addr) {
   open_addr_t temp_my128bID;
   bool res;
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();

   switch (addr->type) {
     case ADDR_16B:
        res= packetfunctions_sameAddress(self, addr,&(self->idmanager_vars).my16bID);
        ENABLE_INTERRUPTS();
        return res;
     case ADDR_64B:
        res= packetfunctions_sameAddress(self, addr,&(self->idmanager_vars).my64bID);
        ENABLE_INTERRUPTS();
        return res;
     case ADDR_128B:
        // build temporary my128bID
        temp_my128bID.type = ADDR_128B;
        memcpy(&temp_my128bID.addr_128b[0],&(self->idmanager_vars).myPrefix.prefix,8);
        memcpy(&temp_my128bID.addr_128b[8],&(self->idmanager_vars).my64bID.addr_64b,8);

        res= packetfunctions_sameAddress(self, addr,&temp_my128bID);
        ENABLE_INTERRUPTS();
        return res;
     case ADDR_PANID:
        res= packetfunctions_sameAddress(self, addr,&(self->idmanager_vars).myPANID);
        ENABLE_INTERRUPTS();
        return res;
     case ADDR_PREFIX:
        res= packetfunctions_sameAddress(self, addr,&(self->idmanager_vars).myPrefix);
        ENABLE_INTERRUPTS();
        return res;
     default:
 openserial_printCritical(self, COMPONENT_IDMANAGER,ERR_WRONG_ADDR_TYPE,
              (errorparameter_t)addr->type,
              (errorparameter_t)2);
        ENABLE_INTERRUPTS();
        return FALSE;
   }
}

void idmanager_triggerAboutRoot(OpenMote* self) {
   uint8_t         number_bytes_from_input_buffer;
   uint8_t         input_buffer[1+8+1+16];
   open_addr_t     myPrefix;
   uint8_t         dodagid[16];
   uint8_t         keyIndex;
   uint8_t*        keyValue;
   
   //=== get command from OpenSerial
   number_bytes_from_input_buffer = openserial_getInputBuffer(self, input_buffer,sizeof(input_buffer));
   if (number_bytes_from_input_buffer!=sizeof(input_buffer)) {
 openserial_printError(self, COMPONENT_IDMANAGER,ERR_INPUTBUFFER_LENGTH,
            (errorparameter_t)number_bytes_from_input_buffer,
            (errorparameter_t)0);
      return;
   };
   
   //=== handle command
   
   // take action (byte 0)
   switch (input_buffer[0]) {
     case ACTION_YES:
 idmanager_setIsDAGroot(self, TRUE);
        (self->idmanager_vars).slotSkip = FALSE;
        break;
     case ACTION_NO:
 idmanager_setIsDAGroot(self, FALSE);
        (self->idmanager_vars).slotSkip = TRUE;
        break;
     case ACTION_TOGGLE:
        if ( idmanager_getIsDAGroot(self)) {
 idmanager_setIsDAGroot(self, FALSE);
           (self->idmanager_vars).slotSkip = TRUE;
        } else {
 idmanager_setIsDAGroot(self, TRUE);
           (self->idmanager_vars).slotSkip = FALSE;
        }
        break;
   }
   
   // store prefix (bytes 1-8)
   myPrefix.type = ADDR_PREFIX;
   memcpy(
      myPrefix.prefix,
      &input_buffer[1],
      sizeof(myPrefix.prefix)
   );
 idmanager_setMyID(self, &myPrefix);
  
   // indicate DODAGid to RPL
   memcpy(&dodagid[0],(self->idmanager_vars).myPrefix.prefix,8);  // prefix
   memcpy(&dodagid[8],(self->idmanager_vars).my64bID.addr_64b,8); // eui64
 icmpv6rpl_writeDODAGid(self, dodagid);

   // store L2 security key index and key value
   // for the moment, keys K1 and K2 are the same
   keyIndex = input_buffer[9];
   keyValue = &input_buffer[10];
 IEEE802154_security_setBeaconKey(self, keyIndex, keyValue);
 IEEE802154_security_setDataKey(self, keyIndex, keyValue);

   return;
}

void idmanager_setJoinKey(OpenMote* self, uint8_t *key) {
    memcpy((self->idmanager_vars).joinKey, key, 16);
}

void idmanager_getJoinKey(OpenMote* self, uint8_t **pKey) {
    *pKey = (self->idmanager_vars).joinKey;
    return; 
}

void idmanager_setJoinAsn(OpenMote* self, asn_t* asn) {
    memcpy(&(self->idmanager_vars).joinAsn, asn, sizeof(asn_t));
}

/**
\brief Trigger this module to print status information, over serial.

debugPrint_* functions are used by the openserial module to continuously print
status information about several modules in the OpenWSN stack.

\returns TRUE if this function printed something, FALSE otherwise.
*/
bool debugPrint_id(OpenMote* self) {
   debugIDManagerEntry_t output;
   
   output.isDAGroot = (self->idmanager_vars).isDAGroot;
   memcpy(output.myPANID,(self->idmanager_vars).myPANID.panid,2);
   memcpy(output.my16bID,(self->idmanager_vars).my16bID.addr_16b,2);
   memcpy(output.my64bID,(self->idmanager_vars).my64bID.addr_64b,8);
   memcpy(output.myPrefix,(self->idmanager_vars).myPrefix.prefix,8);
   
 openserial_printStatus(self, STATUS_ID,(uint8_t*)&output,sizeof(debugIDManagerEntry_t));
   return TRUE;
}

bool debugPrint_joined(OpenMote* self) {
   asn_t output;
   output.byte4         =  (self->idmanager_vars).joinAsn.byte4;
   output.bytes2and3    =  (self->idmanager_vars).joinAsn.bytes2and3;
   output.bytes0and1    =  (self->idmanager_vars).joinAsn.bytes0and1;
 openserial_printStatus(self, STATUS_JOINED,(uint8_t*)&output,sizeof(output));
   return TRUE;
}

//=========================== private =========================================
