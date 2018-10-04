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
 *         RPL DODAG Information Mangement
 * \author
 *         Foren6 Team <foren6@cetic.be>
 *         http://cetic.github.io/foren6/credits.html
 */

#ifndef DODAG_H
#define	DODAG_H

#include "address.h"
#include "rpl_instance.h"
#include "hash_container.h"

#include "node_type.h"
#include "dodag_type.h"
#include "rpl_def.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct di_dodag_ref {
    addr_ipv6_t dodagid;    //Via DIO, DAO
    int16_t version;        //Via DIO
} di_dodag_ref_t;

typedef struct di_dodag_key {
    di_dodag_ref_t ref;
} di_dodag_key_t;

size_t dodag_sizeof();

void dodag_init(void *data, const void *key, size_t key_size);
void dodag_destroy(void *data);
di_dodag_t *dodag_dup(const di_dodag_t * dodag);

void dodag_key_init(di_dodag_key_t * key, addr_ipv6_t dodag_id,
                    uint8_t dodag_version, uint32_t version);
void dodag_ref_init(di_dodag_ref_t * ref, addr_ipv6_t dodag_id,
                    uint8_t dodag_version);
void dodag_set_key(di_dodag_t * dodag, const di_dodag_key_t * key);
const di_dodag_key_t *dodag_get_key(const di_dodag_t * dodag);
bool dodag_has_changed(di_dodag_t * dodag);
void dodag_reset_changed(di_dodag_t * dodag);
void dodag_set_nodes_changed(di_dodag_t * dodag);
void dodag_set_user_data(di_dodag_t * dodag, void *user_data);
void *dodag_get_user_data(const di_dodag_t * dodag);

void dodag_update_from_dio(di_dodag_t * dodag, const rpl_dio_t * dio);
void dodag_update_from_dodag_config(di_dodag_t * dodag,
                                    const rpl_dodag_config_t *
                                    dodag_config);
void dodag_update_from_dodag_prefix_info(di_dodag_t * dodag,
                                         const rpl_prefix_t * prefix);
void dodag_set_rpl_instance(di_dodag_t * dodag,
                            const di_rpl_instance_ref_t * rpl_instance);

void dodag_add_node(di_dodag_t * dodag, di_node_t * node);
void dodag_del_node(di_dodag_t * dodag, di_node_t * node);

const rpl_instance_config_t *dodag_get_instance_config(const di_dodag_t *
                                                       dodag);
const rpl_dodag_config_t *dodag_get_dodag_config(const di_dodag_t *
                                                 dodag);
const rpl_prefix_t *dodag_get_prefix(const di_dodag_t * dodag);
const di_rpl_instance_ref_t *dodag_get_rpl_instance(const di_dodag_t *
                                                    dodag);
hash_container_ptr dodag_get_node(const di_dodag_t * dodag);

#ifdef	__cplusplus
}
#endif
#endif                          /* DODAG_H */
