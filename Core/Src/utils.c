/*
 * utils.c
 *
 *  Created on: 4 Feb 2026
 *      Author: jakub
 */
#include "utils.h"
#include "protocol_defs.h"
extern void send_error(uint8_t error_code);
// Implementacja CRC zgodna z Twoją wersją bitową
uint16_t crc16(const uint8_t* data, size_t length) {
    uint16_t wCrc = 0xFFFF;
    while (length--) {
        wCrc ^= (uint16_t)(*data++) << 8;
        for (int i = 0; i < 8; i++) {
            if (wCrc & 0x8000)
                wCrc = (wCrc << 1) ^ 0x1021;
            else
                wCrc = wCrc << 1;
        }
    }
    return wCrc;
}

// Te funkcje przydadzą się w parserze
uint8_t hex_char_to_val(uint8_t c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

uint8_t is_hex(uint8_t c) {
    if (c >= '0' && c <= '9') return 1;
    if (c >= 'A' && c <= 'F') return 1;
    return 0;
}

size_t decode_hex(const uint8_t* source, size_t source_len, uint8_t* destination, size_t destination_size) {
    if (source_len % 2 != 0) return 0;
    size_t out_len = source_len / 2;
    if (out_len > destination_size) return 0;

    for (size_t i = 0; i < out_len; i++) {
    	if(is_hex(source[i]) == 0){ // Nieporządany znak w polu dane
    		send_error(ERR_PROTO);
    		return 0;
    	}
        destination[i] = (hex_char_to_val(source[2*i]) << 4) | hex_char_to_val(source[2*i+1]);
    }
    return out_len;
}

