#include <stddef.h>
#include <stdint.h>

// Inserta 'data_len' bytes en los LSB de 'pixels' (1 bit por byte de pixel), comenzando en offset 0.
// Devuelve 0 ok, <0 error (capacidad insuficiente).
int lsb1_embed(uint8_t *pixels, size_t pixels_len, const uint8_t *data, size_t data_len);

// Extrae 'data_len' bytes desde LSB de 'pixels'.
int lsb1_extract(const uint8_t *pixels, size_t pixels_len, uint8_t *out, size_t data_len);
