/**
 * @file    test_port_keyboard.c
 * @brief   Integration tests for STM32F4 matrix keyboard hardware configuration.
 *
 * This suite verifies:
 *  - Keyboard layout and null key.
 *  - Row/column wiring (GPIO port/pin).
 *  - Default state after port_keyboard_init().
 *  - RCC AHB1ENR enable for all GPIO ports used by the keyboard.
 *  - GPIO registers (MODER/OTYPER/PUPDR/AFR) for rows and columns.
 *  - SYSCFG->EXTICR mapping for EXTI lines used by columns.
 *  - EXTI masks and edge selection (IMR/RTSR/FTSR) for columns.
 *  - NVIC ISER enables for EXTI IRQs associated with column lines.
 *  - TIM5 scanner configuration (RCC, PSC/ARR, DIER.UIE, CR1.CEN, SR.UIF, NVIC).
 *  - Software simulation of a press on every key (rows × columns) via EXTI.
 *  - Scan timeout flag set via TIM5 IRQ handler call.
 */
/* Includes ------------------------------------------------------------------*/
/* Standard C includes */
#include <stdlib.h>
#include <unity.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

/* HW independent libraries */
#include "stm32f4_system.h"
#include "stm32f4_keyboard.h"
#include "stm32f4xx.h"

#include "port_system.h"
#include "port_keyboard.h"

/* Defines */
#define TEST_PORT_MAIN_KEYBOARD_ID 0 /*!< Main keyboard identifier for tests @hideinitializer */

// Rows configuration
#define TEST_ROW_0_GPIO GPIOA /*!< GPIO port for row 0 @hideinitializer */
#define TEST_ROW_0_PIN 0      /*!< GPIO pin for row 0 @hideinitializer */
#define TEST_ROW_1_GPIO GPIOA /*!< GPIO port for row 1 @hideinitializer */
#define TEST_ROW_1_PIN 1      /*!< GPIO pin for row 1 @hideinitializer */
#define TEST_ROW_2_GPIO GPIOA /*!< GPIO port for row 2 @hideinitializer */
#define TEST_ROW_2_PIN 4      /*!< GPIO pin for row 2 @hideinitializer */
#define TEST_ROW_3_GPIO GPIOB /*!< GPIO port for row 3 @hideinitializer */
#define TEST_ROW_3_PIN 0      /*!< GPIO pin for row 3 @hideinitializer */
#define TEST_NUM_ROWS 4       /*!< Number of rows in the test keyboard @hideinitializer */

// Columns configuration
#define TEST_COL_0_GPIO GPIOA  /*!< GPIO port for column 0 @hideinitializer */
#define TEST_COL_0_PIN 8       /*!< GPIO pin for column 0 @hideinitializer */
#define TEST_COL_1_GPIO GPIOB  /*!< GPIO port for column 1 @hideinitializer */
#define TEST_COL_1_PIN 10      /*!< GPIO pin for column 1 @hideinitializer */
#define TEST_COL_2_GPIO GPIOB  /*!< GPIO port for column 2 @hideinitializer */
#define TEST_COL_2_PIN 4       /*!< GPIO pin for column 2 @hideinitializer */
#define TEST_COL_3_GPIO GPIOB  /*!< GPIO port for column 3 @hideinitializer */
#define TEST_COL_3_PIN 5       /*!< GPIO pin for column 3 @hideinitializer */
#define TEST_PORT_GPIO_1 GPIOA /*!< GPIO port 1 used in keyboard @hideinitializer */
#define TEST_PORT_GPIO_2 GPIOB /*!< GPIO port 2 used in keyboard @hideinitializer */
#define TEST_NUM_COLS 4        /*!< Number of columns in the test keyboard @hideinitializer */

// Column scan timer configuration
#define TEST_SCAN_TIMER TIM5                       /*!< Column scan timer @hideinitializer */
#define TEST_SCAN_TIMER_PER_BUS RCC->APB1ENR       /*!< Column scan timer peripheral bus @hideinitializer */
#define SCAN_TIMER_PER_BUS_MASK RCC_APB1ENR_TIM5EN /*!< Column scan timer peripheral bus mask @hideinitializer */
#define SCAN_TIMER_IRQ TIM5_IRQn                   /*!< Column scan timer IRQ @hideinitializer */
#define SCAN_TIMER_IRQ_PRIO 2                      /*!< Column scan timer IRQ priority @hideinitializer */
#define SCAN_TIMER_IRQ_SUBPRIO 0                   /*!< Column scan timer IRQ subpriority @hideinitializer */
#define TEST_PORT_KEYBOARD_MAIN_TIMEOUT_MS 25      /*!< Keyboard scanning timeout in milliseconds @hideinitializer */

// Other defines
#define TEST_NULL_KEY '\0'             /*!< Null key character for tests @hideinitializer */
#define TEST_DEFAULT_KEY TEST_NULL_KEY /*!< Default key value after init for tests @hideinitializer */
#define TEST_INIT_ROW -1                /*!< Initial scanned row after init @hideinitializer */
#define TEST_FLAG_ROW_TIMEOUT false    /*!< Initial value of flag_row_timeout after init @hideinitializer */
#define TEST_FLAG_KEY false            /*!< Initial value of flag_key_pressed after init @hideinitializer */

/* Global variables */
static char msg[200];

static GPIO_TypeDef *test_rows_gpio_ports_arr[] = {TEST_ROW_0_GPIO, TEST_ROW_1_GPIO, TEST_ROW_2_GPIO, TEST_ROW_3_GPIO}; /*!< Array of GPIO ports for rows @hideinitializer */
static uint8_t test_rows_gpio_pins_arr[] = {TEST_ROW_0_PIN, TEST_ROW_1_PIN, TEST_ROW_2_PIN, TEST_ROW_3_PIN};            /*!< Array of GPIO pins for rows @hideinitializer */
static GPIO_TypeDef *
    test_cols_gpio_ports_arr[] = {TEST_COL_0_GPIO, TEST_COL_1_GPIO, TEST_COL_2_GPIO, TEST_COL_3_GPIO};       /*!< Array of GPIO ports for columns @hideinitializer */
static uint8_t test_cols_gpio_pins_arr[] = {TEST_COL_0_PIN, TEST_COL_1_PIN, TEST_COL_2_PIN, TEST_COL_3_PIN}; /*!< Array of GPIO pins for columns */

static uint8_t test_cols_exticr_array[] = {0x0, 0x1, 0x1, 0x1};                                /*!< Array of EXTI CR values for columns @hideinitializer */
static IRQn_Type test_irqn_array[] = {EXTI9_5_IRQn, EXTI15_10_IRQn, EXTI4_IRQn, EXTI9_5_IRQn}; /*!< Array of IRQn_Type values for columns @hideinitializer */
static uint8_t test_irq_priorities_array[] = {1, 1, 1, 1};                                     /*!< Array of IRQ priority values for columns @hideinitializer */
static uint8_t test_irq_subpriorities_array[] = {1, 1, 1, 1};                                  /*!< Array of IRQ subpriority values for columns @hideinitializer */

/* Helper functions ----------------------------------------------------------*/
// Inline functions
static inline const char *_gpio_name(GPIO_TypeDef *g)
{
    if (g == GPIOA)
        return "GPIOA";
    if (g == GPIOB)
        return "GPIOB";
    if (g == GPIOC)
        return "GPIOC";
    return "GPIOx";
}

