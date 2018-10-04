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
 *         Sniffer input interface
 * \author
 *         Foren6 Team <foren6@cetic.be>
 */

#include "interface_sensnif.h"

#include <circular_buffer.h>
#include <sniffer_packet_parser.h>
#include <descriptor_poll.h>

#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>

static const char expected_magic[4] = {0xC1, 0x1F, 0xFE, 0x72};
static const char expected_magic_legacy[4] = {0x53, 0x6E, 0x69, 0x66};


static const int CMD_FRAME = 0x00;
static const int CMD_CHANNEL = 0x01;
static const int CMD_GET_CHANNEL = 0x81;
static const int CMD_SET_CHANNEL = 0x82;
static const int SNIFFER_PROTO_VERSION = 1;

typedef enum {
    PRS_Magic,
    PRS_Proto,
    PRS_Type,
    PRS_Len,
    PRS_Data,
    PRS_Done
} packet_read_state_e;

typedef struct {
    circular_buffer_t input_buffer;
    pthread_mutex_t mutex;
    int lock_line;
    int serial_line;
    int baudrate;
    int channel;

    //states
    packet_read_state_e current_state;
    packet_read_state_e last_state;
    packet_read_state_e before_switch_state;

    //current packet data
    char pkt_magic[4];
    int legacy;
    unsigned char pkt_type;
    unsigned char pkt_len;
    unsigned char pkt_data[256];
    unsigned short pkt_crc;
    unsigned char pkt_crc_ok;
    unsigned char pkt_rssi;
    unsigned char pkt_lqi;
    unsigned short pkt_timestamp;
    struct timeval start_time;
    int pkt_received_index;
} interface_handle_t;           //*ifreader_t

static void sniffer_interface_init();
static ifreader_t sniffer_interface_open(const char *target, int channel, int baudrate);
static bool sniffer_interface_start(ifreader_t handle);
static void sniffer_interface_stop(ifreader_t handle);
static void sniffer_interface_close(ifreader_t handle);
static void process_input(int fd, void *handle);
static bool read_input(ifreader_t descriptor);
static bool can_read_byte(interface_handle_t * descriptor);
static unsigned char get_byte(interface_handle_t * descriptor);
static void set_serial_attribs(int fd, int baudrate, int parity);

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

    interface.interface_name = "sensnif";
    interface.parameters = INTERFACE_DEVICE | INTERFACE_CHANNEL | INTERFACE_BAUDRATE;
    interface.init = &sniffer_interface_init;
    interface.open = &sniffer_interface_open;
    interface.close = &sniffer_interface_close;
    interface.start = &sniffer_interface_start;
    interface.stop = &sniffer_interface_stop;

    return interface;
}

static void
sniffer_interface_init()
{
    desc_poll_init();
    fprintf(stderr, "snif interface initialized\n");
}

static ifreader_t
sniffer_interface_open(const char *target, int channel, int baudrate)
{
    interface_handle_t *handle;

    handle = (interface_handle_t *) calloc(1, sizeof(interface_handle_t));
    if(!handle) {
        return NULL;
    }

    pthread_mutex_init(&handle->mutex, NULL);
    handle->lock_line = 0;

    fprintf(stderr, "Opening %s\n", target);
    if((handle->serial_line = open(target, O_RDWR | O_NOCTTY | O_SYNC | O_NONBLOCK)) < 0) {
        perror("Cannot open interface");
        return NULL;
    }

    handle->channel = channel;
    handle->baudrate = interfacemgr_baudrate_to_const(baudrate);
    if(handle->baudrate == 0) {
        fprintf(stderr, "Baudrate %d not supported\n", baudrate);
        return NULL;
    }

    handle->input_buffer = circular_buffer_create(32, 1);
    if(handle->input_buffer == NULL) {
        fprintf(stderr, "FATAL: can't allocate input buffer\n");
    }

    handle->current_state = PRS_Magic;
    handle->last_state = PRS_Done;

    ifreader_t ifinstance = interfacemgr_create_handle(target);
    ifinstance->interface_data = handle;
    ifinstance->fcs = false;

    return ifinstance;
}

