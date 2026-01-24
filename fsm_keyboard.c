/**
 * @file fsm_keyboard.c
 * @brief Keyboard sensor FSM main file.
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
void fsm_keyboard_init(fsm_keyboard_t *p_fsm_keyboard, uint32_t keyboard_id)
{
    // Initialize the FSM
    fsm_init(&p_fsm_keyboard->f, fsm_trans_keyboard);

    /* TODO alumnos: */
    // Initialize the fields of the FSM structure
}

/* Public functions -----------------------------------------------------------*/
fsm_keyboard_t *fsm_keyboard_new(uint32_t keyboard_id)
{
    fsm_keyboard_t *p_fsm_keyboard = malloc(sizeof(fsm_keyboard_t)); /* Do malloc to reserve memory of all other FSM elements, although it is interpreted as fsm_t (the first element of the structure) */
    fsm_keyboard_init(p_fsm_keyboard, keyboard_id);                  /* Initialize the FSM */
    return p_fsm_keyboard;
}