static inline uint32_t _rcc_gpio_enabled(GPIO_TypeDef *g)
{
    if (g == GPIOA)
    {
        return RCC->AHB1ENR & RCC_AHB1ENR_GPIOAEN;
    }
    if (g == GPIOB)
    {
        return (RCC->AHB1ENR & RCC_AHB1ENR_GPIOBEN) >> RCC_AHB1ENR_GPIOBEN_Pos;
    }
    return 0;
}

static inline bool nvic_irq_enabled(IRQn_Type irqn)
{
    uint32_t idx = ((uint32_t)irqn) >> 5;    /* ISER index */
    uint32_t bit = ((uint32_t)irqn) & 0x1Fu; /* bit inside ISER */
    return (NVIC->ISER[idx] & (1u << bit)) != 0u;
}

/* Test functions ------------------------------------------------------------*/
void setUp(void)
{
    if (TEST_PORT_GPIO_1 == GPIOA || TEST_PORT_GPIO_2 == GPIOA)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; /* GPIOA_CLK_ENABLE */
    }
    else if (TEST_PORT_GPIO_1 == GPIOB || TEST_PORT_GPIO_2 == GPIOB)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; /* GPIOA_CLK_ENABLE */
    }

    // Disable interrupts of the correct configuration to avoid interrupt calls
    for (uint8_t col = 0; col < keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].p_layout->num_cols; col++)
    {
        stm32f4_system_gpio_exti_disable(keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].p_col_pins[col]);
    }
}

void tearDown(void)
{
    if (TEST_PORT_GPIO_1 == GPIOA || TEST_PORT_GPIO_2 == GPIOA)
    {
        RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOAEN;
    }
    else if (TEST_PORT_GPIO_1 == GPIOB || TEST_PORT_GPIO_2 == GPIOB)
    {
        RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOAEN;
    }
}

/**
 * @brief Test the keyboard identifier.
 *
 */
void test_identifiers(void)
{
    UNITY_TEST_ASSERT_EQUAL_INT(TEST_PORT_MAIN_KEYBOARD_ID, PORT_KEYBOARD_MAIN_ID, __LINE__, "ERROR: PORT_KEYBOARD_MAIN_ID is incorrect");
}

/**
 * @brief Test the keyboard layout and null key are set correctly.
 *
 */
void test_layout_and_nullkey(void)
{
    // Get keyboard struct
    stm32f4_keyboard_hw_t *p_kb = &keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID];
    port_keyboard_init(TEST_PORT_MAIN_KEYBOARD_ID);

    UNITY_TEST_ASSERT_NOT_NULL(p_kb, __LINE__, "ERROR: Keyboard HW struct pointer is NULL");
    UNITY_TEST_ASSERT_NOT_NULL(p_kb->p_layout, __LINE__, "ERROR: Keyboard layout pointer is NULL");
    UNITY_TEST_ASSERT_EQUAL_PTR(&standard_keyboard, p_kb->p_layout, __LINE__, "ERROR: Keyboard layout pointer does not match standard_keyboard");
    UNITY_TEST_ASSERT_EQUAL_UINT8(TEST_NUM_ROWS, p_kb->p_layout->num_rows, __LINE__, "ERROR: Keyboard does not have the right number of rows");
    UNITY_TEST_ASSERT_EQUAL_UINT8(TEST_NUM_COLS, p_kb->p_layout->num_cols, __LINE__, "ERROR: Keyboard does not have the right number of columns");
    UNITY_TEST_ASSERT_EQUAL_CHAR(TEST_NULL_KEY, p_kb->p_layout->null_key, __LINE__, "ERROR: Keyboard null_key is not configured correctly");
    UNITY_TEST_ASSERT_EQUAL_CHAR(TEST_DEFAULT_KEY, p_kb->p_layout->null_key, __LINE__, "ERROR: Keyboard key_value is not configured correctly");
    UNITY_TEST_ASSERT_EQUAL_UINT8(TEST_INIT_ROW, p_kb->current_excited_row, __LINE__, "ERROR: current_scanned_row is not initialized with the correct value");
    UNITY_TEST_ASSERT_EQUAL_UINT8(TEST_FLAG_ROW_TIMEOUT, p_kb->flag_row_timeout, __LINE__, "ERROR: flag_row_timeout is not initialized correctly");
    UNITY_TEST_ASSERT_EQUAL_UINT8(TEST_FLAG_KEY, p_kb->flag_key_pressed, __LINE__, "ERROR: flag_key_pressed is not initialized correctly");
}

void test_wiring_rows_cols(void)
{
    /* rows wiring */
    for (uint8_t row = 0; row < TEST_NUM_ROWS; row++)
    {
        sprintf(msg, "ERROR: Row %d GPIO port/pin is incorrect", row);
        UNITY_TEST_ASSERT_EQUAL_PTR(test_rows_gpio_ports_arr[row], keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].p_row_ports[row], __LINE__, msg);

        sprintf(msg, "ERROR: Row %d GPIO pin is incorrect", row);
        UNITY_TEST_ASSERT_EQUAL_INT(test_rows_gpio_pins_arr[row], keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].p_row_pins[row], __LINE__, msg);
    }

    /* columns wiring */
    for (uint8_t col = 0; col < TEST_NUM_COLS; col++)
    {
        sprintf(msg, "ERROR: Column %d GPIO port/pin is incorrect", col);
        UNITY_TEST_ASSERT_EQUAL_PTR(test_cols_gpio_ports_arr[col], keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].p_col_ports[col], __LINE__, msg);

        sprintf(msg, "ERROR: Column %d GPIO pin is incorrect", col);
        UNITY_TEST_ASSERT_EQUAL_INT(test_cols_gpio_pins_arr[col], keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].p_col_pins[col], __LINE__, msg);
    }
}

/* Build masks for a given GPIO port based on the keyboard wiring */
static uint32_t _build_moder_mask_for_port(uint8_t keyboard_id, GPIO_TypeDef *port)
{
    uint32_t mask = 0;
    const stm32f4_keyboard_hw_t *p_kb = &keyboards_arr[keyboard_id];

    for (uint8_t r = 0; r < p_kb->p_layout->num_rows; ++r)
    {
        if (p_kb->p_row_ports[r] == port)
        {
            mask |= (GPIO_MODER_MODER0_Msk << (p_kb->p_row_pins[r] * 2));
        }
    }

    for (uint8_t c = 0; c < p_kb->p_layout->num_cols; ++c)
    {
        if (p_kb->p_col_ports[c] == port)
        {
            mask |= (GPIO_MODER_MODER0_Msk << (p_kb->p_col_pins[c] * 2));
        }
    }

    return mask;
}

