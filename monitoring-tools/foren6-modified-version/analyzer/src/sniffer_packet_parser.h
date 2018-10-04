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
 *         Sniffer Packet Parser
 * \author
 *         Foren6 Team <foren6@cetic.be>
 *         http://cetic.github.io/foren6/credits.html
 */


#ifndef SNIFFER_PACKET_PARSER_H
#define	SNIFFER_PACKET_PARSER_H

#include <stdbool.h>
#include <stdint.h>
#include <netinet/in.h>
#include <sys/time.h>

#include "data_info/rpl_instance.h"
#include "data_info/dodag.h"
#include "data_info/rpl_def.h"

#ifdef	__cplusplus
extern "C" {
#endif

//Initialize sniffer parser
void sniffer_parser_init();
void sniffer_parser_cleanup();
void sniffer_parser_reset();

//Give data to parse to parser
//Call sensor_info_collector_parse_packet when a packet has been fully received
void sniffer_parser_parse_data(const unsigned char *data, int len,
                               struct timeval timestamp);

int sniffer_parser_get_packet_count();
void sniffer_parser_pause_parser(bool pause);

void sniffer_parser_create_out();
void sniffer_parser_close_out();

#ifdef	__cplusplus
}
#endif
#endif                          /* SNIFFER_PACKET_PARSER_H */
