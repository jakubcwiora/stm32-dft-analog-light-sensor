/*
 * message_builder.h
 *
 *  Created on: 19 Jan 2026
 *      Author: jakub
 */

#ifndef INC_MESSAGE_BUILDER_H_
#define INC_MESSAGE_BUILDER_H_

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t* buffer;      // Wskaźnik do bufora, w którym budujemy wiadomość
    size_t   buffer_size; // Całkowity rozmiar bufora
    size_t   current_pos; // Aktualna pozycja zapisu (ile bajtów już zapisano)
} message_builder;

/**
 * @brief Inicjalizuje konstruktor wiadomości.
 * @param builder Wskaźnik do struktury MessageBuilder.
 * @param buffer Wskaźnik do bufora, w którym będzie budowana wiadomość.
 * @param size Rozmiar tego bufora.
 */
void message_builder_init(message_builder* builder, uint8_t* buffer, size_t size);

/**
 * @brief Dodaje ciąg znaków (tekst) do wiadomości.
 * @param builder Wskaźnik do zainicjowanej struktury MessageBuilder.
 * @param text Tekst do dodania.
 * @return true jeśli operacja się powiodła, false jeśli zabrakło miejsca w buforze.
 */
bool message_builder_add_text(message_builder* builder, const char* text);

/**
 * @brief Dodaje surowe dane binarne do wiadomości.
 * @param builder Wskaźnik do zainicjowanej struktury MessageBuilder.
 * @param data Wskaźnik na dane do dodania (np. adres zmiennej).
 * @param size Rozmiar danych w bajtach.
 * @return true jeśli operacja się powiodła, false jeśli zabrakło miejsca w buforze.
 */
bool message_builder_add_raw(message_builder* builder, const void* data, size_t size);

#endif /* INC_MESSAGE_BUILDER_H_ */
