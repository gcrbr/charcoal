#include <strings.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

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

struct ip_address {
    char address[256];
    unsigned short port;
};

#ifdef _WIN32
    void win_enable_colors() {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
    }
#endif


void init_random(int entropy) {
    srand(time(NULL) ^ entropy);
}

char *random_string(int length) {
    char *ALPHA = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuwvxyz0123456789_";
    char *result = (char *)malloc(sizeof(char) * (length + 1));
    for(int i = 0; i < length; ++i) {
        result[i] = ALPHA[rand() % strlen(ALPHA)];
    }
    result[length] = '\0';
    return result;
}

int ipaddress_valid(const char *ip) {
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

void parse_proxy(const char *proxy, struct ip_address *ip) {
    sscanf(proxy, "%16[^:]:%hu", ip->address, &ip->port);
}


int resolve_hostname(const char *hostname , char *ip) {
	struct hostent *he;
	struct in_addr **addr_list;
	if((he = gethostbyname(hostname)) == NULL) {
		return 0;
	}
	addr_list = (struct in_addr **)he->h_addr_list;
	for(int i = 0; addr_list[i] != NULL; ++i) {
		strcpy(ip, inet_ntoa(*addr_list[i]));
		return 1;
	}
	return 0;
}

int resolve_srv(const char* host, char *dname, unsigned short *port) {
    #ifdef _WIN32
        #include <windns.h>
        #ifdef _MSC_VER
            #pragma comment(lib, "Dnsapi.lib")
        #endif

        PDNS_RECORD pdns;
        if(DnsQuery_A(strcat("_minecraft.tcp.", host), DNS_TYPE_SRV, DNS_QUERY_BYPASS_CACHE, NULL, &pdns, NULL)) {
            return 0;
        }
        if(pdns != NULL) {
            resolve_hostname(pdns->Data.SRV.pNameTarget, dname);
            *port = pdns->Data.SRV.wPort;
        }
        return 0;
    #else
        struct __res_state res;
        if (res_ninit(&res) != 0) {
            return 0;
        }
        unsigned char answer[256];
        int len;
        if ((len = res_nsearch(&res, strcat("_minecraft._tcp.", host), ns_c_in, ns_t_srv, answer, sizeof(answer))) < 0) {
            return 0;
        }
        ns_msg handle;
        ns_rr rr;
        ns_initparse(answer, len, &handle);
        for (int i = 0; i < ns_msg_count(handle, ns_s_an); i++) {
            if (ns_parserr(&handle, ns_s_an, i, &rr) < 0 || ns_rr_type(rr) != ns_t_srv) {
                continue;
            }
            *port = ns_get16(ns_rr_rdata(rr) + 2 * NS_INT16SZ);
            if (dn_expand(ns_msg_base(handle), ns_msg_end(handle), ns_rr_rdata(rr) + 3 * NS_INT16SZ, dname, 256) == -1) {
                continue;
            }
            resolve_hostname(dname, dname);
            return 1;
        }
        return 0;
    #endif
}

void print_buffer(const char *buffer, int size) {
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

void zerofill(char *buffer, int size) {
    memset(buffer, '\0', size);
}