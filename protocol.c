#include <math.h>
#include <stdio.h>

extern int protocol;

#define VERSION_SIZE 55
struct  {
    int protocol_number;
    char *version_number;
} versions[VERSION_SIZE] = {
    {47, "1.8"},
    {47, "1.8.1"},
    {47, "1.8.2"},
    {47, "1.8.3"},
    {47, "1.8.4"},
    {47, "1.8.5"},
    {47, "1.8.6"},
    {47, "1.8.7"},
    {47, "1.8.8"},
    {47, "1.8.9"},
    {107, "1.9"},
    {108, "1.9.1"},
    {109, "1.9.2"},
    {110, "1.9.3"},
    {110, "1.9.4"},
    {210, "1.10"},
    {210, "1.10.1"},
    {210, "1.10.2"},
    {316, "1.11.1"},
    {316, "1.11.2"},
    {335, "1.12"},
    {338, "1.12.1"},
    {340, "1.12.2"},
    {393, "1.13"},
    {401, "1.13.1"},
    {404, "1.13.2"},
    {477, "1.14"},
    {480, "1.14.1"},
    {485, "1.14.2"},
    {490, "1.14.3"},
    {498, "1.14.4"},
    {573, "1.15"},
    {575, "1.15.1"},
    {578, "1.15.2"},
    {735, "1.16"},
    {736, "1.16.1"},
    {751, "1.16.2"},
    {753, "1.16.3"},
    {754, "1.16.4"},
    {754, "1.16.5"},
    {755, "1.17"},
    {756, "1.17.1"},
    {757, "1.18"},
    {757, "1.18.1"},
    {758, "1.18.2"},
    {759, "1.19"},
    {760, "1.19.1"},
    {760, "1.19.2"},
    {761, "1.19.3"},
    {762, "1.19.4"},
    {763, "1.20"},
    {763, "1.20.1"},
    {764, "1.20.2"},
    {765, "1.20.3"},
    {765, "1.20.4"}
};

int get_protocol_by_version(char *version) {
    for(int i = 0; i < VERSION_SIZE; ++i) {
        if(!strcmp(versions[i].version_number, version)) {
            return versions[i].protocol_number;
        }
    }
    return 0;
}

void to_varint(int number, char **result, int *size) {
    *size = floor((log(number) / log(128 * 2)) + 1);
    if(*size == 1) {
        *result = (char *)malloc(1);
        *result[0] = (char)number;
        return;
    }
    int index = 0;
    *result = (char *)malloc(sizeof(char) * (*size));
    while(1) {
        if(!(number & ~0x7f)) {
            (*result)[index++] = (char)number;
            break;
        }
        (*result)[index++] = (number & 0x7f) | 0x80;
        number = (int)((unsigned int)number >> 7);
    }
}

struct packet {
    char *buffer;
    int size;
};

void join_packet(const char *username, struct packet *packet) {
    char *username_len_buf, *packet_len_buf;
    int username_len_size, packet_len_size;
    to_varint(strlen(username), &username_len_buf, &username_len_size);
    int packet_len = 1 + username_len_size + strlen(username);
    to_varint(packet_len, &packet_len_buf, &packet_len_size);
    packet->buffer = (char *)malloc(sizeof(char) * (packet_len + packet_len_size));
    int written = 0;
    memcpy(packet->buffer, packet_len_buf, packet_len_size);
    written += packet_len_size;
    (packet->buffer)[written++] = (char)0;
    memcpy(packet->buffer + written, username_len_buf, username_len_size);
    written += username_len_size;
    memcpy(packet->buffer + written, username, strlen(username));
    written += strlen(username);
    packet->size = written;
    free(username_len_buf);
    free(packet_len_buf);
}

void chat_message_packet(const char *message, struct packet *packet) {
    char *message_len_buf, *packet_len_buf;
    int message_len_size, packet_len_size;
    to_varint(strlen(message), &message_len_buf, &message_len_size);
    int packet_len = 1 + 1 + message_len_size + strlen(message);
    to_varint(packet_len, &packet_len_buf, &packet_len_size);
    packet->buffer = (char *)malloc(sizeof(char) * (packet_len + packet_len_size));
    int written = 0;
    memcpy(packet->buffer, packet_len_buf, packet_len_size);
    written += packet_len_size;
    (packet->buffer)[written++] = (char)0;
    (packet->buffer)[written++] = (char)1;
    memcpy(packet->buffer + written, message_len_buf, message_len_size);
    written += message_len_size;
    memcpy(packet->buffer + written, message, strlen(message) > 256 ? 256 : strlen(message));
    written += strlen(message);
    packet->size = written;
    free(message_len_buf);
    free(packet_len_buf);
}

void ping_packet(const char *address, unsigned short port, int action, struct packet *packet) {
    char *protocol_buf, *address_len_buf, *port_buf, *packet_len_buf;
    int protocol_size, address_len_size, port_size, packet_len_size;
    to_varint(protocol, &protocol_buf, &protocol_size);
    to_varint(strlen(address), &address_len_buf, &address_len_size);
    int packet_len = 1 + protocol_size + address_len_size + strlen(address) + 2 + 1;
    to_varint(packet_len, &packet_len_buf, &packet_len_size);
    packet->buffer = (char *)malloc(sizeof(char) * (packet_len + packet_len_size));
    int written = 0;
    memcpy(packet->buffer, packet_len_buf, packet_len_size);
    written += packet_len_size;
    (packet->buffer)[written++] = (char)0;
    memcpy(packet->buffer + written, protocol_buf, protocol_size);
    written += protocol_size;
    memcpy(packet->buffer + written, address_len_buf, address_len_size);
    written += address_len_size;
    memcpy(packet->buffer + written, address, strlen(address));
    written += strlen(address);
    (packet->buffer)[written++] = (char)(port >> 8);
    (packet->buffer)[written++] = (char)(port & 0x80ff);
    (packet->buffer)[written++] = (char)action;
    packet->size = written;
    free(protocol_buf);
    free(address_len_buf);
    free(packet_len_buf);
}

void player_packet(int onground, struct packet *packet) {
    packet->buffer = (char *)malloc(sizeof(char) * 4);
    packet->buffer[0] = (char)3;
    packet->buffer[0] = (char)0;
    packet->buffer[0] = (char)3;
    packet->buffer[0] = (char)onground;
    packet->size = 4;
}

int send_packet(int sock_fd, struct packet packet) {
    return send(sock_fd, packet.buffer, packet.size, MSG_NOSIGNAL);
}