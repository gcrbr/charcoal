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
#include <resolv.h>
#include <arpa/nameser.h>

#include "protocol.c"
#include "utils.c"
#include "proxy.c"

#define MODE_STAY 0
#define MODE_FLOOD 1

#define HTTPS 0
#define SOCKS5 1

int protocol;
int bot_mode = MODE_FLOOD;
int proxy_mode = HTTPS;

void bot_connect(const char *address, const char *resolved_address, const unsigned short port, const char *username, const int thread, const char *message, const char *proxy_address, const unsigned short proxy_port) {
    struct sockaddr_in server;
    int sock;

    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "[%sERROR%s][%sTHREAD%03d%s] Unable to initialize socket\n", COLOR_RED, COLOR_RESET, COLOR_BLUE, thread, COLOR_RESET);
        return;
    }
    
    server.sin_family = AF_INET;
    server.sin_port = htons(proxy_port == 0 ? port : proxy_port);

    if(!inet_pton(AF_INET, proxy_port == 0 ? resolved_address : proxy_address, &server.sin_addr)) {
        fprintf(stderr, "[%sERROR%s][%sTHREAD%03d%s] Unable to resolve IP address '%s'\n", COLOR_RED, COLOR_RESET, COLOR_BLUE, thread, COLOR_RESET, resolved_address);
        exit(1);
    }

    if((connect(sock, (struct sockaddr*)&server, sizeof(server))) < 0) {
        fprintf(stderr, "[%sERROR%s][%sTHREAD%03d%s] Unable to connect to server\n", COLOR_RED, COLOR_RESET, COLOR_BLUE, thread, COLOR_RESET);
        if(proxy_port == 0) {
            exit(1);
        }else {
            return;
        }
    }

    char proxy_str[256];
    bzero(proxy_str, 256);
    if(proxy_port > 0) {
        sprintf(proxy_str, " with proxy %s:%d", proxy_address, proxy_port);
    }
    printf("[%sINFO%s][%sTHREAD%03d%s] Bot '%s' connecting%s\n", COLOR_GREEN, COLOR_RESET, COLOR_BLUE, thread, COLOR_RESET, username, proxy_str);

    char *packet;
    int packet_size = 0;

    if(proxy_port > 0) {
        if(proxy_mode == HTTPS) {
            char https[256];
            build_https_proxy_connect(https, address, port);
            send(sock, https, strlen(https), MSG_NOSIGNAL);
        }else if(proxy_mode == SOCKS5) {
            char *socks5_connect_request;
            int socks5_size = 0;
            send(sock, socks5_handshake, 3, MSG_NOSIGNAL);
            build_socks5_proxy_connect(address, port, &socks5_connect_request, &socks5_size);
            send(sock, socks5_connect_request, socks5_size, MSG_NOSIGNAL);
            free(socks5_connect_request);
            sleep(1);
        }
    }

    ping_packet(address, port, 2, &packet, &packet_size);
    send(sock, packet, packet_size, MSG_NOSIGNAL);
    join_packet(username, &packet, &packet_size);
    send(sock, packet, packet_size, MSG_NOSIGNAL);
    
    sleep(1);

    if(strlen(message) > 0) {
        chat_message_packet(message, &packet, &packet_size);
        send(sock, packet, packet_size, MSG_NOSIGNAL);
    }

    if(bot_mode == MODE_STAY) {
        while(1) {
            if(send(sock, "\0", 1, MSG_NOSIGNAL) == -1) {
                printf("[%sINFO%s][%sTHREAD%03d%s] Connection closed\n", COLOR_GREEN, COLOR_RESET, COLOR_BLUE, thread, COLOR_RESET);
                break;
            }
        }
    }

    close(sock);
}

struct args {
    char *address;
    char *resolved_address;
    unsigned short port;
    int thread;
    char **proxies;
    int proxy_amount;
    char *message;
};

void *bot_loop(void *args) {
    char proxy_addr[16];
    unsigned short proxy_port = 0;
    while(1) {
        if((((struct args *)args)->proxy_amount) > 0) {
            char *selected_proxy = (((struct args *)args)->proxies)[rand() % (((struct args *)args)->proxy_amount)];
            parse_proxy(selected_proxy, proxy_addr, &proxy_port);
            if(proxy_port == 0 || !ipaddress_valid(proxy_addr)) {
                continue;
            }
        }
        bot_connect(
            ((struct args *)args)->address,
            ((struct args *)args)->resolved_address,
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
    printf("usage: %s -i address [-p port] [-t thread number] [-m mode] [-c message] [-v protocol version] [-x proxy file] [-k proxy type]\n", executable);
    exit(1);
}

int main(int argc, char *argv[]) {
    printf(">>>>> %sCHARCOAL%s for Minecraft (github.com/gcrbr)\n\n", COLOR_CHARCOAL, COLOR_RESET);

    protocol = 47;

    int thread_number = 10;
    char address[256];
    int port = 25565;
    char proxy_file[256];
    char message[256] = "";

    init_random();

    int opt;
    while((opt = getopt(argc, argv, "i:p:t:x:m:v:c:hk:")) > 0) {
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
                    bot_mode = MODE_STAY;
                }else if(!strcmp(optarg, "flood")) {
                    bot_mode = MODE_FLOOD;
                }else {
                    fprintf(stderr, "[%sERROR%s] Invalid mode '%s', available modes: STAY, FLOOD\n", COLOR_RED, COLOR_RESET, optarg);
                    exit(1);
                }
                break;
            case 'k':
                strtolower(optarg);
                if(!strcmp(optarg, "https")) {
                    proxy_mode = HTTPS;
                }else if(!strcmp(optarg, "socks") || strcmp(optarg, "socks5")) {
                    proxy_mode = SOCKS5;
                }else {
                    fprintf(stderr, "[%sERROR%s] Invalid proxy type '%s', available types: HTTPS, SOCKS5\n", COLOR_RED, COLOR_RESET, optarg);
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

    char *conn_address;
    unsigned short conn_port = port;
    if(!ipaddress_valid(address)) {
        char ip[16];
        if(resolve_hostname(address, ip)) {
            conn_address = strdup(ip);
        }else {
            if(!resolve_srv(address, conn_address, &conn_port)) {
                fprintf(stderr, "[%sERROR%s] Invalid IP address or hostname\n", COLOR_RED, COLOR_RESET);
                exit(1);
            }
        }
    }

    pthread_t threads[thread_number];
    
    for(int i = 0; i < thread_number; ++i) {
        struct args *func_args = (struct args *)malloc(sizeof(struct args));
        func_args->address = address;
        func_args->resolved_address = conn_address;
        func_args->port = conn_port;
        func_args->thread = i+1;
        func_args->proxies = proxies;
        func_args->proxy_amount = proxy_amount;
        func_args->message = message;
        pthread_create(&threads[i], NULL, bot_loop, (void *)func_args);
    }

    for(int i=0; i < thread_number; ++i) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}