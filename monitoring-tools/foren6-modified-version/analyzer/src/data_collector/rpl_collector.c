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
 *         RPL-related information collector
 * \author
 *         Foren6 Team <foren6@cetic.be>
 *         http://cetic.github.io/foren6/credits.html
 */

#include "rpl_collector.h"
#include "rpl_event_callbacks.h"
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../rpl_packet_parser.h"
#include "../data_info/address.h"
#include "../data_info/node.h"
#include "../data_info/dodag.h"
#include "../data_info/rpl_instance.h"
#include "../data_info/route.h"
#include "../data_info/link.h"
#include "../data_info/metric.h"
#include "../data_info/rpl_data.h"

#include "../utlist.h"

void
rpl_collector_parse_dio(packet_info_t pkt_info,
                        rpl_dio_t * dio,
                        rpl_dio_opt_config_t * dodag_config,
                        rpl_dio_opt_metric_t * metric,
                        rpl_dio_opt_prefix_t * prefix,
                        rpl_dio_opt_route_t * route_info)
{
    //fprintf(stderr, "Received DIO\n");

    di_node_t *node;
    di_dodag_t *dodag;
    di_rpl_instance_t *rpl_instance;

    bool node_created;
    bool dodag_created;
    bool rpl_instance_created;

    //Get node, dodag and rpl_instance using their keys. If the requested object does not exist, created it.

    di_rpl_instance_ref_t rpl_instance_ref;

    rpl_instance_ref_init(&rpl_instance_ref, dio->rpl_instance_id);
    rpl_instance =
        rpldata_get_rpl_instance(&rpl_instance_ref, HVM_CreateIfNonExistant,
                                 &rpl_instance_created);

    di_dodag_ref_t dodag_ref;

    dodag_ref_init(&dodag_ref, dio->dodagid, dio->version_number);
    dodag =
        rpldata_get_dodag(&dodag_ref, HVM_CreateIfNonExistant,
                          &dodag_created);

    di_node_ref_t node_ref;

    node_ref_init(&node_ref, pkt_info.src_wpan_address);
    node =
        rpldata_get_node(&node_ref, HVM_CreateIfNonExistant, &node_created);

    dodag_add_node(dodag, node);

    if(dio->rank == 0
       || dio->rank == rpl_tool_get_analyser_config()->root_rank) {
        //Only update instance and DODAG if it comes from a RPL Root or Virtual Root
        rpl_instance_add_dodag(rpl_instance, dodag);
        rpl_instance_set_mop(rpl_instance, dio->mode_of_operation);

        dodag_update_from_dio(dodag, dio);
        dodag_update_from_dodag_config(dodag, dodag_config);
        dodag_update_from_dodag_prefix_info(dodag, prefix);
    }

    node_add_packet_count(node, 1);
    node_update_dio_interval(node, pkt_info.timestamp);
    node_set_ip(node, pkt_info.src_ip_address);

    node_update_from_dio(node, dio, dodag);
    node_update_from_metric(node, metric);
    node_update_from_dodag_config(node, dodag_config, dodag);
    node_update_from_dodag_prefix_info(node, prefix, dodag);

    if(dio->rank == 0
       || dio->rank == rpl_tool_get_analyser_config()->root_rank) {
        dodag_set_nodes_changed(dodag);
    }
}

