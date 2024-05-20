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