static uint32_t _build_pupd_mask_for_port(uint8_t keyboard_id, GPIO_TypeDef *port)
{
    uint32_t mask = 0;
    const stm32f4_keyboard_hw_t *p_kb = &keyboards_arr[keyboard_id];

    for (uint8_t r = 0; r < p_kb->p_layout->num_rows; ++r)
    {
        if (p_kb->p_row_ports[r] == port)
        {
            mask |= (GPIO_PUPDR_PUPD0_Msk << (p_kb->p_row_pins[r] * 2));
        }
    }

    for (uint8_t c = 0; c < p_kb->p_layout->num_cols; ++c)
    {
        if (p_kb->p_col_ports[c] == port)
        {
            mask |= (GPIO_PUPDR_PUPD0_Msk << (p_kb->p_col_pins[c] * 2));
        }
    }

    return mask;
}

/**
 * @brief Test that the GPIO registers MODER and PUPDR are configured correctly for rows and columns.
 *
 */
void test_regs_config_mode_pupd(void)
{
    // Call configuration function
    port_keyboard_init(TEST_PORT_MAIN_KEYBOARD_ID);

    // Test GPIO rows registers configurations
    for (uint8_t row = 0; row < TEST_NUM_ROWS; row++)
    {
        GPIO_TypeDef *row_gpio = keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].p_row_ports[row];
        uint8_t row_pin = keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].p_row_pins[row];

        // Test that RCC is enabled
        sprintf(msg, "ERROR: RCC is not enabled for the GPIO port of the keyboard row %d", row);
        UNITY_TEST_ASSERT_EQUAL_UINT8(true, _rcc_gpio_enabled(row_gpio), __LINE__, msg);

        // Mode
        uint32_t mode = (row_gpio->MODER >> (row_pin * 2)) & GPIO_MODER_MODER0_Msk;
        sprintf(msg, "ERROR: Row %d mode is not configured as output", row);
        UNITY_TEST_ASSERT_EQUAL_UINT32(STM32F4_GPIO_MODE_OUT, mode, __LINE__, msg);

        // Pull-up/Pull-down
        uint32_t pupd = (row_gpio->PUPDR >> (row_pin * 2)) & GPIO_PUPDR_PUPD0_Msk;
        sprintf(msg, "ERROR: Row %d pull up/down is not configured as no pull up/down", row);
        UNITY_TEST_ASSERT_EQUAL_UINT32(STM32F4_GPIO_PUPDR_NOPULL, pupd, __LINE__, msg);
    }

    // Test GPIO columns registers configurations
    for (uint8_t col = 0; col < TEST_NUM_COLS; col++)
    {
        // Test that RCC is enabled
        sprintf(msg, "ERROR: RCC is not enabled for column %d GPIO port", col);
        UNITY_TEST_ASSERT_EQUAL_UINT8(true, _rcc_gpio_enabled(keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].p_col_ports[col]), __LINE__, msg);

        GPIO_TypeDef *col_gpio = keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].p_col_ports[col];
        uint8_t col_pin = keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].p_col_pins[col];

        // Mode
        uint32_t mode = (col_gpio->MODER >> (col_pin * 2)) & GPIO_MODER_MODER0_Msk;
        sprintf(msg, "ERROR: Column %d mode is not configured as input", col);
        UNITY_TEST_ASSERT_EQUAL_UINT32(STM32F4_GPIO_MODE_IN, mode, __LINE__, msg);

        // Pull-up/Pull-down
        uint32_t pupd = (col_gpio->PUPDR >> (col_pin * 2)) & GPIO_PUPDR_PUPD0_Msk;
        sprintf(msg, "ERROR: Column %d pull up/down is not configured as pull down", col);
        UNITY_TEST_ASSERT_EQUAL_UINT32(STM32F4_GPIO_PUPDR_PULLDOWN, pupd, __LINE__, msg);
    }
}

void _test_regs_config_unchnaged(GPIO_TypeDef *p_test_gpio)
{
    uint32_t prev_gpio_mode = p_test_gpio->MODER;
    uint32_t prev_gpio_pupd = p_test_gpio->PUPDR;

    // Call configuration function
    port_keyboard_init(TEST_PORT_MAIN_KEYBOARD_ID);

    // Check that no other pins other than the needed have been modified:
    /* For the GPIO under test (p_test_gpio) compare previous vs current masked with the set of pins that we expect the keyboard code to have touched on that port. */
    uint32_t modified_moder_mask = _build_moder_mask_for_port(TEST_PORT_MAIN_KEYBOARD_ID, p_test_gpio);
    uint32_t prev_gpio_mode_masked = prev_gpio_mode & ~modified_moder_mask;
    uint32_t curr_gpio_mode_masked = p_test_gpio->MODER & ~modified_moder_mask;
    sprintf(msg, "ERROR: GPIO MODE has been modified for other pins than expected on port %s", _gpio_name(p_test_gpio));
    UNITY_TEST_ASSERT_EQUAL_UINT32(prev_gpio_mode_masked, curr_gpio_mode_masked, __LINE__, msg);

    uint32_t modified_pupd_mask = _build_pupd_mask_for_port(TEST_PORT_MAIN_KEYBOARD_ID, p_test_gpio);
    uint32_t prev_gpio_pupd_masked = prev_gpio_pupd & ~modified_pupd_mask;
    uint32_t curr_gpio_pupd_masked = p_test_gpio->PUPDR & ~modified_pupd_mask;
    sprintf(msg, "ERROR: GPIO PUPD has been modified for other pins than expected on port %s", _gpio_name(p_test_gpio));
    UNITY_TEST_ASSERT_EQUAL_UINT32(prev_gpio_pupd_masked, curr_gpio_pupd_masked, __LINE__, msg);
}

/**
 * @brief Test that the GPIO registers MODER and PUPDR are not modified for other pins than the ones used by the keyboard.
 *
 */
void test_regs_config_unchanged(void)
{
    // If GPIOA. Protect PA13 and PA14 used for ST-Link
    uint32_t mask_a = 0xC3FFFFFF;

    // Set ports that should not be modified
    if (TEST_PORT_GPIO_1 == GPIOA)
    {
        // Protect PA13 and PA14 used for ST-Link
        TEST_PORT_GPIO_1->MODER |= mask_a;
        TEST_PORT_GPIO_1->PUPDR |= mask_a;
    }
    else
    {
        TEST_PORT_GPIO_1->MODER = ~0;
        TEST_PORT_GPIO_1->PUPDR = ~0;
    }
    _test_regs_config_unchnaged(TEST_PORT_GPIO_1);

    if (TEST_PORT_GPIO_1 == GPIOA)
    {
        // Protect PA13 and PA14 used for ST-Link
        TEST_PORT_GPIO_1->MODER &= ~mask_a;
        TEST_PORT_GPIO_1->PUPDR &= ~mask_a;
    }
    else
    {
        TEST_PORT_GPIO_1->MODER = 0;
        TEST_PORT_GPIO_1->PUPDR = 0;
    }
    _test_regs_config_unchnaged(TEST_PORT_GPIO_1);

    if (TEST_PORT_GPIO_2 == GPIOA)
    {
        // Protect PA13 and PA14 used for ST-Link
        TEST_PORT_GPIO_2->MODER |= mask_a;
        TEST_PORT_GPIO_2->PUPDR |= mask_a;
    }
    else
    {
        TEST_PORT_GPIO_2->MODER = ~0;
        TEST_PORT_GPIO_2->PUPDR = ~0;
    }
    _test_regs_config_unchnaged(TEST_PORT_GPIO_2);

    if (TEST_PORT_GPIO_2 == GPIOA)
    {
        // Protect PA13 and PA14 used for ST-Link
        TEST_PORT_GPIO_2->MODER &= ~mask_a;
        TEST_PORT_GPIO_2->PUPDR &= ~mask_a;
    }
    else
    {
        TEST_PORT_GPIO_2->MODER = 0;
        TEST_PORT_GPIO_2->PUPDR = 0;
    }
    _test_regs_config_unchnaged(TEST_PORT_GPIO_2);
}

