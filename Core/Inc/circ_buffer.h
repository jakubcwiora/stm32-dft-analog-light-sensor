/*
 * circ_buffer.h
 *
 *  Created on: 17 Jan 2026
 *      Author: jakub
 */


#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "cmsis_gcc.h"

// Nowa, generyczna struktura bufora kołowego
typedef struct {
    uint8_t * const buffer; // Wskaźnik do zewnętrznej tablicy (const, bo adres się nie zmienia)
    const uint16_t size;    // Rozmiar tej tablicy (const, bo się nie zmienia)
    volatile uint16_t head;          // Indeks zapisu
    volatile uint16_t tail;          // Indeks odczytu
    // 'count' jest usunięty - jest źródłem błędów i niepotrzebny!
} circ_buf_t;

// --- Deklaracje funkcji ---

// Inicjalizuje bufor. Zwraca true jeśli się udało.
bool cbuf_init(volatile circ_buf_t* cbuf, uint8_t* buffer_array, uint16_t size);

// Zwraca ilość elementów w buforze
uint16_t cbuf_count(volatile circ_buf_t* cbuf);

// Zwraca wolne miejsce w buforze
uint16_t cbuf_free_space(volatile circ_buf_t* cbuf);

// Sprawdza, czy bufor jest pełny
bool cbuf_is_full(volatile circ_buf_t* cbuf);

bool cbuf_push(volatile circ_buf_t* cbuf, uint8_t data);

bool cbuf_pop(volatile circ_buf_t* cbuf, uint8_t* data);

bool cbuf_is_empty(volatile circ_buf_t* cbuf);
/* SRC_CIRC_BUFFER_H_ */
