/**
 * @file stm32f4_button.h
 * @brief Header for stm32f4_button.c file.
 * @author alumno1
 * @author alumno2
 * @date fecha
 */
#ifndef STM32F4_BUTTON_H_
#define STM32F4_BUTTON_H_
/* Includes ------------------------------------------------------------------*/
/* Standard C includes */
#include <stdint.h>

/* HW dependent includes */
#include "stm32f4xx.h"

/* Defines and enums ----------------------------------------------------------*/
/* Defines */

/* Typedefs --------------------------------------------------------------------*/
/**
 * @brief Structure to define the HW dependencies of a button status.
 */
typedef struct
{
    GPIO_TypeDef *p_port;
    uint8_t pin;
    uint8_t pupd_mode;
    bool flag_pressed;
} stm32f4_button_hw_t;

/* Global variables */
/**
 * @brief Array of elements that represents the HW characteristics of the buttons
 *
 * This is an **extern** variable that is defined in `stm32f4_button.c`. It represents an array of hardware buttons.
 */
extern stm32f4_button_hw_t buttons_arr[];

#endif /* STM32F4_BUTTON_H_ */