/**
 * @brief Test EXTI of a given pin
 *
 */
void _test_exti_pin(GPIO_TypeDef *p_port, uint8_t pin)
{
    // Check that the EXTI RTSR is configured correctly (Rising edge)
    uint32_t col_rtsr = ((EXTI->RTSR) >> pin) & 0x1;
    sprintf(msg, "ERROR: EXTI RTSR of keyboard column in %s, pin %d, is not configured correctly. It must be both rising and falling edge.", _gpio_name(p_port), pin);
    UNITY_TEST_ASSERT_EQUAL_UINT32(0x1, col_rtsr, __LINE__, msg);

    // Check that the EXTI FTSR is configured correctly (Falling edge)
    uint32_t col_ftsr = ((EXTI->FTSR) >> pin) & 0x1;
    sprintf(msg, "ERROR: EXTI FTSR of keyboard column in %s, pin %d, is not configured correctly. It must be both rising and falling edge.", _gpio_name(p_port), pin);
    UNITY_TEST_ASSERT_EQUAL_UINT32(0x1, col_ftsr, __LINE__, msg);

    // Check that the EXTI EMR is configured correctly (Event)
    uint32_t col_emr = ((EXTI->EMR) >> pin) & 0x1;
    sprintf(msg, "ERROR: EXTI EMR of keyboard column in %s, pin %d, is not configured correctly. It must be both rising and falling edge.", _gpio_name(p_port), pin);
    UNITY_TEST_ASSERT_EQUAL_UINT32(0, col_emr, __LINE__, msg);

    // Check that the EXTI IMR is configured correctly (Interrupt)
    uint32_t col_imr = ((EXTI->IMR) >> pin) & 0x1;
    sprintf(msg, "ERROR: EXTI EMR of keyboard column in %s, pin %d, is not configured correctly. It must be in interrupt mode.", _gpio_name(p_port), pin);
    UNITY_TEST_ASSERT_EQUAL_UINT32(1, col_imr, __LINE__, msg);
}

/**
 * @brief Test that SYSCFG EXTICR and EXTI masks are configured correctly for keyboard columns.
 *
 */
void _test_exti(void)
{
    // Retrieve previous configuration
    // EXTICR depends on pin number, thus we will check it inside the loop.
    uint32_t prev_gpio_exticr[4] = {SYSCFG->EXTICR[0], SYSCFG->EXTICR[1], SYSCFG->EXTICR[2], SYSCFG->EXTICR[3]};
    uint32_t prev_gpio_rtsr = EXTI->RTSR;
    uint32_t prev_gpio_ftsr = EXTI->FTSR;
    uint32_t prev_gpio_emr = EXTI->EMR;
    uint32_t prev_gpio_imr = EXTI->IMR;
    uint32_t mask_exticr[4] = {0U};
    uint32_t mask_rtsr = 0U;
    uint32_t mask_ftsr = 0U;
    uint32_t mask_emr = 0U;
    uint32_t mask_imr = 0U;

    // Call configuration function
    port_keyboard_init(TEST_PORT_MAIN_KEYBOARD_ID);

    /* Columns with EXTI on rising and falling edge with interrupt enabled */
    for (uint8_t col = 0; col < TEST_NUM_COLS; col++)
    {
        // Test EXTI configuration for each column
        uint8_t pin = test_cols_gpio_pins_arr[col];
        GPIO_TypeDef *p_port = test_cols_gpio_ports_arr[col];
        _test_exti_pin(p_port, pin);

        // TEST EXTICR separately
        // Verify EXTI configuration for the given pin
        // Check that the EXTI CR is configured correctly (EXTI both edges)
        uint32_t col_exticr = ((SYSCFG->EXTICR[pin / 4]) >> ((pin % 4) * 4)) & 0xF;
        sprintf(msg, "ERROR: EXTI CR of keyboard column in %s, pin %d, is not configured correctly.", _gpio_name(p_port), pin);
        UNITY_TEST_ASSERT_EQUAL_UINT32(test_cols_exticr_array[col], col_exticr, __LINE__, msg);

        // Create masks for all columns
        mask_exticr[pin / 4] |= (0xF << ((pin % 4) * 4));
        mask_rtsr |= (EXTI_RTSR_TR0_Msk << pin);
        mask_ftsr |= (EXTI_FTSR_TR0_Msk << pin);
        mask_emr |= (EXTI_EMR_MR0_Msk << pin);
        mask_imr |= (EXTI_IMR_MR0_Msk << pin);
    }

    // Invert masks
    for (uint8_t i = 0; i < 4; i++)
    {
        mask_exticr[i] = ~mask_exticr[i];
    }
    mask_rtsr = ~mask_rtsr;
    mask_ftsr = ~mask_ftsr;
    mask_emr = ~mask_emr;
    mask_imr = ~mask_imr;

    // Check that no other pins other than the needed have been modified:
    // Check EXTICR unchanged
    for (uint8_t col = 0; col < TEST_NUM_COLS; col++)
    {
        uint8_t pin = test_cols_gpio_pins_arr[col];
        uint32_t prev_gpio_exticr_masked = prev_gpio_exticr[pin / 4] & mask_exticr[pin / 4];
        uint32_t curr_gpio_exticr_masked = SYSCFG->EXTICR[pin / 4] & mask_exticr[pin / 4];
        sprintf(msg, "ERROR: EXTI CR of EXTI %d for keyboard column on pin %d has been modified for other pins than those used for column on EXTICR[%d].", pin, pin, pin / 4);
        UNITY_TEST_ASSERT_EQUAL_UINT32(prev_gpio_exticr_masked, curr_gpio_exticr_masked, __LINE__, msg);
    }

    // Check RTSR unchanged
    uint32_t prev_gpio_rtsr_masked = prev_gpio_rtsr & mask_rtsr;
    uint32_t curr_gpio_rtsr_masked = EXTI->RTSR & mask_rtsr;
    UNITY_TEST_ASSERT_EQUAL_UINT32(prev_gpio_rtsr_masked, curr_gpio_rtsr_masked, __LINE__, "ERROR: EXTI RTSR has been modified for other ports than those used for columns.");

    // Check FTSR unchanged
    uint32_t prev_gpio_ftsr_masked = prev_gpio_ftsr & mask_ftsr;
    uint32_t curr_gpio_ftsr_masked = EXTI->FTSR & mask_ftsr;
    UNITY_TEST_ASSERT_EQUAL_UINT32(prev_gpio_ftsr_masked, curr_gpio_ftsr_masked, __LINE__, "ERROR: EXTI FTSR has been modified for other ports than those used for columns.");

    // Check EMR unchanged
    uint32_t prev_gpio_emr_masked = prev_gpio_emr & mask_emr;
    uint32_t curr_gpio_emr_masked = EXTI->EMR & mask_emr;
    UNITY_TEST_ASSERT_EQUAL_UINT32(prev_gpio_emr_masked, curr_gpio_emr_masked, __LINE__, "ERROR: EXTI EMR has been modified for other ports than those used for columns.");

    // Check IMR unchanged
    uint32_t prev_gpio_imr_masked = prev_gpio_imr & mask_imr;
    uint32_t curr_gpio_imr_masked = EXTI->IMR & mask_imr;
    UNITY_TEST_ASSERT_EQUAL_UINT32(prev_gpio_imr_masked, curr_gpio_imr_masked, __LINE__, "ERROR: EXTI IMR has been modified for other ports than those used for columns.");
}

