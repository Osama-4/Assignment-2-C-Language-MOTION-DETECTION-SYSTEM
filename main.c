#include "main.h"
#include <stdint.h>

/*----Pin assignments----*/
#define TRIG_PIN GPIO_PIN_0 /* PA0-> HC-SR04 TRIG */
#define ECHO_PIN GPIO_PIN_1 /* PA1 <-HC-SR04 ECHO */
#define CLK_PIN GPIO_PIN_6  /* PA6-> TM1637 CLK */
#define DIO_PIN GPIO_PIN_7  /* PA7-> TM1637 DIO */
#define RED_LED_PIN GPIO_PIN_9  /* PA9-> Red LED */
#define BLUE_LED_PIN GPIO_PIN_10 /* PA10-> Blue LED */
#define BUZZER_PIN GPIO_PIN_11   /* PB11-> Buzzer */

/*----TM1637 commands----*/
#define TM1637_ADDR_AUTO 0x40
#define TM1637_START_ADDR 0xC0
#define TM1637_BRIGHTNESS 0x8F /* display ON, max brightness */

/*----Detection----*/
#define DETECTION_THRESHOLD 50u /* cm */

/*----Prototypes----*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
uint32_t measureDistance(void);
void TM1637_Init(void);
void TM1637_Start(void);
void TM1637_Stop(void);
void TM1637_WriteByte(uint8_t data);
void TM1637_WriteCommand(uint8_t command);
void updateDisplay(uint16_t count);
void Error_Handler(void);

/*----Globals----*/
uint16_t personCount = 0;

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    TM1637_Init();
    updateDisplay(personCount);

    while (1)
    {
        uint32_t distance = measureDistance();
      
      //========================
      //OSAMA MOHAMMED SIDDIG: Detection logic
        if (distance > 0 && distance < DETECTION_THRESHOLD)
        {
            /* object detected within threshold */
            HAL_GPIO_WritePin(GPIOA, BLUE_LED_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(GPIOA, RED_LED_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOB, BUZZER_PIN, GPIO_PIN_SET);

            personCount++;
            updateDisplay(personCount);

            HAL_Delay(1000); /* hold to avoid double-counting */
        }
        else
        {
            /* path clear-idle indication */
            HAL_GPIO_WritePin(GPIOA, BLUE_LED_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOA, RED_LED_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(GPIOB, BUZZER_PIN, GPIO_PIN_RESET);
        }

        HAL_Delay(100);
    }
}

//======================================================
/*----HC-SR04 ultrasonic----*/
// OSAMA MOHAMMED SIDDIG: Responsible for ultrasonic sensor logic
uint32_t measureDistance(void)
{
    uint32_t startTime, endTime;

    /* 10 us trigger pulse */
    HAL_GPIO_WritePin(GPIOA, TRIG_PIN, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOA, TRIG_PIN, GPIO_PIN_RESET);

    /* wait for echo rising edge */
    uint32_t timeout = HAL_GetTick() + 10;
    while (HAL_GPIO_ReadPin(GPIOA, ECHO_PIN) == GPIO_PIN_RESET)
        if (HAL_GetTick() >= timeout) return 0;
    startTime = HAL_GetTick();

    /* wait for echo falling edge */
    timeout = HAL_GetTick() + 50;
    while (HAL_GPIO_ReadPin(GPIOA, ECHO_PIN) == GPIO_PIN_SET)
        if (HAL_GetTick() >= timeout) return 0;
    endTime = HAL_GetTick();

    return ((endTime - startTime) * 34300) / 2000; /* cm */
}

/*----TM1637 4-digit display (push-pull bit-bang)----*/
//Abdelrahman Mohamed: Responsible for initializing display
void TM1637_Start(void)
{
    HAL_GPIO_WritePin(GPIOA, CLK_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, DIO_PIN, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOA, DIO_PIN, GPIO_PIN_RESET);
}

void TM1637_Stop(void)
{
    HAL_GPIO_WritePin(GPIOA, CLK_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, DIO_PIN, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOA, CLK_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, DIO_PIN, GPIO_PIN_SET);
}

void TM1637_WriteByte(uint8_t data)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        HAL_GPIO_WritePin(GPIOA, CLK_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, DIO_PIN,
            (data & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOA, CLK_PIN, GPIO_PIN_SET);
        HAL_Delay(1);
        data >>= 1;
    }
    /* acknowledge clock */
    HAL_GPIO_WritePin(GPIOA, CLK_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, DIO_PIN, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOA, CLK_PIN, GPIO_PIN_SET);
    HAL_Delay(1);
}

void TM1637_WriteCommand(uint8_t command)
{
    TM1637_Start();
    TM1637_WriteByte(command);
    TM1637_Stop();
}

void TM1637_Init(void)
{
    HAL_GPIO_WritePin(GPIOA, CLK_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, DIO_PIN, GPIO_PIN_SET);
    HAL_Delay(1);
    TM1637_WriteCommand(TM1637_ADDR_AUTO);
    TM1637_WriteCommand(TM1637_BRIGHTNESS);
}

//Abdelrahman Mohamed: Responsible for showing count on TM1637
void updateDisplay(uint16_t count)
{
    static const uint8_t digitToSegment[] = {
        0x3F, 0x06, 0x5B, 0x4F, 0x66,
        0x6D, 0x7D, 0x07, 0x7F, 0x6F
    };

    uint8_t digits[4];
    digits[0] = (count / 1000) % 10;
    digits[1] = (count / 100) % 10;
    digits[2] = (count / 10) % 10;
    digits[3] = count % 10;

    TM1637_WriteCommand(TM1637_ADDR_AUTO);

    TM1637_Start();
    TM1637_WriteByte(TM1637_START_ADDR);
    for (int i = 0; i < 4; i++)
        TM1637_WriteByte(digitToSegment[digits[i]]);
    TM1637_Stop();

    TM1637_WriteCommand(TM1637_BRIGHTNESS);
}

/*----GPIO initialisation----*/
//Abdelaziz Alsayed: Responsible for LEDs and buzzer setup
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Red + Blue LEDs (PA9, PA10) */
    GPIO_InitStruct.Pin = RED_LED_PIN | BLUE_LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Buzzer (PB11) */
    GPIO_InitStruct.Pin = BUZZER_PIN;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* HC-SR04 trigger (PA0) */
    GPIO_InitStruct.Pin = TRIG_PIN;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* HC-SR04 echo (PA1) */
    GPIO_InitStruct.Pin = ECHO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* TM1637 CLK + DIO (PA6, PA7)-push-pull, NOT open-drain */
    GPIO_InitStruct.Pin = CLK_PIN | DIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/*----System clock (8 MHz HSI)----*/
//OSAMA MOHAMMED & Abdelaziz Alsayed: Responsible for system clock configuration
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
        | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
        Error_Handler();
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) { }
}
