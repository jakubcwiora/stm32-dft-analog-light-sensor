# STM32 DFT Analog Light Sensor

A project realized during the 3rd semester of my studies at Bydgoszcz University of Science and Technology. 

This repository contains the C-based firmware for an STM32F103RB microcontroller that interfaces with an analog light sensor, processes the signals using Discrete Fourier Transform (DFT), and communicates with a PC using a custom asynchronous protocol.

## Features

1. **PC Communication:** Asynchronous communication between the STM32F103RB and a PC using interrupts and circular buffers.
2. **Custom Communication Protocol:**
   * Frame addressing
   * Transmission of arbitrary data
   * Verification of transmitted data correctness and order
3. **Hardware & Signal Processing:**
   * DMA-based Analog-to-Digital Converter (ADC/DAC) with a 512-word circular buffer.
   * Interrupt handling at the half-full and completely-full buffer states.
   * Sampling frequency set to 8kHz.
   * Calculates the Discrete Fourier Transform (DFT) for every 256 samples.
   * Allows the PC to read amplitudes for individual frequency bins.

---

## Communication Protocol Details

The protocol is designed to ensure safe and reliable data exchange between the PC and the STM32 board. 

* **Frame Length:** Minimum = 14 bytes, Maximum = 254 bytes.
* **Sender / Receiver Identifiers:**
  * `PC` (0x50 0x43) – Computer
  * `ST` (0x53 0x54) – STM32F103RB board

### Frame Structure
| Field | Start of Frame | Sender | Receiver | Data Identifier | Data | Checksum | End of Frame |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Value** | `(` / `0x28` | Any* | Any* | Encoded Hex | Encoded Hex | Encoded Hex | `)` / `0x29` |
| **Raw Size** | 1 byte | 2 bytes | 2 bytes | 1 byte | 1 - 120 bytes | 2 bytes | 1 byte |
| **Encoded Size** | 1 byte | 2 bytes | 2 bytes | 2 bytes | 2 - 240 bytes | 4 bytes | 1 byte |

*\* Sender and Receiver bytes can be any value that does not contain `0x28` or `0x29`.*

### Field Encoding
Data identifier, payload data, and checksum fields are hex-encoded. Raw bytes are converted to two-byte ASCII hex representations in the range `[0x30, 0x39] ∪ [0x41, 0x46]` (Characters `0-9` and `A-F`), ensuring leading zeros are included.

### Payload Data
* Data strings must be of an even length due to the hex encoding. If they are not, an `ERR_PROTO` error is returned.
* If any invalid character (outside the allowed hex range) is detected during decoding, `ERR_PROTO` is returned.
* Once successfully decoded, data fields are interpreted as Big Endian and converted to Little Endian for internal arithmetic operations.

### Commands and Responses (Data Identifiers)
The Data Identifier is a 1-byte value (2 bytes encoded) used to distinguish message types:

| Value | Name | Data Length | Description |
| :--- | :--- | :--- | :--- |
| **`0x00`** | `GET_DFT_ON_INDEX` | 1 byte | Request amplitude at a specific index. The parameter is a 1-byte index. |
| **`0x01`** | `GET_DFT_IN_RANGE` | 2 bytes | Request amplitudes in a range. The parameters are 2 bytes: start index and end index. |
| **`0x64`** | `RSP_DFT_ON_INDEX` | 3 bytes | Response for a single index. 1st byte is the index, 2nd and 3rd bytes are the value (16-bit). |
| **`0x65`** | `RSP_DFT_IN_RANGE` | Max 20 bytes | Response for a range. Sent as a sequence of frames containing up to 8 consecutive values (padded with zeros if fewer).<br><br>**Structure:**<br>1. Total frames in sequence<br>2. Current frame number<br>3. Number of values in this frame (max 8)<br>4. First index in this frame<br>5-20. Amplitude values |
| **`0xC8`** | `ERR_PARAM` | 1 byte | Parameter Value Error. |
| **`0xC9`** | `ERR_CRC` | 1 byte | Checksum Error. |
| **`0xCA`** | `ERR_UNKNOWN` | 1 byte | Unknown Identifier Error. |
| **`0xCB`** | `ERR_PROTO` | 1 byte | Frame Processing Error. |

### Checksum: CRC16/CCITT-FALSE
Calculated locally based on the frame's data and compared against the received checksum.
* **Polynomial:** `0x1021`
* **Initial Value:** `0xFFFF`
* **Calculation:** Each processed byte is shifted left by 8 bits and XORed with the current checksum. It is then shifted left by one bit 8 times; if the most significant bit is 1, the value is XORed with the polynomial `0x1021`.
* **Transmission:** It is transmitted as 4 hex-encoded bytes, effectively preventing reserved frame boundaries (Start of Frame / End of Frame characters) from appearing inside the checksum payload.

---

## Error Handling

The protocol implements robust error handling for various edge cases:

* **Multiple Start of Frame (SOF) characters:** Reading restarts with every new SOF character. Previous buffer contents are discarded.
* **Multiple End of Frame (EOF) characters:** Ignored if no SOF was previously detected.
* **SOF detected inside an ongoing frame:** Discards current progress and restarts the reading process.
* **Missing EOF:** If a frame exceeds the maximum allowed length without an EOF character, an `ERR_PROTO` message is sent. Otherwise, incomplete frames are simply ignored.
* **Invalid Receiver:** If the frame is not addressed to the receiving device, it is ignored entirely.
* **Invalid Checksum:** If the calculated checksum does not match the received one, an `ERR_CRC` message is sent. This takes priority over an unknown identifier error.
* **Unknown Data Identifier:** An `ERR_UNKNOWN` message is sent back to the sender.

---

**Author:** Jakub Ćwiora  
**Microprocessors - Project Documentation**