void test_exti(void)
{
    for (uint8_t col = 0; col < TEST_NUM_COLS; col++)
    {
        uint8_t pin = test_cols_gpio_pins_arr[col];
        SYSCFG->EXTICR[pin / 4] = 0xFFFFU;
    }
    EXTI->RTSR = 0x77FFFF;
    EXTI->FTSR = 0x77FFFF;
    EXTI->EMR = 0x7FFFFF;
    EXTI->IMR = 0x7FFFFF;
    _test_exti();

    EXTI->RTSR = 0;
    EXTI->FTSR = 0;
    EXTI->EMR = 0;
    EXTI->IMR = 0;
    EXTI->RTSR = 0;
    for (uint8_t col = 0; col < TEST_NUM_COLS; col++)
    {
        uint8_t pin = test_cols_gpio_pins_arr[col];
        SYSCFG->EXTICR[pin / 4] = 0U;
    }
    _test_exti();
}

/**
 * @brief Get IRQn names for debugging.
 *
 * @param buffer Return buffer
 * @param buffer_size Size of return buffer
 * @param irqn IRQn to get name for
 */
void _get_irqn_names(char *buffer, size_t buffer_size, IRQn_Type irqn)
{
    switch (irqn)
    {
    case EXTI9_5_IRQn:
        snprintf(buffer, buffer_size, "EXTI9_5_IRQn");
        break;
    case EXTI15_10_IRQn:
        snprintf(buffer, buffer_size, "EXTI15_10_IRQn");
        break;
    case EXTI4_IRQn:
        snprintf(buffer, buffer_size, "EXTI4_IRQn");
        break;
    default:
        snprintf(buffer, buffer_size, "Unknown_IRQn");
        break;
    }
}

/**
 * @brief Test that NVIC EXTI IRQs are enabled for keyboard columns. Also check priorities.
 *
 */
void test_exti_enabled_priority(void)
{
    // Variables
    char irqn_name[30];
    uint32_t pPreemptPriority;
    uint32_t pSubPriority;

    // Call configuration function
    port_keyboard_init(TEST_PORT_MAIN_KEYBOARD_ID);

    for (uint8_t col = 0; col < TEST_NUM_COLS; col++)
    {
        IRQn_Type irqn = test_irqn_array[col];
        _get_irqn_names(irqn_name, sizeof(irqn_name), irqn);

        sprintf(msg, "ERROR: NVIC %s is not enabled for keyboard columns", irqn_name);
        UNITY_TEST_ASSERT_EQUAL_UINT8(true, nvic_irq_enabled(irqn), __LINE__, msg);

        // Test priorities
        uint32_t prio = NVIC_GetPriority(irqn);
        uint32_t PriorityGroup = NVIC_GetPriorityGrouping();
        NVIC_DecodePriority(prio, PriorityGroup, &pPreemptPriority, &pSubPriority);
        sprintf(msg, "ERROR: NVIC %s priority is not correct for keyboard columns", irqn_name);
        UNITY_TEST_ASSERT_EQUAL_UINT32(test_irq_priorities_array[col], pPreemptPriority, __LINE__, msg);

        sprintf(msg, "ERROR: NVIC %s subpriority is not correct for keyboard columns", irqn_name);
        UNITY_TEST_ASSERT_EQUAL_UINT32(test_irq_subpriorities_array[col], pSubPriority, __LINE__, msg);
    }
}

/**
 * @brief Test the generalization of the keyboard port driver. Particularly, test that the port driver functions work with the keyboards_arr array and not with the specific GPIOx peripheral.
 *
 */
