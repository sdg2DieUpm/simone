/**
 * @file keyboards.c
 * @brief Keyboard layouts and related functions.
 * @author Sistemas Digitales II
 * @date 2026-01-01
 */

/* Includes ------------------------------------------------------------------*/
#include "keyboards.h"

/* Keyboard layouts ----------------------------------------------------------*/

// Standard 4x4 keyboard layout
#define STANDARD_KEYBOARD_NUM_ROWS 4 /*!< Number of rows in the standard keyboard matrix @hideinitializer */
#define STANDARD_KEYBOARD_NUM_COLS 4 /*!< Number of columns in the standard keyboard matrix @hideinitializer */
#define STANDARD_NULL_KEY '\0'       /*!< Null character to represent no key pressed in the standard keyboard @hideinitializer */

/**
 * @brief Standard 4x4 keyboard layout with hash and star keys.
 *
 */
static const char standard_keyboard_layout[STANDARD_KEYBOARD_NUM_ROWS][STANDARD_KEYBOARD_NUM_COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};

const keyboard_t standard_keyboard = {
    .num_rows = STANDARD_KEYBOARD_NUM_ROWS,
    .num_cols = STANDARD_KEYBOARD_NUM_COLS,
    .null_key = STANDARD_NULL_KEY,
    .keys = (const char *)standard_keyboard_layout};