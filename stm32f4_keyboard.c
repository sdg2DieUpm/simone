/**
 * @file stm32f4_keyboard.c
 * @brief Portable functions to interact with the keyboard FSM library. All portable functions must be implemented in this file.
 * @author alumno1
 * @author alumno2
 * @date date
 */

/* Standard C includes */

/* HW dependent includes */

/* Microcontroller dependent includes */

/* Typedefs --------------------------------------------------------------------*/

/* Global variables */
/* Static arrays for main keyboard (pointed by the double pointers in the struct) */

/**
 * @brief Array of GPIO ports for the rows of the main keyboard.
 *
 */
static GPIO_TypeDef *keyboard_main_row_ports[] = {
    STM32F4_KEYBOARD_MAIN_ROW_0_GPIO,
    STM32F4_KEYBOARD_MAIN_ROW_1_GPIO,
    STM32F4_KEYBOARD_MAIN_ROW_2_GPIO,
    STM32F4_KEYBOARD_MAIN_ROW_3_GPIO};

/**
 * @brief Array of GPIO pins for the rows of the main keyboard.
 *
 */
static uint8_t keyboard_main_row_pins[] = {
    STM32F4_KEYBOARD_MAIN_ROW_0_PIN,
    STM32F4_KEYBOARD_MAIN_ROW_1_PIN,
    STM32F4_KEYBOARD_MAIN_ROW_2_PIN,
    STM32F4_KEYBOARD_MAIN_ROW_3_PIN};

/**
 * @brief Array of GPIO ports for the columns of the main keyboard.
 *
 */
static GPIO_TypeDef *keyboard_main_col_ports[] = {
    STM32F4_KEYBOARD_MAIN_COL_0_GPIO,
    STM32F4_KEYBOARD_MAIN_COL_1_GPIO,
    STM32F4_KEYBOARD_MAIN_COL_2_GPIO,
    STM32F4_KEYBOARD_MAIN_COL_3_GPIO};

/**
 * @brief Array of GPIO pins for the columns of the main keyboard.
 *
 */
static uint8_t keyboard_main_col_pins[] = {
    STM32F4_KEYBOARD_MAIN_COL_0_PIN,
    STM32F4_KEYBOARD_MAIN_COL_1_PIN,
    STM32F4_KEYBOARD_MAIN_COL_2_PIN,
    STM32F4_KEYBOARD_MAIN_COL_3_PIN};

/* Private functions ----------------------------------------------------------*/

/* Public functions -----------------------------------------------------------*/
void port_keyboard_init(uint32_t keyboard_id)
{
    /* Get the keyboard sensor */
    stm32f4_keyboard_hw_t *p_keyboard = _stm32f4_keyboard_get(keyboard_id);

    /* TO-DO alumnos: */

    /* Rows configuration */

    /* Columns configuration */

    /* Clean/set all configurations */

    /* Configure timer */
}