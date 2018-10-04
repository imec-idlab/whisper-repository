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

#include <pcap/pcap.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <expat.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>

#include "rpl_packet_parser.h"
#include "sniffer_packet_parser.h"
#include "descriptor_poll.h"
#include "packet_parsers/parser_register.h"
#include "data_info/hash_container.h"
#include "sha1.h"

static bool sniffer_parser_reset_requested = false;
static enum {
    XPS_Run,
    XPS_Paused
} xml_parser_state = false;
pthread_mutex_t state_lock;

char outpcap_name[128];

static pcap_t *pd = NULL;
static pcap_dumper_t *pdumper = NULL;
static pcap_t *pd_out = NULL;
static pcap_dumper_t *pdumper_out = NULL;
static FILE *pcap_output = NULL;

static int tshark_pid = 0;
static int pipe_tshark_stdin = 0;       //We will write packets here
static int pipe_tshark_stdout = 0;      //We will read dissected packets from here
static pthread_mutex_t new_packet_mutex;

static int packet_count;
static int packet_input_count;
static int embedded_proto;

#define LAST_PACKET_NUMBER 100
static hash_container_ptr last_packets;
struct packet_data {
    void *data;
    int len;
};

static XML_Parser dissected_packet_parser;      //parse output of tshark

static void process_events(int fd, void *data);
static void tshark_parser_reset();
static bool spawn_piped_process(const char *command, char *const arguments[],
                                int *pid, int *process_input_pipe,
                                int *process_output_pipe);
static void tshark_exited();

static void parse_xml_start_element(void *data, const char *el,
                                    const char **attr);
static void parse_xml_end_element(void *data, const char *el);

static bool check_duplicate_packet(const unsigned char *data, int len);

//Initialize sniffer parser
void
sniffer_parser_init()
{
    struct sigaction sigchld_action = {
        .sa_handler = SIG_DFL,
        .sa_flags = SA_NOCLDWAIT
    };
    /* prevent zombie child */
    //sigaction(SIGCHLD, &sigchld_action, NULL);
    pthread_mutex_init(&new_packet_mutex, NULL);
    pthread_mutex_init(&state_lock, NULL);

    parser_register_all();

    last_packets = hash_create(sizeof(struct packet_data), NULL);
}

void
sniffer_parser_cleanup()
{
    parser_cleanup();
}

void
sniffer_parser_reset()
{
    hash_clear(last_packets);
    tshark_parser_reset();
}
//Give data to parse to parser
void
sniffer_parser_parse_data(const unsigned char *data, int len,
                          struct timeval timestamp)
{
    struct pcap_pkthdr pkt_hdr;

    pkt_hdr.caplen = len;
    pkt_hdr.len = len + 2;      //FCS is not captured (2 bytes)
    pkt_hdr.ts = timestamp;
    pthread_mutex_lock(&new_packet_mutex);
    packet_input_count++;

    if(check_duplicate_packet(data, len)) {
        pcap_dump((u_char *) pdumper, &pkt_hdr, data);
        pcap_dump_flush(pdumper);
        if(pdumper_out) {
            pcap_dump((u_char *) pdumper_out, &pkt_hdr, data);
            pcap_dump_flush(pdumper_out);
        }
        fflush(pcap_output);
    }

    pthread_mutex_unlock(&new_packet_mutex);
    //fflush(stdout);
    //fprintf(stderr, "New packet captured\n");
    //write(STDOUT_FILENO, data, len);
}

int
sniffer_parser_get_packet_count()
{
    return packet_count;
}

void
sniffer_parser_pause_parser(bool pause)
{
    pthread_mutex_lock(&state_lock);
    if(pause) {
        xml_parser_state = XPS_Paused;
    } else {
        xml_parser_state = XPS_Run;
    }
    pthread_mutex_unlock(&state_lock);
}


static void
process_events(int fd, void *data)
{
    char buffer[512];
    int nbread;

    if(sniffer_parser_reset_requested) {
        tshark_parser_reset();
        sniffer_parser_reset_requested = false;
    }

    while(1) {
        nbread = read(pipe_tshark_stdout, buffer, 512);
        if(nbread <= 0)
            break;

        while(1) {
            pthread_mutex_lock(&state_lock);
            if(xml_parser_state != XPS_Paused) {
                break;
            }
            pthread_mutex_unlock(&state_lock);
            usleep(100000);
        }

        if(!XML_Parse(dissected_packet_parser, buffer, nbread, false)) {
            buffer[nbread] = 0;
            fprintf(stderr, "Bad XML input: %s\n%s\n",
                    buffer,
                    XML_ErrorString(XML_GetErrorCode
                                    (dissected_packet_parser)));
        }
        pthread_mutex_unlock(&state_lock);
    }
}

