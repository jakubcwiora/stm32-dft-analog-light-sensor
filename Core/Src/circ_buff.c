/*
 * circ_buff.c
 *
 *  Created on: 17 Jan 2026
 *      Author: jakub
 */


#include "circ_buffer.h"


uint16_t cbuf_free_space(volatile circ_buf_t* cbuf) {
    // Pamiętaj, że jedno miejsce jest rezerwowane
    return (cbuf->size - 1) - cbuf_count(cbuf);
}

bool cbuf_is_full(volatile circ_buf_t* cbuf) {
    return cbuf_free_space(cbuf) == 0;
}

bool cbuf_is_empty(volatile circ_buf_t* cbuf) {

    uint16_t head, tail;
   // __disable_irq();
    head = cbuf->head;
    tail = cbuf->tail;
   // __enable_irq();
    return head == tail;
}

bool cbuf_push(volatile circ_buf_t* cbuf, uint8_t data) {
    uint16_t next_head = (cbuf->head + 1) % cbuf->size;

    // Sprawdzamy, czy następna pozycja head nie dogoni tail
    if (next_head == cbuf->tail) {
        return false; // Pełny
    }

    cbuf->buffer[cbuf->head] = data;
    cbuf->head = next_head; // Ta operacja jest atomowa na ARM Cortex-M
    return true;
}

bool cbuf_pop(volatile circ_buf_t* cbuf, uint8_t* data) {
    // Sprawdzamy, czy jest coś do pobrania
    if (cbuf_is_empty(cbuf)) {
        return false; // Pusty
    }

    *data = cbuf->buffer[cbuf->tail];
    cbuf->tail = (cbuf->tail + 1) % cbuf->size; // Ta operacja też jest atomowa
    return true;
}
