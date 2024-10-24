/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

#include "gpio.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "fm11.h"
#include "stdio.h"
#include "utility.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define FM11_I2C_ADDRESS (0xAE)
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define FM441_NDEF_Header_EEaddress 0x010
#define RESET_SILENCE 0xFFE6
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

GPIO_PinState hall_l_prev_state = GPIO_PIN_RESET;
GPIO_PinState hall_r_prev_state = GPIO_PIN_RESET;

periodic_task_t led_blink_ctr = {0, 500u, led_blink};
periodic_task_t update_eeprom_ctr = {0, 250u, update_eeprom};

uint8_t read_buf[16] = {};  // 测试BUF
uint8_t write_buf[16] = {'L', ':', '0', 'R', ':', '0', 'S', 'p', 'r', 'i', 'n', 't', 'R', 'a', 'y', '\0'};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
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
    MX_USART2_UART_Init();
    MX_TIM2_Init();
    MX_I2C1_Init();
    /* USER CODE BEGIN 2 */

    HAL_GPIO_WritePin(CSN_GPIO_Port, CSN_Pin, GPIO_PIN_SET);
    HAL_Delay(100);

    FM11_init();

    FM11_enable();
    FM11_silence();
    FM11_write_eeprom_page(FM11_USER_DATA_START_EEPROM, write_buf, 16);
    FM11_read_eeprom_page(FM11_USER_DATA_START_EEPROM, read_buf, 16);
    FM11_unsilence();
    FM11_disable();

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1) {
        run_periodically(&led_blink_ctr);
        run_periodically(&update_eeprom_ctr);

        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef       RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef       RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /** Configure the main internal regulator output voltage
     */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLLMUL_4;
    RCC_OscInitStruct.PLL.PLLDIV = RCC_PLLDIV_2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
        Error_Handler();
    }
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2 | RCC_PERIPHCLK_I2C1;
    PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
    PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */

void FN11_init() {
    silence();
}

void led_blink() {
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
}

void update_eeprom() {
    GPIO_PinState hall_l_curr_state = HAL_GPIO_ReadPin(HALL_L_GPIO_Port, HALL_L_Pin);
    GPIO_PinState hall_r_curr_state = HAL_GPIO_ReadPin(HALL_R_GPIO_Port, HALL_R_Pin);

    if (hall_l_curr_state != hall_l_prev_state || hall_r_curr_state != hall_r_prev_state) {
        write_buf[2] = '0' + hall_l_curr_state;
        write_buf[5] = '0' + hall_r_curr_state;

        hall_l_prev_state = hall_l_curr_state;
        hall_r_prev_state = hall_r_curr_state;

        FM11_enable();
        FM11_silence();
        FM11_write_eeprom_page(FM11_USER_DATA_START_EEPROM, write_buf, 16);
        FM11_read_eeprom_page(FM11_USER_DATA_START_EEPROM, read_buf, 16);
        FM11_unsilence();
        FM11_disable();
    }
}

void silence() {
    uint8_t silience_code = 0x33;
    HAL_I2C_Mem_Write(&hi2c1, FM11_I2C_ADDRESS, RESET_SILENCE, I2C_MEMADD_SIZE_16BIT, &silience_code, 1,
                      1000);
}

void un_silience() {
    uint8_t silience_code = 0xCC;
    HAL_I2C_Mem_Write(&hi2c1, FM11_I2C_ADDRESS, RESET_SILENCE, I2C_MEMADD_SIZE_16BIT, &silience_code, 1,
                      1000);
}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {
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
void assert_failed(uint8_t *file, uint32_t line) {
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
