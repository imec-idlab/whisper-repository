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
 *         Address-related Utility Functions
 * \author
 *         Foren6 Team <foren6@cetic.be>
 *         http://cetic.github.io/foren6/credits.html
 */

#include "address.h"
#include "../utlist.h"
#include <string.h>
#include <stdlib.h>
#ifdef __APPLE__
#include "../apple-endian.h"
#endif

static int addr_list_compare(wpan_addr_elt_t * a, wpan_addr_elt_t * b);

void
init_ipv6_addr(addr_ipv6_t * addr)
{
    memset(addr, 0, sizeof(addr_ipv6_t));
}

void
init_prefix(di_prefix_t * prefix)
{
    prefix->length = 0;
    init_ipv6_addr(&prefix->prefix);
}
wpan_addr_elt_t *
addr_wpan_add_to_list(wpan_addr_list_t * list, addr_wpan_t address)
{
    wpan_addr_elt_t search_address;
    wpan_addr_elt_t *addr_ref;

    memset(&search_address, 0, sizeof(search_address)); //Padding in structure must be always 0
    search_address.address = address;
    LL_SEARCH(*list, addr_ref, &search_address, addr_list_compare);
    if(!addr_ref) {
        addr_ref = (wpan_addr_elt_t *) calloc(1, sizeof(wpan_addr_elt_t));
        addr_ref->address = address;
        LL_PREPEND(*list, addr_ref);
    }

    return addr_ref;
}

wpan_addr_elt_t *
addr_wpan_del_from_list(wpan_addr_list_t * list, addr_wpan_t address)
{
    wpan_addr_elt_t search_address;
    wpan_addr_elt_t *addr_ref;

    memset(&search_address, 0, sizeof(search_address)); //Padding in structure must be always 0
    search_address.address = address;
    LL_SEARCH(*list, addr_ref, &search_address, addr_list_compare);
    if(addr_ref) {
        LL_DELETE(*list, addr_ref);
    }

    return addr_ref;
}

addr_wpan_t
addr_get_mac64_from_ip(addr_ipv6_t address)
{
    uint64_t int_id;

    int_id =
        ((uint64_t) be32toh(address.__in6_u.__u6_addr32[2]) << 32) |
        be32toh(address.__in6_u.__u6_addr32[3]);
    return addr_get_mac64_from_int_id(int_id);
}

addr_ipv6_t
addr_get_local_ip_from_mac64(addr_wpan_t mac_address)
{
    uint64_t int_id = addr_get_int_id_from_mac64(mac_address);

    struct in6_addr link_local_address = { .__in6_u = { .__u6_addr8 = {
        0xFE, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        (int_id >> 56) & 0xFF, (int_id >> 48) & 0xFF, (int_id >> 40) & 0xFF, (int_id >> 32) & 0xFF,
        (int_id >> 24) & 0xFF, (int_id >> 16) & 0xFF, (int_id >> 8) & 0xFF, int_id & 0xFF
    }}};

    return link_local_address;
}

addr_ipv6_t
addr_get_global_ip_from_mac64(di_prefix_t prefix, addr_wpan_t mac_address)
{
    uint64_t int_id = addr_get_int_id_from_mac64(mac_address);
    int i;
    uint64_t mask = 0;
    uint8_t *prefix_data = prefix.prefix.__in6_u.__u6_addr8;

    if(prefix.length > 64)
        prefix.length = 64;

    for(i = 0; i < prefix.length; i++) {
        mask = (mask >> 1) | 0x8000000000000000;
    }

    mask = htobe64(mask);

    prefix.prefix.__in6_u.__u6_addr32[0] &= mask & 0xFFFFFFFF;
    prefix.prefix.__in6_u.__u6_addr32[1] &= mask >> 32;
    prefix.prefix.__in6_u.__u6_addr32[2] =
        prefix.prefix.__in6_u.__u6_addr32[3] = 0;

    struct in6_addr global_address = { .__in6_u = { .__u6_addr8 = {
        prefix_data[0], prefix_data[1], prefix_data[2], prefix_data[3],
        prefix_data[4], prefix_data[5], prefix_data[6], prefix_data[7],
        (int_id >> 56) & 0xFF, (int_id >> 48) & 0xFF, (int_id >> 40) & 0xFF, (int_id >> 32) & 0xFF,
        (int_id >> 24) & 0xFF, (int_id >> 16) & 0xFF, (int_id >> 8) & 0xFF, int_id & 0xFF
	}}};

    return global_address;
}

