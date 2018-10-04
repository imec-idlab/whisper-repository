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
 *         PCAP input interface
 * \author
 *         Foren6 Team <foren6@cetic.be>
 */

#include "interface_pcap.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pcap/pcap.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

#if __APPLE__
#define pthread_timedjoin_np(...) (1)
#endif

#ifndef DLT_IEEE802_15_4_NOFCS
#define DLT_IEEE802_15_4_NOFCS 230
#endif

static const char *interface_name = "pcap";
static const unsigned int interface_parameters = INTERFACE_DEVICE;

typedef struct {
    FILE *pf;
    pcap_t *pc;
    bool capture_packets;
    pthread_t thread;
    long first_offset;
} interface_handle_t;           //*ifreader_t

static void interface_init();
static ifreader_t interface_open(const char *target, int channel, int baudrate);
static bool interface_start(ifreader_t handle);
static void interface_stop(ifreader_t handle);
static void interface_close(ifreader_t handle);
static void *interface_thread_process_input(void *data);
static void interface_packet_handler(u_char * param,
                                     const struct pcap_pkthdr *header,
                                     const u_char * pkt_data);

int
interface_get_version()
{
    return 1;
}

interface_t
interface_register()
{
    interface_t interface;

    memset(&interface, 0, sizeof(interface));

    interface.interface_name = interface_name;
    interface.parameters = interface_parameters;
    interface.init = &interface_init;
    interface.open = &interface_open;
    interface.close = &interface_close;
    interface.start = &interface_start;
    interface.stop = &interface_stop;

    return interface;
}

static void
interface_init()
{
    fprintf(stderr, "%s interface initialized\n", interface_name);
}

static ifreader_t
interface_open(const char *target, int channel, int baudrate)
{
    interface_handle_t *handle;
    char errbuf[PCAP_ERRBUF_SIZE];

    (void) channel;
    (void) baudrate;

    handle = (interface_handle_t *) calloc(1, sizeof(interface_handle_t));
    if(!handle)
        return NULL;

    handle->capture_packets = false;

    handle->pf = fopen(target, "r");
    if(handle->pf == NULL) {
        fprintf(stderr, "Cannot open target %s: %s\n", target, strerror(errno));
        free(handle);
        return NULL;
    }
    handle->pc = pcap_fopen_offline(handle->pf, errbuf);
    if(handle->pc == NULL) {
        fprintf(stderr, "Cannot read target %s: %s\n", target, errbuf);
        fclose(handle->pf);
        free(handle);
        return NULL;
    }

    ifreader_t instance = interfacemgr_create_handle(target);
    instance->interface_data = handle;

    if (pcap_datalink(handle->pc) == DLT_EN10MB || pcap_datalink(handle->pc) == DLT_LINUX_SLL) {
        instance->encap_dlt = pcap_datalink(handle->pc);
    } else if(pcap_datalink(handle->pc) == DLT_IEEE802_15_4) {
        instance->encap_dlt = -1;
        instance->fcs = true;
    } else if (pcap_datalink(handle->pc) == DLT_IEEE802_15_4_NOFCS) {
        instance->encap_dlt = -1;
        instance->fcs = false;
    } else {
        fprintf(stderr,
                "This program only supports 802.15.4 and Ethernet or Linux \"cooked\" "
                "encapsulated 802.15.4 sniffers (DLT: %d)\n",
                pcap_datalink(handle->pc));
        interfacemgr_destroy_handle(instance);
        free(handle);
        return NULL;
    }
    handle->first_offset = ftell(handle->pf);
    return instance;
}

static bool
interface_start(ifreader_t handle)
{
    interface_handle_t *descriptor = handle->interface_data;

    if(descriptor->capture_packets == false) {
        descriptor->capture_packets = true;
        if(fseek(descriptor->pf, descriptor->first_offset, SEEK_SET) == -1) {
            fprintf(stderr, "warning, fseek() failed : %s\n", strerror(errno));
        }
        pthread_create(&descriptor->thread, NULL, &interface_thread_process_input, handle);
    }
    return true;
}

static void
interface_stop(ifreader_t handle)
{
    interface_handle_t *descriptor = handle->interface_data;

    if(descriptor->capture_packets == true) {
        struct timespec timeout = { 3, 0 };

        descriptor->capture_packets = false;

        if(pthread_timedjoin_np(descriptor->thread, NULL, &timeout) != 0) {
            pthread_cancel(descriptor->thread);
            pthread_join(descriptor->thread, NULL);
        }
    }
}

static void
interface_close(ifreader_t handle)
{
    interface_handle_t *descriptor = handle->interface_data;

    interface_stop(handle);

    pcap_close(descriptor->pc);
    free(descriptor);
    interfacemgr_destroy_handle(handle);
}

static void *
interface_thread_process_input(void *data)
{
    ifreader_t handle = (ifreader_t) data;
    interface_handle_t *descriptor = handle->interface_data;
    int pcap_result;
    int counter = 0;

    fprintf(stderr, "PCAP reader started\n");

    while(1) {
        pcap_result = pcap_dispatch(descriptor->pc, 1, &interface_packet_handler, (u_char *) handle);
        if(!descriptor->capture_packets || pcap_result < 0) {
            fprintf(stderr, "PCAP reader stopped\n");
            pcap_perror(descriptor->pc, "PCAP end result");
            return NULL;
        }
        if(pcap_result == 0) {
            usleep(100000);
        } else {
            counter++;
            if(counter % 100 == 0)
                usleep(1000);
        }
    }
}

inline int _get_encap_header_size(int encap_dlt) {
    switch (encap_dlt) {
        case DLT_EN10MB:
            return 14;
        case DLT_LINUX_SLL:
            return 16;
        default:
            return 0;
    }
}

static void
interface_packet_handler(u_char * param, const struct pcap_pkthdr *header, const u_char * pkt_data)
{
    int len;
    ifreader_t descriptor = (ifreader_t) param;

    const u_char *pkt_data_802_15_4 = pkt_data + _get_encap_header_size(descriptor->encap_dlt);

    //FCS truncation, if present
    if(descriptor->fcs){
        len = header->caplen == header->len ? header->caplen - 2 : header->caplen;
    }
    else{
        len = header->caplen;
    }

    len -= _get_encap_header_size(descriptor->encap_dlt);
    switch (descriptor->encap_dlt) {
        case DLT_EN10MB:
            if (pkt_data[12] != 0x80 || pkt_data[13] != 0x9a) {
                return;
            }
            break;
        case DLT_LINUX_SLL:
            if (pkt_data[15] != 0xf6) {
                return;
            }
            break;
    }
    interfacemgr_process_packet(descriptor, pkt_data_802_15_4, len, header->ts);
}