void
rpl_collector_parse_dao(packet_info_t pkt_info,
                        rpl_dao_t * dao,
                        rpl_dao_opt_target_t * target,
                        rpl_dao_opt_transit_t * transit)
{

    di_node_t *child, *parent;
    di_link_t *new_link = NULL;
    di_link_t *old_link = NULL;
    di_dodag_t *dodag = NULL;

    bool child_created = false;
    bool parent_created = false;
    bool rpl_instance_created = false;
    bool link_created = false;

    //fprintf(stderr, "Received DAO\n");

    di_node_ref_t node_ref;

    node_ref_init(&node_ref, pkt_info.src_wpan_address);
    child =
        rpldata_get_node(&node_ref, HVM_CreateIfNonExistant, &child_created);
    node_add_packet_count(child, 1);

    node_set_ip(child, pkt_info.src_ip_address);

    node_ref_init(&node_ref, pkt_info.dst_wpan_address);
    parent =
        rpldata_get_node(&node_ref, HVM_CreateIfNonExistant, &parent_created);

    di_rpl_instance_ref_t rpl_instance_ref;

    rpl_instance_ref_init(&rpl_instance_ref, dao->rpl_instance_id);
    rpldata_get_rpl_instance(&rpl_instance_ref, HVM_CreateIfNonExistant,
                             &rpl_instance_created);

    if(addr_compare_ip(node_get_local_ip(parent), &pkt_info.dst_ip_address) !=
       0) {
        node_add_ip_mismatch_error(child);
    }


    if(dao->dodagid_present && node_get_dodag(child)) {
        const di_dodag_ref_t *dodag_ref = node_get_dodag(parent);

        if(dodag_ref) {
            if(addr_compare_ip(&dao->dodagid, &dodag_ref->dodagid) == 0) {
                //Removed as we don't know the dodag version here
                /*
                dodag =
                    rpldata_get_dodag(dodag_ref, HVM_FailIfNonExistant, NULL);
                assert(dodag);
                dodag_add_node(dodag, child);
                */
            } else {
                node_add_dodag_mismatch_error(child);
            }
        }
    }

    if(transit && transit->path_lifetime > 0) {
        di_link_ref_t link_ref;

        link_ref_init(&link_ref, (di_node_ref_t) {
                      node_get_mac64(child)}
                      , (di_node_ref_t) {
                      node_get_mac64(parent)}
        );
        //Clear parents list
        new_link =
            rpldata_get_link(&link_ref, HVM_CreateIfNonExistant,
                             &link_created);
        if(rpl_tool_get_analyser_config()->one_preferred_parent) {
            links_deprecate_all_from(&link_ref);
        }
        link_update(new_link, time(NULL), 1);


        //Check if the parent is in the child routing table, in that case, there might be a routing loop
        di_route_list_t route_table = node_get_routes(child);
        di_route_el_t *route;

        LL_FOREACH(route_table, route) {
            if(addr_compare_ip
               (&route->target.prefix, node_get_global_ip(parent)) == 0) {
                node_add_route_error(child);
                break;
            }
        }

        //link_set_metric(new_link, node_get_metric(child));
    } else if(transit && transit->path_lifetime == 0) {
        //No-Path DAO
        if(target
           && !addr_compare_ip_len(node_get_global_ip(child), &target->target,
                                   target->target_bit_length)) {
            di_link_ref_t link_ref;

            link_ref_init(&link_ref, (di_node_ref_t) {
                          node_get_mac64(child)}
                          , (di_node_ref_t) {
                          node_get_mac64(parent)}
            );
            old_link = rpldata_del_link(&link_ref);
            if(old_link) {
                rpl_event_link(old_link, RET_Deleted);
                //free(old_link);
                //freed when clearing the WSN, see rpldata_clear()
            }
            //fprintf(stderr, "No-Path DAO, child = 0x%llX, parent = 0x%llX\n", child->key.ref.wpan_address, parent->key.ref.wpan_address);
        }
    }

    if(target && transit) {
        di_route_t route;

        route.expiration_time = transit->path_lifetime;
        route.length = target->target_bit_length;
        route.prefix = target->target;

        if(transit->path_lifetime > 0) {
            node_add_route(parent, &route, node_get_mac64(child));
        } else {                //No-path DAO
            node_del_route(parent, &route, node_get_mac64(child));
        }

        if(addr_compare_ip(&target->target, node_get_global_ip(child)) == 0) {
            node_update_from_dao(child, dao, dodag);
            node_update_dao_interval(child, pkt_info.timestamp);
        }
    }
}

