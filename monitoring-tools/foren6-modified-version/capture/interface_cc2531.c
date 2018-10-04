/*
 * This file is part of Foren6, a 6LoWPAN Diagnosis Tool
 * Copyright (C) 2016, CETIC
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
 * \filea
 *         CC2531 input interface
 * \author
 *         Foren6 Team <foren6@cetic.be>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <libusb.h>
#include <pthread.h>

#include "interface_cc2531.h"

#if __APPLE__
#define pthread_timedjoin_np(...) (1)
#endif

const static int USB_VENDOR_ID = 0x0451;
const static int USB_PRODUCT_ID = 0x16ae;
const static int INTERFACE = 0;

const static int TIMEOUT = 200;

const static int DATA_EP = 0x83;
const static int DATA_TIMEOUT = 100;

const static int DIR_OUT = 0x40;
const static int DIR_IN = 0xC0;

const static int GET_IDENT = 0xc0;
const static int SET_POWER = 0xc5;
const static int GET_POWER = 0xc6;
const static int SET_START = 0xd0;
const static int SET_STOP = 0xd1;
const static int SET_CHAN = 0xd2;

static const char *interface_name = "cc2531";
static const unsigned int interface_parameters = INTERFACE_CHANNEL;

typedef struct {
    libusb_device_handle *device_handle;
    pthread_t thread;
    bool capture_packets;
    struct timeval start_time;
} interface_handle_t;

static void interface_init();
static ifreader_t interface_open(const char *target, int channel, int baudrate);
static bool interface_start(ifreader_t handle);
static void interface_stop(ifreader_t handle);
static void interface_close(ifreader_t handle);
static void *interface_thread_process_input(void *data);

static libusb_device_handle * cc2531_open_device(void);
static int cc2531_close_device(libusb_device_handle *handle);
static int cc2531_get_ident(libusb_device_handle *handle);
static int cc2531_enable(libusb_device_handle *handle);
static int cc2531_set_channel(libusb_device_handle *handle, int channel);
static int cc2531_start_capture(libusb_device_handle *handle);
static int cc2531_stop_capture(libusb_device_handle *handle);
static int cc2531_recv(libusb_device_handle *handle, unsigned char *buffer,
        int *size);

int interface_get_version() {
    return 1;
}

interface_t interface_register() {
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

static void interface_init() {
    printf("%s interface initialized\n", interface_name);
}

static ifreader_t interface_open(const char *target, int channel, int baudrate) {
    interface_handle_t *handle;

    (void) baudrate;

    libusb_device_handle *device_handle = cc2531_open_device();
    if (!device_handle)
        return NULL;
    if (!cc2531_enable(device_handle)) {
        cc2531_close_device(device_handle);
        return NULL;
    }
    if (!cc2531_set_channel(device_handle, channel)) {
        cc2531_close_device(device_handle);
        return NULL;
    }
    handle = (interface_handle_t *) calloc(1, sizeof(interface_handle_t));
    if (!handle) {
        cc2531_close_device(device_handle);
        return NULL;
    }

    handle->device_handle = device_handle;
    handle->capture_packets = false;

    ifreader_t instance = interfacemgr_create_handle(target);
    instance->interface_data = handle;

    return instance;
}

static bool interface_start(ifreader_t handle) {
    interface_handle_t *descriptor = handle->interface_data;

    if (descriptor->capture_packets == false) {
        cc2531_start_capture(descriptor->device_handle);
        gettimeofday(&descriptor->start_time, NULL);
        descriptor->capture_packets = true;
        pthread_create(&descriptor->thread, NULL,
                &interface_thread_process_input, handle);
    }
    return true;
}

static void interface_stop(ifreader_t handle) {
    interface_handle_t *descriptor = handle->interface_data;

    if (descriptor->capture_packets == true) {
        printf("Stopping interface\n");
        struct timespec timeout = { 3, 0 };

        descriptor->capture_packets = false;
#if __APPLE__
        //We must wait the USB timeout
        sleep(3);
#endif
        if (pthread_timedjoin_np(descriptor->thread, NULL, &timeout) != 0) {
            pthread_cancel(descriptor->thread);
            pthread_join(descriptor->thread, NULL);
        }
        cc2531_stop_capture(descriptor->device_handle);
    }
}

static void interface_close(ifreader_t handle) {
    interface_handle_t *descriptor = handle->interface_data;

    interface_stop(handle);
    cc2531_close_device(descriptor->device_handle);
    free(descriptor);
    interfacemgr_destroy_handle(handle);
}

static void *
interface_thread_process_input(void *data) {
    ifreader_t handle = (ifreader_t) data;
    interface_handle_t *descriptor = handle->interface_data;
    int status;

    while (1) {
        unsigned char buffer[4096];
        int len = 4096;
        status = cc2531_recv(descriptor->device_handle, buffer, &len);
        if (!descriptor->capture_packets || status < 0) {
            return NULL;
        }
        if (status > 0) {
            struct timeval pkt_time;
            gettimeofday(&pkt_time, NULL);
            if (pkt_time.tv_usec < descriptor->start_time.tv_usec) {
                pkt_time.tv_sec = pkt_time.tv_sec
                        - descriptor->start_time.tv_sec - 1;
                pkt_time.tv_usec = pkt_time.tv_usec + 1000000
                        - descriptor->start_time.tv_usec;
            } else {
                pkt_time.tv_sec = pkt_time.tv_sec
                        - descriptor->start_time.tv_sec;
                pkt_time.tv_usec = pkt_time.tv_usec
                        - descriptor->start_time.tv_usec;
            }
            interfacemgr_process_packet(handle, buffer, len, pkt_time);
        }
    }
}

static libusb_device_handle * cc2531_open_device(void) {
    libusb_device_handle *device_handle = NULL;
    int status;

    status = libusb_init(NULL);
    if (status < 0) {
        printf("cc2531_open_device(): libusb_init error %d\n", status);
        return NULL;
    }

    // Set debugging level 0 .. 3t
    libusb_set_debug(NULL, 0);

    device_handle = libusb_open_device_with_vid_pid(NULL, USB_VENDOR_ID,
            USB_PRODUCT_ID);
    if (device_handle == NULL) {
        printf("cc2531_open_device(): libusb_open_device_with_vid_pid error %d\n", status);
        return NULL;
    }
    status = libusb_set_configuration(device_handle, 1);
    if (status < 0) {
        printf("cc2531_open_device(): libusb_set_configuration error %d\n", status);
        return NULL;
    }

    status = libusb_claim_interface(device_handle, INTERFACE);
    if (status < 0) {
        printf("cc2531_open_device(): libusb_claim_interface error %d\n", status);
        return NULL;
    }

    return device_handle;
}

static int cc2531_close_device(libusb_device_handle *handle) {
    libusb_close(handle);
    libusb_exit(NULL);
    return 1;
}

static int cc2531_get_ident(libusb_device_handle *handle) {
    unsigned char usbbuf[64];
    int status, nbytes;
    bzero(usbbuf, 64);
    status = libusb_control_transfer(handle, DIR_IN, GET_IDENT, 0, 0, usbbuf,
            64, TIMEOUT);
    if (status < 0) {
        printf("cc2531_get_ident(): error %d\n", status);
        return 0;
    }
    printf("Ident: %d: %s\n", status, usbbuf);
    return 1;
}

static int cc2531_enable(libusb_device_handle *handle) {
    unsigned char usbbuf[64];
    int status, nbytes;
    bzero(usbbuf, 64);
    status = libusb_control_transfer(handle, DIR_OUT, SET_POWER, 0, 4, usbbuf,
            0, TIMEOUT);
    if (status < 0) {
        printf("cc2531_enable(): error %d\n", status);
        return 0;
    }
    while (1) {
        status = libusb_control_transfer(handle, DIR_IN, GET_POWER, 0, 0,
                usbbuf, 1, TIMEOUT);
        if (status < 0) {
            printf("cc2531_enable(): error %d\n", status);
            return 0;
        }
        if (usbbuf[0] == 4)
            break;
        usleep(100000);
    }
    return 1;
}

static int cc2531_set_channel(libusb_device_handle *handle, int channel) {

    if (channel < 11 || channel > 26) {
        printf("Unsupported channel %d\n", channel);
        return 0;
    }
    unsigned char usbbuf[64];
    int status, nbytes;
    bzero(usbbuf, 64);
    usbbuf[0] = channel;
    status = libusb_control_transfer(handle, DIR_OUT, SET_CHAN, 0, 0, usbbuf, 1,
            TIMEOUT);
    if (status < 0) {
        printf("cc2531_set_channel(): error %d\n", status);
        return 0;
    }
    printf("set_channel: %d\n", status);
    usbbuf[0] = 0;
    status = libusb_control_transfer(handle, DIR_OUT, SET_CHAN, 0, 1, usbbuf, 1,
            TIMEOUT);
    if (status < 0) {
        printf("cc2531_set_channel(): error %d\n", status);
        return 0;
    }
    return 1;
}

static int cc2531_start_capture(libusb_device_handle *handle) {

    unsigned char usbbuf[64];
    int status, nbytes;
    bzero(usbbuf, 64);
    status = libusb_control_transfer(handle, DIR_OUT, SET_START, 0, 0, usbbuf,
            0, TIMEOUT);
    if (status < 0) {
        printf("cc2531_start_capture(): error %d\n", status);
        return 0;
    }
    return 1;
}

static int cc2531_stop_capture(libusb_device_handle *handle) {
    unsigned char usbbuf[64];
    int status, nbytes;
    bzero(usbbuf, 64);
    status = libusb_control_transfer(handle, DIR_OUT, SET_STOP, 0, 0, usbbuf, 0,
            TIMEOUT);
    if (status < 0) {
        printf("cc2531_stop_capture(): error %d\n", status);
        return 0;
    }
    return 1;
}

static int cc2531_recv(libusb_device_handle *handle, unsigned char *buffer,
        int *size) {
    unsigned char usbbuf[4096];
    int status, nbytes;

    *size = 0;
    status = libusb_interrupt_transfer(handle, DATA_EP, usbbuf, 4096, &nbytes,
            DATA_TIMEOUT);
    // check for timeout and silently ignore
    if (status == LIBUSB_ERROR_TIMEOUT) {
        return 0;
    }
    if (status < 0) {
        printf("error retrieving packet : %d\n", status);
        return -1;
    }
    //printf("Got %d bytes\n", nbytes);
    if (nbytes >= 3) {
        int cmd, cmdLen;
        cmd = usbbuf[0];
        cmdLen = usbbuf[1] + (usbbuf[2] << 8);
        if (nbytes == cmdLen + 3) {
            if (cmd == 0) {
                //printf("Got packet\n");
                int timestamp, pktLen;
                timestamp = usbbuf[3] + (usbbuf[4] << 8) + (usbbuf[5] << 16)
                        + (usbbuf[6] << 24);
                pktLen = usbbuf[7];
                if (nbytes == pktLen + 3 + 5) {
                    *size = pktLen;
                    memcpy(buffer, &usbbuf[8], pktLen);
                } else {
                    //printf("Corrupted packet\n");
                }
            } else if (cmd == 1) {
                //printf("Got heartbeat\n");
            } else {
                //printf("Got unsupported command\n");
            }
        } else {
            //printf("Invalid command len\n");
        }
    }
    return 1;
}
