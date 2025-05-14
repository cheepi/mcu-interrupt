#include "main.h"

#define BTN_PIN    GPIO_PIN_0
#define BTN_PORT   GPIOA

#define LED1_PIN   GPIO_PIN_5   // PA5
#define LED2_PIN   GPIO_PIN_6   // PA6
#define LED3_PIN   GPIO_PIN_13  // PC13 (active-low)

// status nyala/mati per led
typedef enum { PHASE_ON, PHASE_OFF } Phase;

// variabel untuk pattern 2
static Phase    led1_phase = PHASE_ON;
static Phase    led2_phase = PHASE_ON;
static Phase    led3_phase = PHASE_ON;
static uint32_t t_window;
static uint32_t t_a5;
static uint32_t t_a6;
static uint32_t t_blink;

volatile uint8_t pattern = 0;

// callback buat interrupt tombol (ada debounce dikit)
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    static uint32_t last = 0;
    uint32_t now = HAL_GetTick();
    if (GPIO_Pin == BTN_PIN && now - last > 200) {
        pattern = (pattern + 1) & 3;
        last = now;
    }
}

void SystemClock_Config(void);
static void MX_GPIO_Init(void);

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    uint8_t  prev_pattern = 0xFF; // biar langsung ke-trigger pas awal
    uint32_t t_sync       = 0;
    uint32_t t_alt        = 0;
    uint8_t  step_alt     = 0;

    while (1) {
        uint32_t now = HAL_GetTick();

        // kalo ganti pola, reset semua timer & status led
        if (pattern != prev_pattern) {
            t_sync   = now;
            t_alt    = now;
            step_alt = 0;

            if (pattern == 2) {
                // reset semua status led dan timer internal
                led1_phase = PHASE_ON;
                led2_phase = PHASE_ON;
                led3_phase = PHASE_ON;
                t_window   = now;
                t_a5       = now;
                t_a6       = now;
                t_blink    = now;

                // semua nyala
                HAL_GPIO_WritePin(GPIOA, LED1_PIN | LED2_PIN, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(GPIOC, LED3_PIN, GPIO_PIN_RESET);
            }

            // inisialisasi led sesuai pattern
            switch (pattern) {
                case 0:
                    // nyala semua
                    HAL_GPIO_WritePin(GPIOA, LED1_PIN | LED2_PIN, GPIO_PIN_SET);
                    HAL_GPIO_WritePin(GPIOC, LED3_PIN, GPIO_PIN_RESET);
                    break;
                case 1:
                    // mati semua dulu buat blinking sync
                    HAL_GPIO_WritePin(GPIOA, LED1_PIN | LED2_PIN, GPIO_PIN_RESET);
                    HAL_GPIO_WritePin(GPIOC, LED3_PIN, GPIO_PIN_SET);
                    break;
                case 3:
                    // awalnya mati semua, terus nyalain PA5 doang
                    HAL_GPIO_WritePin(GPIOA, LED1_PIN | LED2_PIN, GPIO_PIN_RESET);
                    HAL_GPIO_WritePin(GPIOC, LED3_PIN, GPIO_PIN_SET);
                    HAL_GPIO_WritePin(GPIOA, LED1_PIN, GPIO_PIN_SET);
                    break;
                default:
                    break;
            }
            prev_pattern = pattern;
        }

        // logic per pola
        switch (pattern) {
            case 0:
                // nyala semua, gak perlu diapa-apain
                break;

            case 1:
                // blinking bareng tiap 500ms
                if (now - t_sync >= 500) {
                    t_sync = now;
                    HAL_GPIO_TogglePin(GPIOA, LED1_PIN | LED2_PIN);
                    HAL_GPIO_TogglePin(GPIOC, LED3_PIN);
                }
                break;

            case 2:
                // nyalanya punya rasio beda2 (3:2:1) dengan waktu total 1200ms
                if (now - t_window >= 1200) {
                    t_window   = now;
                    t_a5       = now;
                    t_a6       = now;
                    t_blink    = now;
                    led1_phase = PHASE_ON;
                    led2_phase = PHASE_ON;
                    led3_phase = PHASE_ON;
                    HAL_GPIO_WritePin(GPIOA, LED1_PIN | LED2_PIN, GPIO_PIN_RESET);
                    HAL_GPIO_WritePin(GPIOC, LED3_PIN, GPIO_PIN_RESET);
                }

                // led3: nyala 1100ms, mati 100ms
                if (led3_phase == PHASE_ON && now - t_blink >= 1100) {
                    HAL_GPIO_WritePin(GPIOC, LED3_PIN, GPIO_PIN_SET);
                    led3_phase = PHASE_OFF;
                    t_blink = now;
                } else if (led3_phase == PHASE_OFF && now - t_blink >= 100) {
                    HAL_GPIO_WritePin(GPIOC, LED3_PIN, GPIO_PIN_RESET);
                    led3_phase = PHASE_ON;
                    t_blink = now;
                }

                // led2 (PA6): nyala 300ms, mati 100ms
                if (led2_phase == PHASE_ON && now - t_a6 >= 300) {
                    HAL_GPIO_WritePin(GPIOA, LED2_PIN, GPIO_PIN_SET);
                    led2_phase = PHASE_OFF;
                    t_a6 = now;
                } else if (led2_phase == PHASE_OFF && now - t_a6 >= 100) {
                    HAL_GPIO_WritePin(GPIOA, LED2_PIN, GPIO_PIN_RESET);
                    led2_phase = PHASE_ON;
                    t_a6 = now;
                }

                // led1 (PA5): nyala 500ms, mati 100ms
                if (led1_phase == PHASE_ON && now - t_a5 >= 500) {
                    HAL_GPIO_WritePin(GPIOA, LED1_PIN, GPIO_PIN_SET);
                    led1_phase = PHASE_OFF;
                    t_a5 = now;
                } else if (led1_phase == PHASE_OFF && now - t_a5 >= 100) {
                    HAL_GPIO_WritePin(GPIOA, LED1_PIN, GPIO_PIN_RESET);
                    led1_phase = PHASE_ON;
                    t_a5 = now;
                }
                break;

            case 3:
                // led nyala gantian tiap 500ms
                if (now - t_alt >= 500) {
                    t_alt = now;
                    step_alt = (step_alt + 1) % 3;
                    HAL_GPIO_WritePin(GPIOA, LED1_PIN | LED2_PIN, GPIO_PIN_RESET);
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

    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                       | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
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

    // PA5 & PA6 buat led output
    gpio.Pin   = LED1_PIN | LED2_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    // PC13 juga buat led output (active low)
    gpio.Pin   = LED3_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &gpio);

    // tombol PA0, interrupt rising edge, pull-down
    gpio.Pin   = BTN_PIN;
    gpio.Mode  = GPIO_MODE_IT_RISING;
    gpio.Pull  = GPIO_PULLDOWN;
    HAL_GPIO_Init(BTN_PORT, &gpio);

    HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}
