#include <stdio.h>
#include <strings.h>
#include <stdlib.h>

void read_proxy_file(char *filename, char ***proxy_list, int *size) {
    *size = 0;
    FILE *file;
    char buffer[128];
    if(!(file = fopen(filename, "r"))) {
        fprintf(stderr, "[%sERROR%s] Could not open proxy file\n", COLOR_RED, COLOR_RESET);
        exit(1);
    }
    while(fgets(buffer, 128, file)) {
        (*size)++;
    }
    fclose(file);
    int proxy = 0;
    *proxy_list = (char **)malloc(sizeof(char *) * (*size));
    if(!(file = fopen(filename, "r"))) {
        fprintf(stderr, "[%sERROR%s] Could not open proxy file\n", COLOR_RED, COLOR_RESET);
        exit(1);
    }
    while(fgets(buffer, 128, file)) {
        buffer[strcspn(buffer, "\n")] = 0;
        (*proxy_list)[proxy++] = strdup(buffer);
    }
    fclose(file);
}

void build_https_proxy_connect(char *buffer, struct ip_address ip) {
    sprintf(buffer, "CONNECT %s:%hu HTTP/1.1\r\n\r\n", ip.address, ip.port);
}

char socks5_handshake[3] = "\x05\x01\x00";

void build_socks5_proxy_connect(const struct ip_address ip, char **buffer, int *size) {
    char _buffer[256];
    _buffer[0] = (char)5;
    _buffer[1] = (char)1;
    _buffer[2] = (char)0;
    _buffer[3] = (char)3;
    _buffer[4] = (char)(strlen(ip.address));
    memcpy(_buffer + 5, ip.address, strlen(ip.address));
    _buffer[5 + strlen(ip.address)] = (char)(ip.port >> 8);
    _buffer[6 + strlen(ip.address)] = (char)(ip.port & 0x80ff);
    *size = 7 + strlen(ip.address);
    *buffer = (char *)malloc(*size);
    memcpy(*buffer, _buffer, *size);
}
