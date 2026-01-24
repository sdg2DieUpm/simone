#include <stdio.h>

#include "fsm_keyboard.h"
#include "port_keyboard.h"
#include "port_system.h"
#include "stm32f4_system.h"

/* Defines */

int main(void)
{
    // Initialize the system
    port_system_init();

    // Initialize the keyboard FSM
    // Reserve space memory in the heap for the FSM
    fsm_keyboard_t *p_fsm_keyboard = fsm_keyboard_new(PORT_KEYBOARD_MAIN_DEBOUNCE_TIME_MS, PORT_KEYBOARD_MAIN_ID);
    fsm_keyboard_start_scan(p_fsm_keyboard);
    char null_key = port_keyboard_get_key_value(PORT_KEYBOARD_MAIN_ID);

    while (1)
    {
        // In every iteration, we fire the FSM and retrieve the key pressed
        fsm_keyboard_fire(p_fsm_keyboard);

        char key_value = fsm_keyboard_get_key_value(p_fsm_keyboard);
        if (key_value != null_key)
        {
            printf("Keyboard's %d key %c pressed\n", PORT_KEYBOARD_MAIN_ID, key_value);
            
            // We always reset the key after reading it
            fsm_keyboard_reset_key_value(p_fsm_keyboard);
        }
    }

    return 0;
}
