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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

#include "route.h"
#include "../utlist.h"

di_route_el_t *
route_get(di_route_list_t * list, di_route_t target, addr_wpan_t via_node,
          bool allow_summary)
{
    di_route_el_t *route;

    LL_FOREACH(*list, route) {
        if(route->via_node == via_node
           && target.length == route->target.length
           && !addr_compare_ip_len(&route->target.prefix, &target.prefix,
                                   target.length))
            return route;
    }

    return NULL;
}

di_route_el_t *
route_add(di_route_list_t * list, di_route_t target, addr_wpan_t via_node,
          bool auto_summary, bool * was_already_existing)
{
    di_route_el_t *route;

    LL_FOREACH(*list, route) {
        if(target.length == route->target.length
           && !addr_compare_ip_len(&route->target.prefix, &target.prefix,
                                   target.length)) {
            if(route->via_node == via_node && was_already_existing)
                *was_already_existing = true;
            else
                route->via_node = via_node;
            return route;
        }
    }

    route = (di_route_el_t *) calloc(1, sizeof(di_route_el_t));
    route->target = target;
    route->via_node = via_node;
    LL_PREPEND(*list, route);

    if(was_already_existing)
        *was_already_existing = false;
    return route;
}

bool
route_remove(di_route_list_t * list, di_route_t target, addr_wpan_t via_node)
{
    di_route_el_t *route;

    LL_FOREACH(*list, route) {
        if(route->via_node == via_node
           && target.length == route->target.length
           && !addr_compare_ip_len(&route->target.prefix, &target.prefix,
                                   target.length)) {
            LL_DELETE(*list, route);
            free(route);
            return true;
        }
    }

    return false;
}

bool
route_del_all_outdated(di_route_list_t * list)
{
    return false;
}

di_route_list_t
route_dup(const di_route_list_t * routes)
{
    di_route_list_t new_routes = NULL;
    di_route_el_t *route, *new_route, *last_route;

    last_route = NULL;
    LL_FOREACH(*routes, route) {
        new_route = (di_route_el_t *) malloc(sizeof(di_route_el_t));
        memcpy(new_route, route, sizeof(di_route_el_t));
        if(last_route)
            last_route->next = new_route;
        new_route->next = NULL;
        if(new_routes == NULL)
            new_routes = new_route;
        last_route = new_route;
    }

    return new_routes;
}

void
route_destroy(di_route_list_t * routes)
{
    di_route_el_t *route, *tmp;

    LL_FOREACH_SAFE(*routes, route, tmp) {
        LL_DELETE(*routes, route);
        free(route);
    }
}
