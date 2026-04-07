#include "device_driver.h"

#define RCC_AHB1ENR    (*(volatile unsigned int*)0x40023830)
#define GPIOB_MODER    (*(volatile unsigned int*)0x40020400)
#define GPIOB_OTYPER   (*(volatile unsigned int*)0x40020404)
#define GPIOB_ODR      (*(volatile unsigned int*)0x40020414)

// ---------------------------------
// 패널 번호 기준
// 4번 패널 = 맨위    = module 0
// 3번 패널 = 그 아래 = module 1
// 2번 패널 = 그 아래 = module 2
// 1번 패널 = 맨아래 = module 3
// ---------------------------------

unsigned char num_3[8] = {
    0x00,
    0x6E,
    0x91,
    0x91,
    0x91,
    0x81,
    0x42,
    0x00
};

unsigned char num_2[8] = {
    0x00,
    0x61,
    0x91,
    0x89,
    0x85,
    0x83,
    0x41,
    0x00
};

unsigned char num_1[8] = {
    0x00,
    0x01,
    0x01,
    0xFF,
    0x41,
    0x21,
    0x00,
    0x00
};

void Ready_State(void)
{
    for (int i = 0; i < 5; i++)
    {
        MAX7219_ClearAll();
        MAX7219_FillModule(1);
        MAX7219_FillModule(2);
        TIM2_Delay(500);

        MAX7219_ClearAll();
        TIM2_Delay(500);
    }
}

void Countdown_State(void)
{
    // 숫자 3 + 2번 패널만 ON
    MAX7219_ClearAll();
    MAX7219_FillModule(2);
    MAX7219_ShowPattern(3, num_3);
    Buzzer_Count_Short();
    TIM2_Delay(1000);

    // 숫자 2 + 3번 패널만 ON
    MAX7219_ClearAll();
    MAX7219_FillModule(1);
    MAX7219_ShowPattern(3, num_2);
    Buzzer_Count_Short();
    TIM2_Delay(1000);

    // 숫자 1 + 4번 패널만 ON
    MAX7219_ClearAll();
    MAX7219_FillModule(0);
    MAX7219_ShowPattern(3, num_1);
    Buzzer_Count_Long();
    TIM2_Delay(150);

    // 전체 ON
    MAX7219_FillModulesRange(0, 3);
    TIM2_Delay(1000);

    // 전체 OFF
    MAX7219_ClearAll();
    Buzzer_Start_Race();
}

void Main(void)
{
    Clock_Init();
    GPIO_Init();
    Buzzer_Init();
    MAX7219_Init();
    MAX7219_ClearAll();

    while (1)
    {
        Ready_State();
        Countdown_State();

        while (1)
        {
        }
    }
}