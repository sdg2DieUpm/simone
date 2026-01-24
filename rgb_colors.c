/**
 * @file rgb_colors.c
 * @brief RGB colors and related info.
 * @author Sistemas Digitales II
 * @date 2026-01-01
 */

/* Includes ------------------------------------------------------------------*/
#include "rgb_colors.h"

/* Colors */
/**
 * @brief Red color
 * @hideinitializer
 */
const rgb_color_t color_red = {.r = COLOR_RGB_MAX_VALUE, .g = 0, .b = 0};

/**
 * @brief Green color
 * @hideinitializer
 */
const rgb_color_t color_green = {.r = 0, .g = COLOR_RGB_MAX_VALUE, .b = 0};

/**
 * @brief Blue color
 * @hideinitializer
 */
const rgb_color_t color_blue = {.r = 0, .g = 0, .b = COLOR_RGB_MAX_VALUE};

/**
 * @brief Yellow color
 * @hideinitializer
 */
const rgb_color_t color_yellow = {.r = (uint8_t)((37 * COLOR_RGB_MAX_VALUE) / 100), .g = (uint8_t)((37 * COLOR_RGB_MAX_VALUE) / 100), .b = 0};

/**
 * @brief White color
 * @hideinitializer
 */
const rgb_color_t color_white = {.r = COLOR_RGB_MAX_VALUE, .g = COLOR_RGB_MAX_VALUE, .b = COLOR_RGB_MAX_VALUE};

/**
 * @brief Turquoise color
 * @hideinitializer
 */
const rgb_color_t color_turquoise = {.r = (uint8_t)((10 * COLOR_RGB_MAX_VALUE) / 100), .g = (uint8_t)((35 * COLOR_RGB_MAX_VALUE) / 100), .b = (uint8_t)((32 * COLOR_RGB_MAX_VALUE) / 100)};

/**
 * @brief Color OFF
 * @hideinitializer
 */
const rgb_color_t color_off = {.r = 0, .g = 0, .b = 0};