/*
 * parser.c
 *
 *  Created on: 19 Jan 2026
 *      Author: jakub
 */

#include "parser.h"
#include "utils.h"
#include "protocol_defs.h"
#include <string.h>

// Funkcja zewnętrzna do wysyłania błędów (z main.c)
extern void send_error(uint8_t error_code);

// === Funkcje Prywatne ===

/**
 * @brief Analizuje zebraną ramkę (tekst wewnątrz nawiasów).
 * Format oczekiwany: SND(2) RCV(2) ID_HEX(2) DATA_HEX(N) CRC_HEX(4)
 */
static void ProcessCompleteFrame(const uint8_t* frame_content, size_t length) {

    // 1. Sprawdź minimalną długość i parzystość
    // Min: SND(2) + RCV(2) + ID(2) + DTA(2) + CRC(4) = 12 znaków
	if(length % 2) {
		send_error(ERR_PROTO);
		return;
	}
    if (length < 12) {
        send_error(ERR_PROTO);
        return;
    }

    // 2. Sprawdź Odbiorcę (znaki 2 i 3, czyli indeksy [2] i [3])
    // Oczekujemy "ST" (0x53 0x54)
    if (frame_content[2] != 'S' || frame_content[3] != 'T') {
        // To nie jest ramka do nas, ignorujemy po cichu
        return;
    }

    // Zapisz nadawcę (do wykorzystania w ProcessCommand)
    uint8_t sender[2] = {frame_content[0], frame_content[1]};

    // 3. Wyodrębnij CRC (Ostatnie 4 znaki)
    const uint8_t* crc_ptr = &frame_content[length - 4];
    uint8_t crc_bytes[2];

    // Dekodujemy 4 znaki Hex na 2 bajty binarne
    if (decode_hex(crc_ptr, 4, crc_bytes, 2) != 2) {
        send_error(ERR_PROTO); // CRC nie jest poprawnym hexem
        return;
    }

    uint16_t received_crc = (uint16_t)((crc_bytes[0] << 8) | crc_bytes[1]);

    // 4. Wyodrębnij i zdekoduj Payload (ID + Dane)
    // Payload zaczyna się po nagłówku (indeks 4) i kończy przed CRC (length - 4)
    const uint8_t* payload_hex_ptr = &frame_content[4];
    size_t payload_hex_len = length - 4 - 4; // Total - Header - CRC

    if (payload_hex_len % 2 != 0) { // TODO: cała ramka musi pyć parzysta!!!! sprawdź wcześniej
        send_error(ERR_PROTO); // Nieparzysta liczba znaków hex
        return;
    }

    // Bufor na zdekodowane dane binarne
    uint8_t raw_payload[PARSER_MAX_PAYLOAD_SIZE];

    size_t raw_len = decode_hex(payload_hex_ptr, payload_hex_len, raw_payload, sizeof(raw_payload));

    if (raw_len == 0 && payload_hex_len > 0) {
        // Dekodowanie nie powiodło się (np. złe znaki)
        send_error(ERR_PROTO);
        return;
    }

    // 5. Weryfikacja CRC
    // CRC jest liczone z surowych danych binarnych (ID + Parametry)
    uint16_t calculated_crc = crc16(raw_payload, raw_len);

    if (calculated_crc != received_crc) {
        send_error(ERR_CRC);
        return;
    }

    // 6. Sukces! Przekaż zdekodowane dane do procesora komend
    // raw_payload[0] to ID komendy, reszta to parametry
    ProcessCommand(sender, raw_payload, raw_len);
}

// === Implementacje Funkcji Publicznych ===

void Parser_Init(FrameParser* parser) {
    if (!parser) return;
    parser->state = STATE_WAIT_FOR_START;
    parser->position = 0;
}

void Parser_ProcessByte(FrameParser* parser, uint8_t byte) {
    if (!parser) return;

    // TODO: należy podnieść sprawdzenie początku ramki - WYKONANO
    // Jeśli pojawi się znak startu, zawsze resetujemy parser, niezależnie od stanu
    if (byte == FRAME_START_CHAR) {
        parser->position = 0;
        parser->state = STATE_RECEIVING_FRAME;
        return;
    }

    switch (parser->state) {
        case STATE_WAIT_FOR_START:
            // Czekamy wyłącznie na FRAME_START_CHAR, który jest obsłużony wyżej
            break;

        case STATE_RECEIVING_FRAME:
            if (byte == FRAME_END_CHAR) {
                // Koniec ramki -> Przetwarzaj
                ProcessCompleteFrame(parser->buffer, parser->position);
                parser->state = STATE_WAIT_FOR_START;
            } else {
                // Zbieranie danych
                // Zabezpieczenie przed zapisem poza bufor
                if (parser->position < PARSER_BUFFER_SIZE) {
                    parser->buffer[parser->position] = byte;

                    // TODO: Sprawdzać PO!!!! zmianie position - WYKONANO
                    parser->position++;

                    if (parser->position >= PARSER_BUFFER_SIZE) {
                        // Overflow - ramka wypełniła cały bufor, a nie ma znaku końca
                        send_error(ERR_PROTO);
                        parser->state = STATE_WAIT_FOR_START;
                    }
                }
            }
            break;
    }
}


