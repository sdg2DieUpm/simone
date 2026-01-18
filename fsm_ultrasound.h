/**
 * @file fsm_ultrasound.h
 * @brief Header for fsm_ultrasound.c file.
 * @author alumno1
 * @author alumno2
 * @date fecha
 */

#ifndef FSM_ULTRASOUND_H_
#define FSM_ULTRASOUND_H_

/* Includes ------------------------------------------------------------------*/
/* Standard C includes */

/* Defines and enums ----------------------------------------------------------*/

/* Typedefs --------------------------------------------------------------------*/

/* Function prototypes and explanation -------------------------------------------------*/


/**
 * @brief Set the state of the ultrasound FSM.
 *
 * This function sets the current state of the ultrasound FSM.
 *
 * > &nbsp;&nbsp;&nbsp;&nbsp;💡 This function is important because the struct is private and external functions such as those of the unit tests cannot access the state of the FSM directly. \n
 * 
 * @param p_fsm Pointer to an `fsm_ultrasound_t` struct.
 * @param state New state of the ultrasound FSM.
 */
void fsm_ultrasound_set_state(fsm_ultrasound_t *p_fsm, int8_t state);

#endif /* FSM_ULTRASOUND_H_ */