static bool
sniffer_interface_start(ifreader_t handle)
{
    interface_handle_t *descriptor = (interface_handle_t *) handle->interface_data;

    //If it's a file, don't try to set any serial attribute nor write a sniffer command
    if(isatty(descriptor->serial_line)) {
        unsigned char byte;
        set_serial_attribs(descriptor->serial_line, descriptor->baudrate, 0);
        write(descriptor->serial_line, expected_magic, 4);
        byte = SNIFFER_PROTO_VERSION;
        write(descriptor->serial_line, &byte, 1);
        byte = CMD_SET_CHANNEL;
        write(descriptor->serial_line, &byte, 1);
        byte = 1; //Command len
        write(descriptor->serial_line, &byte, 1);
        byte = descriptor->channel;
        write(descriptor->serial_line, &byte, 1);
        printf("Channel set\n");
    } else {
        fprintf(stderr, "Device is not a tty\n");
    }
    gettimeofday(&descriptor->start_time, NULL);
    return desc_poll_add(descriptor->serial_line, &process_input, handle);
}

static void
sniffer_interface_stop(ifreader_t handle)
{
    interface_handle_t *descriptor = (interface_handle_t *) handle->interface_data;
    desc_poll_del(descriptor->serial_line);

    //If it's a file, don't try to write a sniffer command
    if(isatty(descriptor->serial_line)) {
    }
    fprintf(stderr, "Stopped interface\n");
}

static void
sniffer_interface_close(ifreader_t handle)
{
    interface_handle_t *descriptor = (interface_handle_t *) handle->interface_data;

    sniffer_interface_stop(handle);

    pthread_mutex_lock(&descriptor->mutex);
    descriptor->lock_line = __LINE__;

    circular_buffer_delete(descriptor->input_buffer);
    close(descriptor->serial_line);

    descriptor->serial_line = -1;

    pthread_mutex_unlock(&descriptor->mutex);
    pthread_mutex_destroy(&descriptor->mutex);

    free(descriptor);

    interfacemgr_destroy_handle(handle);
}

static void
process_input(int fd, void *handle)
{
    interface_handle_t *descriptor = (interface_handle_t *) ((ifreader_t) handle)->interface_data;

    if(pthread_mutex_lock(&descriptor->mutex) != 0)
        return;
    descriptor->lock_line = __LINE__;
    if(descriptor->serial_line < 0) {
        return;
    }

    //Read input until our buffer is full or there is no more data to read
    while(read_input(handle) == true);

    //Parse input until there is no more data to parse
    do {

/*
    if(descriptor->last_state != descriptor->current_state)
        fprintf(stderr, "state changed, %d -> %d\n", descriptor->last_state, descriptor->current_state);
*/
        descriptor->before_switch_state = descriptor->current_state;

        switch (descriptor->current_state) {
        case PRS_Magic:
            if(descriptor->last_state != descriptor->current_state) {
                descriptor->pkt_received_index = 0;
                descriptor->legacy = 0;
            }

            if(can_read_byte(descriptor)) {
                descriptor->pkt_magic[descriptor->pkt_received_index] = get_byte(descriptor);
            } else {
                break;
            }
            if(descriptor->pkt_received_index == 0) {
                if (descriptor->pkt_magic[0] == expected_magic[0]) {
                    descriptor->pkt_received_index++;
                } else if (descriptor->pkt_magic[0] == expected_magic_legacy[0]) {
                    descriptor->pkt_received_index++;
                    descriptor->legacy = 1;
                } else {
                    printf("Invalid first magic: %x\n", descriptor->pkt_magic[0]);
                    descriptor->pkt_received_index = 0;
                }
            } else if(! descriptor->legacy && descriptor->pkt_magic[descriptor->pkt_received_index] != expected_magic[descriptor->pkt_received_index]) {
                //Invalid magic number -> reset received packet getByte(serial_line) until we have the magic sequence
                printf("Invalid magic: %x vs %x\n", descriptor->pkt_magic[descriptor->pkt_received_index], expected_magic[descriptor->pkt_received_index]);
                descriptor->pkt_received_index = 0;
            } else if(descriptor->legacy && descriptor->pkt_magic[descriptor->pkt_received_index] != expected_magic_legacy[descriptor->pkt_received_index]) {
                //Invalid magic number -> reset received packet getByte(serial_line) until we have the magic sequence
                printf("Invalid legacy magic: %x vs %x\n", descriptor->pkt_magic[descriptor->pkt_received_index], expected_magic_legacy[descriptor->pkt_received_index]);
                descriptor->pkt_received_index = 0;
            } else {
                descriptor->pkt_received_index++;
            }

            if(descriptor->pkt_received_index >= 4) {
                if (descriptor->legacy) {
                    descriptor->current_state = PRS_Len;
                } else {
                    descriptor->current_state = PRS_Type;
                }
            }
            break;

        case PRS_Proto:
            if(can_read_byte(descriptor)) {
                if (get_byte(descriptor) == SNIFFER_PROTO_VERSION) {
                    descriptor->current_state = PRS_Len;
                } else {
                    descriptor->current_state = PRS_Magic;
                    descriptor->pkt_received_index = 0;
                }
            } else {
                break;
            }
            descriptor->current_state = PRS_Len;
            break;

        case PRS_Type:
            if(can_read_byte(descriptor)) {
                descriptor->pkt_type = get_byte(descriptor);
            } else {
                break;
            }
            descriptor->current_state = PRS_Len;
            break;

        case PRS_Len:
            if(can_read_byte(descriptor)) {
                descriptor->pkt_len = get_byte(descriptor);
            } else {
                break;
            }
            descriptor->current_state = PRS_Data;
            break;

        case PRS_Data:
            if(descriptor->last_state != descriptor->current_state) {
                descriptor->pkt_received_index = 0;
            }
            if(descriptor->pkt_received_index >= descriptor->pkt_len) {
                descriptor->current_state = PRS_Done;
            } else {
                if(can_read_byte(descriptor)) {
                    descriptor->pkt_data[descriptor->pkt_received_index] = get_byte(descriptor);
                } else {
                    break;
                }
                descriptor->pkt_received_index++;
            }
            break;

        case PRS_Done:
            if(descriptor->pkt_len > 0) {
                struct timeval pkt_time;

                gettimeofday(&pkt_time, NULL);
                if(pkt_time.tv_usec < descriptor->start_time.tv_usec) {
                    pkt_time.tv_sec = pkt_time.tv_sec - descriptor->start_time.tv_sec - 1;
                    pkt_time.tv_usec = pkt_time.tv_usec + 1000000 - descriptor->start_time.tv_usec;
                } else {
                    pkt_time.tv_sec = pkt_time.tv_sec - descriptor->start_time.tv_sec;
                    pkt_time.tv_usec = pkt_time.tv_usec - descriptor->start_time.tv_usec;
                }
                sniffer_parser_parse_data(descriptor->pkt_data, descriptor->pkt_len - 2, pkt_time);
            }
            descriptor->current_state = PRS_Magic;
            break;
        }
        descriptor->last_state = descriptor->before_switch_state;
    } while(can_read_byte(descriptor));
    pthread_mutex_unlock(&descriptor->mutex);
}

