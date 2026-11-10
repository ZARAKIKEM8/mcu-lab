#include "main.h"
#include "oled.h"

I2C_HandleTypeDef hi2c1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);

#define KEY_UP     0x01U
#define KEY_DOWN   0x02U
#define KEY_SPEED  0x04U
#define KEY_START  0x08U

#define MODE_SET   0U
#define MODE_RUN   1U
#define MODE_DONE  2U

static uint8_t mode = MODE_SET;
static uint8_t speedLevel = 2;
static uint16_t setSeconds = 30;
static int16_t remainSeconds = 30;
static volatile uint8_t secondFlag = 0;

static const uint16_t speedDuty[3] = {60, 80, 99};

static GPIO_TypeDef *keyPort[4] = {GPIOB, GPIOB, GPIOB, GPIOB};
static const uint16_t keyPin[4] = {
    KEY_TIME_UP_Pin,
    KEY_TIME_DOWN_Pin,
    KEY_SPEED_Pin,
    KEY_START_Pin
};

static void Fan_Start(uint16_t duty)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 99);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    HAL_Delay(250);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, duty);
}

static void Fan_Stop(void)
{
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
}

static void Beep_Once(void)
{
    HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_RESET);
    HAL_Delay(200);
    HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_SET);
}

static uint8_t Key_Scan(void)
{
    static uint8_t last[4] = {1, 1, 1, 1};
    uint8_t result = 0;

    for (uint8_t i = 0; i < 4; i++)
    {
        uint8_t now = HAL_GPIO_ReadPin(keyPort[i], keyPin[i]);
        if ((last[i] == 1U) && (now == 0U))
        {
            HAL_Delay(10);
            if (HAL_GPIO_ReadPin(keyPort[i], keyPin[i]) == GPIO_PIN_RESET)
            {
                result |= (uint8_t)(1U << i);
            }
        }
        last[i] = now;
    }

    return result;
}

static void OLED_Update(void)
{
    OLED_Clear();

    if (mode == MODE_SET)
    {
        OLED_ShowString(0, 0, "SET TIME", 2);
        OLED_ShowString(0, 16, "TIME ", 2);
        OLED_ShowNum(60, 16, setSeconds, 3, 2);
        OLED_ShowChar(96, 16, 's', 2);
        OLED_ShowString(0, 32, "SPEED ", 2);
        OLED_ShowNum(72, 32, speedLevel, 1, 2);
        OLED_ShowString(0, 48, "START PB0", 2);
    }
    else if (mode == MODE_RUN)
    {
        OLED_ShowString(0, 0, "RUNNING", 2);
        OLED_ShowString(0, 16, "LEFT ", 2);
        OLED_ShowNum(60, 16, (uint16_t)remainSeconds, 3, 2);
        OLED_ShowChar(96, 16, 's', 2);
        OLED_ShowString(0, 32, "SPEED ", 2);
        OLED_ShowNum(72, 32, speedLevel, 1, 2);
        OLED_ShowString(0, 48, "FAN ON", 2);
    }
    else
    {
        OLED_ShowString(0, 0, "DONE", 2);
        OLED_ShowString(0, 16, "FAN OFF", 2);
        OLED_ShowString(0, 32, "BEEP ONCE", 2);
    }

    OLED_Refresh();
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        secondFlag = 1;
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_TIM2_Init();
    MX_TIM3_Init();

    OLED_Init();
    HAL_TIM_Base_Start_IT(&htim2);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_STATUS_GPIO_Port, LED_STATUS_Pin, GPIO_PIN_SET);
    OLED_Update();

    while (1)
    {
        uint8_t key = Key_Scan();

        if (mode == MODE_SET)
        {
            if ((key & KEY_UP) != 0U)
            {
                setSeconds += 5;
                if (setSeconds > 999U)
                {
                    setSeconds = 999U;
                }
                OLED_Update();
            }

            if ((key & KEY_DOWN) != 0U)
            {
                if (setSeconds > 5U)
                {
                    setSeconds -= 5;
                }
                if (setSeconds < 5U)
                {
                    setSeconds = 5U;
                }
                OLED_Update();
            }

            if ((key & KEY_SPEED) != 0U)
            {
                speedLevel = (uint8_t)(speedLevel % 3U + 1U);
                OLED_Update();
            }

            if ((key & KEY_START) != 0U)
            {
                mode = MODE_RUN;
                remainSeconds = (int16_t)setSeconds;
                __HAL_TIM_SET_COUNTER(&htim2, 0);
                __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
                secondFlag = 0;
                HAL_GPIO_WritePin(LED_STATUS_GPIO_Port, LED_STATUS_Pin, GPIO_PIN_RESET);
                Fan_Start(speedDuty[speedLevel - 1U]);
                OLED_Update();
            }
        }
        else if (mode == MODE_RUN)
        {
            if ((key & KEY_SPEED) != 0U)
            {
                speedLevel = (uint8_t)(speedLevel % 3U + 1U);
                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, speedDuty[speedLevel - 1U]);
                OLED_Update();
            }

            if (secondFlag != 0U)
            {
                secondFlag = 0;
                remainSeconds--;
                OLED_Update();
            }

            if (remainSeconds <= 0)
            {
                Fan_Stop();
                Beep_Once();
                mode = MODE_DONE;
                HAL_GPIO_WritePin(LED_STATUS_GPIO_Port, LED_STATUS_Pin, GPIO_PIN_SET);
                OLED_Update();
            }
        }
        else
        {
            static uint8_t doneSeconds = 0;

            if (secondFlag != 0U)
            {
                secondFlag = 0;
                doneSeconds++;
                if (doneSeconds >= 2U)
                {
                    doneSeconds = 0;
                    mode = MODE_SET;
                    OLED_Update();
                }
            }
        }
    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    osc.HSIState = RCC_HSI_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK)
    {
        Error_Handler();
    }

    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_STATUS_GPIO_Port, LED_STATUS_Pin, GPIO_PIN_SET);

    gpio.Pin = GPIO_PIN_4 | GPIO_PIN_5;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = BEEP_Pin;
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    HAL_GPIO_Init(BEEP_GPIO_Port, &gpio);

    gpio.Pin = LED_STATUS_Pin;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(LED_STATUS_GPIO_Port, &gpio);

    gpio.Pin = KEY_TIME_UP_Pin | KEY_TIME_DOWN_Pin |
               KEY_SPEED_Pin | KEY_START_Pin;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &gpio);
}

static void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    {
        Error_Handler();
    }
}

static void MX_TIM2_Init(void)
{
    TIM_ClockConfigTypeDef source = {0};
    TIM_MasterConfigTypeDef master = {0};

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 7199;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 9999;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }

    source.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim2, &source) != HAL_OK)
    {
        Error_Handler();
    }

    master.MasterOutputTrigger = TIM_TRGO_RESET;
    master.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &master) != HAL_OK)
    {
        Error_Handler();
    }
}

static void MX_TIM3_Init(void)
{
    TIM_ClockConfigTypeDef source = {0};
    TIM_MasterConfigTypeDef master = {0};
    TIM_OC_InitTypeDef channel = {0};

    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 71;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 99;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
    {
        Error_Handler();
    }

    source.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim3, &source) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
    {
        Error_Handler();
    }

    master.MasterOutputTrigger = TIM_TRGO_RESET;
    master.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &master) != HAL_OK)
    {
        Error_Handler();
    }

    channel.OCMode = TIM_OCMODE_PWM1;
    channel.Pulse = 0;
    channel.OCPolarity = TIM_OCPOLARITY_HIGH;
    channel.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim3, &channel, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_TIM_MspPostInit(&htim3);
}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}
