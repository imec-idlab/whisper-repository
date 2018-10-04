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
 *         PCAP file reader, common for all input interfaces.
 * \author
 *         Foren6 Team <foren6@cetic.be>
 *         http://cetic.github.io/foren6/credits.html
 */

#include "pcap_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

struct pcap_file {
    FILE *file;
    int *packet_pos_infile;
    int packet_count;
    int array_size;
};

typedef struct pcap_hdr_s {
    uint32_t magic_number;      /* magic number */
    uint16_t version_major;     /* major version number */
    uint16_t version_minor;     /* minor version number */
    int32_t thiszone;           /* GMT to local correction */
    uint32_t sigfigs;           /* accuracy of timestamps */
    uint32_t snaplen;           /* max length of captured packets, in octets */
    uint32_t network;           /* data link type */
} pcap_hdr_t;

typedef struct pcaprec_hdr_s {
    uint32_t ts_sec;            /* timestamp seconds */
    uint32_t ts_usec;           /* timestamp microseconds */
    uint32_t incl_len;          /* number of octets of packet saved in file */
    uint32_t orig_len;          /* actual length of packet */
} pcaprec_hdr_t;

pcap_file_t
pcap_parser_open(const char *filename)
{
    FILE *fileptr;
    pcaprec_hdr_t header;
    struct pcap_file *file;

    fileptr = fopen(filename, "rb");
    if(fileptr == NULL)
        return NULL;

    file = malloc(sizeof(struct pcap_file));
    file->file = fileptr;

    fseek(file->file, sizeof(pcap_hdr_t), SEEK_SET);

    file->packet_count = 0;
    file->array_size = 0;
    file->packet_pos_infile = NULL;

    while(fread(&header, 1, sizeof(header), file->file) == sizeof(header)) {
        if(file->packet_count >= file->array_size) {
	    fprintf(stderr, "number packets %d \n", file->packet_count);
            if(file->array_size == 0)
                file->array_size = 1;
            else
                file->array_size *= 2;
            file->packet_pos_infile =
                realloc(file->packet_pos_infile,
                        file->array_size * sizeof(int));
        }
        file->packet_pos_infile[file->packet_count] =
            ftell(file->file) - sizeof(header);
        file->packet_count++;

        fseek(file->file, header.incl_len, SEEK_CUR);
    }

    return file;
}

int
pcap_parser_get(pcap_file_t file, int packet_id, void *packet_data, int *len)
{
    struct pcap_file *currentFile = file;
    pcaprec_hdr_t header;

    fseek(currentFile->file, currentFile->packet_pos_infile[packet_id],
          SEEK_SET);
    fread(&header, 1, sizeof(header), currentFile->file);

    if(packet_data == NULL || *len > header.incl_len)
        *len = header.incl_len;

    if(packet_data)
        *len = fread(packet_data, 1, *len, currentFile->file);

    return 0;
}


void
pcap_parser_close(pcap_file_t file)
{
    struct pcap_file *currentFile = file;

    fclose(currentFile->file);

    free(currentFile->packet_pos_infile);
}
