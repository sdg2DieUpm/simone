/**
 * @file test_fsm_rgb_light.c
 * @brief Unit test for the rgb light FSM.
 *
 * @author Sistemas Digitales II
 * @date 2026-01-01
 */
/* System dependent libraries */
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unity.h>

/* HW independent libraries */
#include "port_rgb_light.h"
#include "port_system.h"
#include "stm32f4_system.h"
#include "stm32f4_rgb_light.h"

/* Include FSM libraries */
#include "fsm.h"
#include "fsm_rgb_light.h"

/* Other includes */
#include "rgb_colors.h"

/* Defines */
#define TEST_COLOR_RGB_MAX_VALUE 255 /*!< Maximum value for the RGB color @hideinitializer */
#define TEST_MAX_LEVEL_INTENSITY 100 /*!< Maximum level of intensity in percentage @hideinitializer */
// RGB timer configuration
#define TEST_RGB_LIGHT_TIM TIM4 /*!< Display RGB timer @hideinitializer */

/* Global variables ---------------------------------------------------------*/
static char msg[200]; /*!< Buffer for the error messages */
static fsm_rgb_light_t *p_fsm_rgb_light;

/* Helper functions ----------------------------------------------------------*/
// Inline functions
static inline const char *_state_name(int state)
{
    if (state == IDLE_RGB)
        return "IDLE_RGB";
    if (state == SET_COLOR)
        return "SET_COLOR";

    return "UNDEFINED STATE";
}

/* Test functions ----------------------------------------------------------*/
void setUp(void)
{
    p_fsm_rgb_light = fsm_rgb_light_new(PORT_RGB_LIGHT_ID);
}

void tearDown(void)
{
    // Nothing to do
}

void test_constants(void)
{
    // Test constants
    UNITY_TEST_ASSERT_EQUAL_UINT8(TEST_COLOR_RGB_MAX_VALUE, COLOR_RGB_MAX_VALUE, __LINE__, "The value of COLOR_RGB_MAX_VALUE is not correct.");
    UNITY_TEST_ASSERT_EQUAL_UINT8(TEST_MAX_LEVEL_INTENSITY, MAX_LEVEL_INTENSITY, __LINE__, "The value of MAX_LEVEL_INTENSITY is not correct.");
}

void test_initial_config(void)
{
    fsm_t *p_inner_fsm = &p_fsm_rgb_light->f;
    UNITY_TEST_ASSERT_EQUAL_PTR(p_fsm_rgb_light, p_inner_fsm, __LINE__, "The inner FSM of fsm_rgb_light_t is not the first field of the struct");

    UNITY_TEST_ASSERT_EQUAL_INT(IDLE_RGB, fsm_get_state(p_inner_fsm), __LINE__, "The initial state of the FSM is not IDLE_RGB");

    // It assumes there are 3 transitions in the table plus the null transition
    fsm_trans_t *last_transition = &p_inner_fsm->p_tt[3];

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
    // Transitions from IDLE_RGB:
    int expected_wait[] = {SET_COLOR};
    uint8_t num_expected_transitions = 1;
    find_and_verify_state_transitions(p_fsm_rgb_light->f.p_tt, IDLE_RGB, expected_wait, num_expected_transitions);

    // Transitions from SET_COLOR:
    int expected_set[] = {SET_COLOR, IDLE_RGB};
    num_expected_transitions = 2;
    find_and_verify_state_transitions(p_fsm_rgb_light->f.p_tt, SET_COLOR, expected_set, num_expected_transitions);
}

/**
 * @brief Check the transition from IDLE_RGB to SET_COLOR
 *
 */
