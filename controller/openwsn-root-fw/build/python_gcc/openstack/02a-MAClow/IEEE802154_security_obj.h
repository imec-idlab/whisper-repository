/**
DO NOT EDIT DIRECTLY!!

This file was 'objectified' by SCons as a pre-processing
step for the building a Python extension module.

This was done on 2018-03-08 18:25:43.277711.
*/
/**
\brief Security operations defined by IEEE802.15.4 standard

\author Savio Sciancalepore <savio.sciancalepore@poliba.it>, June 2015.
\author Giuseppe Piro <giuseppe.piro@poliba.it>, June 2015
\author Gennaro Boggia <gennaro.boggia@poliba.it>, June 2015
\author Luigi Alfredo Grieco <alfredo.grieco@poliba.it>, June 2015
\author Malisa Vucinic <malishav@gmail.com>, June 2015.
*/

#ifndef __IEEE802154_SECURITY_H
#define __IEEE802154_SECURITY_H

/**
\addtogroup helpers
\{
\addtogroup IEEE802154
\{
*/

#include "Python.h"

#include "opendefs_obj.h"
#include "IEEE802154_obj.h"

//=========================== define ==========================================

#ifdef L2_SECURITY_ACTIVE  // Configuring security levels
#define IEEE802154_SECURITY_SUPPORTED        1
#define IEEE802154_SECURITY_LEVEL            IEEE154_ASH_SLF_TYPE_ENC_MIC_32  // encryption + 4 byte authentication tag   
#define IEEE802154_SECURITY_LEVEL_BEACON     IEEE154_ASH_SLF_TYPE_MIC_32      // authentication tag len used for beacons must match the tag len of other frames 
#define IEEE802154_SECURITY_KEYIDMODE        IEEE154_ASH_KEYIDMODE_DEFAULTKEYSOURCE
#define IEEE802154_SECURITY_TAG_LEN          IEEE802154_security_authLengthChecking(IEEE802154_SECURITY_LEVEL)
#define IEEE802154_SECURITY_HEADER_LEN       IEEE802154_security_auxLengthChecking(IEEE802154_SECURITY_KEYIDMODE, IEEE154_ASH_FRAMECOUNTER_SUPPRESSED, 0) // For TSCH we always use implicit 5 byte ASN as Frame Counter
#define IEEE802154_SECURITY_TOTAL_OVERHEAD   IEEE802154_SECURITY_TAG_LEN + IEEE802154_SECURITY_HEADER_LEN
#else /* L2_SECURITY_ACTIVE */
#define IEEE802154_SECURITY_SUPPORTED        0
#define IEEE802154_SECURITY_LEVEL            IEEE154_ASH_SLF_TYPE_NOSEC 
#define IEEE802154_SECURITY_LEVEL_BEACON     IEEE154_ASH_SLF_TYPE_NOSEC 
#define IEEE802154_SECURITY_KEYIDMODE        0
#define IEEE802154_SECURITY_TAG_LEN          0
#define IEEE802154_SECURITY_HEADER_LEN       0
#define IEEE802154_SECURITY_TOTAL_OVERHEAD   0
#endif /* L2_SECURITY_ACTIVE */

#define IEEE802154_SECURITY_KEYINDEX_INVALID 0
//=========================== typedef =========================================

typedef struct{
    uint8_t index;
    uint8_t value[16];
} symmetric_key_802154_t;

//=========================== variables =======================================

typedef struct {
   bool                    dynamicKeying;
   bool                    joinPermitted;
   symmetric_key_802154_t  k1;
   symmetric_key_802154_t  k2;
} ieee802154_security_vars_t;

#include "openwsnmodule_obj.h"
typedef struct OpenMote OpenMote;

//=========================== prototypes ======================================

void IEEE802154_security_init(OpenMote* self);
void IEEE802154_security_prependAuxiliarySecurityHeader(OpenMote* self, OpenQueueEntry_t* msg);
void IEEE802154_security_retrieveAuxiliarySecurityHeader(OpenMote* self, OpenQueueEntry_t* msg, ieee802154_header_iht* tempheader);
owerror_t IEEE802154_security_outgoingFrameSecurity(OpenMote* self, OpenQueueEntry_t* msg);
owerror_t IEEE802154_security_incomingFrame(OpenMote* self, OpenQueueEntry_t* msg);
uint8_t     IEEE802154_security_authLengthChecking(uint8_t securityLevel);
uint8_t     IEEE802154_security_auxLengthChecking(uint8_t keyIdMode, uint8_t frameCounterSuppression, uint8_t frameCounterSize);
uint8_t IEEE802154_security_getBeaconKeyIndex(OpenMote* self);
uint8_t IEEE802154_security_getDataKeyIndex(OpenMote* self);
void IEEE802154_security_setBeaconKey(OpenMote* self, uint8_t index, uint8_t* value);
void IEEE802154_security_setDataKey(OpenMote* self, uint8_t index, uint8_t* value);
uint8_t IEEE802154_security_getSecurityLevel(OpenMote* self, OpenQueueEntry_t *msg);
bool IEEE802154_security_acceptableLevel(OpenMote* self, OpenQueueEntry_t* msg, ieee802154_header_iht* parsedHeader);
bool IEEE802154_security_isConfigured(OpenMote* self);
void IEEE802154_security_setDynamicKeying(OpenMote* self);


/**
\}
\}
*/

#endif
