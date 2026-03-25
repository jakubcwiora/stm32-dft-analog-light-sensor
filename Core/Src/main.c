/* USER CODE BEGIN Header */
/**
 * @file           : main.c
 * @brief          : Main program body
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "arm_math.h"
#include "arm_const_structs.h"
#include "circ_buffer.h"
#include "message_builder.h"
#include "parser.h"
#include "utils.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BUFFER_SIZE_UART 2048
#define BUFFER_SIZE_ADC  512
#define FFT_SIZE         256
#define FRAME_MAX_LEN    255
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
__IO bool uart_tx_busy = false;

uint8_t rx_array[BUFFER_SIZE_UART];
uint8_t tx_array[BUFFER_SIZE_UART];
uint8_t dma_uart_rx_buffer[BUFFER_SIZE_UART];

// DSP Buffers
uint16_t adc_dma_buffer[BUFFER_SIZE_ADC];
q31_t fft_input_buffer[FFT_SIZE * 2];
q31_t fft_output[FFT_SIZE];

__IO circ_buf_t Rx_buff = { .buffer = rx_array, .size = BUFFER_SIZE_UART, .head = 0, .tail = 0 };
__IO circ_buf_t Tx_buff = { .buffer = tx_array, .size = BUFFER_SIZE_UART, .head = 0, .tail = 0 };

FrameParser parser;

volatile uint8_t adc_half_cplt = 0;
volatile uint8_t adc_full_cplt = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void process_data_from_circular_buffer(void);
void process_fft_data(uint16_t *source_buffer);

// Functions exposed to command_processor.c
void SendFrame(uint8_t id, const uint8_t *data, size_t len, const uint8_t *rec);
void send_dft_data(uint8_t index, uint16_t value);
void send_error(uint8_t error_code, const uint8_t* rec);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  Parser_Init(&parser);

  // Start Peripherals
  HAL_UARTEx_ReceiveToIdle_DMA(&huart2, dma_uart_rx_buffer, BUFFER_SIZE_UART);
  HAL_ADC_Start_DMA(&hadc1, (uint32_t*) adc_dma_buffer, BUFFER_SIZE_ADC);
  HAL_TIM_Base_Start(&htim3);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	// 1. Handle Communication
	process_data_from_circular_buffer();

	// 2. Handle DSP (First Half)
	if (adc_half_cplt) {
		adc_half_cplt = 0;
		process_fft_data(&adc_dma_buffer[0]);
	}

	// 3. Handle DSP (Second Half)
	if (adc_full_cplt) {
		adc_full_cplt = 0;
		process_fft_data(&adc_dma_buffer[FFT_SIZE]);
	}
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV8;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void process_data_from_circular_buffer(void) {
	uint8_t byte_to_process;
	while (cbuf_pop(&Rx_buff, &byte_to_process)) {

//		 // ================= ECHO START =================
//		        // 1. Wrzuć odebrany znak do bufora nadawczego (TX)
//		        cbuf_push(&Tx_buff, byte_to_process);
//
//		        if (byte_to_process == '\r') {
//		             cbuf_push(&Tx_buff, '\n');
//		        }
//
//		        // 2. Sprawdź czy transmisja trwa.
//		        __disable_irq();
//		        if (!uart_tx_busy && !cbuf_is_empty(&Tx_buff)) {
//		            uint8_t tx_byte;
//		            if (cbuf_pop(&Tx_buff, &tx_byte)) {
//		                uart_tx_busy = true;
//		                HAL_UART_Transmit_IT(&huart2, &tx_byte, 1);
//		            }
//		        }
//		        __enable_irq();
//		        // ================= ECHO END ===================

		Parser_ProcessByte(&parser, byte_to_process);
	}
}

void process_fft_data(uint16_t *source_buffer) {
    int32_t sum = 0;

    // 1. Oblicz średnią, aby idealnie usunąć DC Offset (Indeks zerowy wyniku dft)
    for (int i = 0; i < FFT_SIZE; i++) {
        sum += source_buffer[i];
    }
    int32_t avg = sum / FFT_SIZE;

    // 2. Przygotuj dane do Q31
    for (int i = 0; i < FFT_SIZE; i++) {
        // Odejmujemy realną średnią zamiast sztywnego 2048
        int32_t val = (int32_t)source_buffer[i] - avg;


        fft_input_buffer[i * 2] = (val << 14);
        fft_input_buffer[i * 2 + 1] = 0;       // Imaginary
    }

    // 3. Wykonaj FFT
    arm_cfft_q31(&arm_cfft_sR_q31_len256, fft_input_buffer, 0, 1);

    // 4. Oblicz magnitudę
    arm_cmplx_mag_q31(fft_input_buffer, fft_output, FFT_SIZE);

    // 5. Opcjonalne czyszczenie bin 0 (jeśli obliczona średnia nie była idealna)
    fft_output[0] = 0;
}

void SendFrame(uint8_t id, const uint8_t *data, size_t len, const uint8_t *rec) {
	message_builder builder;
	uint8_t build_buffer[FRAME_MAX_LEN];
	char hex_tmp[10];
	char reciever[2] = {rec[0], rec[1]};
	message_builder_init(&builder, build_buffer, sizeof(build_buffer));
	message_builder_add_text(&builder, "(ST");
	message_builder_add_text(&builder, reciever); // FIX: dodajemy adresata z parametrów
	sprintf(hex_tmp, "%02X", id);
	message_builder_add_text(&builder, hex_tmp);

	// Encode Data Payload to Hex
	for (size_t i = 0; i < len; i++) {
		sprintf(hex_tmp, "%02X", data[i]);
		message_builder_add_text(&builder, hex_tmp);
	}

	// Calculate CRC on RAW binary data (ID + Payload)
	uint8_t crc_calc_buf[len + 1];
	crc_calc_buf[0] = id;
	if (len > 0)
		memcpy(&crc_calc_buf[1], data, len);

	uint16_t crc_val = crc16(crc_calc_buf, len + 1);

	sprintf(hex_tmp, "%04X", crc_val);
	message_builder_add_text(&builder, hex_tmp);
	message_builder_add_text(&builder, ")"); // Footer

	// Push to TX Buffer
	for (size_t i = 0; i < builder.current_pos; ++i) {
		cbuf_push(&Tx_buff, build_buffer[i]);
	}

	// Trigger Transmission
	__disable_irq();
	if (!uart_tx_busy && !cbuf_is_empty(&Tx_buff)) {
		uint8_t byte;
		if (cbuf_pop(&Tx_buff, &byte)) {
			uart_tx_busy = true;
			HAL_UART_Transmit_IT(&huart2, &byte, 1);
		}
	}
	__enable_irq();
}



void send_error(uint8_t error_code, const uint8_t* rec) {
	uint8_t payload = error_code;
	SendFrame(error_code, &payload, 1, rec);
}

/* Callbacks -----------------------------------------------------------------*/

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART2) {
		uint8_t data;
		if (cbuf_pop(&Tx_buff, &data)) {
			HAL_UART_Transmit_IT(&huart2, &data, 1);
		} else {
			uart_tx_busy = false;
		}
	}
}

//TODO: Można brać rx_empy z dm i przetwarzać na bierzaco
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
	if (huart->Instance == USART2) {
		for (uint16_t i = 0; i < Size; i++) {
			cbuf_push(&Rx_buff, dma_uart_rx_buffer[i]);
		}
		HAL_UARTEx_ReceiveToIdle_DMA(huart, dma_uart_rx_buffer,
				BUFFER_SIZE_UART);
	}
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc) {
	adc_half_cplt = 1;
//	// --- TEST WIZUALNY ---
//	    static uint8_t led_divider = 0;
//	    led_divider++;
//
//	    // 31 przerwań na sekundę / 30 = zmiana stanu ok. 1 raz na sekundę
//	    if (led_divider >= 30) {
//	        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin); // Zmień stan diody
//	        led_divider = 0;
//	    }
//	    // ---------------------
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
	adc_full_cplt = 1;
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
