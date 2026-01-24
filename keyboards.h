/**
 * @file keyboards.h
 * @brief Header to define keyboard layouts and related constants.
 * @author Sistemas Digitales II
 * @date 2026-01-01
 */

#ifndef KEYBOARDS_H_
#define KEYBOARDS_H_

/* Includes ------------------------------------------------------------------*/
/* Standard C includes */
#include <stdint.h>

/* Typedefs --------------------------------------------------------------------*/
/**
 * @brief Structure to define the keyboard key mapping and related information.
 */
typedef struct
{
    uint8_t num_rows; /*!< Number of rows in the keyboard matrix */
    uint8_t num_cols; /*!< Number of columns in the keyboard matrix */
    char null_key;    /*!< Character to represent no key pressed */
    const char *keys; /*!< Pointer to the keyboard layout matrix */
} keyboard_t;

// Keyboards must be defined in keyboards.c, and declared here as extern
// Standard keyboard
extern const keyboard_t standard_keyboard;

#endif /* KEYBOARDS_H_ */