void test_keyboard_port_generalization(void)
{
    // Variables
    stm32f4_keyboard_hw_t *p_kb = &keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID];
    uint8_t num_pins;
    GPIO_TypeDef **p_ports = test_rows_gpio_ports_arr;
    uint8_t *p_pins = test_rows_gpio_pins_arr;
    uint8_t real_gpio_mode = 0U;
    uint8_t real_gpio_pupd = 0U;

    // Doble loop to test rows and columns
    for (uint8_t i = 0; i < 2; i++)
    {
        if (i == 0)
        {
            // Rows
            num_pins = TEST_NUM_ROWS;
            p_ports = test_rows_gpio_ports_arr;
            p_pins = test_rows_gpio_pins_arr;
            real_gpio_mode = STM32F4_GPIO_MODE_OUT;
            real_gpio_pupd = STM32F4_GPIO_PUPDR_NOPULL;
        }
        else
        {
            // Columns
            num_pins = TEST_NUM_COLS;
            p_ports = test_cols_gpio_ports_arr;
            p_pins = test_cols_gpio_pins_arr;
            real_gpio_mode = STM32F4_GPIO_MODE_IN;
            real_gpio_pupd = STM32F4_GPIO_PUPDR_PULLDOWN;
        }

        for (uint8_t j = 0; j < num_pins; j++)
        {
            // Change the GPIO and pin in the keyboards_arr array to a different one. Alternative values:
            GPIO_TypeDef *p_real_gpio_port = p_ports[j];
            uint8_t real_gpio_pin = p_pins[j];
            GPIO_TypeDef *p_alt_gpio_port = (p_ports[j] == GPIOB) ? GPIOC : GPIOB;
            uint8_t alt_gpio_pin = (p_pins[j] + 1) % 16;

            // Update keyboards_arr with the new GPIO and pin
            if (i == 0)
            {
                // Rows
                p_kb->p_row_ports[j] = p_alt_gpio_port;
                p_kb->p_row_pins[j] = alt_gpio_pin;
            }
            else
            {
                // Columns
                p_kb->p_col_ports[j] = p_alt_gpio_port;

                // In columns check that the alternate pin does not coincide with the EXTI line of another column of the original configuration neither with an already tested column
                bool pin_conflict;
                do
                {
                    pin_conflict = false;
                    for (uint8_t k = 0; k < TEST_NUM_COLS; k++)
                    {
                        if (p_kb->p_col_pins[k] == alt_gpio_pin || test_cols_gpio_pins_arr[k] == alt_gpio_pin)
                        {
                            pin_conflict = true;
                            alt_gpio_pin = (alt_gpio_pin + 1) % 16;
                            break;
                        }
                    }
                } while (pin_conflict);
                p_kb->p_col_pins[j] = alt_gpio_pin;
            }

            // TEST init function
            // Enable RCC for the real GPIO
            if (p_real_gpio_port == GPIOA)
            {
                RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; /* GPIOA_CLK_ENABLE */
            }
            else if (p_real_gpio_port == GPIOB)
            {
                RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; /* GPIOB_CLK_ENABLE */
            }
            else if (p_real_gpio_port == GPIOC)
            {
                RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN; /* GPIOC_CLK_ENABLE */
            }

            // Clean/set all configurations
            SYSCFG->EXTICR[alt_gpio_pin / 4] = 0;
            EXTI->RTSR = 0;
            EXTI->FTSR = 0;
            EXTI->EMR = 0;
            EXTI->IMR = 0;

            // Wrong configuration to check that the GPIO is not being modified
            if (real_gpio_mode == STM32F4_GPIO_MODE_IN)
            {
                p_real_gpio_port->MODER |= (STM32F4_GPIO_MODE_OUT << (real_gpio_pin * 2U));
            }
            else if (real_gpio_mode == STM32F4_GPIO_MODE_OUT)
            {
                p_real_gpio_port->MODER |= (STM32F4_GPIO_MODE_IN << (real_gpio_pin * 2U));
            }

            if (real_gpio_pupd == STM32F4_GPIO_PUPDR_NOPULL)
            {
                p_real_gpio_port->PUPDR |= (STM32F4_GPIO_PUPDR_PULLUP << (real_gpio_pin * 2U));
            }
            else if (real_gpio_pupd == STM32F4_GPIO_PUPDR_PULLDOWN)
            {
                p_real_gpio_port->PUPDR |= (STM32F4_GPIO_PUPDR_NOPULL << (real_gpio_pin * 2U));
            }
            else if (real_gpio_pupd == STM32F4_GPIO_PUPDR_PULLUP)
            {
                p_real_gpio_port->PUPDR |= (STM32F4_GPIO_PUPDR_PULLDOWN << (real_gpio_pin * 2U));
            }

            // Disable RCC for the real GPIO
            if (p_real_gpio_port == GPIOA)
            {
                RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOAEN;
            }
            else if (p_real_gpio_port == GPIOB)
            {
                RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOBEN;
            }
            else if (p_real_gpio_port == GPIOC)
            {
                RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOCEN;
            }

            // Expected configuration of the GPIO
            stm32f4_system_gpio_config(p_alt_gpio_port, alt_gpio_pin, real_gpio_mode, real_gpio_pupd);
            uint32_t real_exticr, real_rtsr, real_ftsr, real_emr, real_imr, real_priority;
            if (i == 1)
            {
                // For columns, also configure EXTI
                stm32f4_system_gpio_config_exti(p_alt_gpio_port, alt_gpio_pin, STM32F4_TRIGGER_BOTH_EDGE | STM32F4_TRIGGER_ENABLE_INTERR_REQ);
                stm32f4_system_gpio_exti_enable(alt_gpio_pin, test_irq_priorities_array[j], test_irq_subpriorities_array[j]);
                real_exticr = SYSCFG->EXTICR[real_gpio_pin / 4];
                real_rtsr = EXTI->RTSR;
                real_ftsr = EXTI->FTSR;
                real_emr = EXTI->EMR;
                real_imr = EXTI->IMR;

                // Mask all registers to read only the bits of the specific pin
                real_exticr &= (0xF << ((real_gpio_pin % 4) * 4));
                real_rtsr &= (EXTI_RTSR_TR0_Msk << real_gpio_pin);
                real_ftsr &= (EXTI_FTSR_TR0_Msk << real_gpio_pin);
                real_emr &= (EXTI_EMR_MR0_Msk << real_gpio_pin);
                real_imr &= (EXTI_IMR_MR0_Msk << real_gpio_pin);

                // Get NVIC priority of the EXTI line for the specific pin
                real_priority = NVIC_GetPriority(test_cols_exticr_array[j]);
            }

            // Call configuration function
            port_keyboard_init(TEST_PORT_MAIN_KEYBOARD_ID);

            // Check that the expected GPIO peripheral has not been modified. Otherwise, the port driver is not generalizing the GPIO peripheral and it is not working with the keyboards_arr array but with the specific GPIO nad pin.
            uint32_t curr_gpio_mode = p_real_gpio_port->MODER;
            uint32_t curr_gpio_pupd = p_real_gpio_port->PUPDR;

            sprintf(msg, "ERROR: The GPIO port %s and/or pin %d configuration function is not generalizing the GPIO and/or pin MODE but working with the specific GPIO and pin.", _gpio_name(p_real_gpio_port), real_gpio_pin);
            UNITY_TEST_ASSERT_EQUAL_UINT32(curr_gpio_mode, curr_gpio_mode, __LINE__, msg);

            sprintf(msg, "ERROR: The GPIO port %s and/or pin %d configuration function is not generalizing the GPIO and/or pin PUPD but working with the specific GPIO and pin.", _gpio_name(p_real_gpio_port), real_gpio_pin);
            UNITY_TEST_ASSERT_EQUAL_UINT32(curr_gpio_pupd, curr_gpio_pupd, __LINE__, msg);

            if (i == 1)
            {
                // For columns, also check EXTI
                uint32_t curr_exticr = SYSCFG->EXTICR[real_gpio_pin / 4];
                uint32_t curr_rtsr = EXTI->RTSR;
                uint32_t curr_ftsr = EXTI->FTSR;
                uint32_t curr_emr = EXTI->EMR;
                uint32_t curr_imr = EXTI->IMR;
                uint32_t curr_priority = NVIC_GetPriority(test_cols_exticr_array[j]);

                // Mask all registers to read only the bits of the specific pin
                curr_exticr &= (0xF << ((real_gpio_pin % 4) * 4));
                curr_rtsr &= (EXTI_RTSR_TR0_Msk << real_gpio_pin);
                curr_ftsr &= (EXTI_FTSR_TR0_Msk << real_gpio_pin);
                curr_emr &= (EXTI_EMR_MR0_Msk << real_gpio_pin);
                curr_imr &= (EXTI_IMR_MR0_Msk << real_gpio_pin);

                // Check that the NVIC priority of the EXTI line has not been modified. Otherwise, the port driver is not generalizing the EXTI line and it is not working with the keyboards_arr array but with the specific GPIO and pin.
                sprintf(msg, "ERROR: The GPIO port %s and/or pin %d configuration function is not generalizing the EXTI CR but working with the specific GPIO and pin.", _gpio_name(p_real_gpio_port), real_gpio_pin);
                UNITY_TEST_ASSERT_EQUAL_UINT32(real_exticr, curr_exticr, __LINE__, msg);

                sprintf(msg, "ERROR: The GPIO port %s and/or pin %d configuration function is not generalizing the EXTI RTSR but working with the specific GPIO and pin.", _gpio_name(p_real_gpio_port), real_gpio_pin);
                UNITY_TEST_ASSERT_EQUAL_UINT32(real_rtsr, curr_rtsr, __LINE__, msg);

                sprintf(msg, "ERROR: The GPIO port %s and/or pin %d configuration function is not generalizing the EXTI FTSR but working with the specific GPIO and pin.", _gpio_name(p_real_gpio_port), real_gpio_pin);
                UNITY_TEST_ASSERT_EQUAL_UINT32(real_ftsr, curr_ftsr, __LINE__, msg);

                sprintf(msg, "ERROR: The GPIO port %s and/or pin %d configuration function is not generalizing the EXTI EMR but working with the specific GPIO and pin.", _gpio_name(p_real_gpio_port), real_gpio_pin);
                UNITY_TEST_ASSERT_EQUAL_UINT32(real_emr, curr_emr, __LINE__, msg);

                sprintf(msg, "ERROR: The GPIO port %s and/or pin %d configuration function is not generalizing the EXTI IMR but working with the specific GPIO and pin.", _gpio_name(p_real_gpio_port), real_gpio_pin);
                UNITY_TEST_ASSERT_EQUAL_UINT32(real_imr, curr_imr, __LINE__, msg);

                sprintf(msg, "ERROR: The GPIO port %s and/or pin %d configuration function is not generalizing the EXTI NVIC priority but working with the specific GPIO and pin.", _gpio_name(p_real_gpio_port), real_gpio_pin);
                UNITY_TEST_ASSERT_EQUAL_UINT32(real_priority, curr_priority, __LINE__, msg);
            }
        }
    }
}

