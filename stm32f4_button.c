/**
 * @file stm32f4_button.c
 * @brief Portable functions to interact with the button FSM library. All portable functions must be implemented in this file.
 * @author alumno1
 * @author alumno2
 * @date fecha
 */

/* Includes ------------------------------------------------------------------*/
/* Standard C includes */

/* HW dependent includes */
#include "port_button.h" // Used to get general information about the buttons (ID, etc.)
#include "port_system.h" // Used to get the system tick

/* Microcontroller dependent includes */
// TO-DO alumnos: include the necessary files to interact with the GPIOs


/* Typedefs --------------------------------------------------------------------*/
typedef struct
{
    GPIO_TypeDef *p_port;
    uint8_t pin;
    uint8_t pupd_mode;
    bool flag_pressed;
} stm32f4_button_hw_t;

/* Global variables ------------------------------------------------------------*/

/* Private functions ----------------------------------------------------------*/
/**
 * @brief Get the button status struct with the given ID.
 *
 * @param button_id Button ID.
 *
 * @return Pointer to the button state struct.
 * @return NULL If the button ID is not valid.
 */
stm32f4_button_hw_t *_stm32f4_button_get(uint32_t button_id)
{
    // Return the pointer to the button with the given ID. If the ID is not valid, return NULL.
    if (button_id < sizeof(buttons_arr) / sizeof(buttons_arr[0]))
    {
        return &buttons_arr[button_id];
    }
    else
    {
        return NULL;
    }
}

/* Public functions -----------------------------------------------------------*/
void port_button_init(uint32_t button_id)
{
    // Retrieve the button struct using the private function and the button ID
    stm32f4_button_hw_t *p_button = _stm32f4_button_get(button_id);


    /* TO-DO alumnos */
}

void stm32f4_button_set_new_gpio(uint32_t button_id, GPIO_TypeDef *p_port, uint8_t pin)
{
    stm32f4_button_hw_t *p_button = _stm32f4_button_get(button_id);
    p_button->p_port = p_port;
    p_button->pin = pin;
}