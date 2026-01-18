/**
 * @file fsm_display.c
 * @brief Display system FSM main file.
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
fsm_display_t *fsm_display_new(uint32_t display_id)
{
    fsm_display_t *p_fsm_display = malloc(sizeof(fsm_display_t)); /* Do malloc to reserve memory of all other FSM elements, although it is interpreted as fsm_t (the first element of the structure) */
    fsm_display_init(p_fsm_display, display_id); /* Initialize the FSM */
    return p_fsm_display;
}