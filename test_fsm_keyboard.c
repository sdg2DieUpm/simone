/**
 * @file test_fsm_keyboard.c
 * @brief Unit test for the keyboard FSM.
 *
 * @author Sistemas Digitales II
 * @date 2026-01-01
 */
/* System dependent libraries */
#include <stdlib.h>
#include <string.h>
#include <unity.h>

/* HW independent libraries */
#include "port_keyboard.h"
#include "port_system.h"
#include "stm32f4_system.h"
#include "stm32f4_keyboard.h"

/* Include FSM libraries */
#include "fsm.h"
#include "fsm_keyboard.h"

/* Defines */
#define TEST_MAIN_KEYBOARD_DEBOUNCE_TIME_MS 150                          /*!< Keyboard debounce time in milliseconds @hideinitializer */
#define TEST_SHORT_PRESS_TIME (TEST_MAIN_KEYBOARD_DEBOUNCE_TIME_MS - 10) /*!< Key press for a short time  @hideinitializer */
#define TEST_LONG_PRESS_TIME (TEST_MAIN_KEYBOARD_DEBOUNCE_TIME_MS + 10)  /*!< Key press for a long time  @hideinitializer */
#define TEST_PORT_MAIN_KEYBOARD_ID 0                                     /*!< Main keyboard identifier for tests @hideinitializer */
#define TEST_NUM_ROWS 4                                                  /*!< Number of rows in the test keyboard @hideinitializer */
#define TEST_NUM_COLS 4                                                  /*!< Number of cols in the test keyboard @hideinitializer */

/* Global variables */
static char msg[200];
static fsm_keyboard_t *p_fsm_keyboard;

/* Helper functions ----------------------------------------------------------*/
// Inline functions
static inline const char *_state_name(int state)
{
    if (state == KEYBOARD_RELEASED_WAIT_ROW)
        return "KEYBOARD_RELEASED_WAIT_ROW";
    if (state == KEYBOARD_PRESSED_WAIT)
        return "KEYBOARD_PRESSED_WAIT";
    if (state == KEYBOARD_PRESSED)
        return "KEYBOARD_PRESSED";
    if (state == KEYBOARD_RELEASED_WAIT)
        return "KEYBOARD_RELEASED_WAIT";

    return "UNDEFINED STATE";
}

/* Test functions ------------------------------------------------------------*/
void setUp(void)
{
    p_fsm_keyboard = fsm_keyboard_new(TEST_MAIN_KEYBOARD_DEBOUNCE_TIME_MS, TEST_PORT_MAIN_KEYBOARD_ID);

    // Disable interrupts of the correct configuration to avoid interrupt calls
    for (uint8_t col = 0; col < keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].p_layout->num_cols; col++)
    {
        stm32f4_system_gpio_exti_disable(keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].p_col_pins[col]);
    }
}

void tearDown(void)
{
    // Nothing to do
}

void test_initial_config(void)
{
    fsm_t *p_inner_fsm = &p_fsm_keyboard->f;
    UNITY_TEST_ASSERT_EQUAL_PTR(p_fsm_keyboard, p_inner_fsm, __LINE__, "The inner FSM of fsm_keyboard_t is not the first field of the struct");

    UNITY_TEST_ASSERT_EQUAL_INT(KEYBOARD_RELEASED_WAIT_ROW, fsm_get_state(p_inner_fsm), __LINE__, "The initial state of the FSM is not KEYBOARD_RELEASED_WAIT_ROW");

    // It assumes there are 6 transitions in the table plus the null transition
    fsm_trans_t *last_transition = &p_inner_fsm->p_tt[5];

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
    // Transitions from KEYBOARD_RELEASED_WAIT_ROW:
    int expected_wait_row[] = {KEYBOARD_RELEASED_WAIT_ROW, KEYBOARD_PRESSED_WAIT};
    uint8_t num_expected_transitions = 2;
    find_and_verify_state_transitions(p_fsm_keyboard->f.p_tt, KEYBOARD_RELEASED_WAIT_ROW, expected_wait_row, num_expected_transitions);

    // Transitions from KEYBOARD_PRESSED_WAIT:
    int expected_pressed_wait[] = {KEYBOARD_PRESSED};
    num_expected_transitions = 1;
    find_and_verify_state_transitions(p_fsm_keyboard->f.p_tt, KEYBOARD_PRESSED_WAIT, expected_pressed_wait, num_expected_transitions);

    // Transitions from KEYBOARD_PRESSED:
    int expected_pressed[] = {KEYBOARD_RELEASED_WAIT};
    num_expected_transitions = 1;
    find_and_verify_state_transitions(p_fsm_keyboard->f.p_tt, KEYBOARD_PRESSED, expected_pressed, num_expected_transitions);

    // Transitions from KEYBOARD_RELEASED_WAIT:
    int expected_released_wait[] = {KEYBOARD_RELEASED_WAIT_ROW};
    num_expected_transitions = 1;
    find_and_verify_state_transitions(p_fsm_keyboard->f.p_tt, KEYBOARD_RELEASED_WAIT, expected_released_wait, num_expected_transitions);
}

