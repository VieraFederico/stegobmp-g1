#pragma once
#include <stdint.h>
#include <stddef.h>

// Fuerza empaquetado 1 byte
#pragma pack(push, 1)
typedef struct {
    uint16_t bfType;      // 'BM' = 0x4D42 (LE)
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;   // 54 en BMP v3 24bpp típico
} BITMAPFILEHEADER;       // 14 bytes

typedef struct {
    uint32_t biSize;          // 40
    int32_t  biWidth;         // 100
    int32_t  biHeight;        // 100 (positivo = bottom-up)
    uint16_t biPlanes;        // 1
    uint16_t biBitCount;      // 24
    uint32_t biCompression;   // 0 (BI_RGB)
    uint32_t biSizeImage;     // 30000 (100*3*100 con stride 300)
    int32_t  biXPelsPerMeter; // 2835
    int32_t  biYPelsPerMeter; // 2835
    uint32_t biClrUsed;       // 0
    uint32_t biClrImportant;  // 0
} BITMAPINFOHEADER;           // 40 bytes
#pragma pack(pop)

typedef struct {
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    uint8_t *pixels;       // buffer crudo (BGR + padding por fila)
    size_t   pixelsSize;   // cantidad de bytes de pixels (desde bfOffBits hasta EOF)
} Bmp;

int  bmp_read(const char *path, Bmp *out);
int  bmp_write(const char *path, const Bmp *bmp);
void bmp_free(Bmp *bmp);
size_t bmp_payload_size(const Bmp *bmp);