static bool
read_input(ifreader_t handle)
{
    interface_handle_t *descriptor = (interface_handle_t *) handle->interface_data;
    unsigned char data;
    size_t nbread;

    if(!circular_buffer_is_full(descriptor->input_buffer)) {
        errno = 0;
        nbread = read(descriptor->serial_line, &data, 1);
        if(nbread == 1) {
            //fprintf(stderr, "Received data 0x%02X\n", data);
            circular_buffer_push_front(descriptor->input_buffer, &data);
            return true;
        } else if(nbread == 0 && errno != EAGAIN) {
            sniffer_interface_stop(handle);
            fprintf(stderr, "File read completed\n");
            perror("Stop interface");
        }
    }
    return false;
}

static bool
can_read_byte(interface_handle_t * descriptor)
{
    return !circular_buffer_is_empty(descriptor->input_buffer);
}

static unsigned char
get_byte(interface_handle_t * descriptor)
{
    unsigned char *data;

    data = (unsigned char *)circular_buffer_pop_back(descriptor->input_buffer);

    if(data) {
        //fprintf(stderr, "Read data from buffer 0x%02X\n", *data);
        return *data;
    } else {
        fprintf(stderr, "empty buffer !\n");
        return 0;
    }
}

static void
set_serial_attribs(int fd, int baudrate, int parity)
{
    struct termios tty;

    memset(&tty, 0, sizeof(tty));
    if(tcgetattr(fd, &tty) != 0) {
        perror("Can't get serial line attributes");
        //Probably just a file ... so set it non blocking as expected
        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
        return;
    }

    cfsetospeed(&tty, baudrate);
    cfsetispeed(&tty, baudrate);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK;     // ignore break signal
    tty.c_lflag = 0;            // no signaling chars, no echo,
    // no canonical processing
    tty.c_oflag = 0;            // no remapping, no delays
    tty.c_cc[VMIN] = 1;         // read never return 0 because of no data available, O_NONBLOCK take care of that feature
    tty.c_cc[VTIME] = 0;        // 0 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);     // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);    // ignore modem controls,
    // enable reading
    tty.c_cflag &= ~(PARENB | PARODD);  // shut off parity
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if(tcsetattr(fd, TCSANOW, &tty) != 0)
        perror("Can't set serial line attributes");
}
