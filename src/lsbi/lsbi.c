#include "lsbi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MASK_BITS 4  // 4-bit mask for patterns

// Helper: get bit from data buffer (MSB first ordering)
static inline uint8_t get_bit(const uint8_t *buf, size_t bit_idx) {
    size_t byte = bit_idx / 8;
    int    bit  = 7 - (int)(bit_idx % 8);
    return (uint8_t)((buf[byte] >> bit) & 1u);
}

// Helper: set bit in data buffer (MSB first ordering)
static inline void set_bit(uint8_t *buf, size_t bit_idx, uint8_t v) {
    size_t byte = bit_idx / 8;
    int    bit  = 7 - (int)(bit_idx % 8);
    if (v) buf[byte] |=  (uint8_t)(1u << bit);
    else   buf[byte] &= (uint8_t)~(1u << bit);
}

// LSBI pattern from bits 1-2 (2nd and 3rd LSBs) of carrier byte
static inline int pattern_of(uint8_t carrier_byte) {
    uint8_t last3 = carrier_byte & 0x07u;  // Get last 3 bits
    return (int)(last3 >> 1);              // Use bits 2-1 as pattern (0..3)
}

// Check if a carrier byte index is an R channel (should be skipped)
// In BMP: pixels are stored as BGR BGR BGR...
// So B=0, G=1, R=2, B=3, G=4, R=5...
// Skip when (index % 3) == 2 for R channel
static inline int is_r_channel(size_t carrier_idx) {
    return (carrier_idx % 3) == 2;  // Skip R channel (every 3rd byte starting at 2)
}

// Count how many non-R carriers are needed for n bits
static inline size_t carriers_needed_for_bits(size_t bits) {
    // We use 2 out of 3 bytes (skip R)
    // So for n bits, we need ceil(n * 3/2) carriers
    return (bits * 3 + 1) / 2;
}

// Write 4-bit mask into LSBs of first 4 non-R carrier bytes
static void write_mask(uint8_t *pixels, size_t pixels_len, uint8_t mask4) {
    int bits_written = 0;
    size_t idx = 0;
    
    while (bits_written < MASK_BITS && idx < pixels_len) {
        if (!is_r_channel(idx)) {
            uint8_t bit = (mask4 >> bits_written) & 1u;
            pixels[idx] = (uint8_t)((pixels[idx] & 0xFEu) | bit);
            bits_written++;
        }
        idx++;
    }
}

// Read 4-bit mask from LSBs of first 4 non-R carrier bytes
static uint8_t read_mask(const uint8_t *pixels, size_t pixels_len) {
    uint8_t m = 0;
    int bits_read = 0;
    size_t idx = 0;
    
    while (bits_read < MASK_BITS && idx < pixels_len) {
        if (!is_r_channel(idx)) {
            m |= (uint8_t)((pixels[idx] & 1u) << bits_read);
            bits_read++;
        }
        idx++;
    }
    return m;
}

