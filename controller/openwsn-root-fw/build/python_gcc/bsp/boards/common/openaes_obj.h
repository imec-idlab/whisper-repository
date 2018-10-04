/**
DO NOT EDIT DIRECTLY!!

This file was 'objectified' by SCons as a pre-processing
step for the building a Python extension module.

This was done on 2018-03-08 18:28:25.163873.
*/
/**************************************************************
AES128
Author:   Uli Kretzschmar
MSP430 Systems
Freising
AES software support for encryption and decryption
ECCN 5D002 TSU - Technology / Software Unrestricted
Source: http://is.gd/o9RSPq
**************************************************************/
#ifndef __OPENAES_H__
#define __OPENAES_H__

#include "openwsnmodule_obj.h"
typedef struct OpenMote OpenMote;

//=========================== prototypes ======================================

owerror_t openaes_enc(OpenMote* self, uint8_t* buffer, uint8_t* key);

#endif /* __OPENAES_H__ */
