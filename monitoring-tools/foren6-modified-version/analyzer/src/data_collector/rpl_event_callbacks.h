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
 *         RPL event callbacks
 * \author
 *         Foren6 Team <foren6@cetic.be>
 *         http://cetic.github.io/foren6/credits.html
 */

#ifndef RPL_EVENT_CALLBACKS_H
#define	RPL_EVENT_CALLBACKS_H

#include "../data_info/node.h"
#include "../data_info/dodag.h"
#include "../data_info/rpl_instance.h"
#include "../data_info/link.h"
#include "../data_info/6lowpan_def.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef enum rpl_event_type {
    RET_Created,
    RET_Updated,
    RET_Deleted
} rpl_event_type_e;

typedef struct rpl_event_callbacks {
    void (*onNodeEvent) (di_node_t * node, rpl_event_type_e event_type);
    void (*onDodagEvent) (di_dodag_t * dodag,
                          rpl_event_type_e event_type);
    void (*onLinkEvent) (di_link_t * link, rpl_event_type_e event_type);
    void (*onRplInstanceEvent) (di_rpl_instance_t * rpl_instance,
                                rpl_event_type_e event_type);
    void (*onPacketEvent) (int packet_id, packet_info_t packet_info);
    void (*onClearEvent) ();
} rpl_event_callbacks_t;


void rpl_event_set_callbacks(rpl_event_callbacks_t * callbacks);

void rpl_event_packet(int packet_id, packet_info_t const *pkt_info);
void rpl_event_node(di_node_t * node, rpl_event_type_e type);
void rpl_event_dodag(di_dodag_t * dodag, rpl_event_type_e type);
void rpl_event_link(di_link_t * link, rpl_event_type_e type);
void rpl_event_rpl_instance(di_rpl_instance_t * rpl_instance,
                            rpl_event_type_e type);

//create a WSN version if needed and return true if at least one object has changed
bool rpl_event_commit_changed_objects(int packet_id, double timestamp);

void rpl_event_process_events(int wsn_version);

void rpl_event_clear();

#ifdef	__cplusplus
}
#endif
#endif                          /* RPL_EVENT_CALLBACKS_H */