void test_activation(void)
{
    // Check that the transition does not happen if the rgb light is not active
    fsm_rgb_light_set_status(p_fsm_rgb_light, false);

    // Fire the transition
    fsm_rgb_light_fire(p_fsm_rgb_light);

    // Check the state
    fsm_t *p_inner_fsm = &p_fsm_rgb_light->f;
    UNITY_TEST_ASSERT_EQUAL_INT(IDLE_RGB, fsm_get_state(p_inner_fsm), __LINE__, "The FSM should remain in the IDLE_RGB state if the state of the rgb light is not active");

    // Check that the transition happens if the rgb light is active
    fsm_rgb_light_set_status(p_fsm_rgb_light, true);

    // Fire the transition
    fsm_rgb_light_fire(p_fsm_rgb_light);

    // Check the state
    UNITY_TEST_ASSERT_EQUAL_INT(SET_COLOR, fsm_get_state(p_inner_fsm), __LINE__, "The FSM should move to the SET_COLOR state if the state of the rgb light is active");

    // Check that the output function is called and thus the color set is OFF
    uint32_t arr = TEST_RGB_LIGHT_TIM->ARR;
    uint32_t ccr_red = TEST_RGB_LIGHT_TIM->CCR1;
    uint32_t ccr_green = TEST_RGB_LIGHT_TIM->CCR3;
    uint32_t ccr_blue = TEST_RGB_LIGHT_TIM->CCR4;

    uint32_t pwm_red_test = (ccr_red * TEST_COLOR_RGB_MAX_VALUE) / (arr + 1);
    uint32_t pwm_green_test = (ccr_green * TEST_COLOR_RGB_MAX_VALUE) / (arr + 1);
    uint32_t pwm_blue_test = (ccr_blue * TEST_COLOR_RGB_MAX_VALUE) / (arr + 1);

    sprintf(msg, "ERROR: RGB LIGHT red LED is not OFF when the rgb light is activated for the first time. Expected red level: %d, actual: %ld", 0, pwm_red_test);
    UNITY_TEST_ASSERT_UINT32_WITHIN(1, 0, pwm_red_test, __LINE__, msg);

    sprintf(msg, "ERROR: RGB LIGHT green LED is not OFF when the rgb light is activated for the first time. Expected green level: %d, actual: %ld", 0, pwm_green_test);
    UNITY_TEST_ASSERT_UINT32_WITHIN(1, 0, pwm_green_test, __LINE__, msg);

    sprintf(msg, "ERROR: RGB LIGHT blue LED is not OFF when the rgb light is activated for the first time. Expected blue level: %d, actual: %ld", 0, pwm_blue_test);
    UNITY_TEST_ASSERT_UINT32_WITHIN(1, 0, pwm_blue_test, __LINE__, msg);
}

/**
 * @brief Check the self-transition in SET_COLOR
 *
 */
