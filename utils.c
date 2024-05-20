#define COLOR_BLACK "\x1b[30m"
#define COLOR_RED "\x1b[31m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_BLUE "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN "\x1b[36m"
#define COLOR_WHITE "\x1b[37m"
#define COLOR_RESET "\x1b[0m"
#define COLOR_CHARCOAL "\x1b[37;40m"

void init_random() {
    srand(time(NULL));
}

char *random_string(int length) {
    char *ALPHA = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuwvxyz0123456789_";
    char *result = (char *)malloc(length);
    for(int i = 0; i < length; ++i) {
        result[i] = ALPHA[rand() % strlen(ALPHA)];
    }
    return result;
}

int ipaddress_valid(char *ip) {
    for(int i = 0; i < strlen(ip); ++i) {
        if(((int)ip[i] < 48 || (int)ip[i] > 57) && (int)ip[i] != 46) {
            return 0;
        }
    }
    int _ip[4];
    if(!(sscanf(ip, "%3d.%3d.%3d.%3d", &_ip[0], &_ip[1], &_ip[2], &_ip[3]))) {
        return 0;
    }
    for(int i = 0; i < 4; ++i) {
        if(_ip[i] < 0 || _ip[i] > 255) {
            return 0;
        }
    }
    return 1;
}

void parse_proxy(char *proxy, char *address, int *port) {
    sscanf(proxy, "%[^:]:%d", address, port);
}


int resolve_hostname(char *hostname , char *ip) {
	struct hostent *he;
	struct in_addr **addr_list;
	if((he = gethostbyname(hostname)) == NULL) {
		return 0;
	}
	addr_list = (struct in_addr **)he->h_addr_list;
	for(int i = 0; addr_list[i] != NULL; i++) {
		strcpy(ip, inet_ntoa(*addr_list[i]));
		return 1;
	}
	return 0;
}

void print_buffer(char *buffer, int size) {
    for(int i = 0; i < size; ++i) {
        char c = buffer[i];
        if((int)c >= 32 && (int)c <= 126) {
            printf("%c", c);
        }else {
            printf("\\x%02x", c);
        }
    }
    printf("\n");
}

void strtolower(char *str) {
    for(int i = 0; i < strlen(str); ++i) {
        str[i] = tolower(str[i]);
    }
}