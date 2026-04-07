#include "device_driver.h"
#include <stdio.h>

// --- 1. ADC 초기화 (PA6: 1838 수신기 OUT 핀 연결) ---
void ADC_Init(void)
{
    RCC->AHB1ENR |= (1 << 0);   // GPIOA 클럭 활성화
    RCC->APB2ENR |= (1 << 8);   // ADC1 클럭 활성화
    
    GPIOA->MODER |= (3 << 6*2);  // PA6를 아날로그 모드로 설정
    
    ADC1->CR2 &= ~(1 << 0);      // ADC 비활성화 (설정 전)
    ADC1->SQR3 = 6;              // 채널 6(PA6) 선택
    ADC1->CR2 |= (1 << 0);       // ADC 활성화
}

int Get_ADC_Value(void)
{
    ADC1->SR &= ~(1 << 1);         // EOC 클리어
    ADC1->CR2 |= (1 << 30);        // 변환 시작
    
    uint32_t timeout = 10000;
    while(!(ADC1->SR & (1 << 1))) { 
        if(--timeout == 0) return 4095; 
    }
    
    return (int)ADC1->DR;          // 0(신호 강함) ~ 4095(신호 없음)
}

// --- 2. 16MHz 클럭 전용 38kHz PWM 초기화 (PA0, PA1) ---
void PWM_38kHz_Init(void)
{
    // 1. 클럭 활성화
    RCC->AHB1ENR |= (1 << 0);   // GPIOA
    RCC->APB1ENR |= (1 << 0);   // TIM2
    
    // 2. PA0, PA1을 AF 모드로 (이미 되어있겠지만 확실히)
    GPIOA->MODER &= ~((3 << 0*2) | (3 << 1*2));
    GPIOA->MODER |=  ((2 << 0*2) | (2 << 1*2));
    
    // 3. AF1 (TIM2) 연결
    GPIOA->AFR[0] &= ~((0xF << 0) | (0xF << 4));
    GPIOA->AFR[0] |=  ((1 << 0) | (1 << 4));

    // 4. 타이머 상세 설정 (16MHz / 38000 = 421.05)
    TIM2->CR1 &= ~(1 << 0);     // 일단 정지
    TIM2->PSC = 0;              // 분주 없음
    TIM2->ARR = 420;            // 주기 설정
    TIM2->CNT = 0;              // 카운터 초기화

    // 5. 출력 모드 설정 (PWM Mode 1)
    TIM2->CCMR1 = (6 << 4) | (6 << 12); // CH1, CH2 PWM1
    TIM2->CCER |= (1 << 0) | (1 << 4);   // 신호 출력 허용
    
    TIM2->CR1 |= (1 << 0);      // 타이머 시작
}

// --- 3. 시스템 초기화 ---
void Sys_Init(int baud) 
{
    // FPU 및 클럭 설정
    SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2); 
    RCC->CR |= (1 << 0); 
    while(!(RCC->CR & (1 << 1)));
    RCC->CFGR = 0; 

    Uart2_Init(baud);      
    ADC_Init();            
    PWM_38kHz_Init();      
    
    setvbuf(stdout, NULL, _IONBF, 0);
}

// --- 4. 메인 루프 (거리 측정 및 방향 판정) ---
void Main(void)
{
    Sys_Init(38400); 
    
    printf("\n[16MHz Mode: 1838 IR Tracker Ready]\n");
    printf("Target ARR: 420 (38kHz)\n");

    for(;;)
    {
        // [Step 1] LEFT LED (PA0) 38kHz 발사
        TIM2->CCR1 = 210;                    // Duty 50%로 발사
        for(volatile int i=0; i<30000; i++);  // 센서 응답 대기
        int val1 = Get_ADC_Value();
        TIM2->CCR1 = 0;                      // 끄기
        
        for(volatile int i=0; i<60000; i++);  // 잔상 제거 대기

        // [Step 2] RIGHT LED (PA1) 38kHz 발사
        TIM2->CCR2 = 210;                    // Duty 50%로 발사
        for(volatile int i=0; i<30000; i++);
        int val2 = Get_ADC_Value();
        TIM2->CCR2 = 0;                      // 끄기

        // [Step 3] 결과 출력 및 판정
        // 1838 수신기는 감지되면 값이 4095에서 아래로 떨어집니다.
        printf("L:%d | R:%d -> ", val1, val2);

        // 감지 문턱값 설정 (보통 3500 이하로 떨어지면 감지된 것)
        if (val1 < 3000 && val1 < val2 - 400) {
            printf("<<< LEFT is closer\n");
        } 
        else if (val2 < 3000 && val2 < val1 - 400) {
            printf("RIGHT is closer >>>\n");
        }
        else {
            printf("Balanced / No Object\n");
        }

        // 전체 루프 속도 조절
        for(volatile int k=0; k<1500000; k++);
    }
}