int lsbi_embed(uint8_t *pixels, size_t pixels_len, const uint8_t *data, size_t data_len) {
    if (!pixels || !data) return -1;

    size_t bits_needed = data_len * 8;
    
    // Calculate capacity needed
    size_t mask_carriers = carriers_needed_for_bits(MASK_BITS);
    size_t data_carriers = carriers_needed_for_bits(bits_needed);
    
    if (pixels_len < mask_carriers + data_carriers) {
        fprintf(stderr, "LSBI: Need %zu carriers, have %zu\n", 
                mask_carriers + data_carriers, pixels_len);
        return -1;
    }

    // 1) Pre-compute statistics: changed/unchanged per pattern
    size_t changed[4]   = {0,0,0,0};
    size_t unchanged[4] = {0,0,0,0};

    // Start after mask area
    size_t carrier_idx = 0;
    int mask_count = 0;
    
    // Skip mask storage area
    while (mask_count < MASK_BITS && carrier_idx < pixels_len) {
        if (!is_r_channel(carrier_idx)) mask_count++;
        carrier_idx++;
    }

    // Scan through data to compute pattern statistics
    size_t scan_idx = carrier_idx;
    for (size_t bi = 0; bi < bits_needed; ++bi) {
        // Find next non-R carrier
        while (scan_idx < pixels_len && is_r_channel(scan_idx)) {
            scan_idx++;
        }
        if (scan_idx >= pixels_len) return -1;

        uint8_t orig = pixels[scan_idx];
        int pat = pattern_of(orig);

        uint8_t bit = get_bit(data, bi);
        uint8_t before_lsb = orig & 1u;

        if (before_lsb != bit) changed[pat]++;
        else unchanged[pat]++;
        
        scan_idx++;
    }

    // 2) Decide mask: invert patterns where changed > unchanged
    uint8_t mask = 0;
    for (int p = 0; p < 4; ++p) {
        if (changed[p] > unchanged[p]) {
            mask |= (uint8_t)(1u << p);
        }
    }

    // 3) Embed data with pattern-based inversion
    carrier_idx = 0;
    mask_count = 0;
    
    // Skip past mask storage area
    while (mask_count < MASK_BITS && carrier_idx < pixels_len) {
        if (!is_r_channel(carrier_idx)) mask_count++;
        carrier_idx++;
    }

    // Embed the data bits
    for (size_t bi = 0; bi < bits_needed; ++bi) {
        // Find next non-R carrier
        while (carrier_idx < pixels_len && is_r_channel(carrier_idx)) {
            carrier_idx++;
        }
        if (carrier_idx >= pixels_len) return -1;

        uint8_t c = pixels[carrier_idx];
        
        // Set LSB with message bit (standard LSB)
        uint8_t bit = get_bit(data, bi);
        c = (uint8_t)((c & 0xFEu) | bit);

        // Apply inversion if pattern is marked in mask
        int pat = pattern_of(c);
        if (mask & (1u << pat)) {
            c ^= 0x01u;
        }

        pixels[carrier_idx] = c;
        carrier_idx++;
    }

    // 4) Write mask to first 4 non-R carrier bytes
    write_mask(pixels, pixels_len, mask);

    return 0;
}

int lsbi_extract(const uint8_t *pixels, size_t pixels_len, uint8_t *out, size_t data_len) {
    if (!pixels || !out) return -1;

    size_t bits_to_recover = data_len * 8;
    
    // Check capacity
    size_t mask_carriers = carriers_needed_for_bits(MASK_BITS);
    size_t data_carriers = carriers_needed_for_bits(bits_to_recover);
    
    if (pixels_len < mask_carriers + data_carriers) {
        fprintf(stderr, "LSBI extract: Need %zu carriers, have %zu\n",
                mask_carriers + data_carriers, pixels_len);
        return -1;
    }

    // 1) Read mask from first 4 non-R carriers
    uint8_t mask = read_mask(pixels, pixels_len);

    // 2) Skip past mask storage and extract data bits
    memset(out, 0, data_len);
    
    size_t carrier_idx = 0;
    int mask_count = 0;
    
    // Skip mask area
    while (mask_count < MASK_BITS && carrier_idx < pixels_len) {
        if (!is_r_channel(carrier_idx)) mask_count++;
        carrier_idx++;
    }

    // Extract data bits
    for (size_t bi = 0; bi < bits_to_recover; ++bi) {
        // Find next non-R carrier
        while (carrier_idx < pixels_len && is_r_channel(carrier_idx)) {
            carrier_idx++;
        }
        if (carrier_idx >= pixels_len) return -1;

        uint8_t c = pixels[carrier_idx];
        
        // Get pattern and LSB
        int pat = pattern_of(c);
        uint8_t lsb = c & 1u;
        
        // Apply inversion if pattern is marked
        if (mask & (1u << pat)) {
            lsb ^= 1u;
        }

        // Write recovered bit to output
        set_bit(out, bi, lsb);
        
        carrier_idx++;
    }

    return 0;
}