void
rpl_collector_parse_dao_ack(packet_info_t pkt_info,
                            rpl_dao_ack_t * dao_ack)
{
    di_node_t *node = NULL;
    di_node_ref_t node_ref;
    bool node_created = false;

    node_ref_init(&node_ref, pkt_info.src_wpan_address);
    node =
      rpldata_get_node(&node_ref, HVM_CreateIfNonExistant, &node_created);
    node_add_packet_count(node, 1);
    node_set_ip(node, pkt_info.src_ip_address);
    node_update_from_dao_ack(node, dao_ack);
}

void
rpl_collector_parse_dis(packet_info_t pkt_info,
                        rpl_dis_opt_info_req_t * request)
{
    bool node_created;
    di_node_t *node;

    //fprintf(stderr, "Received DIS\n");

    di_node_ref_t node_ref;

    node_ref_init(&node_ref, pkt_info.src_wpan_address);
    node = rpldata_get_node(&node_ref, HVM_CreateIfNonExistant, &node_created); //nothing to do with the node, but be sure it exists in the node list
    node_add_packet_count(node, 1);

    node_set_ip(node, pkt_info.src_ip_address);
    node_update_from_dis(node, request);
}

void
rpl_collector_parse_data(packet_info_t pkt_info,
                         rpl_hop_by_hop_opt_t * rpl_info)
{
    di_node_t *src, *dst = NULL;
    di_link_t *link;

    bool src_created, dst_created;
    bool link_created;

    //fprintf(stderr, "Received Data\n");

    di_node_ref_t node_ref;

    node_ref_init(&node_ref, pkt_info.src_wpan_address);
    src = rpldata_get_node(&node_ref, HVM_CreateIfNonExistant, &src_created);

    node_add_packet_count(src, 1);
    if(pkt_info.hop_limit == 64) {
        //Update only if own packet
        node_set_ip(src, pkt_info.src_ip_address);
    }
    node_update_from_hop_by_hop(src, rpl_info);

    if(pkt_info.dst_wpan_address != 0
       && pkt_info.dst_wpan_address != ADDR_MAC64_BROADCAST) {
        node_ref_init(&node_ref, pkt_info.dst_wpan_address);
        dst =
            rpldata_get_node(&node_ref, HVM_CreateIfNonExistant,
                             &dst_created);

        /* Add the parent node to the parents list of the child node if not already done */

        if(rpl_info) {
            if (rpl_info->rank_error) {
                node_add_rank_error(src);
            }
            if (rpl_info->forwarding_error) {
                node_add_forward_error(src);
            }
            if(rpl_info->packet_toward_root) {
                di_link_ref_t link_ref;

                link_ref_init(&link_ref, (di_node_ref_t) {
                              node_get_mac64(src)}
                              , (di_node_ref_t) {
                              node_get_mac64(dst)}
                );
                link =
                    rpldata_get_link(&link_ref, HVM_CreateIfNonExistant,
                                     &link_created);
                if(rpl_tool_get_analyser_config()->one_preferred_parent) {
                    links_deprecate_all_from(&link_ref);
                }
                link_update(link, time(NULL), 1);
                //link_set_metric(new_link, node_get_metric(src));

                di_route_t route;

                route.expiration_time = 0;
                route.length = 128;
                route.prefix = pkt_info.src_ip_address;
                node_add_route(dst, &route, node_get_mac64(src));

                //Check if the parent is in the child routing table, in that case, there might be a routing loop
                di_route_list_t route_table = node_get_routes(src);
                di_route_el_t *route_el;

                LL_FOREACH(route_table, route_el) {
                    if(addr_compare_ip
                       (&route_el->target.prefix,
                        node_get_global_ip(dst)) == 0) {
                        node_add_route_error(src);
                        break;
                    }
                }

                if(node_has_rank(dst)
                   && rpl_info->sender_rank < node_get_rank(dst))
                    node_add_upward_error(src);
            } else {
                di_route_t route;

                route.expiration_time = 0;
                route.length = 128;
                route.prefix = pkt_info.dst_ip_address;
                node_add_route(src, &route, node_get_mac64(dst));

                if(node_has_rank(dst)
                   && rpl_info->sender_rank > node_get_rank(dst))
                    node_add_downward_error(src);
            }
        }
    }
}
