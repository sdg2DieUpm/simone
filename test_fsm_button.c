/**
 * @file test_fsm_button.c
 * @brief Unit test for the button FSM.
 *
 * @author Sistemas Digitales II
 * @date 2026-01-01
 */
/* System dependent libraries */
#include <stdlib.h>
#include <string.h>
#include <unity.h>

/* HW independent libraries */
#include "port_button.h"
#include "port_system.h"
#include "stm32f4_system.h"
#include "stm32f4_button.h"

/* Include FSM libraries */
#include "fsm.h"
#include "fsm_button.h"

/* Defines */
#define USER_BUTTON_DEBOUNCE_TIME_MS 150 /*!< Button debounce time in milliseconds */

/* Global variables */
static fsm_button_t *p_fsm_button;

/* Helper functions ----------------------------------------------------------*/
// Inline functions
static inline const char *_state_name(int state)
{
    if (state == BUTTON_RELEASED)
        return "BUTTON_RELEASED";
    if (state == BUTTON_PRESSED_WAIT)
        return "BUTTON_PRESSED_WAIT";
    if (state == BUTTON_PRESSED)
        return "BUTTON_PRESSED";
    if (state == BUTTON_RELEASED_WAIT)
        return "BUTTON_RELEASED_WAIT";

    return "UNDEFINED STATE";
}

/* Test functions ------------------------------------------------------------*/
void setUp(void)
{
    p_fsm_button = fsm_button_new(USER_BUTTON_DEBOUNCE_TIME_MS, PORT_USER_BUTTON_ID);
    stm32f4_system_gpio_exti_disable(STM32F4_USER_BUTTON_PIN); // Disable EXTI to avoid unwanted interrupts
}

void tearDown(void)
{
    // Nothing to do
}

void test_initial_config(void)
{
    fsm_t *p_inner_fsm = &p_fsm_button->f;
    UNITY_TEST_ASSERT_EQUAL_PTR(p_fsm_button, p_inner_fsm, __LINE__, "The inner FSM of fsm_button_t is not the first field of the struct");

    UNITY_TEST_ASSERT_EQUAL_INT(BUTTON_RELEASED, fsm_get_state(p_inner_fsm), __LINE__, "The initial state of the FSM is not BUTTON_RELEASED");

    // It assumes there are 4 transitions in the table plus the null transition
    fsm_trans_t *last_transition = &p_inner_fsm->p_tt[4];

    UNITY_TEST_ASSERT_EQUAL_INT(-1, last_transition->orig_state, __LINE__, "The origin state of the last transition of the FSM should be -1");
    UNITY_TEST_ASSERT_EQUAL_INT(NULL, last_transition->in, __LINE__, "The input condition function of the last transition of the FSM should be NULL");
    UNITY_TEST_ASSERT_EQUAL_INT(-1, last_transition->dest_state, __LINE__, "The destination state of the last transition of the FSM should be -1");
    UNITY_TEST_ASSERT_EQUAL_INT(NULL, last_transition->out, __LINE__, "The output modification function of the last transition of the FSM should be NULL");
}

/**
 * @brief Verifica las transiciones de un estado de origen en una tabla FSM.
 *
 * Esta función comprueba dos condiciones:
 * 1. Que todas las transiciones definidas en `p_expected_next_states` existan en `p_tt` para el `origin_state`.
 * 2. Que no existan transiciones desde `origin_state` en `p_tt` cuyos destinos no estén en `p_expected_next_states`.
 *
 * @param p_tt Puntero a la tabla de transiciones de la FSM (terminada en {-1, ...}).
 * @param origin_state El estado de origen cuyas transiciones se quieren verificar.
 * @param p_expected_next_states Array con los estados de destino esperados.
 * @param expected_count El número de elementos en `p_expected_next_states`.
 */
