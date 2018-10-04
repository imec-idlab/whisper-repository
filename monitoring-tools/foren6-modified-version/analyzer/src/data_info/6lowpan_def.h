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
 *         6LoWPAN Information
 * \author
 *         Foren6 Team <foren6@cetic.be>
 *         http://cetic.github.io/foren6/credits.html
 */


#ifndef SIXLOWPAN_DEF_H_
#define SIXLOWPAN_DEF_H_

#include <stdbool.h>
#include <stdint.h>
#include <netinet/in.h>
#include <sys/time.h>
#include "address.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef enum packet_type {
    PT_None,
    PT_ICMPv6_Unknown,
    PT_PING_ECHO,
    PT_PING_REPLY,
    PT_NDP_RS,
    PT_NDP_RA,
    PT_NDP_NS,
    PT_NDP_NA,
    PT_NDP_Redirect,
    PT_6ND_DAR,
    PT_6ND_DAC,
    PT_RPL_Unknown,
    PT_DIS,
    PT_DIO,
    PT_DAO,
    PT_DAO_ACK,
    PT_UDP,
    PT_TCP,
    PT_6P_ADD, //6top packet types
    PT_6P_DELETE,
    PT_6P_RELOCATE,
    PT_6P_COUNT,
    PT_6P_LIST,
    PT_6P_CLEAR,
    PT_6P_SIGNAL,
    PT_6P_RESPONSE,
    PT_6P_CONFIRMATION, //--
    PT_IPv6_Unknown
} packet_type_e;

typedef struct packet_info {
    addr_wpan_t src_wpan_address;
    addr_wpan_t dst_wpan_address;
    addr_ipv6_t src_ip_address;
    addr_ipv6_t dst_ip_address;
    double timestamp;
    int hop_limit;
    packet_type_e type;
} packet_info_t;

typedef struct sixlowpan_config {
    bool has_seen_local_address;
    bool is_custom_local_address;
    addr_ipv6_t local_address;

    bool has_seen_global_address;
    bool is_custom_global_address;
    addr_ipv6_t global_address;

    bool is_prefix_valid;
} sixlowpan_config_t;

typedef struct sixlowpan_config_delta {
    bool has_changed;

    bool local_address;
    bool global_address;
} sixlowpan_config_delta_t;

typedef struct sixlowpan_statistics {
    int packet_count;
} sixlowpan_statistics_t;

typedef struct sixlowpan_statistics_delta {
    bool has_changed;
    int packet_count;
} sixlowpan_statistics_delta_t;

typedef struct sixlowpan_errors {
    int invalid_ip;
    int invalid_prefix;
} sixlowpan_errors_t;

typedef struct sixlowpan_errors_delta {
    bool has_changed;
    int invalid_ip;
    int invalid_prefix;
} sixlowpan_errors_delta_t;

void init_sixlowpan_config(sixlowpan_config_t * config);
void init_sixlowpan_statistics(sixlowpan_statistics_t * statistics);
void init_sixlowpan_statistics_delta(sixlowpan_statistics_delta_t *
                                     delta);
void init_sixlowpan_errors(sixlowpan_errors_t * errors);
void init_sixlowpan_errors_delta(sixlowpan_errors_delta_t * delta);

void sixlowpan_config_delta(const sixlowpan_config_t * left,
                            const sixlowpan_config_t * right,
                            sixlowpan_config_delta_t * delta);
void sixlowpan_statistics_delta(const sixlowpan_statistics_t * left,
                                const sixlowpan_statistics_t * right,
                                sixlowpan_statistics_delta_t * delta);
void sixlowpan_errors_delta(const sixlowpan_errors_t * left,
                            const sixlowpan_errors_t * right,
                            sixlowpan_errors_delta_t * delta);

#ifdef  __cplusplus
}
#endif
#endif
