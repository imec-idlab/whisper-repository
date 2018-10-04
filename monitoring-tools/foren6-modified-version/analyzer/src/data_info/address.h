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

#ifndef ADDRESS_H
#define	ADDRESS_H

#include <arpa/inet.h>
#include <stdbool.h>

#ifdef	__cplusplus
extern "C" {
#endif

#define ADDR_MAC64_BROADCAST 0xFFFF

    typedef struct in6_addr addr_ipv6_t;
    typedef uint64_t addr_wpan_t;

#ifdef __APPLE__
#define __in6_u __u6_addr
#ifndef s6_addr16
#define s6_addr16   __u6_addr.__u6_addr16
#endif
#endif

typedef struct di_prefix {
    uint8_t length;
    addr_ipv6_t prefix;
} di_prefix_t;

typedef struct wpan_addr_list {
    addr_wpan_t address;
    struct wpan_addr_list *next;
} wpan_addr_elt_t, *wpan_addr_list_t;

void init_ipv6_addr(addr_ipv6_t * addr);
void init_prefix(di_prefix_t * prefix);

wpan_addr_elt_t *addr_wpan_add_to_list(wpan_addr_list_t * list,
                                       addr_wpan_t address);
wpan_addr_elt_t *addr_wpan_del_from_list(wpan_addr_list_t * list,
                                         addr_wpan_t address);

addr_wpan_t addr_get_mac64_from_ip(addr_ipv6_t ip_address);
addr_ipv6_t addr_get_local_ip_from_mac64(addr_wpan_t mac_address);
addr_ipv6_t addr_get_global_ip_from_mac64(di_prefix_t prefix,
                                          addr_wpan_t mac_address);
uint64_t addr_get_int_id_from_mac64(addr_wpan_t mac_address);
addr_wpan_t addr_get_mac64_from_int_id(uint64_t int_id);

//For list search
//Return 0 if equal
int addr_compare_ip(const addr_ipv6_t * a, const addr_ipv6_t * b);
int addr_compare_wpan(const addr_wpan_t * a, const addr_wpan_t * b);
int addr_compare_ip_len(const addr_ipv6_t * a, const addr_ipv6_t * b, int bit_len); //Compare len bits
int addr_prefix_compare(const di_prefix_t * a, const addr_ipv6_t * b);

int prefix_compare(const di_prefix_t * a, const di_prefix_t * b);

bool addr_is_ip_any(addr_ipv6_t address);
bool addr_is_ip_multicast(addr_ipv6_t address);
bool addr_is_ip_local(addr_ipv6_t address);
bool addr_is_ip_global(addr_ipv6_t address);
bool addr_is_mac64_broadcast(addr_wpan_t address);

#ifdef	__cplusplus
}
#endif
#endif                          /* ADDRESS_H */