void test_new_color(void)
{
    // Set state to SET_COLOR
    p_fsm_rgb_light->f.current_state = SET_COLOR;

    // Force status to active
    fsm_rgb_light_set_status(p_fsm_rgb_light, true);

    // Check the state assuming that the flag new_color is not set at the beginning
    // Check that the transition does not happen if the flag new_color is not set. Because it is a self-transition, instead of calling the fsm_fire, we do it directly
    fsm_t *p_fsm = &p_fsm_rgb_light->f;
    fsm_trans_t *p_t;
    for (p_t = p_fsm->p_tt; p_t->orig_state >= 0; ++p_t)
    {
        if ((p_fsm->current_state == p_t->orig_state) && p_t->in(p_fsm))
        {
            bool in_check_status = p_t->in(p_fsm);
            UNITY_TEST_ASSERT_EQUAL_INT(false, in_check_status, __LINE__, "The input condition function of the transition from SET_COLOR to SET_COLOR should return false if the new_color flag is not set");
        }
    }

    // Set state to SET_COLOR
    p_fsm_rgb_light->f.current_state = SET_COLOR;

    // Set an arbitrary intensity and its arbitrary color
    srand(time(NULL)); // Random seed using time lib
    uint8_t test_intensity = (uint8_t)(rand() % TEST_MAX_LEVEL_INTENSITY);

    uint8_t test_color_red = (uint8_t)(rand() % TEST_COLOR_RGB_MAX_VALUE);

    uint8_t test_color_green = (uint8_t)(rand() % TEST_COLOR_RGB_MAX_VALUE);

    uint8_t test_color_blue = (uint8_t)(rand() % TEST_COLOR_RGB_MAX_VALUE);
    rgb_color_t color_test = {.r = test_color_red, .g = test_color_green, .b = test_color_blue};

    printf("Testing arbitrary color [R, G, B] = [%d, %d, %d] within the range [0, %d], and an arbitrary intensity of %d\n", test_color_red, test_color_green, test_color_blue, TEST_COLOR_RGB_MAX_VALUE, test_intensity);

    // Set the color and the intensity
    fsm_rgb_light_set_color_intensity(p_fsm_rgb_light, color_test, test_intensity);

    // Check that the self-transition happens, meaning that the new_color flag is set
    p_fsm = &p_fsm_rgb_light->f;

    for (p_t = p_fsm->p_tt; p_t->orig_state >= 0; ++p_t)
    {
        if ((p_fsm->current_state == p_t->orig_state) && p_t->in(p_fsm))
        {
            bool in_check_status = p_t->in(p_fsm);
            uint32_t state = p_fsm_rgb_light->f.current_state;

            // The transition should happen to the same state if the new_color flag is set. Check both conditions
            if (in_check_status)
            {
                p_t->out(p_fsm); // Call the output function for the sake of the test
            }

            // Check the state
            UNITY_TEST_ASSERT_EQUAL_INT(true, in_check_status, __LINE__, "The input condition function of the transition from SET_COLOR to SET_COLOR should return true if the new_color flag is set");
            UNITY_TEST_ASSERT_EQUAL_INT(SET_COLOR, state, __LINE__, "The FSM should remain in the SET_COLOR state if the new_color flag is set");
        }
    }

    // Check that it is idle and it is active
    bool is_active = fsm_rgb_light_get_status(p_fsm_rgb_light);
    bool idle_and_active = fsm_rgb_light_check_activity(p_fsm_rgb_light);
    UNITY_TEST_ASSERT_EQUAL_INT(true, is_active & !idle_and_active, __LINE__, "The FSM should be active and idle if the new_color flag is set");

    /// Check that the colors are set and corrected
    uint8_t expected_r = (uint8_t)(test_color_red * (test_intensity / (float)TEST_MAX_LEVEL_INTENSITY) + 0.5f);
    uint8_t expected_g = (uint8_t)(test_color_green * (test_intensity / (float)TEST_MAX_LEVEL_INTENSITY) + 0.5f);
    uint8_t expected_b = (uint8_t)(test_color_blue * (test_intensity / (float)TEST_MAX_LEVEL_INTENSITY) + 0.5f);
    sprintf(msg, "ERROR: RGB LIGHT red LED for a value of %d has not been corrected correctly by an intensity of %d.", test_color_red, test_intensity);
    UNITY_TEST_ASSERT_EQUAL_UINT8(expected_r, p_fsm_rgb_light->color.r, __LINE__, msg);
    sprintf(msg, "ERROR: RGB LIGHT green LED for a value of %d has not been corrected correctly by an intensity of %d.", test_color_green, test_intensity);
    UNITY_TEST_ASSERT_EQUAL_UINT8(expected_g, p_fsm_rgb_light->color.g, __LINE__, msg);
    sprintf(msg, "ERROR: RGB LIGHT blue LED for a value of %d has not been corrected correctly by an intensity of %d.", test_color_blue, test_intensity);
    UNITY_TEST_ASSERT_EQUAL_UINT8(expected_b, p_fsm_rgb_light->color.b, __LINE__, msg);

    // Check that the color PWM levels are set
    uint32_t arr = TEST_RGB_LIGHT_TIM->ARR;
    uint32_t ccr_red = TEST_RGB_LIGHT_TIM->CCR1;
    uint32_t ccr_green = TEST_RGB_LIGHT_TIM->CCR3;
    uint32_t ccr_blue = TEST_RGB_LIGHT_TIM->CCR4;

    // Given the intensity set, compute the expected color
    uint32_t pwm_red_test = (ccr_red * TEST_COLOR_RGB_MAX_VALUE) / (arr + 1);
    uint32_t pwm_green_test = (ccr_green * TEST_COLOR_RGB_MAX_VALUE) / (arr + 1);
    uint32_t pwm_blue_test = (ccr_blue * TEST_COLOR_RGB_MAX_VALUE) / (arr + 1);

    sprintf(msg, "ERROR: RGB LIGHT red LED is not set to the correct color in PWM. Expected red level: %d, actual: %ld", expected_r, pwm_red_test);
    UNITY_TEST_ASSERT_UINT32_WITHIN(1, expected_r, pwm_red_test, __LINE__, msg);

    sprintf(msg, "ERROR: RGB LIGHT green LED is not set to the correct color in PWM. Expected green level: %d, actual: %ld", expected_g, pwm_green_test);
    UNITY_TEST_ASSERT_UINT32_WITHIN(1, expected_g, pwm_green_test, __LINE__, msg);

    sprintf(msg, "ERROR: RGB LIGHT blue LED is not set to the correct color in PWM. Expected blue level: %d, actual: %ld", expected_b, pwm_blue_test);
    UNITY_TEST_ASSERT_UINT32_WITHIN(1, expected_b, pwm_blue_test, __LINE__, msg);
}

