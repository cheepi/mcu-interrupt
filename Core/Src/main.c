#include "main.h"

#define BTN_EXTI_PIN  GPIO_PIN_0   // PA0: tombol interrupt langsung ke default
#define BTN_EXTI_PORT GPIOA

#define BTN_LOOP_PIN  GPIO_PIN_1   // PA1: tombol loop biasa
#define BTN_LOOP_PORT GPIOA

#define LED1_PIN   GPIO_PIN_5   // PA5
#define LED2_PIN   GPIO_PIN_6   // PA6
#define LED3_PIN   GPIO_PIN_13  // PC13 (active-low)

typedef enum { PHASE_ON, PHASE_OFF } Phase;

// state untuk pattern 2
static Phase    led1_phase = PHASE_ON;
static Phase    led2_phase = PHASE_ON;
static Phase    led3_phase = PHASE_ON;
static uint32_t t_window, t_a5, t_a6, t_blink;

volatile uint8_t pattern = 0;  // 0 - 3

// PA0 EXTI: langsung reset ke default (pattern 0)
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == BTN_EXTI_PIN) {
        pattern = 0;
    }
}

void SystemClock_Config(void);
static void MX_GPIO_Init(void);

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    uint8_t  prev_pattern = 0xFF;
    uint32_t t_sync        = HAL_GetTick();
    uint32_t t_alt         = HAL_GetTick();
    uint8_t  step_alt      = 0;
    uint32_t last_loop_btn = 0;

    while (1) {
        uint32_t now = HAL_GetTick();

        // tombol muter (PA1)
        if (HAL_GPIO_ReadPin(BTN_LOOP_PORT, BTN_LOOP_PIN) == GPIO_PIN_SET) {
            if (now - last_loop_btn > 200) {
                pattern = (pattern + 1) & 3;
                last_loop_btn = now;
            }
        }

        // kalau ganti pattern, reset timer & initial state
        if (pattern != prev_pattern) {
            t_sync   = now;
            t_alt    = now;
            step_alt = 0;

            if (pattern == 2) {
                // reset semua fase & timer pattern 2
                led1_phase = PHASE_ON;
                led2_phase = PHASE_ON;
                led3_phase = PHASE_ON;
                t_window   = now;
                t_a5       = now;
                t_a6       = now;
                t_blink    = now;
                HAL_GPIO_WritePin(GPIOA, LED1_PIN|LED2_PIN, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(GPIOC, LED3_PIN, GPIO_PIN_RESET);
            }

            // initial state per pattern lain
            switch (pattern) {
                case 0:
                    // nothing here, handled below
                    break;
                case 1:
                    HAL_GPIO_WritePin(GPIOA, LED1_PIN|LED2_PIN, GPIO_PIN_RESET);
                    HAL_GPIO_WritePin(GPIOC, LED3_PIN, GPIO_PIN_SET);
                    break;
                case 3:
                    HAL_GPIO_WritePin(GPIOA, LED1_PIN|LED2_PIN, GPIO_PIN_RESET);
                    HAL_GPIO_WritePin(GPIOC, LED3_PIN, GPIO_PIN_SET);
                    HAL_GPIO_WritePin(GPIOA, LED1_PIN, GPIO_PIN_SET);
                    break;
            }
            prev_pattern = pattern;
        }

        switch (pattern) {
            case 0: {
                // default circular pairs 0→1→2→0 tiap 500ms
                static uint8_t phase = 0;
                static uint32_t t0  = 0;
                if (now - t0 >= 1000) {
                    t0 = now;
                    phase = (phase + 1) % 3;
                }
                if (phase == 0) {
                    // PA5+PA6 ON, PC13 OFF
                    HAL_GPIO_WritePin(GPIOA, LED1_PIN|LED2_PIN, GPIO_PIN_SET);
                    HAL_GPIO_WritePin(GPIOC, LED3_PIN, GPIO_PIN_SET);
                } else if (phase == 1) {
                    // PA6+PC13 ON, PA5 OFF
                    HAL_GPIO_WritePin(GPIOA, LED1_PIN, GPIO_PIN_RESET);
                    HAL_GPIO_WritePin(GPIOA, LED2_PIN, GPIO_PIN_SET);
                    HAL_GPIO_WritePin(GPIOC, LED3_PIN, GPIO_PIN_RESET);
                } else {
                    // PC13+PA5 ON, PA6 OFF
                    HAL_GPIO_WritePin(GPIOA, LED1_PIN, GPIO_PIN_SET);
                    HAL_GPIO_WritePin(GPIOA, LED2_PIN, GPIO_PIN_RESET);
                    HAL_GPIO_WritePin(GPIOC, LED3_PIN, GPIO_PIN_RESET);
                }
                break;
            }

            case 1:
                // sync blink 500ms
                if (now - t_sync >= 500) {
                    t_sync = now;
                    HAL_GPIO_TogglePin(GPIOA, LED1_PIN|LED2_PIN);
                    HAL_GPIO_TogglePin(GPIOC, LED3_PIN);
                }
                break;

            case 2:
                // ratio 3:2:1 over 1200ms
                if (now - t_window >= 1200) {
                    t_window   = now;
                    t_a5       = now;
                    t_a6       = now;
                    t_blink    = now;
                    led1_phase = PHASE_ON;
                    led2_phase = PHASE_ON;
                    led3_phase = PHASE_ON;
                    HAL_GPIO_WritePin(GPIOA, LED1_PIN|LED2_PIN, GPIO_PIN_RESET);
                    HAL_GPIO_WritePin(GPIOC, LED3_PIN, GPIO_PIN_RESET);
                }
                // LED3: ON1100/OFF100
                if (led3_phase == PHASE_ON && now - t_blink >= 1100) {
                    HAL_GPIO_WritePin(GPIOC, LED3_PIN, GPIO_PIN_SET);
                    led3_phase = PHASE_OFF;
                    t_blink    = now;
                } else if (led3_phase == PHASE_OFF && now - t_blink >= 100) {
                    HAL_GPIO_WritePin(GPIOC, LED3_PIN, GPIO_PIN_RESET);
                    led3_phase = PHASE_ON;
                    t_blink    = now;
                }
                // LED2: ON300/OFF100
                if (led2_phase == PHASE_ON && now - t_a6 >= 300) {
                    HAL_GPIO_WritePin(GPIOA, LED2_PIN, GPIO_PIN_SET);
                    led2_phase = PHASE_OFF;
                    t_a6       = now;
                } else if (led2_phase == PHASE_OFF && now - t_a6 >= 100) {
                    HAL_GPIO_WritePin(GPIOA, LED2_PIN, GPIO_PIN_RESET);
                    led2_phase = PHASE_ON;
                    t_a6       = now;
                }
                // LED1: ON500/OFF100
                if (led1_phase == PHASE_ON && now - t_a5 >= 500) {
                    HAL_GPIO_WritePin(GPIOA, LED1_PIN, GPIO_PIN_SET);
                    led1_phase = PHASE_OFF;
                    t_a5       = now;
                } else if (led1_phase == PHASE_OFF && now - t_a5 >= 100) {
                    HAL_GPIO_WritePin(GPIOA, LED1_PIN, GPIO_PIN_RESET);
                    led1_phase = PHASE_ON;
                    t_a5       = now;
                }
                break;

            case 3:
                // alternate blink 500ms
                if (now - t_alt >= 500) {
                    t_alt = now;
                    step_alt = (step_alt + 1) % 3;
                    HAL_GPIO_WritePin(GPIOA, LED1_PIN|LED2_PIN, GPIO_PIN_RESET);
                    HAL_GPIO_WritePin(GPIOC, LED3_PIN, GPIO_PIN_SET);
                    if (step_alt == 0)
                        HAL_GPIO_WritePin(GPIOA, LED1_PIN, GPIO_PIN_SET);
                    else if (step_alt == 1)
                        HAL_GPIO_WritePin(GPIOA, LED2_PIN, GPIO_PIN_SET);
                    else
                        HAL_GPIO_WritePin(GPIOC, LED3_PIN, GPIO_PIN_RESET);
                }
                break;
        }
    }
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    __HAL_RCC_PWR_CLK_ENABLE();

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState       = RCC_HSI_ON;
    osc.PLL.PLLState   = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&osc);

    clk.ClockType      = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                       |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_0);
}

static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE();

    // led PA5 & PA6
    gpio.Pin   = LED1_PIN|LED2_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    // led PC13
    gpio.Pin   = LED3_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &gpio);

    // tombol muter PA1
    gpio.Pin  = BTN_LOOP_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(BTN_LOOP_PORT, &gpio);

    // tombol interrupt PA0
    gpio.Pin  = BTN_EXTI_PIN;
    gpio.Mode = GPIO_MODE_IT_RISING;
    gpio.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(BTN_EXTI_PORT, &gpio);

    HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}