static void
parse_xml_start_element(void *data, const char *el, const char **attr)
{
    if(!strcmp(el, "packet")) {
        parser_begin_packet();
        packet_count++;
        embedded_proto = 0;
    } else if(!strcmp(el, "proto")) {
        embedded_proto++;
    } else if(!strcmp(el, "field") && embedded_proto == 1 ) {
        /* Parse packet fields */
        int i;
        const char *nameStr = NULL;
        const char *showStr = NULL;
        const char *valueStr = NULL;
        int64_t valueInt = 0;

        for(i = 0; attr[i]; i += 2) {
            if(!strcmp(attr[i], "name"))
                nameStr = attr[i + 1];
            else if(!strcmp(attr[i], "show"))
                showStr = attr[i + 1];
            else if(!strcmp(attr[i], "value"))
                valueStr = attr[i + 1];
        }

        if(valueStr)
            valueInt = strtoll(valueStr, NULL, 16);

        if(nameStr != NULL && showStr != NULL)
            parser_parse_field(nameStr, showStr, valueStr, valueInt);
    }
}

static void
parse_xml_end_element(void *data, const char *el)
{
    if(!strcmp(el, "packet"))
        parser_end_packet();
    else if(!strcmp(el, "proto"))
        embedded_proto--;
}

/*
 * Reset tshark and parser
 */
static void
tshark_parser_reset()
{
    char context0[8 + 8 + 1 + INET6_ADDRSTRLEN];

    if(tshark_pid) {
        signal(SIGPIPE, SIG_IGN);
        kill(tshark_pid, SIGKILL);      //Kill old tshark process to avoid multiple spawned processes at the same time
    }

/*
 Closed by pcap_dump_close(pdumper);
	if(pcap_output)
		fclose(pcap_output);
	if(pipe_tshark_stdin)
		close(pipe_tshark_stdin);
*/
    if(pipe_tshark_stdout) {
        desc_poll_del(pipe_tshark_stdout);
        close(pipe_tshark_stdout);
    }

    if(pd)
        pcap_close(pd);
    if(pdumper)
        pcap_dump_close(pdumper);
    if(pd_out)
        pcap_close(pd_out);
    if(pdumper_out) {
        pcap_dump_close(pdumper_out);
        pdumper_out = pcap_dump_open(pd_out, outpcap_name);
    }

    if(dissected_packet_parser)
        XML_ParserFree(dissected_packet_parser);

    tshark_pid = 0;
    pcap_output = NULL;
    pd = NULL;
    pdumper = NULL;
    pd_out = NULL;
    pipe_tshark_stdin = 0;
    pipe_tshark_stdout = 0;

    packet_count = 0;

    signal(SIGPIPE, &tshark_exited);

    strcpy(context0, "6lowpan.context0:");
    inet_ntop(AF_INET6,
              (const char *)&rpl_tool_get_analyser_config()->context0,
              context0 + strlen(context0), INET6_ADDRSTRLEN);

    char *parse_option;
    if(rpl_tool_get_analyser_config()->old_tshark) {
        parse_option = "-R";
    } else {
        parse_option = "-Y";
    }
    //removed ipv6 filter so we can parse 6top messages
    //No 6top filter exists at this point of writing
    if(spawn_piped_process("tshark", (char *const[]) {
                           "tshark", "-i", "-", "-V", "-T", "pdml", parse_option,
                           "", "-l", "-o", context0, NULL}, &tshark_pid,
                           &pipe_tshark_stdin,
                           &pipe_tshark_stdout) == false) {
        perror("Can't spawn tshark process");
        return;
    }

    pcap_output = fdopen(pipe_tshark_stdin, "w");
    if(pcap_output == NULL) {
        fprintf(stderr, "pipe %d: ", pipe_tshark_stdin);
        perror("Can't open tshark stdin pipe with fopen");
        return;
    }
//      pcap_output = stdout;

    pd = pcap_open_dead(DLT_IEEE802_15_4, 255);
    pdumper = pcap_dump_fopen(pd, pcap_output);

    struct tm *current_time;
    time_t cur_time = time(NULL);

    current_time = localtime(&cur_time);
//      sprintf(outpcap_name, "out_%02d%02d%04d-%02d%02d%02d-%d.pcap", current_time->tm_mday, current_time->tm_mon, current_time->tm_year + 1900, current_time->tm_hour, current_time->tm_min, current_time->tm_sec, getpid());
    sprintf(outpcap_name, "out.pcap");
    pd_out = pcap_open_dead(DLT_IEEE802_15_4, 255);

    dissected_packet_parser = XML_ParserCreate(NULL);
    XML_SetElementHandler(dissected_packet_parser, &parse_xml_start_element,
                          &parse_xml_end_element);

    desc_poll_add(pipe_tshark_stdout, &process_events, NULL);
}

