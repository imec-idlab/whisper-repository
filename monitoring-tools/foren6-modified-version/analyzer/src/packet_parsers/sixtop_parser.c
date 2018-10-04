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
 *         6TOP Messages Parser
 * \author
 *         Christophe Verdonck <Christophe.Verdonck@student.uantwerpen.be>
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include <locale.h>

#include "sixtop_parser.h"
#include "../data_info/6lowpan_def.h"
#include "../data_info/sixtop_def.h"
#include "../data_collector/rpl_event_callbacks.h" //packet
#include "../data_collector/sixtop_collector.h"
#include "../utlist.h"

#ifdef __APPLE__
#include "../apple-endian.h"
#endif

#include "../data_info/metric.h"

static sixtop_packet_content_t current_packet;

static double sixtop_parser_metric_etx_to_double(uint64_t value);
static char *sixtop_parser_metric_etx_to_string(uint64_t value);

static void sixtop_parser_cleanup();
static void sixtop_parser_begin_packet();
static void sixtop_parser_parse_field(const char *nameStr, const char *showStr,
                                   const char *valueStr, int64_t valueInt);
static void sixtop_parser_end_packet();

parser_t
sixtop_parser_register()
{
    parser_t parser;
    di_metric_type_t metric_etx;

    parser.parser_name = "sixtop";
    parser.cleanup = &sixtop_parser_cleanup;
    parser.begin_packet = &sixtop_parser_begin_packet;
    parser.parse_field = &sixtop_parser_parse_field;
    parser.end_packet = &sixtop_parser_end_packet;

    metric_etx.name = "ETX";
    metric_etx.to_display_value = &sixtop_parser_metric_etx_to_double;
    metric_etx.to_string = &sixtop_parser_metric_etx_to_string;
    metric_add_type(&metric_etx);

    sixtop_collector_init(); //initiate the collector (has state)

    return parser;    
}

static double
sixtop_parser_metric_etx_to_double(uint64_t value)
{
    return value / 128.0;
}

static char *
sixtop_parser_metric_etx_to_string(uint64_t value)
{
    char *str = malloc(10);

    sprintf(str, "%f", value / 128.0);
    return str;
}

static void
sixtop_parser_cleanup()
{
	sixtop_collector_cleanup();
}

static void
sixtop_parser_begin_packet()
{
	memset(&current_packet, 0, sizeof(current_packet));
	current_packet.type = TYPE_NONE;
	current_packet.code = CODE_NONE;
	current_packet.cells = NULL;
	//fprintf(stderr, "6Top parser called with new packet\n");
}

static void
sixtop_parser_parse_field(const char *nameStr, const char *showStr,
                       const char *valueStr, int64_t valueInt)
{
    if(valueStr == NULL) {
        if(!strcmp(nameStr, "frame.number")) {
            current_packet.packet_id = strtol(showStr, NULL, 10) - 1;   //wireshark's first packet is number 1
        } else if(!strcmp(nameStr, "frame.time_epoch")) {
            char *oldlocale = setlocale(LC_NUMERIC, "C");

            current_packet.pkt_info.timestamp = strtod(showStr, NULL);
            setlocale(LC_NUMERIC, oldlocale);
        }
	} else if(!strcmp(nameStr, "wpan.src64")) {
        uint64_t addr = strtoull(valueStr, NULL, 16);
        current_packet.pkt_info.src_wpan_address = htobe64(addr);
    } else if(!strcmp(nameStr, "wpan.dst64")) {
        uint64_t addr = strtoull(valueStr, NULL, 16);
        current_packet.pkt_info.dst_wpan_address = htobe64(addr);
    } else if(!strcmp(nameStr, "wpan.dst_addr64")) {
        uint64_t addr = strtoull(valueStr, NULL, 16);
        current_packet.pkt_info.dst_wpan_address = htobe64(addr);
    } else if(current_packet.code == CODE_NONE &&
			  !strcmp(nameStr, "wpan.6top_code")){
		switch (valueInt) {
		case SIXTOP_CODE_SUCCESS:
			current_packet.code = CODE_SUCCESS;
			break;
		case SIXTOP_CODE_ADD:
			current_packet.code = CODE_ADD;
			break;
		case SIXTOP_CODE_DELETE:
			current_packet.code = CODE_DELETE;
			break;
		case SIXTOP_CODE_RELOCATE:
			current_packet.code = CODE_RELOCATE;
			break;			
		case SIXTOP_CODE_COUNT:
			current_packet.code = CODE_COUNT;
			break;
		case SIXTOP_CODE_LIST:
			current_packet.code = CODE_LIST;
			break;
		case SIXTOP_CODE_SIGNAL:
			current_packet.code = CODE_SIGNAL;
			break;
		case SIXTOP_CODE_CLEAR:
			current_packet.code = CODE_CLEAR;
			break;
		default:
			current_packet.code = CODE_UNKNOWN;
			break;
		}
	} else if(current_packet.type == TYPE_NONE &&
			  !strcmp(nameStr, "wpan.6top_type")){
		switch(valueInt){
		case SIXTOP_TYPE_REQUEST:
			current_packet.type = TYPE_REQUEST;
			break;
		case SIXTOP_TYPE_RESPONSE:
			current_packet.type = TYPE_RESPONSE;
			break;
		case SIXTOP_TYPE_CONFIRMATION:
			current_packet.type = TYPE_CONFIRMATION;
			break;
		default:
			current_packet.type = TYPE_UNKNOWN;
			break;
		}
	} else if(!strcmp(nameStr, "wpan.6top_version")){
		current_packet.version = valueInt;
	} else if(!strcmp(nameStr, "wpan.6top_sfid")){
		current_packet.sfid = valueInt;
	} else if(!strcmp(nameStr, "wpan.6top_seqnum")){
		current_packet.seqnum = valueInt;
	} else if(!strcmp(nameStr, "wpan.6top_cell_option_tx")){
		current_packet.cell_options.tx = valueInt;
	} else if(!strcmp(nameStr, "wpan.6top_cell_option_rx")){
		current_packet.cell_options.rx = valueInt;
	} else if(!strcmp(nameStr, "wpan.6top_cell_option_shared")){
		current_packet.cell_options.shared = valueInt;
	} else if(!strcmp(nameStr, "wpan.6top_num_cells")){
		current_packet.num_cells = valueInt;
	} else if(!strcmp(nameStr, "wpan.6top_cell")){
	    cell_t* cell = calloc(1,sizeof(cell_t));
	    cell->cell_id = valueInt;
	    LL_APPEND(current_packet.cells,cell);
	    current_packet.cell_list_length++;
    }
}

