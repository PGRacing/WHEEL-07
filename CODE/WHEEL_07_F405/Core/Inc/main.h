/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define STATUS_LED_Pin GPIO_PIN_13
#define STATUS_LED_GPIO_Port GPIOC
#define PADDLE_DOWN_Pin GPIO_PIN_4
#define PADDLE_DOWN_GPIO_Port GPIOA
#define PADDLE_UP_Pin GPIO_PIN_5
#define PADDLE_UP_GPIO_Port GPIOA
#define WHEEL_BTN1_Pin GPIO_PIN_6
#define WHEEL_BTN1_GPIO_Port GPIOA
#define WHEEL_BTN2_Pin GPIO_PIN_7
#define WHEEL_BTN2_GPIO_Port GPIOA
#define WHEEL_BTN3_Pin GPIO_PIN_4
#define WHEEL_BTN3_GPIO_Port GPIOC
#define WHEEL_BTN4_Pin GPIO_PIN_5
#define WHEEL_BTN4_GPIO_Port GPIOC
#define WHEEL_BTN5_Pin GPIO_PIN_0
#define WHEEL_BTN5_GPIO_Port GPIOB
#define WHEEL_BTN6_Pin GPIO_PIN_1
#define WHEEL_BTN6_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
