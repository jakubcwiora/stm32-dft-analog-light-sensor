/*
 * protocol_defs.h
 *
 *  Created on: Feb 4, 2026
 *      Author: jakub
 *      Description: Wspólne definicje kodów komend i błędów protokołu.
 */

#ifndef PROTOCOL_DEFS_H_
#define PROTOCOL_DEFS_H_

// === Stałe Protokołu ===
#define FRAME_START_CHAR '('
#define FRAME_END_CHAR   ')'

// Odbiorcy / Nadawcy (ASCII values)
#define ID_PC_STR "PC"
#define ID_ST_STR "ST"

// === Identyfikatory Danych (Command IDs) ===
typedef enum {
    // --- Komendy (Requests) ---
    CMD_GET_DFT_ON_INDEX = 0x00, // Pobierz wartość dla pojedynczego indeksu
    CMD_GET_DFT_IN_RANGE = 0x01, // Pobierz wartości z zakresu

    // --- Odpowiedzi (Responses) ---
    RSP_DFT_ON_INDEX     = 0x64, // Odpowiedź: pojedyncza wartość (3 bajty)
    RSP_DFT_IN_RANGE     = 0x65, // Odpowiedź: sekwencja wartości (max 8 na ramkę)

    // --- Błędy (Errors) ---
    ERR_PARAM            = 0xC8, // Błąd Wartości Parametru
    ERR_CRC              = 0xC9, // Błąd Sumy Kontrolnej
    ERR_UNKNOWN          = 0xCA, // Nieznany identyfikator
    ERR_PROTO            = 0xCB, // Błąd Przy Odczycie Ramki (np. zła długość)
} ProtocolID;

#endif /* PROTOCOL_DEFS_H_ */
