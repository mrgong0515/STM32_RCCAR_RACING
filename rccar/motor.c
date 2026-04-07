#include "device_driver.h"
#include <stdio.h>

void Motor_Init(void)
{
    // 1. 방향 제어용 GPIO 초기화 (PA0, PA1, PA4, PA5)
    Macro_Set_Bit(RCC->AHB1ENR, 0); 
    Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, 0);  // PA0 -> Output
    Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, 2);  // PA1 -> Output
    Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, 8);  // PA4 -> Output
    Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, 10); // PA5 -> Output

    // 2. 속도 제어용 PWM GPIO 초기화 (PB4, PB5 -> TIM3_CH1, TIM3_CH2)
    Macro_Set_Bit(RCC->AHB1ENR, 1); 
    Macro_Write_Block(GPIOB->MODER, 0xF, 0xA, 8);     // PB4, PB5 -> Alternate Function (1010)
    Macro_Write_Block(GPIOB->AFR[0], 0xFF, 0x22, 16); // PB4, PB5 -> AF02 연결 (TIM3)

    // 3. 타이머3 초기화 (PWM 모드 설정)
    Macro_Set_Bit(RCC->APB1ENR, 1); 

    // TIMXCLK을 이용하여 100Hz 주파수로 설정 (스위칭 노이즈 대폭 감소를 위해 10000.0으로 수정)
    // ARR을 100-1로 설정하여 0~100% 듀티비 직관적 입력 구성
    TIM3->PSC = (unsigned int)(TIMXCLK / 10000.0 + 0.5) - 1; 
    TIM3->ARR = 100 - 1; 

    // CH1, CH2 PWM Mode 1 설정 및 Preload 활성화
    TIM3->CCMR1 = (6 << 4) | (1 << 3) | (6 << 12) | (1 << 11);
    
    // CH1, CH2 출력 활성화
    TIM3->CCER = (1 << 0) | (1 << 4); 

    // 초기 듀티비 0% 설정
    TIM3->CCR1 = 0; 
    TIM3->CCR2 = 0; 

    Macro_Set_Bit(TIM3->EGR, 0);
    TIM3->CR1 = (1 << 0); // 타이머 카운터 시작
}

void Motor_Set_PWM(int left_duty, int right_duty)
{
    // 입력된 듀티비를 0~100 사이로 제한하여 오작동 방지
    if (left_duty < 0) left_duty = 0;
    if (left_duty > 100) left_duty = 100;
    if (right_duty < 0) right_duty = 0;
    if (right_duty > 100) right_duty = 100;

    // 타이머 캡처/비교 레지스터에 듀티비 적용
    TIM3->CCR1 = left_duty;  // 좌측 모터 속도 (PB4, ENA)
    TIM3->CCR2 = right_duty; // 우측 모터 속도 (PB5, ENB)
}

void Control_Motor_By_Joystick(char joy)
{
    switch(joy)
    {
        case '8': // 직진
            printf(" >> Action: Forward\n");
            GPIOA->ODR = (1 << 0) | (0 << 1) | (1 << 4) | (0 << 5);
            Motor_Set_PWM(100, 100);
            break;
        case '2': // 후진
            printf(" >> Action: Backward\n");
            GPIOA->ODR = (0 << 0) | (1 << 1) | (0 << 4) | (1 << 5);
            Motor_Set_PWM(100, 100);
            break;
        case '4': // 좌회전
            printf(" >> Action: Turn Left\n");
            GPIOA->ODR = (0 << 0) | (1 << 1) | (1 << 4) | (0 << 5); 
            Motor_Set_PWM(100, 100);
            break;
        case '6': // 우회전
            printf(" >> Action: Turn Right\n");
            GPIOA->ODR = (1 << 0) | (0 << 1) | (0 << 4) | (1 << 5); 
            Motor_Set_PWM(100, 100);
            break;
        case '7': // 전진 좌회전 (대각선)
            printf(" >> Action: Forward Left\n");
            GPIOA->ODR = (1 << 0) | (0 << 1) | (1 << 4) | (0 << 5);
            Motor_Set_PWM(25, 100);
            break;
        case '9': // 전진 우회전 (대각선)
            printf(" >> Action: Forward Right\n");
            GPIOA->ODR = (1 << 0) | (0 << 1) | (1 << 4) | (0 << 5);
            Motor_Set_PWM(100, 25); 
            break;
        case '1': // 후진 좌회전 (대각선)
            printf(" >> Action: Backward Left\n");
            GPIOA->ODR = (0 << 0) | (1 << 1) | (0 << 4) | (1 << 5);
            Motor_Set_PWM(25, 100);
            break;
        case '3': // 후진 우회전 (대각선)
            printf(" >> Action: Backward Right\n");
            GPIOA->ODR = (0 << 0) | (1 << 1) | (0 << 4) | (1 << 5);
            Motor_Set_PWM(100, 25);
            break;
        case '5': // 중립 / 정지
        default:
            printf(" >> Action: Stop\n");
            GPIOA->ODR &= ~((1 << 0) | (1 << 1) | (1 << 4) | (1 << 5));
            Motor_Set_PWM(0, 0);
            break;
    }
}