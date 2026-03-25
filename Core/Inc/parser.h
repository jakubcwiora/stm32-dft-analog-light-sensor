/*
 * parser.h
 *
 *  Created on: 19 Jan 2026
 *      Author: jakub
 */

#ifndef PARSER_H_
#define PARSER_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Maksymalna długość ramki (ASCII) + margines
#define PARSER_BUFFER_SIZE 300
// Maksymalny rozmiar zdekodowanych danych binarnych (ID + Parametry)
#define PARSER_MAX_PAYLOAD_SIZE 120

typedef enum {
    STATE_WAIT_FOR_START,
    STATE_RECEIVING_FRAME
} ParserState;

typedef struct {
    ParserState state;
    uint8_t buffer[PARSER_BUFFER_SIZE];
    uint16_t position;
} FrameParser;

/**
 * @brief Inicjalizuje strukturę parsera.
 */
void Parser_Init(FrameParser* parser);

/**
 * @brief Przetwarza pojedynczy bajt odebrany z UART.
 *        Powinna być wołana w pętli głównej dla każdego znaku z bufora kołowego.
 */
void Parser_ProcessByte(FrameParser* parser, uint8_t byte);

// Deklaracja funkcji zewnętrznej (zaimplementowanej w command_processor.c)
extern void ProcessCommand(uint8_t* sender, const uint8_t* payload, size_t length);

#endif /* PARSER_H_ */
