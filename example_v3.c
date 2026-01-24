#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "fsm_rgb_light.h"
#include "port_rgb_light.h"
#include "port_system.h"
#include "stm32f4_system.h"

/* Defines */
#define PORT_RGB_LIGHT_ID 0 /*!< Ultrasound sensor identifier @hideinitializer */

int main(void)
{
    // Initialize the system
    port_system_init();

    // Initialize the button FSM
    // Reserve space memory in the heap for the FSM
    fsm_rgb_light_t *p_fsm_rgb_light = fsm_rgb_light_new(PORT_RGB_LIGHT_ID);

    srand(time(NULL)); // Random seed using time lib
    while (1)
    {
        // In every iteration, we activate the rgb light and fire the FSM
        fsm_rgb_light_set_status(p_fsm_rgb_light, true);

        // Set an arbitrary color
        uint8_t r = (uint8_t)(rand() % COLOR_RGB_MAX_VALUE);
        uint8_t g = (uint8_t)(rand() % COLOR_RGB_MAX_VALUE);
        uint8_t b = (uint8_t)(rand() % COLOR_RGB_MAX_VALUE);
        rgb_color_t color_test = {.r = r, .g = g, .b = b};

        // Set a new color
        printf("Testing color [R, G, B] = [%d, %d, %d]\n", r, g, b);
        fsm_rgb_light_set_color(p_fsm_rgb_light, color_test);
        fsm_rgb_light_fire(p_fsm_rgb_light);

        // Set a new intensity
        uint8_t intensity_perc = MAX_LEVEL_INTENSITY;

        // While there is something visible
        while((p_fsm_rgb_light->color.r > 1) && (p_fsm_rgb_light->color.g > 1) && (p_fsm_rgb_light->color.b > 1))
        {
            printf("[%ld] Display color at intensity of %d percentage: [R, G, B] = [%d, %d, %d]\n", port_system_get_millis(), intensity_perc, p_fsm_rgb_light->color.r, p_fsm_rgb_light->color.g, p_fsm_rgb_light->color.b);
            fsm_rgb_light_set_intensity(p_fsm_rgb_light, intensity_perc);
            fsm_rgb_light_fire(p_fsm_rgb_light);
            port_system_delay_ms(10);
            intensity_perc -= 1;
        }
        // Stop the rgb light to ensure that the RGB LED is turned off
        fsm_rgb_light_set_status(p_fsm_rgb_light, false);
        fsm_rgb_light_fire(p_fsm_rgb_light);
    }

    return 0;
}
