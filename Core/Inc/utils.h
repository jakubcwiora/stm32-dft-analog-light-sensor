/*
 * utils.h
 *
 *  Created on: 4 Feb 2026
 *      Author: jakub
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Oblicza CRC16-CCITT FALSE (Poly: 0x1021, Init: 0xFFFF) */
uint16_t crc16(const uint8_t* data, size_t length);

/* Konwertuje ciąg znaków Hex ASCII na surowe bajty.
 * Zwraca liczbę zapisanych bajtów. */
size_t decode_hex(const uint8_t* source, size_t source_len, uint8_t* destination, size_t destination_size);

/* Konwertuje surowe bajty na ciąg Hex ASCII */
void encode_hex(const uint8_t* source, size_t source_len, char* destination);

/* Pomocnicza funkcja do konwersji pojedynczego znaku hex na wartość */
uint8_t hex_char_to_val(uint8_t c);

#endif /* UTILS_H_ */




