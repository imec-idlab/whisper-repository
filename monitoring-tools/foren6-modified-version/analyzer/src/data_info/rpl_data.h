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
 *         RPL Data Access
 * \author
 *         Foren6 Team <foren6@cetic.be>
 *         http://cetic.github.io/foren6/credits.html
 */

#ifndef RPL_DATA_H
#define	RPL_DATA_H

#include "link.h"
#include "node.h"
#include "dodag.h"
#include "rpl_instance.h"
#include "hash_container.h"

#ifdef	__cplusplus
extern "C" {
#endif

void rpldata_init();

//These function use version 0. Modification to other version should not be allowed.
di_node_t *rpldata_get_node(const di_node_ref_t * node_ref,
                            hash_value_mode_e value_mode,
                            bool * was_already_existing);
di_dodag_t *rpldata_get_dodag(const di_dodag_ref_t * dodag_ref,
                              hash_value_mode_e value_mode,
                              bool * was_already_existing);
di_rpl_instance_t *rpldata_get_rpl_instance(const di_rpl_instance_ref_t *
                                            rpl_instance_ref,
                                            hash_value_mode_e value_mode,
                                            bool * was_already_existing);
di_link_t *rpldata_get_link(const di_link_ref_t * link_ref,
                            hash_value_mode_e value_mode,
                            bool * was_already_existing);
di_link_t *rpldata_del_link(const di_link_ref_t * link_ref);

hash_container_ptr rpldata_get_nodes(uint32_t version);
hash_container_ptr rpldata_get_dodags(uint32_t version);
hash_container_ptr rpldata_get_rpl_instances(uint32_t version);
hash_container_ptr rpldata_get_links(uint32_t version);


uint32_t rpldata_add_node_version();
uint32_t rpldata_add_dodag_version();
uint32_t rpldata_add_rpl_instance_version();
uint32_t rpldata_add_link_version();

void rpldata_wsn_create_version(int packed_id, double timestamp);
double rpldata_wsn_version_get_timestamp(uint32_t version);
uint32_t rpldata_wsn_version_get_packet_count(uint32_t version);
uint32_t rpldata_wsn_version_get_has_errors(uint32_t version);

uint32_t rpldata_get_wsn_last_version();

void rpldata_clear();

#ifdef	__cplusplus
}
#endif
#endif                          /* RPL_DATA_H */
