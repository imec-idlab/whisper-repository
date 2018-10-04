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

#include <stdio.h>

#include "sixtop_collector.h"
#include "../data_info/node.h"
#include "../data_info/rpl_data.h"
#include "../uthash.h"

typedef struct sixtop_collector_data_key {
	uint8_t sfid;
	uint8_t seqnum;
	addr_wpan_t initiator;
	addr_wpan_t receiver;	
} sixtop_collector_data_key_t;

typedef struct sixtop_collector_data {
	sixtop_collector_data_key_t key;
	sixtop_code_e code;
	bool three_step;
	cell_options_t cell_options;
	uint8_t num_cells;
	cell_t* cells_to_relocate;
	UT_hash_handle hh;
} sixtop_collector_data;


struct sixtop_collector_data* request_table = NULL;

void
sixtop_collector_init()
{
	request_table = NULL;
}

void
sixtop_collector_cleanup()
{
	//free memory
	sixtop_collector_data *request,*tmp;
	HASH_ITER(hh,request_table,request,tmp){
		cell_t *cell,*cell_tmp;
		LL_FOREACH_SAFE(request->cells_to_relocate,cell,cell_tmp){
			LL_DELETE(request->cells_to_relocate,cell);
			free(cell);
		}
		free(request);
	}
}

cell_options_t
sixtop_collector_invert_celloptions(cell_options_t options)
{
	cell_options_t inverted_options;
	inverted_options.tx = options.rx;
	inverted_options.rx = options.tx;
	inverted_options.shared = options.shared;
	return inverted_options;
}

bool
sixtop_collector_valid_celloptions(cell_options_t options)
{
	if(options.rx == 0 && options.tx == 0){ //shared 0 or 1 invalid
		return false;
	}
	return true;
}

