/**
 * @file motor.c
 * @brief DC 모터 방향 및 속도 제어 드라이버
 * @details RC카의 바퀴를 구동하는 모터 드라이버 모듈(예: L298N)에 전달할 
 * 방향 제어용 디지털 출력 핀과 속도 제어용 PWM 신호를 설정하고 제어합니다.
 */

#include "device_driver.h"
#include <stdio.h>

// 모터 제어용 GPIO 핀 매크로 정의
#define PIN_M1_IN1 (1 << 0) // PA0 (좌측 모터 전진)
#define PIN_M1_IN2 (1 << 1) // PA1 (좌측 모터 후진)
#define PIN_M2_IN3 (1 << 4) // PA4 (우측 모터 전진)
#define PIN_M2_IN4 (1 << 5) // PA5 (우측 모터 후진)

// 주행 방향별 GPIO 상태 조합
#define MOVE_FORWARD  (PIN_M1_IN1 | PIN_M2_IN3)
#define MOVE_BACKWARD (PIN_M1_IN2 | PIN_M2_IN4)
#define MOVE_LEFT     (PIN_M1_IN2 | PIN_M2_IN3) // 제자리 좌로 돌아
#define MOVE_RIGHT    (PIN_M1_IN1 | PIN_M2_IN4) // 제자리 우로 돌아
#define MOVE_ALL_PINS (PIN_M1_IN1 | PIN_M1_IN2 | PIN_M2_IN3 | PIN_M2_IN4)

// PWM 속도 제어용 상수 정의
#define SPEED_MAX       100 // 직진, 제자리 회전 시 최대 속도
#define SPEED_DIAG_FAST 90  // 대각선 주행 시 바깥쪽 바퀴 속도
#define SPEED_DIAG_SLOW 30  // 대각선 주행 시 안쪽 바퀴 속도 
#define SPEED_STOP      0

/**
 * @brief 모터 구동용 하드웨어 초기화
 * @details 모터의 정/역방향 제어를 위한 GPIOA 핀과
 * 속도 제어를 위한 GPIOB 핀(TIM3 PWM)을 초기화합니다.
 */
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
    Macro_Write_Block(GPIOB->MODER, 0xF, 0xA, 8);     // PB4, PB5 -> Alternate Function
    Macro_Write_Block(GPIOB->AFR[0], 0xFF, 0x22, 16); // PB4, PB5 -> AF02 연결 (TIM3)

    // 3. 타이머3 초기화 (PWM 모드 설정)
    Macro_Set_Bit(RCC->APB1ENR, 1); 

    // TIMXCLK을 이용하여 100Hz 주파수로 설정
    TIM3->PSC = (unsigned int)(TIMXCLK / 10000.0 + 0.5) - 1; 
    TIM3->ARR = 100 - 1; 

    // CH1, CH2 PWM Mode 1 설정 및 Preload 활성화
    TIM3->CCMR1 = (6 << 4) | (1 << 3) | (6 << 12) | (1 << 11);
    
    // CH1, CH2 출력 활성화
    TIM3->CCER = (1 << 0) | (1 << 4); 

    // 초기 듀티비 0% 설정
    TIM3->CCR1 = SPEED_STOP; 
    TIM3->CCR2 = SPEED_STOP; 

    Macro_Set_Bit(TIM3->EGR, 0);
    TIM3->CR1 = (1 << 0); // 타이머 카운터 시작
}

/**
 * @brief 양쪽 모터의 속도(PWM 듀티비) 개별 설정
 * @details 입력받은 듀티비 값을 타이머의 비교 레지스터에 적용합니다.
 * @param left_duty 좌측 모터의 듀티비 (0 ~ 100)
 * @param right_duty 우측 모터의 듀티비 (0 ~ 100)
 */
void Motor_Set_PWM(int left_duty, int right_duty)
{
    // 입력된 듀티비를 0~100 사이로 제한
    if (left_duty < SPEED_STOP) left_duty = SPEED_STOP;
    if (left_duty > SPEED_MAX) left_duty = SPEED_MAX;
    if (right_duty < SPEED_STOP) right_duty = SPEED_STOP;
    if (right_duty > SPEED_MAX) right_duty = SPEED_MAX;

    // 타이머 캡처/비교 레지스터에 듀티비 적용
    TIM3->CCR1 = left_duty;  // 좌측 모터 속도 (PB4, ENA)
    TIM3->CCR2 = right_duty; // 우측 모터 속도 (PB5, ENB)
}

/**
 * @brief 수신된 조이스틱 방향 데이터에 따른 차량 통합 제어
 * @details 1부터 9까지의 문자 데이터를 해석하여 방향 핀과 속도를 제어합니다.
 * @param joy 방향을 나타내는 1바이트 문자 (1~9 방향, 5는 정지)
 */
void Control_Motor_By_Joystick(char joy)
{
    // 새로운 방향을 설정하기 전 모든 방향 핀 초기화
    GPIOA->ODR &= ~MOVE_ALL_PINS;

    switch(joy)
    {
        case '8': // 직진
            printf(" >> Action: Forward\n");
            GPIOA->ODR |= MOVE_FORWARD;
            Motor_Set_PWM(SPEED_MAX, SPEED_MAX);
            break;
        case '2': // 후진
            printf(" >> Action: Backward\n");
            GPIOA->ODR |= MOVE_BACKWARD;
            Motor_Set_PWM(SPEED_MAX, SPEED_MAX);
            break;
        case '4': // 좌회전 (좌로 돌아)
            printf(" >> Action: Turn Left\n");
            GPIOA->ODR |= MOVE_LEFT; 
            Motor_Set_PWM(SPEED_MAX, SPEED_MAX);
            break;
        case '6': // 우회전 (우로 돌아)
            printf(" >> Action: Turn Right\n");
            GPIOA->ODR |= MOVE_RIGHT; 
            Motor_Set_PWM(SPEED_MAX, SPEED_MAX);
            break;
        case '7': // 전진 좌회전 (대각선)
            printf(" >> Action: Forward Left\n");
            GPIOA->ODR |= MOVE_FORWARD;
            Motor_Set_PWM(SPEED_DIAG_FAST, SPEED_DIAG_SLOW);
            break;
        case '9': // 전진 우회전 (대각선)
            printf(" >> Action: Forward Right\n");
            GPIOA->ODR |= MOVE_FORWARD;
            Motor_Set_PWM(SPEED_DIAG_SLOW, SPEED_DIAG_FAST); 
            break;
        case '1': // 후진 좌회전 (대각선)
            printf(" >> Action: Backward Left\n");
            GPIOA->ODR |= MOVE_BACKWARD;
            Motor_Set_PWM(SPEED_DIAG_FAST, SPEED_DIAG_SLOW);
            break;
        case '3': // 후진 우회전 (대각선)
            printf(" >> Action: Backward Right\n");
            GPIOA->ODR |= MOVE_BACKWARD;
            Motor_Set_PWM(SPEED_DIAG_SLOW, SPEED_DIAG_FAST);
            break;
        case '5': // 정지
        default:
            printf(" >> Action: Stop\n");
            Motor_Set_PWM(SPEED_STOP, SPEED_STOP);
            break;
    }
}