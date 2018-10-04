/*
 * This file is part of Foren6, a 6LoWPAN Diagnosis Tool
 * Copyright (C) 2013, CETIC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/**
 * \file
 *         RPL Instances
 * \author
 *         Foren6 Team <foren6@cetic.be>
 *         http://cetic.github.io/foren6/credits.html
 */


#ifndef RPL_INSTANCE_H
#define	RPL_INSTANCE_H

#include <stdbool.h>
#include <stdint.h>
#include "hash_container.h"

#include "rpl_instance_type.h"
#include "dodag_type.h"
#include "rpl_def.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct {
    int16_t rpl_instance;   //Via DIO, DAO
} di_rpl_instance_ref_t;

typedef struct di_rpl_instance_key {
    di_rpl_instance_ref_t ref;
} di_rpl_instance_key_t;

size_t rpl_instance_sizeof();

void rpl_instance_init(void *data, const void *key, size_t key_size);
void rpl_instance_destroy(void *data);
di_rpl_instance_t *rpl_instance_dup(const di_rpl_instance_t *
                                    rpl_instance);

void rpl_instance_key_init(di_rpl_instance_key_t * key,
                           uint8_t rpl_instance, uint32_t version);
void rpl_instance_ref_init(di_rpl_instance_ref_t * ref,
                           uint8_t rpl_instance);
void rpl_instance_set_key(di_rpl_instance_t * rpl_instance,
                          const di_rpl_instance_key_t * key);
void rpl_instance_set_mop(di_rpl_instance_t * rpl_instance,
                          di_rpl_mop_e mop);
void rpl_instance_set_user_data(di_rpl_instance_t * rpl_instance,
                                void *user_data);
void rpl_instance_add_dodag(di_rpl_instance_t * rpl_instance,
                            di_dodag_t * dodag);
void rpl_instance_del_dodag(di_rpl_instance_t * rpl_instance,
                            di_dodag_t * dodag);

bool rpl_instance_has_changed(di_rpl_instance_t * rpl_instance);
void rpl_instance_reset_changed(di_rpl_instance_t * rpl_instance);

const di_rpl_instance_key_t *rpl_instance_get_key(const di_rpl_instance_t
                                                  * rpl_instance);
di_rpl_mop_e rpl_instance_get_mop(const di_rpl_instance_t * rpl_instance);
void *rpl_instance_get_user_data(const di_rpl_instance_t * rpl_instance);

#ifdef	__cplusplus
}
#endif
#endif                          /* RPL_INSTANCE_H */
