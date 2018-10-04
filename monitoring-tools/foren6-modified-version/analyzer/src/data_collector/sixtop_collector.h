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
 *         6TOP-related data collection
 * \author
 *         Christophe Verdonck <Christophe.Verdonck@student.uantwerpen.be>
 */

#ifndef SIXTOP_COLLECTOR_H
#define	SIXTOP_COLLECTOR_H

#include "../data_info/sixtop_def.h"
#include "../packet_parsers/sixtop_parser.h"

#ifdef	__cplusplus
extern "C" {
#endif

void sixtop_collector_init();
void sixtop_collector_cleanup();
void sixtop_collector_parse_request(sixtop_packet_content_t packet);
void sixtop_collector_parse_response(sixtop_packet_content_t packet);
void sixtop_collector_parse_confirmation(sixtop_packet_content_t packet);

#ifdef	__cplusplus
}
#endif
#endif                          /* SIXTOP_COLLECTOR_H */
