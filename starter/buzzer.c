/**
 * @file buzzer.c
 * @brief 부저 하드웨어 제어 드라이버 (스타터 보드용)
 * @details 스타터 보드에 연결된 부저(PB0)를 제어하여 레이싱 출발 전 카운트다운 및 
 * 출발 신호와 같은 다양한 패턴의 비프(Beep) 효과음을 출력하는 기능을 제공합니다.
 */

#include "device_driver.h"

#define RCC_AHB1ENR    (*(volatile unsigned int*)0x40023830)
#define GPIOB_MODER    (*(volatile unsigned int*)0x40020400)
#define GPIOB_OTYPER   (*(volatile unsigned int*)0x40020404)
#define GPIOB_ODR      (*(volatile unsigned int*)0x40020414)

// Buzzer: PB0 (A3)
#define BUZZER_ON()   (GPIOB_ODR |=  (1 << 0))
#define BUZZER_OFF()  (GPIOB_ODR &= ~(1 << 0))

/**
 * @brief 부저 하드웨어 초기화
 * @details 부저가 연결된 GPIOB 클럭을 활성화하고, PB0 핀을 Push-pull 방식의 
 * 출력 모드로 설정합니다. 초기 상태는 부저가 울리지 않도록 OFF 상태로 만듭니다.
 */
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

/**
 * @brief 커스텀 딜레이 비프음 출력
 * @details 지정된 밀리초(ms) 단위의 시간만큼 부저를 켜고 끄는 기본 동작을 수행합니다.
 * @param on_ms 부저를 울릴 시간 (밀리초 단위)
 * @param off_ms 부저를 끈 후 대기할 시간 (밀리초 단위)
 */
void Buzzer_Beep(int on_ms, int off_ms)
{
    BUZZER_ON();

    if (on_ms > 0)
        TIM2_Delay(on_ms);

    BUZZER_OFF();

    if (off_ms > 0)
        TIM2_Delay(off_ms);
}

/**
 * @brief 짧은 카운트다운 효과음 (숫자 3, 2)
 * @details 80ms 동안 짧게 부저를 울려 경기 시작 전 초기 카운트다운 효과를 냅니다.
 */
void Buzzer_Count_Short(void)
{
    Buzzer_Beep(80, 10);
}

/**
 * @brief 약간 긴 카운트다운 효과음 (숫자 1)
 * @details 140ms 동안 부저를 울려 카운트다운의 마지막 임박 상태를 알립니다.
 */
void Buzzer_Count_Long(void)
{
    Buzzer_Beep(140, 10);
}

/**
 * @brief 레이싱 출발 효과음
 * @details 짧은 비프음 3회 연속 출력 후 긴 비프음을 울려 실제 모터스포츠의 
 * 출발 신호(띠, 띠, 띠, 삐-)와 유사한 사운드 패턴을 연출합니다.
 */
void Buzzer_Start_Race(void)
{
    Buzzer_Beep(50, 30);
    Buzzer_Beep(50, 30);
    Buzzer_Beep(50, 50);
    Buzzer_Beep(220, 10);
}