static void
sixtop_parser_set_packet_type(sixtop_packet_content_t *current_packet)
{
	switch(current_packet->type){
	case TYPE_REQUEST:
		switch(current_packet->code){
		case CODE_ADD:
			current_packet->pkt_info.type = PT_6P_ADD;			
			break;
		case CODE_DELETE:
			current_packet->pkt_info.type = PT_6P_DELETE;			
			break;
		case CODE_RELOCATE:
			current_packet->pkt_info.type = PT_6P_RELOCATE;
			break;
		case CODE_COUNT:
			current_packet->pkt_info.type = PT_6P_COUNT;
			break;
		case CODE_LIST:
			current_packet->pkt_info.type = PT_6P_LIST;
			break;
		case CODE_SIGNAL:
			current_packet->pkt_info.type = PT_6P_SIGNAL;
			break;
		case CODE_CLEAR:
			current_packet->pkt_info.type = PT_6P_CLEAR;
			break;
		default:
			break;
		}
		break;
	case TYPE_RESPONSE:
		current_packet->pkt_info.type = PT_6P_RESPONSE;
		break;
	case TYPE_CONFIRMATION:
		current_packet->pkt_info.type = PT_6P_CONFIRMATION;
		break;
	case TYPE_UNKNOWN:
		break;
	default:
		break;
	}
}

static void
sixtop_parser_end_packet()
{
	if(current_packet.version != 0){
		fprintf(stderr,"Packet encountered with unsupported 6top version. Only version 0 is implemented.\n");
		return;
	}
	if(current_packet.type != TYPE_NONE && current_packet.code != CODE_NONE){
		sixtop_parser_set_packet_type(&current_packet);
		rpl_event_packet(current_packet.packet_id, &current_packet.pkt_info);
		switch(current_packet.type){
		case TYPE_REQUEST:
			sixtop_collector_parse_request(current_packet);
			break;
		case TYPE_RESPONSE:
			sixtop_collector_parse_response(current_packet);
			break;
		case TYPE_CONFIRMATION:
			sixtop_collector_parse_confirmation(current_packet);
			break;
		case TYPE_UNKNOWN:
			fprintf(stderr,"sixtop packet received with unknown type");
			break;
		default:
			break; // any other packet
		}
		//cleanup current_packet
		cell_t *cell,*tmp;
		LL_FOREACH_SAFE(current_packet.cells,cell,tmp){
			LL_DELETE(current_packet.cells,cell);
			free(cell);
		}
		
		rpl_event_commit_changed_objects(current_packet.packet_id,
                                         current_packet.pkt_info.timestamp);
	}
}
