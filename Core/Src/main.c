/*
 * People counter - HC-SR04 + TM1637 4-digit display
 * STM32F103, plain HAL. No LCD here; the LEDs and buzzer show what's going on.
 *
 * PA0  trig      PA1  echo
 * PA6  tm1637 clk  PA7 tm1637 dio
 * PA9  red led   PA10 blue led   PA11 buzzer
 */

#include "main.h"
#include <stdint.h>

/* pins (everything sits on port A) */
#define TRIG_PIN   GPIO_PIN_0
#define ECHO_PIN   GPIO_PIN_1
#define CLK_PIN    GPIO_PIN_6
#define DIO_PIN    GPIO_PIN_7
#define RED_LED    GPIO_PIN_9
#define BLUE_LED   GPIO_PIN_10
#define BUZZER     GPIO_PIN_11

/* little shortcuts so the loop doesn't get noisy */
#define RED(s)   HAL_GPIO_WritePin(GPIOA, RED_LED,  (s))
#define BLUE(s)  HAL_GPIO_WritePin(GPIOA, BLUE_LED, (s))
#define BUZZ(s)  HAL_GPIO_WritePin(GPIOA, BUZZER,   (s))

/* detection tuning */
#define CAL_SAMPLES  10
#define MARGIN_CM    20     /* must be this much closer than baseline to count */
#define MIN_DIST     5
#define MAX_DIST     300

void SystemClock_Config(void);
static void GPIO_Init(void);
void Error_Handler(void);

uint32_t micros(void);
uint32_t readDistance(void);

void TM1637_Init(void);
void TM1637_Start(void);
void TM1637_Stop(void);
void TM1637_WriteByte(uint8_t data);
void TM1637_Command(uint8_t cmd);
void showCount(uint16_t n);

uint16_t count    = 0;
uint32_t baseline = 0;


int main(void)
{
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();

    TM1637_Init();
    showCount(count);

    RED(GPIO_PIN_SET);
    BLUE(GPIO_PIN_RESET);
    BUZZ(GPIO_PIN_RESET);

    /* --- calibrate against the empty path --- */
    uint32_t sum  = 0;
    uint8_t  good = 0;

    for (int i = 0; i < CAL_SAMPLES; i++) {
        HAL_GPIO_TogglePin(GPIOA, BLUE_LED);     /* blink while measuring */
        uint32_t d = readDistance();
        if (d >= MIN_DIST && d <= MAX_DIST) {
            sum += d;
            good++;
        }
        HAL_Delay(100);
    }

    /* trust the baseline only if at least half the samples were sane */
    if (good >= CAL_SAMPLES / 2) {
        baseline = sum / good;
    } else {
        /* sensor's probably miswired - park here and flash both LEDs */
        while (1) {
            HAL_GPIO_TogglePin(GPIOA, RED_LED);
            HAL_GPIO_TogglePin(GPIOA, BLUE_LED);
            HAL_Delay(200);
        }
    }

    BLUE(GPIO_PIN_RESET);
    RED(GPIO_PIN_SET);     /* idle = red on */

    while (1) {
        uint32_t dist = readDistance();

        if (dist < MIN_DIST || dist > MAX_DIST) {
            HAL_Delay(100);   /* bad/no echo, just try again */
            continue;
        }

        if (dist + MARGIN_CM < baseline) {
            /* someone walked in front of the sensor */
            BLUE(GPIO_PIN_SET);
            BUZZ(GPIO_PIN_SET);
            RED(GPIO_PIN_RESET);

            count++;
            showCount(count);

            /* don't move on until the path reads clear 3 times in a row,
               otherwise one person gets counted a bunch of times */
            uint8_t clear = 0;
            while (clear < 3) {
                HAL_Delay(100);
                uint32_t d = readDistance();
                if (d >= MIN_DIST && d <= MAX_DIST) {
                    if (d + MARGIN_CM >= baseline)
                        clear++;
                    else
                        clear = 0;
                }
            }

            BLUE(GPIO_PIN_RESET);
            BUZZ(GPIO_PIN_RESET);
            RED(GPIO_PIN_SET);
            HAL_Delay(300);
        } else {
            RED(GPIO_PIN_SET);
            BLUE(GPIO_PIN_RESET);
            BUZZ(GPIO_PIN_RESET);
        }

        HAL_Delay(100);
    }
}


/* ---- HC-SR04 ---- */

/* rough microsecond timer pulled out of SysTick */
uint32_t micros(void)
{
    uint32_t ms   = HAL_GetTick();
    uint32_t load = SysTick->LOAD;
    uint32_t val  = SysTick->VAL;
    return ms * 1000 + ((load - val) * 1000) / load;
}

