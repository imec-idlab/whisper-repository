/**
DO NOT EDIT DIRECTLY!!

This file was 'objectified' by SCons as a pre-processing
step for the building a Python extension module.

This was done on 2018-03-08 18:27:34.206481.
*/
/**
\brief Applications running on top of the OpenWSN stack.

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, September 2014.
*/

#include "opendefs_obj.h"

// CoAP
#include "opencoap_obj.h"
#include "c6t_obj.h"
#include "cinfo_obj.h"
#include "cleds_obj.h"
#include "cjoin_obj.h"
#include "cwellknown_obj.h"
#include "rrt_obj.h"
// UDP
#include "uecho_obj.h"
#include "uinject_obj.h"
#include "userialbridge_obj.h"
#include "uexpiration_obj.h"
#include "uexpiration_monitor_obj.h"
#include "whisper_obj.h"
#include "whispersender_obj.h"

//=========================== variables =======================================

//=========================== prototypes ======================================

//=========================== public ==========================================

//=========================== private =========================================

void openapps_init(OpenMote* self) {
   //-- 04-TRAN
 opencoap_init(self);     // initialize before any of the CoAP applications
   
   // CoAP
   // c6t_init(self);
 cinfo_init(self);
 cleds__init(self);
   // cjoin_init(self);
 cwellknown_init(self);
   // rrt_init(self);
 whisper_init(self);
 whispersender_init(self);
   // UDP
   // uecho_init(self);
   // uinject_init(self);
   // userialbridge_init(self);
   // uexpiration_init(self);
   // umonitor_init(self);
}
