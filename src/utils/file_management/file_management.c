#include "file_management.h"
#include <stdio.h>
#include <stdlib.h>

int read_file(const char *path, uint8_t **buf, size_t *len) {
    *buf = NULL; *len = 0;
    FILE *f = fopen(path, "rb");

    if (!f) 
        return -1;

    if (fseek(f, 0, SEEK_END) != 0) { 
        fclose(f); 
        return -2; 
    }

    long sz = ftell(f);

    if (sz < 0) { 
        fclose(f); 
        return -3; 
    }

    if (fseek(f, 0, SEEK_SET) != 0) { 
        fclose(f); 
        return -4; 
    }

    *buf = (uint8_t*)malloc((size_t)sz);

    if (!*buf) { 
        fclose(f); 
        return -5; 
    }

    if (fread(*buf, 1, (size_t)sz, f) != (size_t)sz) { 
        free(*buf); 
        fclose(f); 
        return -6; 
    }

    fclose(f);
    *len = (size_t)sz;

    return 0;
}

int write_file(const char *path, const uint8_t *buf, size_t len) {
    FILE *f = fopen(path, "wb");

    if (!f) 
        return -1;

    if (fwrite(buf, 1, len, f) != len) { 
        fclose(f); 
        return -2; 
    }

    fclose(f);

    return 0;
}