/**
 * @file fsm_button.c
 * @brief Button FSM main file.
 * @author alumno1
 * @author alumno2
 * @date fecha
 */

/* Includes ------------------------------------------------------------------*/
/* Standard C includes */

/* HW dependent includes */
#include "port_button.h"
#include "port_system.h"

/* Project includes */
#include "fsm_button.h"

/* State machine input or transition functions */

/* State machine output or action functions */

/* Other auxiliary functions */
void fsm_button_init(fsm_button_t *p_fsm_button, uint32_t debounce_time, uint32_t button_id)
{
    fsm_init(&p_fsm_button->f, fsm_trans_button);

    /* TODO alumnos: */
}

/* Public functions -----------------------------------------------------------*/
fsm_button_t *fsm_button_new(uint32_t debounce_time, uint32_t button_id)
{
    fsm_button_t *p_fsm_button = malloc(sizeof(fsm_button_t)); /* Do malloc to reserve memory of all other FSM elements, although it is interpreted as fsm_t (the first element of the structure) */
    fsm_button_init(p_fsm_button, debounce_time, button_id);   /* Initialize the FSM */
    return p_fsm_button;                                       /* Composite pattern: return the fsm_t pointer as a fsm_button_t pointer */
}

/* FSM-interface functions. These functions are used to interact with the FSM */
void fsm_button_fire(fsm_button_t *p_fsm)
{
    fsm_fire(&p_fsm->f); // Is it also possible to it in this way: fsm_fire((fsm_t *)p_fsm);
}

void fsm_button_destroy(fsm_button_t *p_fsm)
{
    free(&p_fsm->f);
}

fsm_t *fsm_button_get_inner_fsm(fsm_button_t *p_fsm)
{
    return &p_fsm->f;
}

uint32_t fsm_button_get_state(fsm_button_t *p_fsm)
{
    return p_fsm->f.current_state;
}