static bool
spawn_piped_process(const char *command, char *const arguments[], int *pid,
                    int *process_input_pipe, int *process_output_pipe)
{
    int child_pid;
    int stdin_pipe[2];
    int stdout_pipe[2];
    static const int PIPE_READ = 0;
    static const int PIPE_WRITE = 1;

    if(pipe(stdin_pipe))
        return false;

    if(pipe(stdout_pipe)) {
        close(stdin_pipe[PIPE_READ]);
        close(stdin_pipe[PIPE_WRITE]);

        return false;
    }

    child_pid = fork();
    if(child_pid == 0) {
        int result;

        //In child process
        dup2(stdin_pipe[PIPE_READ], STDIN_FILENO);
        dup2(stdout_pipe[PIPE_WRITE], STDOUT_FILENO);

        // all these are for use by parent only
        close(stdin_pipe[PIPE_READ]);
        close(stdin_pipe[PIPE_WRITE]);
        close(stdout_pipe[PIPE_READ]);
        close(stdout_pipe[PIPE_WRITE]);

        result = execvp(command, arguments);
        perror("Failed to spawn process");
        exit(result);
    } else if(child_pid > 0) {
        close(stdin_pipe[PIPE_READ]);
        close(stdout_pipe[PIPE_WRITE]);

        fcntl(stdout_pipe[PIPE_READ], F_SETFL,
              fcntl(stdout_pipe[PIPE_READ], F_GETFL, 0) | O_NONBLOCK);

        if(pid)
            *pid = child_pid;
        if(process_input_pipe)
            *process_input_pipe = stdin_pipe[PIPE_WRITE];
        if(process_output_pipe)
            *process_output_pipe = stdout_pipe[PIPE_READ];

        return true;
    } else {
        close(stdin_pipe[PIPE_READ]);
        close(stdin_pipe[PIPE_WRITE]);
        close(stdout_pipe[PIPE_READ]);
        close(stdout_pipe[PIPE_WRITE]);

        perror("Failed to fork");

        return false;
    }
}

/*
 * tshark exited, so restart it (maybe a crash ?)
 * called when SIGPIPE
 */
static void
tshark_exited()
{
    /* Prevent spawn flood */
    signal(SIGPIPE, SIG_IGN);
    tshark_pid = 0;
    //sniffer_parser_reset_requested = true;
    //fprintf(stderr, "tshark exited, parser reset requested\n");
    fprintf(stderr, "tshark exited\n");
    rpl_tool_report_error("Could not start tshark");
}

static bool
check_duplicate_packet(const unsigned char *data, int len)
{
    uint32_t hashed_data[5];
    struct packet_data pkt_data;
    hash_iterator_ptr it = hash_begin(NULL, NULL);
    bool is_duplicate;

    sha1_buffer((const char *)data, len, hashed_data);

    if(hash_find(last_packets, (hash_key_t) {
                 hashed_data, sizeof(hashed_data)}
                 , it)) {
        struct packet_data *old_pkt;

        old_pkt = (struct packet_data *)hash_it_value(it);
        free(old_pkt->data);

        hash_it_delete_value(it);
        is_duplicate = true;
    } else {
        is_duplicate = false;
        if(hash_size(last_packets) >= LAST_PACKET_NUMBER) {
            //values are added to the end of the hash, so to remove the oldest value, remove the one at the beginning
            struct packet_data *old_pkt;

            hash_begin(last_packets, it);
            old_pkt = (struct packet_data *)hash_it_value(it);
            free(old_pkt->data);
            hash_it_delete_value(it);
        }
    }

    hash_it_destroy(it);

    pkt_data.data = malloc(len);
    memcpy(pkt_data.data, data, len);
    pkt_data.len = len;
    hash_add(last_packets, (hash_key_t) {
             hashed_data, sizeof(hashed_data)}
             , &pkt_data, NULL, HAM_FailIfExists, NULL);

    return is_duplicate == false;
}

void
sniffer_parser_create_out()
{
    sniffer_parser_close_out();
    unlink(outpcap_name);
    pdumper_out = pcap_dump_open(pd_out, outpcap_name);
}

void
sniffer_parser_close_out()
{
    if(pdumper_out) {
        pcap_dump_close(pdumper_out);
        pdumper_out = NULL;
    }
}
