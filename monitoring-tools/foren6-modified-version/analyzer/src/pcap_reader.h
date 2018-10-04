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

#ifndef PCAP_READER_H
#define PCAP_READER_H


#ifdef	__cplusplus
extern "C" {
#endif

typedef void *pcap_file_t;

pcap_file_t pcap_parser_open(const char *filename);
int pcap_parser_get(pcap_file_t file, int packet_id, void *packet_data,
                    int *len);
void pcap_parser_close(pcap_file_t file);


#ifdef	__cplusplus
}
#endif
#endif                          // PCAP_READER_H