void test_check_off(void)
{
    // Set state to SET_COLOR
    p_fsm_rgb_light->f.current_state = SET_COLOR;

    // Check that the function returns true if the rgb light is not active
    fsm_rgb_light_set_status(p_fsm_rgb_light, false);

    // Fire the transition
    fsm_rgb_light_fire(p_fsm_rgb_light);

    // Check the state
    fsm_t *p_inner_fsm = &p_fsm_rgb_light->f;
    UNITY_TEST_ASSERT_EQUAL_INT(IDLE_RGB, fsm_get_state(p_inner_fsm), __LINE__, "The FSM should move to the IDLE_RGB state if the rgb light is not active");

    // Check that the color is OFF (CCxR are disabled)
    uint32_t ccer_red = TEST_RGB_LIGHT_TIM->CCER & TIM_CCER_CC1E;
    uint32_t ccer_green = TEST_RGB_LIGHT_TIM->CCER & TIM_CCER_CC3E;
    uint32_t ccer_blue = TEST_RGB_LIGHT_TIM->CCER & TIM_CCER_CC4E;

    UNITY_TEST_ASSERT_EQUAL_UINT32(0, ccer_red, __LINE__, "The red LED should be disabled if the rgb light is not active");
    UNITY_TEST_ASSERT_EQUAL_UINT32(0, ccer_green, __LINE__, "The green LED should be disabled if the rgb light is not active");
    UNITY_TEST_ASSERT_EQUAL_UINT32(0, ccer_blue, __LINE__, "The blue LED should be disabled if the rgb light is not active");

    // Check that it is not idle and it is not active
    bool is_active = fsm_rgb_light_get_status(p_fsm_rgb_light);
    bool idle_and_active = fsm_rgb_light_check_activity(p_fsm_rgb_light);
    UNITY_TEST_ASSERT_EQUAL_INT(false, is_active & !idle_and_active, __LINE__, "The FSM should not be active and not idle if the rgb light is not active");
}

int main(void)
{
    port_system_init();
    UNITY_BEGIN();

    RUN_TEST(test_constants);
    RUN_TEST(test_initial_config);
    RUN_TEST(test_transitions);
    RUN_TEST(test_activation);
    RUN_TEST(test_new_color);
    RUN_TEST(test_check_off);

    exit(UNITY_END());
}