void
sixtop_collector_execute_request(sixtop_collector_data *request,sixtop_packet_content_t packet)
{
	di_node_t *recv_node;	
	bool node_created;
	di_node_ref_t node_ref;
	node_ref_init(&node_ref, request->key.receiver);
	recv_node = rpldata_get_node(&node_ref, HVM_CreateIfNonExistant, &node_created);
	
	di_node_t *init_node;	
	node_ref_init(&node_ref, request->key.initiator);
	init_node = rpldata_get_node(&node_ref, HVM_CreateIfNonExistant, &node_created);

	//Get the right neighbor or create one if it does not yet exist.
	sixtop_neighbor_key_t init_key;
	init_key.neighbor_mac = request->key.receiver;
	sixtop_neighbor_t* init_neighbor = node_get_sixtop_neighbor(init_node,&init_key);
	if(init_neighbor == NULL){
		init_neighbor = calloc(1,sizeof(sixtop_neighbor_t));
		init_neighbor->key = init_key;
		node_add_sixtop_neighbor(init_node,init_neighbor);
		init_neighbor = node_get_sixtop_neighbor(init_node,&init_key);
	}
	
	sixtop_neighbor_key_t recv_key;
	recv_key.neighbor_mac = request->key.initiator;
	sixtop_neighbor_t* recv_neighbor = node_get_sixtop_neighbor(recv_node,&recv_key);
	if(recv_neighbor == NULL){
		recv_neighbor = calloc(1,sizeof(sixtop_neighbor_t));
		recv_neighbor->key = recv_key;
		node_add_sixtop_neighbor(recv_node,recv_neighbor);
		recv_neighbor = node_get_sixtop_neighbor(recv_node,&recv_key);
	}

	cell_t* cell;
	switch(request->code){
	case CODE_ADD:
		LL_FOREACH(packet.cells,cell){
			neighbor_cell_t* new_cell_init = calloc(1,sizeof(neighbor_cell_t));
			neighbor_cell_key_t init_key;
			init_key.cell_id = cell->cell_id;
			new_cell_init->key = init_key;
			new_cell_init->cell_options = request->cell_options;
			node_add_sixtop_cell(init_node,init_neighbor,new_cell_init);

			neighbor_cell_t* new_cell_recv = calloc(1,sizeof(neighbor_cell_t));
			neighbor_cell_key_t recv_key;
			recv_key.cell_id = cell->cell_id;			
			new_cell_recv->key = recv_key;
			new_cell_recv->cell_options = sixtop_collector_invert_celloptions(request->cell_options);
			node_add_sixtop_cell(recv_node,recv_neighbor,new_cell_recv);
		}
		//fprintf(stderr,"ADDED!\n");		
		break;
	case CODE_DELETE:
		LL_FOREACH(packet.cells,cell){
			node_delete_sixtop_cell(init_node,init_neighbor,(neighbor_cell_key_t*) &cell->cell_id);
			node_delete_sixtop_cell(recv_node,recv_neighbor,(neighbor_cell_key_t*) &cell->cell_id);
		}
		//fprintf(stderr,"Deleted!\n");
		break;
	case CODE_RELOCATE:		
		LL_FOREACH(packet.cells,cell){
			//NOTE: Relocate can be partly succesfull.
			//delete the first item in the relocate list
			neighbor_cell_key_t* key = (neighbor_cell_key_t*) &request->cells_to_relocate->cell_id;
			node_delete_sixtop_cell(init_node,init_neighbor,key);
			node_delete_sixtop_cell(recv_node,recv_neighbor,key);
			//delete first item of list
			LL_DELETE(request->cells_to_relocate,request->cells_to_relocate);
			//Add replacing cell
			neighbor_cell_t* new_cell_init = calloc(1,sizeof(neighbor_cell_t));
			neighbor_cell_key_t init_key;
			init_key.cell_id = cell->cell_id;
			new_cell_init->key = init_key;
			new_cell_init->cell_options = request->cell_options;
			node_add_sixtop_cell(init_node,init_neighbor,new_cell_init);

			neighbor_cell_t* new_cell_recv = calloc(1,sizeof(neighbor_cell_t));
			neighbor_cell_key_t recv_key;
			recv_key.cell_id = cell->cell_id;			
			new_cell_recv->key = recv_key;
			new_cell_recv->cell_options = sixtop_collector_invert_celloptions(request->cell_options);
			node_add_sixtop_cell(recv_node,recv_neighbor,new_cell_recv);			
		}
		break;
	case CODE_CLEAR:
		//automatically removes the cells as well
		node_delete_sixtop_neighbor(init_node,&init_neighbor->key);
		node_delete_sixtop_neighbor(recv_node,&recv_neighbor->key);
		break;
	default:
		fprintf(stderr,"Invalid request code received.");
		break;
	}

	//remove neighbors with 0 cells:
	if(node_sixtop_neighbor_is_empty(init_node,&init_neighbor->key))
		node_delete_sixtop_neighbor(init_node,&init_neighbor->key);
	if(node_sixtop_neighbor_is_empty(recv_node,&recv_neighbor->key))
		node_delete_sixtop_neighbor(recv_node,&recv_neighbor->key);	

	//release the request
	cell_t *elt,*tmp;
	LL_FOREACH_SAFE(request->cells_to_relocate,elt,tmp) {
      LL_DELETE(request->cells_to_relocate,elt);
      free(elt);
    }
	HASH_DELETE(hh,request_table,request);
	free(request);	
}

void
sixtop_collector_parse_request(sixtop_packet_content_t packet)
{
	//Create node we heard sending a request if it not yet known to us.
	bool node_created;
	di_node_ref_t node_ref;	    
	node_ref_init(&node_ref, packet.pkt_info.src_wpan_address);
	rpldata_get_node(&node_ref, HVM_CreateIfNonExistant, &node_created);
	// --
	
	if(!(packet.code == CODE_ADD ||
	   packet.code == CODE_DELETE ||
	   packet.code == CODE_RELOCATE ||
	   packet.code == CODE_CLEAR))
		return; // count/list/signal do not have impact on our schedule and don't need processing
	if(!sixtop_collector_valid_celloptions(packet.cell_options) && packet.code != CODE_CLEAR){
		fprintf(stderr,"6top: Received invalid celloptions. Dropping packet.\n");
		return;
	}
	
	sixtop_collector_data *request,*old_request;

	request = calloc(1,sizeof(sixtop_collector_data));
	request->key.sfid = packet.sfid;
	request->key.seqnum = packet.seqnum;
	request->key.initiator = packet.pkt_info.src_wpan_address;
	request->key.receiver = packet.pkt_info.dst_wpan_address;
	request->code = packet.code;
	if(packet.code == CODE_RELOCATE){
		if(packet.cell_list_length == packet.num_cells)
			request->three_step = true;			
	}
	else if(packet.code == CODE_CLEAR){
		request->three_step = false; //always two-step
	}
	else{
		if(packet.cell_list_length == 0)
			request->three_step = true;		
	}
	request->cell_options = packet.cell_options;
	request->num_cells = packet.num_cells;
	if(packet.code == CODE_RELOCATE){
		if(packet.cell_list_length < packet.num_cells){
			//error. Clear memory
			fprintf(stderr,"6top: Relocate with less relocated cells than advertised.\n");
			free(request);
			return;
		}
		int i = 0;
		cell_t* cell = packet.cells;
		while(i<packet.num_cells){
			//Copy cell
			cell_t* new_cell = calloc(1,sizeof(cell_t));
			new_cell->cell_id = cell->cell_id;
			//Add new cell to relocate celllist
			LL_APPEND(request->cells_to_relocate,new_cell);
			//move on
			cell = cell->next;
			i++;
		}
		
	}
	HASH_FIND(hh,request_table,&request->key,sizeof(sixtop_collector_data_key_t),old_request);
	if(old_request != NULL){
		fprintf(stderr,"6top: Got a request which was already received. Replacing request.\n");
		HASH_DELETE(hh,request_table,old_request);
		HASH_ADD(hh,request_table,key,sizeof(sixtop_collector_data_key_t),request);
	}
	else{
		HASH_ADD(hh,request_table,key,sizeof(sixtop_collector_data_key_t),request);
	}	
}

