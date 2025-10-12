#include "bmp_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int bmp_read(const char *path, Bmp *out) {
    FILE *f = fopen(path, "rb");

    if (!f) { 
        fprintf(stderr, "[bmp] no pude abrir %s\n", path); 
        return -1; 
    }

    if (fread(&out->fileHeader, sizeof(BITMAPFILEHEADER), 1, f) != 1) {
        fprintf(stderr, "[bmp] fread fileHeader\n"); 
        fclose(f); 
        return -2;
    }

    if (out->fileHeader.bfType != 0x4D42) { // 'BM' en LE
        fprintf(stderr, "[bmp] bfType != 'BM' (0x%04X)\n", out->fileHeader.bfType); 
        fclose(f); 
        return -3;
    }

    if (fread(&out->infoHeader, sizeof(BITMAPINFOHEADER), 1, f) != 1) {
        fprintf(stderr, "[bmp] fread infoHeader\n"); 
        fclose(f); 
        return -4;
    }

    if (out->infoHeader.biSize != 40) {
        fprintf(stderr, "[bmp] biSize != 40 (=%u)\n", out->infoHeader.biSize); 
        fclose(f); 
        return -5;
    }

    if (out->infoHeader.biBitCount != 24) {
        fprintf(stderr, "[bmp] biBitCount != 24 (=%u)\n", out->infoHeader.biBitCount); 
        fclose(f); 
        return -6;
    }

    if (out->infoHeader.biCompression != 0) {
        fprintf(stderr, "[bmp] biCompression != 0 (=%u)\n", out->infoHeader.biCompression); 
        fclose(f); 
        return -7;
    }

    // tamaño total del archivo
    if (fseek(f, 0, SEEK_END) != 0) { 
        fclose(f); 
        return -8; 
    }

    long fileSize = ftell(f);

    if (fileSize < 0) { 
        fclose(f); 
        return -9; 
    }

    // offset válido
    if ((long)out->fileHeader.bfOffBits >= fileSize) {
        fprintf(stderr, "[bmp] bfOffBits fuera de rango (%u >= %ld)\n", out->fileHeader.bfOffBits, fileSize);
        fclose(f); 
        return -10;
    }

    out->pixelsSize = (size_t)(fileSize - out->fileHeader.bfOffBits);
    out->pixels = (uint8_t*)malloc(out->pixelsSize);

    if (!out->pixels) { 
        fclose(f); 
        return -11; 
    }

    if (fseek(f, out->fileHeader.bfOffBits, SEEK_SET) != 0) { 
        fclose(f); free(out->pixels); 
        return -12; 
    }

    if (fread(out->pixels, 1, out->pixelsSize, f) != out->pixelsSize) {
        fprintf(stderr, "[bmp] fread pixels\n"); 
        fclose(f); 
        free(out->pixels); 
        return -13;
    }

    fclose(f);

    // log útil:
    int32_t w = out->infoHeader.biWidth, h = out->infoHeader.biHeight;
    int rowSize = ((w * 3) + 3) & ~3;
    fprintf(stderr, "[bmp] OK %dx%d, rowSize=%d, pixels=%zu bytes\n", w, h, rowSize, out->pixelsSize);
    return 0;
}

int bmp_write(const char *path, const Bmp *bmp) {
    FILE *f = fopen(path, "wb");

    if (!f) 
        return -1;
        
    if (fwrite(&bmp->fileHeader, sizeof(BITMAPFILEHEADER), 1, f) != 1) { 
        fclose(f); 
        return -2; 
    }

    if (fwrite(&bmp->infoHeader, sizeof(BITMAPINFOHEADER), 1, f) != 1) { 
        fclose(f); 
        return -3; 
    }

    if (fseek(f, bmp->fileHeader.bfOffBits, SEEK_SET) != 0) { 
        fclose(f); 
        return -4; 
    }

    if (fwrite(bmp->pixels, 1, bmp->pixelsSize, f) != bmp->pixelsSize) { 
        fclose(f); 
        return -5; 
    }

    fclose(f);
    return 0;
}

void bmp_free(Bmp *bmp) {
    if (bmp) {
        free(bmp->pixels);
        memset(bmp, 0, sizeof(*bmp));
    }
}

size_t bmp_payload_size(const Bmp *bmp) {
    return bmp->pixelsSize; // usar todo el bloque de píxeles + padding de filas
}
