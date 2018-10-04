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
 *         Routing Table Management
 * \author
 *         Foren6 Team <foren6@cetic.be>
 *         http://cetic.github.io/foren6/credits.html
 */

#ifndef ROUTE_H
#define	ROUTE_H

#include <stdbool.h>
#include "address.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct di_route {
    uint8_t length;
    addr_ipv6_t prefix;
    uint32_t expiration_time;
} di_route_t;

typedef struct di_route_el {
    di_route_t target;
    addr_wpan_t via_node;

    void *user_data;

    struct di_route_el *next;
} di_route_el_t, *di_route_list_t;

di_route_el_t *route_get(di_route_list_t * list, di_route_t route_prefix,
                         addr_wpan_t via_node, bool allow_summary);
di_route_el_t *route_add(di_route_list_t * list, di_route_t route_prefix,
                         addr_wpan_t via_node, bool auto_summary,
                         bool * was_already_existing);
bool route_remove(di_route_list_t * list, di_route_t route_prefix,
                  addr_wpan_t via_node);
bool route_del_all_outdated(di_route_list_t * list);

di_route_list_t route_dup(const di_route_list_t * routes);
void route_destroy(di_route_list_t * routes);

#ifdef	__cplusplus
}
#endif
#endif                          /* ROUTE_H */