uint32_t readDistance(void)
{
    /* ~10us trigger pulse */
    HAL_GPIO_WritePin(GPIOA, TRIG_PIN, GPIO_PIN_SET);
    for (volatile int i = 0; i < 72; i++);
    HAL_GPIO_WritePin(GPIOA, TRIG_PIN, GPIO_PIN_RESET);

    /* wait for echo high */
    uint32_t timeout = HAL_GetTick() + 10;
    while (HAL_GPIO_ReadPin(GPIOA, ECHO_PIN) == GPIO_PIN_RESET)
        if (HAL_GetTick() >= timeout) return 0;
    uint32_t start = micros();

    /* then wait for it to drop */
    timeout = HAL_GetTick() + 50;
    while (HAL_GPIO_ReadPin(GPIOA, ECHO_PIN) == GPIO_PIN_SET)
        if (HAL_GetTick() >= timeout) return 0;
    uint32_t end = micros();

    return ((end - start) * 34) / 2000;   /* us -> cm */
}


/* ---- TM1637 display ---- */

static void tm_delay(void)
{
    for (volatile int i = 0; i < 72; i++);
}

void TM1637_Start(void)
{
    HAL_GPIO_WritePin(GPIOA, CLK_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, DIO_PIN, GPIO_PIN_SET);
    tm_delay();
    HAL_GPIO_WritePin(GPIOA, DIO_PIN, GPIO_PIN_RESET);
    tm_delay();
    HAL_GPIO_WritePin(GPIOA, CLK_PIN, GPIO_PIN_RESET);
    tm_delay();
}

void TM1637_Stop(void)
{
    HAL_GPIO_WritePin(GPIOA, DIO_PIN, GPIO_PIN_RESET);
    tm_delay();
    HAL_GPIO_WritePin(GPIOA, CLK_PIN, GPIO_PIN_SET);
    tm_delay();
    HAL_GPIO_WritePin(GPIOA, DIO_PIN, GPIO_PIN_SET);
    tm_delay();
}

void TM1637_WriteByte(uint8_t data)
{
    for (uint8_t i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(GPIOA, CLK_PIN, GPIO_PIN_RESET);
        tm_delay();
        HAL_GPIO_WritePin(GPIOA, DIO_PIN,
                          (data & 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        tm_delay();
        HAL_GPIO_WritePin(GPIOA, CLK_PIN, GPIO_PIN_SET);
        tm_delay();
        data >>= 1;
    }

    /* ack tick */
    HAL_GPIO_WritePin(GPIOA, CLK_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, DIO_PIN, GPIO_PIN_SET);
    tm_delay();
    HAL_GPIO_WritePin(GPIOA, CLK_PIN, GPIO_PIN_SET);
    tm_delay();
    HAL_GPIO_WritePin(GPIOA, CLK_PIN, GPIO_PIN_RESET);
    tm_delay();
}

void TM1637_Command(uint8_t cmd)
{
    TM1637_Start();
    TM1637_WriteByte(cmd);
    TM1637_Stop();
}

void TM1637_Init(void)
{
    HAL_GPIO_WritePin(GPIOA, CLK_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, DIO_PIN, GPIO_PIN_SET);
    HAL_Delay(1);
    TM1637_Command(0x40);   /* auto-increment address */
    TM1637_Command(0x8F);   /* display on, brightness max */
}

void showCount(uint16_t n)
{
    static const uint8_t seg[] = {
        0x3F, 0x06, 0x5B, 0x4F, 0x66,
        0x6D, 0x7D, 0x07, 0x7F, 0x6F
    };

    uint8_t d[4];
    d[0] = (n / 1000) % 10;
    d[1] = (n / 100)  % 10;
    d[2] = (n / 10)   % 10;
    d[3] =  n         % 10;

    TM1637_Command(0x40);
    TM1637_Start();
    TM1637_WriteByte(0xC0);            /* address of first digit */
    for (int i = 0; i < 4; i++)
        TM1637_WriteByte(seg[d[i]]);
    TM1637_Stop();
    TM1637_Command(0x8F);
}


/* ---- setup ---- */

static void GPIO_Init(void)
{
    GPIO_InitTypeDef io = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* push-pull outputs: trigger, both LEDs, buzzer */
    io.Pin   = TRIG_PIN | RED_LED | BLUE_LED | BUZZER;
    io.Mode  = GPIO_MODE_OUTPUT_PP;
    io.Pull  = GPIO_NOPULL;
    io.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &io);

    /* echo input */
    io.Pin  = ECHO_PIN;
    io.Mode = GPIO_MODE_INPUT;
    io.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &io);

    /* clk/dio push-pull - drive both edges so we don't depend on pull-ups.
       (open-drain only works if the module already has pull-ups on board) */
    io.Pin   = CLK_PIN | DIO_PIN;
    io.Mode  = GPIO_MODE_OUTPUT_PP;
    io.Pull  = GPIO_NOPULL;
    io.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &io);
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    osc.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState            = RCC_HSI_ON;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState        = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) Error_Handler();

    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                         RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_0) != HAL_OK) Error_Handler();
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) { }
}
