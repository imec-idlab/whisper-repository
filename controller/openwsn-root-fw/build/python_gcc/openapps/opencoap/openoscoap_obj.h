/**
DO NOT EDIT DIRECTLY!!

This file was 'objectified' by SCons as a pre-processing
step for the building a Python extension module.

This was done on 2018-03-08 18:25:52.671431.
*/
#ifndef __OPENOSCOAP_H
#define __OPENOSCOAP_H

/**
\addtogroup Transport
\{
\addtogroup openCoap
\{
*/

#include "Python.h"

#include "opencoap_obj.h"

//=========================== define ==========================================

//=========================== typedef =========================================
typedef enum {
   OSCOAP_DERIVATION_TYPE_KEY           = 0,
   OSCOAP_DERIVATION_TYPE_IV            = 1,
} oscoap_derivation_t;

//=========================== module variables ================================

typedef struct {
    uint8_t dummy; // needed for compilation, not used
} openoscoap_vars_t;

#include "openwsnmodule_obj.h"
typedef struct OpenMote OpenMote;

//=========================== prototypes ======================================

void openoscoap_init_security_context(OpenMote* self, oscoap_security_context_t *ctx, 
                                uint8_t* senderID, 
                                uint8_t senderIDLen,
                                uint8_t* recipientID,
                                uint8_t recipientIDLen,
                                uint8_t* masterSecret,
                                uint8_t masterSecretLen,
                                uint8_t* masterSalt,
                                uint8_t masterSaltLen);

owerror_t openoscoap_protect_message(OpenMote* self, 
        oscoap_security_context_t *context, 
        uint8_t version, 
        uint8_t code,
        coap_option_iht* options,
        uint8_t optionsLen,
        OpenQueueEntry_t* msg,
        uint16_t sequenceNumber);

owerror_t openoscoap_unprotect_message(OpenMote* self, 
        oscoap_security_context_t *context, 
        uint8_t version, 
        uint8_t code,
        coap_option_iht* options,
        uint8_t* optionsLen,
        OpenQueueEntry_t* msg,
        uint16_t sequenceNumber);

uint16_t openoscoap_get_sequence_number(OpenMote* self, oscoap_security_context_t *context);

uint8_t openoscoap_parse_compressed_COSE(OpenMote* self, uint8_t *buffer,
        uint8_t bufferLen,
        uint16_t* sequenceNumber,
        uint8_t** kid,
        uint8_t* kidLen);
/**
\}
\}
*/

#endif
