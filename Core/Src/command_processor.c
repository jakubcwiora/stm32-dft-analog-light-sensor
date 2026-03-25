
/*
 * command_processor.c
 *
 *  Created on: Feb 4, 2026
 *      Author: jakub
 */

#include "parser.h"
#include "arm_math.h"
#include <string.h>
#include "command_processor.h"
// === Ustawienia Skalowania ===
// Wartość Q31 to liczby rzędu 2 miliardów. Musimy je przesunąć w prawo,
// żeby zmieściły się w uint16 (0-65535).
// Im mniejsza wartość, tym większa "czułość", ale ryzyko przekroczenia 65535.
#define FFT_OUTPUT_SHIFT 12

// === Zmienne Zewnętrzne ===
// Tablica z wynikami FFT zdefiniowana w main.c
extern q31_t fft_output[256];

// Funkcje z main.c do wysyłania odpowiedzi
extern void SendFrame(uint8_t id, const uint8_t* data, size_t len);
extern void send_error(uint8_t error_code);

// === Definicje błędów ze specyfikacji ===
#define ERR_PARAM   0xC8
#define ERR_UNKNOWN 0xCA

// === Funkcje Pomocnicze ===

// Konwertuje Q31 (int32) na uint16 z nasyceniem (clipping)
static uint16_t ScaleQ31ToUint16(q31_t value) {
    q31_t scaled = value >> FFT_OUTPUT_SHIFT;

    if (scaled > 65535) {
        return 65535; // Nasycenie (max uint16)
    } else if (scaled < 0) {
        return 0;     // Powinno być dodatnie (amplituda), ale dla bezpieczeństwa
    } else {
        return (uint16_t)scaled;
    }
}

// === Główna Funkcja Przetwarzania ===

void ProcessCommand(uint8_t* sender, const uint8_t* payload, size_t length) {
    if (length < 1) {
        // Pusta komenda?
        return;
    }

    uint8_t cmd_id = payload[0];
    const uint8_t* params = &payload[1];


    switch (cmd_id) {
        // ====================================================================
        // 0x00: GET_DFT_ON_INDEX
        // Param: [Index]
        // Odp:   0x64 [Index, ValHi, ValLo]
        // ====================================================================
        case 0x00: {
            uint8_t index = params[0];

            // Weryfikacja zakresu (FFT ma 256 prążków)
            if (index >= 256) {
                send_error(ERR_PARAM);
                return;
            }

            // Pobranie i skalowanie wartości
            uint16_t val = ScaleQ31ToUint16(fft_output[index]);

            // Budowa odpowiedzi
            uint8_t response[3];
            response[0] = index;
            response[1] = (val >> 8) & 0xFF; // Big Endian
            response[2] = val & 0xFF;

            SendFrame(0x64, response, 3); // RSP_DFT_ON_INDEX
            break;
        }

        // ====================================================================
        // 0x01: GET_DFT_IN_RANGE
        // Param: [Start Index, End Index]
        // Odp:   Seria ramek 0x65
        // ====================================================================
        case 0x01: {
            uint8_t start_idx = params[0];
            uint8_t end_idx = params[1];

            // 1. Walidacja parametrów
            if (start_idx > end_idx || end_idx >= 256) {
                send_error(ERR_PARAM);
                return;
            }

            uint16_t total_items = end_idx - start_idx + 1;

            // Specyfikacja: max 8 wartości na ramkę
            // Obliczamy ile ramek (stron) potrzebujemy
            uint8_t items_per_frame = 8;
            uint8_t total_frames = (total_items + items_per_frame - 1) / items_per_frame;

            uint8_t current_idx = start_idx;

            // 2. Pętla wysyłająca kolejne ramki (strony)
            for (uint8_t frame_num = 1; frame_num <= total_frames; frame_num++) {

                // Ile elementów w tej konkretnej ramce?
                uint8_t items_left = end_idx - current_idx + 1;
                uint8_t items_in_this_frame = (items_left > items_per_frame) ? items_per_frame : items_left;

                // Przygotowanie bufora odpowiedzi 0x65
                // Rozmiar: 4 bajty nagłówka + (items * 2) bajtów danych
                uint8_t resp_len = 4 + (items_in_this_frame * 2);
                uint8_t response[20]; // Max rozmiar (4 + 16 = 20)

                // Nagłówek ramki 0x65
                response[0] = total_frames;         // 1. Ilość ramek w sekwencji
                response[1] = frame_num;            // 2. Numer ramki w sekwencji
                response[2] = items_in_this_frame;  // 3. Ilość wartości w tej ramce
                response[3] = current_idx;          // 4. Pierwszy indeks w tej ramce

                // Wypełnianie danymi
                for (int i = 0; i < items_in_this_frame; i++) {
                    uint16_t val = ScaleQ31ToUint16(fft_output[current_idx + i]);

                    // Indeksy w tablicy response zaczynają się od 4
                    response[4 + (i * 2)]     = (val >> 8) & 0xFF; // Hi
                    response[4 + (i * 2) + 1] = val & 0xFF;        // Lo
                }

                // Wysłanie ramki
                SendFrame(0x65, response, resp_len);

                // Przesunięcie indeksu dla następnej pętli
                current_idx += items_in_this_frame;
            }
            break;
        }

        // ====================================================================
        // Nieznana komenda
        // ====================================================================
        default:
            send_error(ERR_UNKNOWN);
            break;
    }
}
