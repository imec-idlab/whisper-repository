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

#ifndef RPL_COLLECTOR_H
#define	RPL_COLLECTOR_H

#include "../sniffer_packet_parser.h"
#include "../data_info/6lowpan_def.h"

#ifdef	__cplusplus
extern "C" {
#endif

void rpl_collector_parse_dio(packet_info_t pkt_info, rpl_dio_t * dio,
                             rpl_dio_opt_config_t * dodag_config,
                             rpl_dio_opt_metric_t * metric,
                             rpl_dio_opt_prefix_t * prefix,
                             rpl_dio_opt_route_t * route_info);
void rpl_collector_parse_dao(packet_info_t pkt_info, rpl_dao_t * dao,
                             rpl_dao_opt_target_t * target,
                             rpl_dao_opt_transit_t * transit);
void rpl_collector_parse_dao_ack(packet_info_t pkt_info,
                                 rpl_dao_ack_t * dao_ack);
void rpl_collector_parse_dis(packet_info_t pkt_info,
                             rpl_dis_opt_info_req_t * request);
void rpl_collector_parse_data(packet_info_t pkt_info,
                              rpl_hop_by_hop_opt_t * rpl_info);

#ifdef	__cplusplus
}
#endif
#endif                          /* RPL_COLLECTOR_H */
