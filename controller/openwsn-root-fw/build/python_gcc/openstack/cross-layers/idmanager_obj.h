/**
DO NOT EDIT DIRECTLY!!

This file was 'objectified' by SCons as a pre-processing
step for the building a Python extension module.

This was done on 2018-03-08 18:25:47.752860.
*/
#ifndef __IDMANAGER_H
#define __IDMANAGER_H

/**
\addtogroup cross-layers
\{
\addtogroup IDManager
\{
*/

#include "Python.h"

#include "opendefs_obj.h"

//=========================== define ==========================================

#define ACTION_YES      'Y'
#define ACTION_NO       'N'
#define ACTION_TOGGLE   'T'

static const uint8_t linklocalprefix[] = {
   0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

//=========================== typedef =========================================

BEGIN_PACK

typedef struct {
   bool          isDAGroot;
   uint8_t       myPANID[2];
   uint8_t       my16bID[2];
   uint8_t       my64bID[8];
   uint8_t       myPrefix[8];
} debugIDManagerEntry_t;

END_PACK

//=========================== module variables ================================

typedef struct {
   bool          isDAGroot;
   open_addr_t   myPANID;
   open_addr_t   my16bID;
   open_addr_t   my64bID;
   open_addr_t   myPrefix;
   bool          slotSkip;
   uint8_t       joinKey[16];
   asn_t         joinAsn;
} idmanager_vars_t;

#include "openwsnmodule_obj.h"
typedef struct OpenMote OpenMote;

//=========================== prototypes ======================================

void idmanager_init(OpenMote* self);
bool idmanager_getIsDAGroot(OpenMote* self);
void idmanager_setIsDAGroot(OpenMote* self, bool newRole);
bool idmanager_getIsSlotSkip(OpenMote* self);
open_addr_t* idmanager_getMyID(OpenMote* self, uint8_t type);
owerror_t idmanager_setMyID(OpenMote* self, open_addr_t* newID);
bool idmanager_isMyAddress(OpenMote* self, open_addr_t* addr);
void idmanager_triggerAboutRoot(OpenMote* self);
void idmanager_setJoinKey(OpenMote* self, uint8_t *key);
void idmanager_setJoinAsn(OpenMote* self, asn_t *asn);
void idmanager_getJoinKey(OpenMote* self, uint8_t **pKey);

bool debugPrint_id(OpenMote* self);
bool debugPrint_joined(OpenMote* self);
/**
\}
\}
*/

#endif