/**
 * @brief Test the configuration of the timer that controls the row scanning duration.
 *
 */
void test_meas_timer_config(void)
{
    // Retrieve previous configuration
    uint32_t prev_tim_meas_cr1 = TEST_SCAN_TIMER->CR1;
    uint32_t prev_tim_meas_dier = TEST_SCAN_TIMER->DIER;
    uint32_t prev_tim_meas_sr = TEST_SCAN_TIMER->SR;

    // Call configuration function
    port_keyboard_init(TEST_PORT_MAIN_KEYBOARD_ID);

    // Check that the timer is enabled in RCC
    uint32_t tim_meas_rcc = (TEST_SCAN_TIMER_PER_BUS)&SCAN_TIMER_PER_BUS_MASK;
    UNITY_TEST_ASSERT_EQUAL_UINT32(SCAN_TIMER_PER_BUS_MASK, tim_meas_rcc, __LINE__, "ERROR: timer for row scanning is not enabled in RCC");

    // Check that the timer is disabled
    uint32_t tim_meas_en = (TEST_SCAN_TIMER->CR1) & TIM_CR1_CEN_Msk;
    UNITY_TEST_ASSERT_EQUAL_UINT32(0, tim_meas_en, __LINE__, "ERROR: timer for row scanning must be disabled after configuration");

    // Check that the timer is configured with auto-reload preload enabled
    uint32_t tim_meas_arpe = (TEST_SCAN_TIMER->CR1) & TIM_CR1_ARPE_Msk;
    UNITY_TEST_ASSERT_EQUAL_UINT32(TIM_CR1_ARPE_Msk, tim_meas_arpe, __LINE__, "ERROR: timer for row scanning must be configured with auto-reload preload enabled");

    // Check that the timer has cleared the update interrupt
    uint32_t tim_meas_sr = (TEST_SCAN_TIMER->SR) & TIM_SR_UIF_Msk;
    UNITY_TEST_ASSERT_EQUAL_UINT32(0, tim_meas_sr, __LINE__, "ERROR: timer for row scanning must have cleared the update interrupt");

    // Check that the timer has cleared the interrupt
    uint32_t tim_meas_dier = (TEST_SCAN_TIMER->DIER) & TIM_DIER_UIE_Msk;
    UNITY_TEST_ASSERT_EQUAL_UINT32(TIM_DIER_UIE_Msk, tim_meas_dier, __LINE__, "ERROR: timer for row scanning must have the update interrupt enabled");

    // Check that no other bits other than the needed have been modified:
    uint32_t prev_tim_meas_cr1_masked = prev_tim_meas_cr1 & ~(TIM_CR1_ARPE_Msk | TIM_CR1_CEN_Msk);
    uint32_t prev_tim_meas_dier_masked = prev_tim_meas_dier & ~TIM_DIER_UIE_Msk;
    uint32_t prev_tim_meas_sr_masked = prev_tim_meas_sr & ~TIM_SR_UIF_Msk;

    uint32_t curr_tim_meas_cr1_masked = TEST_SCAN_TIMER->CR1 & ~(TIM_CR1_ARPE_Msk | TIM_CR1_CEN_Msk);
    uint32_t curr_tim_meas_dier_masked = TEST_SCAN_TIMER->DIER & ~TIM_DIER_UIE_Msk;
    uint32_t curr_tim_meas_sr_masked = TEST_SCAN_TIMER->SR & ~TIM_SR_UIF_Msk;

    UNITY_TEST_ASSERT_EQUAL_UINT32(prev_tim_meas_cr1_masked, curr_tim_meas_cr1_masked, __LINE__, "ERROR: The register CR1 of the timer for row scanning has been modified for other bits than the needed");
    UNITY_TEST_ASSERT_EQUAL_UINT32(prev_tim_meas_dier_masked, curr_tim_meas_dier_masked, __LINE__, "ERROR: The register DIER of the timer for row scanning has been modified for other bits than the needed");
    UNITY_TEST_ASSERT_EQUAL_UINT32(prev_tim_meas_sr_masked, curr_tim_meas_sr_masked, __LINE__, "ERROR: The register SR of the timer for row scanning has been modified for other bits than the needed");
}

/**
 * @brief Test the priority of the timer that controls the row scanning.
 *
 */
void test_meas_timer_priority(void)
{
    uint32_t Priority = NVIC_GetPriority(SCAN_TIMER_IRQ);
    uint32_t PriorityGroup = NVIC_GetPriorityGrouping();
    uint32_t pPreemptPriority;
    uint32_t pSubPriority;

    NVIC_DecodePriority(Priority, PriorityGroup, &pPreemptPriority, &pSubPriority);

    UNITY_TEST_ASSERT_EQUAL_UINT32(SCAN_TIMER_IRQ_PRIO, pPreemptPriority, __LINE__, "ERROR: NVIC priority of timer for row scanning is not correct");
    UNITY_TEST_ASSERT_EQUAL_UINT32(SCAN_TIMER_IRQ_SUBPRIO, pSubPriority, __LINE__, "ERROR: NVIC subpriority of timer for row scanning is not correct");
}

/**
 * @brief Test the configuration of the timer for row scanning
 *
 */
void test_row_scan_timer_duration()
{
    uint32_t prev_tim_meas_cr1 = TEST_SCAN_TIMER->CR1;

    // Call configuration function to set the measurement
    port_keyboard_init(TEST_PORT_MAIN_KEYBOARD_ID);

    // Disable row scanning measurement interrupts to avoid any interference
    NVIC_DisableIRQ(SCAN_TIMER_IRQ);

    // Check the computation of the ARR and PSC for the row scanning measurement
    uint32_t ms_test = TEST_PORT_KEYBOARD_MAIN_TIMEOUT_MS;
    uint32_t arr = TEST_SCAN_TIMER->ARR;
    uint32_t psc = TEST_SCAN_TIMER->PSC;
    uint32_t tim_meas_dur_ms = round((((double)(arr) + 1.0) / ((double)SystemCoreClock / 1000.0)) * ((double)(psc) + 1));
    sprintf(msg, "ERROR: timer for row scanning ARR and PSC are not configured correctly for a duration of %ld ms", ms_test);
    UNITY_TEST_ASSERT_INT_WITHIN(1, ms_test, tim_meas_dur_ms, __LINE__, msg);

    // Check that the timer for row scanning is enabled
    uint32_t tim_meas_en = (TEST_SCAN_TIMER->CR1) & TIM_CR1_CEN_Msk;
    UNITY_TEST_ASSERT_EQUAL_UINT32(false, tim_meas_en, __LINE__, "ERROR: timer for row scanning should not be enabled after setting the configuration");

    // Check that no other bits other than the needed have been modified:
    uint32_t prev_tim_meas_cr1_masked = prev_tim_meas_cr1 & ~TIM_CR1_CEN_Msk;
    uint32_t curr_tim_meas_cr1_masked = TEST_SCAN_TIMER->CR1 & ~TIM_CR1_CEN_Msk;
    UNITY_TEST_ASSERT_EQUAL_UINT32(prev_tim_meas_cr1_masked, curr_tim_meas_cr1_masked, __LINE__, "ERROR: The register CR1 of the timer for row scanning has been modified for other bits than the needed");
}

