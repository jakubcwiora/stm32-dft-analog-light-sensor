/*
 * message_builder.c
 *
 *  Created on: 19 Jan 2026
 *      Author: jakub
 */


#include "message_builder.h"


void message_builder_init(message_builder* builder, uint8_t* buffer, size_t size) {
    builder->buffer = buffer;
    builder->buffer_size = size;
    builder->current_pos = 0; // Zaczynamy pisać od początku
}

bool message_builder_add_text(message_builder* builder, const char* text) {
    size_t text_len = strlen(text);
    // Sprawdź, czy jest wystarczająco miejsca
    if ((builder->current_pos + text_len) > builder->buffer_size) {
        return false; // Błąd: za mało miejsca
    }
    // Skopiuj tekst i zaktualizuj pozycję
    memcpy(builder->buffer + builder->current_pos, text, text_len);
    builder->current_pos += text_len;
    return true;
}

bool message_builder_add_raw(message_builder* builder, const void* data, size_t size) {
    // Sprawdź, czy jest wystarczająco miejsca
    if ((builder->current_pos + size) > builder->buffer_size) {
        return false; // Błąd: za mało miejsca
    }
    // Skopiuj dane i zaktualizuj pozycję
    memcpy(builder->buffer + builder->current_pos, data, size);
    builder->current_pos += size;
    return true;
}