void find_and_verify_state_transitions(const fsm_trans_t *p_tt, int origin_state, const int *p_expected_next_states, int expected_count)
{
    char msg[128];
    int transitions_from_origin_found = 0;

    // Array de longitud variable flags para rastrear qué destinos esperados hemos encontrado
    bool found_flags[expected_count];
    memset(found_flags, false, sizeof(found_flags));

    // Recorrer la tabla de transiciones a testear
    for (int i = 0; p_tt[i].orig_state != -1; ++i)
    {
        // Nos centramos solo en las transiciones que parten del  estado de origen
        if (p_tt[i].orig_state == origin_state)
        {
            transitions_from_origin_found++;
            bool is_an_expected_transition = false;

            // Buscar si el destino encontrado está en la lista de esperados
            for (int j = 0; j < expected_count; ++j)
            {
                if (p_tt[i].dest_state == p_expected_next_states[j])
                {
                    found_flags[j] = true; // Encontrado
                    is_an_expected_transition = true;
                    break; // No seguir buscando
                }
            }

            // Comprobar si se encontró una transición extra (no esperada).
            if (!is_an_expected_transition)
            {
                sprintf(msg, "ERROR: It has been found an unexpected transition from state %s to state %s.", _state_name(origin_state), _state_name(p_tt[i].dest_state));
                TEST_FAIL_MESSAGE(msg);
            }
        }
    }

    // Comprobar si faltó alguna de las transiciones esperadas
    for (int j = 0; j < expected_count; ++j)
    {
        if (!found_flags[j])
        {
            sprintf(msg, "ERROR: The expected transition form state %s to state %s has not been found.", _state_name(origin_state), _state_name(p_expected_next_states[j]));
            TEST_FAIL_MESSAGE(msg); // Fallar inmediatamente.
        }
    }

    // Comprobación de que el número de transiciones encontradas es igual al esperado. Para identificar casos raros como destinos duplicados en la tabla de FSM.
    sprintf(msg, "ERROR: It expected %d transitions from the state %s, but %d were found.", expected_count, _state_name(origin_state), transitions_from_origin_found);
    UNITY_TEST_ASSERT_EQUAL_INT(expected_count, transitions_from_origin_found, __LINE__, msg);
}

void test_transitions(void)
{
    // Transitions from BUTTON_RELEASED:
    int expected_released[] = {BUTTON_PRESSED_WAIT};
    uint8_t num_expected_transitions = 1;
    find_and_verify_state_transitions(p_fsm_button->f.p_tt, BUTTON_RELEASED, expected_released, num_expected_transitions);

    // Transitions from BUTTON_PRESSED_WAIT:
    int expected_pressed_wait[] = {BUTTON_PRESSED};
    num_expected_transitions = 1;
    find_and_verify_state_transitions(p_fsm_button->f.p_tt, BUTTON_PRESSED_WAIT, expected_pressed_wait, num_expected_transitions);

    // Transitions from BUTTON_PRESSED:
    int expected_pressed[] = {BUTTON_RELEASED_WAIT};
    num_expected_transitions = 1;
    find_and_verify_state_transitions(p_fsm_button->f.p_tt, BUTTON_PRESSED, expected_pressed, num_expected_transitions);

    // Transitions from BUTTON_RELEASED_WAIT:
    int expected_released_wait[] = {BUTTON_RELEASED};
    num_expected_transitions = 1;
    find_and_verify_state_transitions(p_fsm_button->f.p_tt, BUTTON_RELEASED_WAIT, expected_released_wait, num_expected_transitions);
}

void _test_button_press(uint32_t press_time)
{
    // First transition
    buttons_arr[PORT_USER_BUTTON_ID].flag_pressed = true;

    fsm_button_fire(p_fsm_button);
    UNITY_TEST_ASSERT_EQUAL_INT(BUTTON_PRESSED_WAIT, p_fsm_button->f.current_state, __LINE__, "The FSM did not change to BUTTON_PRESSED_WAIT after pressing the button");

    // Second transition
    port_system_delay_ms(press_time);
    fsm_button_fire(p_fsm_button);
    uint32_t debounce_time_ms = fsm_button_get_debounce_time_ms(p_fsm_button);

    if (press_time < debounce_time_ms)
    {
        UNITY_TEST_ASSERT_EQUAL_INT(BUTTON_PRESSED_WAIT, p_fsm_button->f.current_state, __LINE__, "The FSM did not change to BUTTON_RELEASED_WAIT after pressing the button for a short time");
    }
    else
    {
        UNITY_TEST_ASSERT_EQUAL_INT(BUTTON_PRESSED, p_fsm_button->f.current_state, __LINE__, "The FSM did not change to BUTTON_PRESSED after pressing the button for a long time");

        // Third transition
        buttons_arr[PORT_USER_BUTTON_ID].flag_pressed = false;
        
        fsm_button_fire(p_fsm_button);
        UNITY_TEST_ASSERT_EQUAL_INT(BUTTON_RELEASED_WAIT, p_fsm_button->f.current_state, __LINE__, "The FSM did not change to BUTTON_RELEASED_WAIT after releasing the button");

        // Fourth transition
        debounce_time_ms = fsm_button_get_debounce_time_ms(p_fsm_button);

        port_system_delay_ms(debounce_time_ms + 1);
        fsm_button_fire(p_fsm_button);
        UNITY_TEST_ASSERT_EQUAL_INT(BUTTON_RELEASED, p_fsm_button->f.current_state, __LINE__, "The FSM did not change to BUTTON_RELEASED after releasing the button");
    }
}

void test_short_button_press(void)
{
    _test_button_press(100);
}

void test_long_button_press(void)
{
    _test_button_press(1000);
}

int main(void)
{
    port_system_init();
    UNITY_BEGIN();

    RUN_TEST(test_initial_config);
    RUN_TEST(test_transitions);
    RUN_TEST(test_short_button_press);
    RUN_TEST(test_long_button_press);

    exit(UNITY_END());
}