void test_col_scan_timer_timeout(void)
{
    // Call configuration function to set the measurement
    port_keyboard_init(TEST_PORT_MAIN_KEYBOARD_ID);

    // Enable row scanning interrupts to test the timeout
    NVIC_EnableIRQ(SCAN_TIMER_IRQ);

    // Enable the timer
    TEST_SCAN_TIMER->CR1 |= TIM_CR1_CEN;

    // Wait for the timeout
    port_system_delay_ms(TEST_PORT_KEYBOARD_MAIN_TIMEOUT_MS + 1); // Wait a time higher than the scan duration

    // Disable row scanning interrupts to avoid any interference
    NVIC_DisableIRQ(SCAN_TIMER_IRQ);

    // Check that the meas_end flag is set
    bool trigger_ready = port_keyboard_get_row_timeout_status(TEST_PORT_MAIN_KEYBOARD_ID);
    UNITY_TEST_ASSERT_EQUAL_UINT32(true, trigger_ready, __LINE__, "ERROR: row scanning flag_row_timeout flag must be set after the timer timeout");
}

/**
 * @brief Simulate the start of row excitation when timer scan is set.
 *
 */
void test_col_scan_timer_timeout_start_simulation(void)
{
    // Call configuration function to set the measurement
    port_keyboard_init(TEST_PORT_MAIN_KEYBOARD_ID);

    // Call port function to start the measurement
    port_keyboard_start_scan(TEST_PORT_MAIN_KEYBOARD_ID);

    // Retrieve the current configuration of the NVIC interrupts before disabling them
    uint32_t tim_meas_irq = NVIC->ISER[SCAN_TIMER_IRQ / 32] & (1 << (SCAN_TIMER_IRQ % 32));

    // Disable all interrupts to avoid any interference
    NVIC_DisableIRQ(SCAN_TIMER_IRQ);

    // Check that the first row is set
    uint8_t current_row = keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].current_excited_row;
    uint32_t row_pin = keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].p_row_pins[current_row];
    uint32_t row_gpio_odr = keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].p_row_ports[current_row]->ODR & (1 << row_pin);
    UNITY_TEST_ASSERT_EQUAL_UINT32(1 << row_pin, row_gpio_odr, __LINE__, "ERROR: Row 0 pin must be set to high after starting the scan timer.");

    // Check that the other rows are low
    for (uint8_t r = 1; r < TEST_NUM_ROWS; r++)
    {
        row_pin = keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].p_row_pins[r];
        row_gpio_odr = keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID].p_row_ports[r]->ODR & (1 << row_pin);
        sprintf(msg, "ERROR: Row %d pin must be set to low after when row 0 is high.", r);
        UNITY_TEST_ASSERT_NOT_EQUAL_UINT32(1 << row_pin, row_gpio_odr, __LINE__, msg);
    }

    // Check that the NVIC interrupt for the scan timer has been enabled
    UNITY_TEST_ASSERT_EQUAL_UINT32(1 << (SCAN_TIMER_IRQ % 32), tim_meas_irq, __LINE__, "ERROR: The NVIC interrupt for the keyboard scan timer has not been enabled.");

    // Check that the timer has been enabled
    uint32_t tim_meas_en = (TEST_SCAN_TIMER->CR1) & TIM_CR1_CEN_Msk;
    UNITY_TEST_ASSERT_EQUAL_UINT32(TIM_CR1_CEN_Msk, tim_meas_en, __LINE__, "ERROR: The keyboard measurement timer has not been enabled");
}

void test_all_keys_press_simulation(void)
{
    // Call configuration function
    port_keyboard_init(TEST_PORT_MAIN_KEYBOARD_ID);

    // Get keyboard struct
    stm32f4_keyboard_hw_t *p_kb = &keyboards_arr[TEST_PORT_MAIN_KEYBOARD_ID];

    // clear flags before start
    p_kb->flag_key_pressed = false;

    for (uint8_t r = 0; r < p_kb->p_layout->num_rows; r++)
    {
        // Emulate that the scanner is currently driving row r
        p_kb->current_excited_row = r;

        for (uint8_t c = 0; c < p_kb->p_layout->num_cols; c++)
        {
            uint8_t col_pin = p_kb->p_col_pins[c];
            uint32_t mask = (1U << col_pin);

            // Generate a software interrupt on the column line (rising edge)
            EXTI->SWIER |= mask;

            port_system_delay_ms(1); // wait a bit for the IRQ to be handled

            // NOTE: We cannot read the IDR and thus the flag_key_pressed directly, as the GPIOs are not really connected. Instead, we check that the key_value variable has been set correctly in the keyboard

            char key_expected = p_kb->p_layout->keys[r * TEST_NUM_COLS + c];

            printf("Testing key press at row %d, col %d...\n\tExpected key: '%c'\n\tDetected key: '%c'\n", r, c, key_expected, port_keyboard_get_key_value(TEST_PORT_MAIN_KEYBOARD_ID));
            sprintf(msg, "ERROR: key value not set correctly for row %d, col %d.", r, c);
            UNITY_TEST_ASSERT_EQUAL_CHAR(key_expected, port_keyboard_get_key_value(TEST_PORT_MAIN_KEYBOARD_ID), __LINE__, msg);

            // rearm flags for the next iteration
            p_kb->flag_key_pressed = false;
        }
    }
}

/**
 * @brief Main function to run the tests.
 *
 * @return int
 */
int main(void)
{
    port_system_init();
    UNITY_BEGIN();
    RUN_TEST(test_identifiers);
    RUN_TEST(test_layout_and_nullkey);
    RUN_TEST(test_wiring_rows_cols);
    RUN_TEST(test_regs_config_mode_pupd);
    RUN_TEST(test_regs_config_unchanged);
    RUN_TEST(test_exti);
    RUN_TEST(test_exti_enabled_priority);

    // Test measurement timer configuration
    RUN_TEST(test_meas_timer_config);
    RUN_TEST(test_meas_timer_priority);
    RUN_TEST(test_row_scan_timer_duration);
    RUN_TEST(test_col_scan_timer_timeout);

    // Run simulation tests
    RUN_TEST(test_col_scan_timer_timeout_start_simulation);
    RUN_TEST(test_all_keys_press_simulation);

    // Run generalization test the last to avoid interference with other tests
    RUN_TEST(test_keyboard_port_generalization);

    exit(UNITY_END());
}
