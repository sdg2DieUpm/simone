/**
 * @file fsm_rgb_light.c
 * @brief RGB light system FSM main file.
 * @author alumno1
 * @author alumno2
 * @date fecha
 */

/* Includes ------------------------------------------------------------------*/
/* Standard C includes */

/* HW dependent includes */

/* Project includes */

/* Typedefs --------------------------------------------------------------------*/

/* Private functions -----------------------------------------------------------*/

/* State machine input or transition functions */

/* State machine output or action functions */

/* Other auxiliary functions */

/* Public functions -----------------------------------------------------------*/
fsm_rgb_light_t *fsm_rgb_light_new(uint8_t rgb_light_id)
{
    fsm_rgb_light_t *p_fsm_rgb_light = malloc(sizeof(fsm_rgb_light_t)); /* Do malloc to reserve memory of all other FSM elements, although it is interpreted as fsm_t (the first element of the structure) */
    fsm_rgb_light_init(p_fsm_rgb_light, rgb_light_id); /* Initialize the FSM */
    return p_fsm_rgb_light;
}