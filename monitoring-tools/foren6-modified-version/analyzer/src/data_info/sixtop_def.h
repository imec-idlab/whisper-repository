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
 *         6TOP Definitions
 * \author
 *         Christophe Verdonck <Christophe.Verdonck@student.uantwerpen.be>
 */

#ifndef SIXTOP_DEF_H
#define	SIXTOP_DEF_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "6lowpan_def.h"
#include "../uthash.h"
#include "../utlist.h"

#define SIXTOP_CODE_SUCCESS 0
#define SIXTOP_CODE_ADD 1
#define SIXTOP_CODE_DELETE 2
#define SIXTOP_CODE_RELOCATE 3
#define SIXTOP_CODE_COUNT 4
#define SIXTOP_CODE_LIST 5
#define SIXTOP_CODE_SIGNAL 6
#define SIXTOP_CODE_CLEAR 7

#define SIXTOP_TYPE_REQUEST 0
#define SIXTOP_TYPE_RESPONSE 1
#define SIXTOP_TYPE_CONFIRMATION 2

typedef enum sixtop_code {
	CODE_NONE,
	CODE_SUCCESS,
	CODE_ADD,
	CODE_DELETE,
	CODE_RELOCATE,
	CODE_COUNT,
	CODE_LIST,
	CODE_SIGNAL,
	CODE_CLEAR,
	CODE_UNKNOWN
} sixtop_code_e;

typedef enum sixtop_type {
	TYPE_NONE,
	TYPE_REQUEST,
	TYPE_RESPONSE,
	TYPE_CONFIRMATION,
	TYPE_UNKNOWN
} sixtop_type_e;	

typedef struct cell_options {
	bool tx;
	bool rx;
	bool shared;
} cell_options_t;

	//typedef struct cell cell_t;

typedef struct cell {
	unsigned int cell_id; //just slot_offset concat with channel_offset
	uint16_t slot_offset;
	uint16_t channel_offset;
	struct cell* next; //utlist
} cell_t;	

typedef struct neighbor_cell_key {
	unsigned int cell_id;
} neighbor_cell_key_t;
	
typedef struct neighbor_cell {
	neighbor_cell_key_t key;
	cell_options_t cell_options;
	UT_hash_handle hh;
} neighbor_cell_t;

typedef struct sixtop_neighbor_key {
	addr_wpan_t neighbor_mac;
} sixtop_neighbor_key_t;
	
typedef struct sixtop_neighbor {
	sixtop_neighbor_key_t key;
	struct neighbor_cell* cells;
	UT_hash_handle hh;
} sixtop_neighbor_t;

typedef struct sixtop_packet_data {
	int packet_id;
	packet_info_t pkt_info; //to be in line with front-end, only wpan used
	uint8_t version;
	sixtop_type_e type;
	sixtop_code_e code;
	uint8_t sfid;
	uint8_t seqnum;
	cell_options_t cell_options;
	uint8_t num_cells;
	uint8_t cell_list_length;
	cell_t* cells; // linked list of cells
} sixtop_packet_content_t;	

#ifdef	__cplusplus
}
#endif
#endif                          /* SIXTOP_DEF_H */