uint64_t
addr_get_int_id_from_mac64(addr_wpan_t mac_address)
{
    return mac_address ^ 0x0200000000000000;
}

addr_wpan_t
addr_get_mac64_from_int_id(uint64_t int_id)
{
    return int_id ^ 0x0200000000000000;
}

int
addr_compare_ip(const addr_ipv6_t * a, const addr_ipv6_t * b)
{
    int res;

    res = a->__in6_u.__u6_addr32[0] - b->__in6_u.__u6_addr32[0];
    if(res)
        return res;
    res = a->__in6_u.__u6_addr32[1] - b->__in6_u.__u6_addr32[1];
    if(res)
        return res;
    res = a->__in6_u.__u6_addr32[2] - b->__in6_u.__u6_addr32[2];
    if(res)
        return res;
    return a->__in6_u.__u6_addr32[3] - b->__in6_u.__u6_addr32[3];
}

int
addr_compare_wpan(const addr_wpan_t * a, const addr_wpan_t * b)
{
    return *a - *b;
}

int
addr_compare_ip_len(const addr_ipv6_t * a, const addr_ipv6_t * b, int bit_len)
{
    int res, i;
    uint64_t mask;

    if(bit_len < 64) {
        for(mask = i = 0; i < bit_len; i++) {
            mask = (mask >> 1) | 0x8000000000000000;
        }
        mask = htobe64(mask);
    } else
        mask = 0xFFFFFFFFFFFFFFFF;

    res = (a->__in6_u.__u6_addr32[0] & (mask & 0xFFFFFFFF)) - (b->__in6_u.__u6_addr32[0] & (mask & 0xFFFFFFFF));
    if(res)
        return res;
    res = (a->__in6_u.__u6_addr32[1] & (mask >> 32)) - (b->__in6_u.__u6_addr32[1] & (mask >> 32));
    if(res)
        return res;

    if(bit_len > 64) {
        for(mask = i = 0; i < bit_len - 64; i++) {
            mask = (mask >> 1) | 0x8000000000000000;
        }
        mask = htobe64(mask);
        res = (a->__in6_u.__u6_addr32[2] & (mask & 0xFFFFFFFF)) - (b->__in6_u.__u6_addr32[2] & (mask & 0xFFFFFFFF));
        if(res)
            return res;
        return (a->__in6_u.__u6_addr32[3] & (mask >> 32)) -
            (b->__in6_u.__u6_addr32[3] & (mask >> 32));
    } else
        return res;
}

int
prefix_compare(const di_prefix_t * a, const di_prefix_t * b)
{
    if(a->length == b->length && !addr_compare_ip_len(&a->prefix, &b->prefix, a->length)) {
        return 0;
    } else {
        return 1;
    }
}

int
addr_prefix_compare(const di_prefix_t * a, const addr_ipv6_t * b)
{
    return addr_compare_ip_len(&a->prefix, b, a->length);
}

bool
addr_is_ip_any(addr_ipv6_t address)
{
    static const addr_ipv6_t any_address = { {{0}} };

    if(!memcmp(&any_address, &address, 16))
        return true;

    return false;
}

bool
addr_is_ip_multicast(addr_ipv6_t address)
{
    return address.__in6_u.__u6_addr8[0] == 0xFF;
}

bool
addr_is_ip_local(addr_ipv6_t address)
{
    return address.__in6_u.__u6_addr8[0] == 0xFE;
}

bool
addr_is_ip_global(addr_ipv6_t address)
{
    return address.__in6_u.__u6_addr8[0] != 0xFE && address.__in6_u.__u6_addr8[0] != 0xFF;
}

bool
addr_is_mac64_broadcast(addr_wpan_t address)
{
    return address == 0xFFFF || address == 0;
}

static int
addr_list_compare(wpan_addr_elt_t * a, wpan_addr_elt_t * b)
{
    return addr_compare_wpan(&a->address, &b->address);
}