void
sixtop_collector_parse_response(sixtop_packet_content_t packet)
{
	//Create node we heard sending a response if it not yet known to us.
	bool node_created;
	di_node_ref_t node_ref;
	node_ref_init(&node_ref, packet.pkt_info.src_wpan_address);
	rpldata_get_node(&node_ref, HVM_CreateIfNonExistant, &node_created);
	// --
	
	sixtop_collector_data_key_t* key = calloc(1,sizeof(sixtop_collector_data_key_t));
	key->sfid =  packet.sfid;
	key->seqnum = packet.seqnum;
	key->initiator =  packet.pkt_info.dst_wpan_address; //Switched role because this is response
	key->receiver = packet.pkt_info.src_wpan_address; //same same

	sixtop_collector_data *request;
	HASH_FIND(hh,request_table,key,sizeof(sixtop_collector_data_key_t),request);
	free(key);

	if(request == NULL){
		//reponse to unknown request
		//fprintf(stderr,"response to unknown request \n");
		return;
	}
	//request exists at this point
	if(packet.code != CODE_SUCCESS){
		//error response
		//release the request
		cell_t *elt,*tmp;
		LL_FOREACH_SAFE(request->cells_to_relocate,elt,tmp) {
			LL_DELETE(request->cells_to_relocate,elt);
			free(elt);
		}
		HASH_DELETE(hh,request_table,request);		
		free(request);		
		return;
	}
	if(request->three_step){
		return; //nothing to do at this point, wait for confirmation
	}

	sixtop_collector_execute_request(request,packet);	
}

void
sixtop_collector_parse_confirmation(sixtop_packet_content_t packet)
{
	//Create node we heard sending a confirmation if it not yet known to us.
	bool node_created;
	di_node_ref_t node_ref;
	node_ref_init(&node_ref, packet.pkt_info.src_wpan_address);
	rpldata_get_node(&node_ref, HVM_CreateIfNonExistant, &node_created);
	// --
	
	sixtop_collector_data_key_t* key = calloc(1,sizeof(sixtop_collector_data_key_t));
	key->sfid =  packet.sfid;
	key->seqnum = packet.seqnum;
	key->initiator =  packet.pkt_info.src_wpan_address;
	key->receiver = packet.pkt_info.dst_wpan_address;

	sixtop_collector_data *request;
	HASH_FIND(hh,request_table,key,sizeof(sixtop_collector_data_key_t),request);
	free(key);

	if(request == NULL){
		//confirmation to unknown request
		//fprintf(stderr,"confirmation to unknown request \n");
		return;
	}
	//request exists at this point
	if(packet.code != CODE_SUCCESS){
		//error response
		//release the request
		cell_t *elt,*tmp;
		LL_FOREACH_SAFE(request->cells_to_relocate,elt,tmp) {
			LL_DELETE(request->cells_to_relocate,elt);
			free(elt);
		}
		HASH_DELETE(hh,request_table,request);		
		free(request);		
		return;
	}
	sixtop_collector_execute_request(request,packet);
}
