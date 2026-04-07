#include "device_driver.h"

#define RCC_AHB1ENR    (*(volatile unsigned int*)0x40023830)
#define GPIOB_MODER    (*(volatile unsigned int*)0x40020400)
#define GPIOB_OTYPER   (*(volatile unsigned int*)0x40020404)
#define GPIOB_ODR      (*(volatile unsigned int*)0x40020414)

// 부저: A3 = PB0
#define BUZZER_ON()   (GPIOB_ODR |=  (1 << 0))
#define BUZZER_OFF()  (GPIOB_ODR &= ~(1 << 0))

void Buzzer_Init(void)
{
    // GPIOB 클럭 활성화
    RCC_AHB1ENR |= (1 << 1);

    // PB0 출력 모드
    GPIOB_MODER &= ~(3 << (0 * 2));
    GPIOB_MODER |=  (1 << (0 * 2));

    // Push-pull
    GPIOB_OTYPER &= ~(1 << 0);

    BUZZER_OFF();
}

void Buzzer_Beep(int on_ms, int off_ms)
{
    BUZZER_ON();

    if (on_ms > 0)
        TIM2_Delay(on_ms);

    BUZZER_OFF();

    if (off_ms > 0)
        TIM2_Delay(off_ms);
}

// 숫자 3, 2용 짧은 삑
void Buzzer_Count_Short(void)
{
    Buzzer_Beep(80, 10);
}

// 숫자 1용 조금 더 긴 삑
void Buzzer_Count_Long(void)
{
    Buzzer_Beep(140, 10);
}

// 스타트용 레이싱 느낌
void Buzzer_Start_Race(void)
{
    Buzzer_Beep(50, 30);
    Buzzer_Beep(50, 30);
    Buzzer_Beep(50, 50);
    Buzzer_Beep(220, 10);
}