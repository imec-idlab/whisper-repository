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
 *         Node State Information
 * \author
 *         Foren6 Team <foren6@cetic.be>
 *         http://cetic.github.io/foren6/credits.html
 */

#ifndef NODE_H
#define	NODE_H

#include "node_type.h"

#include "metric.h"
#include "address.h"
#include "route.h"
#include "dodag.h"
#include "6lowpan_def.h"
#include "rpl_def.h"
#include "sixtop_def.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct {
    addr_wpan_t wpan_address;
} di_node_ref_t;

typedef struct di_node_key {
    di_node_ref_t ref;
} di_node_key_t;

// Base functions

size_t node_sizeof();

void node_init(void *data, const void *key, size_t key_size);
void node_destroy(void *data);
di_node_t *node_dup(di_node_t * node);
void node_set_changed(di_node_t * node);
bool node_has_changed(di_node_t * node);
void node_reset_changed(di_node_t * node);
void node_fill_delta(di_node_t * node, di_node_t const *prev_node);

void node_key_init(di_node_key_t * key, addr_wpan_t wpan_address,
                   uint32_t version);
void node_ref_init(di_node_ref_t * ref, addr_wpan_t wpan_address);
void node_set_key(di_node_t * node, const di_node_key_t * key);
const di_node_key_t *node_get_key(const di_node_t * node);

void node_set_dodag(di_node_t * node, const di_dodag_ref_t * dodag_ref);
const di_dodag_ref_t *node_get_dodag(const di_node_t * node);
uint16_t node_get_simple_id(const di_node_t * node);

// Data setters

void node_set_ip(di_node_t * node, addr_ipv6_t address);

void node_add_route(di_node_t * node, const di_route_t * route_prefix,
                    addr_wpan_t via_node);
void node_del_route(di_node_t * node, const di_route_t * route_prefix,
                    addr_wpan_t via_node);

void node_update_from_dio(di_node_t * node, const rpl_dio_t * dio,
                          const di_dodag_t * dodag);
void node_update_from_metric(di_node_t * node,
                             const rpl_metric_t * metric);
void node_update_from_hop_by_hop(di_node_t * node,
                                 const rpl_hop_by_hop_opt_t * hop_by_hop);
void node_update_from_dao(di_node_t * node, const rpl_dao_t * dao,
                          const di_dodag_t * dodag);
void node_update_from_dao_ack(di_node_t * node,
                              const rpl_dao_ack_t * dao_ack);
void node_update_from_dodag_config(di_node_t * node,
                                   const rpl_dodag_config_t * config,
                                   const di_dodag_t * dodag);
void node_update_from_dodag_prefix_info(di_node_t * node,
                                        const rpl_prefix_t * prefix_info,
                                        const di_dodag_t * dodag);
void node_update_from_dodag(di_node_t * node, const di_dodag_t * dodag);

void node_update_from_dis(di_node_t * node,
                          const rpl_dis_opt_info_req_t * dis);

void node_add_packet_count(di_node_t * node, int count);

void node_update_dao_interval(di_node_t * node, double timestamp);
void node_update_dio_interval(di_node_t * node, double timestamp);

void node_add_rank_error(di_node_t * node);
void node_add_forward_error(di_node_t * node);
void node_add_upward_error(di_node_t * node);
void node_add_downward_error(di_node_t * node);
void node_add_route_error(di_node_t * node);
void node_add_dodag_version_error(di_node_t * node);
void node_add_ip_mismatch_error(di_node_t * node);
void node_add_dodag_mismatch_error(di_node_t * node);
void node_add_dodag_config_mismatch_error(di_node_t * node);

//6top
void node_add_sixtop_neighbor(di_node_t* node,sixtop_neighbor_t* neighbor);
void node_delete_sixtop_neighbor(di_node_t* node,sixtop_neighbor_key_t* neighbor);
void node_add_sixtop_cell(di_node_t* node,sixtop_neighbor_t* neighbor,neighbor_cell_t* cell);
void node_delete_sixtop_cell(di_node_t* node,sixtop_neighbor_t* neighbor,neighbor_cell_key_t* cell);

//Data getters

addr_wpan_t node_get_mac64(const di_node_t * node);
const sixlowpan_config_t *node_get_sixlowpan_config(const di_node_t *
                                                    node);
const sixlowpan_config_delta_t *node_get_sixlowpan_config_delta(const
                                                                di_node_t
                                                                * node);
const addr_ipv6_t *node_get_local_ip(const di_node_t * node);
const addr_ipv6_t *node_get_global_ip(const di_node_t * node);

di_route_list_t node_get_routes(const di_node_t * node);
bool node_get_routes_delta(const di_node_t * node);

const rpl_instance_config_t *node_get_instance_config(const di_node_t *
                                                      node);
const rpl_instance_config_delta_t *node_get_instance_config_delta(const
                                                                  di_node_t
                                                                  * node);
const rpl_instance_config_delta_t
    *node_get_actual_instance_config_delta(const di_node_t * node);

const rpl_instance_data_t *node_get_instance_data(const di_node_t * node);
const rpl_instance_data_delta_t *node_get_instance_data_delta(const
                                                              di_node_t *
                                                              node);
bool node_has_rank(const di_node_t * node);
int node_get_rank(const di_node_t * node);

const rpl_dodag_config_t *node_get_dodag_config(const di_node_t * node);
const rpl_dodag_config_delta_t *node_get_dodag_config_delta(const
                                                            di_node_t *
                                                            node);
const rpl_dodag_config_delta_t *node_get_actual_dodag_config_delta(const
                                                                   di_node_t
                                                                   *
                                                                   node);

const rpl_prefix_t *node_get_dodag_prefix_info(const di_node_t * node);
const rpl_prefix_delta_t *node_get_dodag_prefix_info_delta(const di_node_t
                                                           * node);
const rpl_prefix_delta_t *node_get_actual_dodag_prefix_info_delta(const
                                                                  di_node_t
                                                                  * node);

const sixlowpan_statistics_t *node_get_sixlowpan_statistics(const
                                                            di_node_t *
                                                            node);
const sixlowpan_statistics_delta_t
    *node_get_sixlowpan_statistics_delta(const di_node_t * node);

const rpl_statistics_t *node_get_rpl_statistics(const di_node_t * node);
const rpl_statistics_delta_t *node_get_rpl_statistics_delta(const
                                                            di_node_t *
                                                            node);

const sixlowpan_errors_t *node_get_sixlowpan_errors(const di_node_t *
                                                    node);
const sixlowpan_errors_delta_t *node_get_sixlowpan_errors_delta(const
                                                                di_node_t
                                                                * node);

const rpl_errors_t *node_get_rpl_errors(const di_node_t * node);
const rpl_errors_delta_t *node_get_rpl_errors_delta(const di_node_t *
                                                    node);

int node_get_has_errors(const di_node_t * node);

void nodes_clear_all_errors();
	
//6top	
const sixtop_neighbor_t* node_get_sixtop_neighbor(di_node_t* node,sixtop_neighbor_key_t* key);
bool node_sixtop_neighbor_is_empty(di_node_t* node,sixtop_neighbor_key_t* neighborkey);	
const neighbor_cell_t* node_get_sixtop_cell(sixtop_neighbor_t* neighbor,neighbor_cell_key_t* key);
struct sixtop_neighbor* node_get_sixtop_neighbors(const di_node_t * node);
	

#ifdef	__cplusplus
}
#endif
#endif                          /* NODE_H */