void test_keyboard_press(void)
{
    // First transition
    keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].flag_row_timeout = true;

    fsm_keyboard_fire(p_fsm_keyboard);
    UNITY_TEST_ASSERT_EQUAL_INT(KEYBOARD_RELEASED_WAIT_ROW, p_fsm_keyboard->f.current_state, __LINE__, "The FSM did not change to KEYBOARD_RELEASED_WAIT_ROW after the row timeout.");

    UNITY_TEST_ASSERT_EQUAL_UINT8(false, keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].flag_row_timeout, __LINE__, "The FSM did not clear the flag flag_row_timeout after changing to KEYBOARD_RELEASED_WAIT_ROW.");

    // Check that the second row is set
    uint8_t current_row = keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].current_excited_row;
    uint32_t row_pin = keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].p_row_pins[current_row];
    uint32_t row_gpio_odr = keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].p_row_ports[current_row]->ODR & (1 << row_pin);

    sprintf(msg, "ERROR: Row %d pin must be set to high after starting the scan timer", current_row);
    UNITY_TEST_ASSERT_EQUAL_UINT32(1 << row_pin, row_gpio_odr, __LINE__, msg);

    // Check that the other rows are low
    for (uint8_t r = 1; r < TEST_NUM_ROWS; r++)
    {
        row_pin = keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].p_row_pins[r];
        row_gpio_odr = keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].p_row_ports[r]->ODR & (1 << row_pin);
        sprintf(msg, "ERROR: Row %d pin must be set to low when another row is high.", r);
        UNITY_TEST_ASSERT_NOT_EQUAL_UINT32(1 << row_pin, row_gpio_odr, __LINE__, msg);
    }

    // Second transition
    keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].flag_key_pressed = true;

    fsm_keyboard_fire(p_fsm_keyboard);
    UNITY_TEST_ASSERT_EQUAL_INT(KEYBOARD_PRESSED_WAIT, p_fsm_keyboard->f.current_state, __LINE__, "The FSM did not change to KEYBOARD_PRESSED_WAIT after pressing the keyboard");

    // Third transition
    port_system_delay_ms(TEST_SHORT_PRESS_TIME);
    fsm_keyboard_fire(p_fsm_keyboard);
    uint32_t debounce_time_ms = p_fsm_keyboard->debounce_time_ms;

    UNITY_TEST_ASSERT_EQUAL_INT(KEYBOARD_PRESSED_WAIT, p_fsm_keyboard->f.current_state, __LINE__, "The FSM did not keep in KEYBOARD_PRESSED_WAIT after pressing the keyboard for a short time lower than the debounce time.");

    UNITY_TEST_ASSERT_EQUAL_UINT8(false, keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].flag_row_timeout, __LINE__, "The FSM did not clear the flag flag_key_pressed after changing to KEYBOARD_RELEASED_WAIT.");

    // Long press
    port_system_delay_ms(TEST_LONG_PRESS_TIME);
    fsm_keyboard_fire(p_fsm_keyboard);

    UNITY_TEST_ASSERT_EQUAL_INT(KEYBOARD_PRESSED, p_fsm_keyboard->f.current_state, __LINE__, "The FSM did not change to KEYBOARD_PRESSED after pressing the keyboard for a long time.");

    // Go back an forth from Third to Fourth transition to test all keys
    for (uint8_t r = 0; r < TEST_NUM_ROWS; r++)
    {
        keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].current_excited_row = r;
        for (uint8_t c = 0; c < TEST_NUM_COLS; c++)
        {
            keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].col_idx_interrupt = c;

            // Force state
            fsm_set_state(&p_fsm_keyboard->f, KEYBOARD_PRESSED);

            // Release the key
            keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].flag_key_pressed = false;

            char expected_key_value = keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].p_layout->keys[r * TEST_NUM_COLS + c];

            fsm_keyboard_fire(p_fsm_keyboard);
            sprintf(msg, "The FSM did not change to KEYBOARD_RELEASED_WAIT after releasing the key for row %d, col %d.", r, c);

            UNITY_TEST_ASSERT_EQUAL_INT(KEYBOARD_RELEASED_WAIT, p_fsm_keyboard->f.current_state, __LINE__, msg);

            sprintf(msg, "ERROR: key value not set correctly for row %d, col %d.", r, c);
            char read_value = fsm_keyboard_get_key_value(p_fsm_keyboard);
            UNITY_TEST_ASSERT_EQUAL_CHAR(expected_key_value, read_value, __LINE__, msg);
        }
    }
    // Fifth transition
    port_system_delay_ms(debounce_time_ms + 1);
    fsm_keyboard_fire(p_fsm_keyboard);
    UNITY_TEST_ASSERT_EQUAL_INT(KEYBOARD_RELEASED_WAIT_ROW, p_fsm_keyboard->f.current_state, __LINE__, "The FSM did not change to KEYBOARD_RELEASED_WAIT_ROW after releasing the keyboard");
}

int main(void)
{
    port_system_init();
    UNITY_BEGIN();

    RUN_TEST(test_initial_config);
    RUN_TEST(test_transitions);
    RUN_TEST(test_keyboard_press);
    exit(UNITY_END());
}
