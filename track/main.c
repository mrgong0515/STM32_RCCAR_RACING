#include "device_driver.h"
#include <stdio.h>

// --- ADC 값 읽기 함수 (본체가 없어서 에러가 났던 부분) ---
int Get_ADC_Value(void)
{
    // 1. 상태 레지스터의 EOC(End of Conversion) 비트 클리어
    ADC1->SR &= ~(1 << 1);        
    
    // 2. 변환 시작 (SWSTART)
    ADC1->CR2 |= (1 << 30);       
    
    // 3. 변환 완료 대기 (Timeout 설정으로 무한 루프 방지)
    uint32_t timeout = 10000;
    while(!(ADC1->SR & (1 << 1))) { 
        if(--timeout == 0) return -1; 
    }
    
    // 4. 결과값 리턴 (0 ~ 4095)
    return (int)ADC1->DR;         
}

void ADC_Init(void)
{
    RCC->AHB1ENR |= (1 << 0);  // GPIOA 클럭
    RCC->APB2ENR |= (1 << 8);  // ADC1 클럭
    
    GPIOA->MODER |= (3 << 6*2); // PA6 아날로그 모드
    
    ADC1->CR2 &= ~(1 << 0);     // ADON 끄기
    ADC1->SQR3 = 6;             // 6번 채널(PA6) 선택
    ADC1->CR2 |= (1 << 0);      // ADON 켜기
}

void Sys_Init(int baud) 
{
    SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2); 
    RCC->CR |= (1 << 0); 
    while(!(RCC->CR & (1 << 1)));
    RCC->CFGR = 0; 

    Uart2_Init(baud);     
    Uart1_Init(baud);
    ADC_Init();            // PA6 초기화 (adc.c에 이 함수는 있나요?)
    
    setvbuf(stdout, NULL, _IONBF, 0);
}

void Main(void)
{
    Sys_Init(38400); 
    
    // IR LED 제어용 GPIOA 초기화 (PA0, PA1)
    RCC->AHB1ENR |= (1 << 0); 
    GPIOA->MODER &= ~((3 << 0*2) | (3 << 1*2));
    GPIOA->MODER |=  ((1 << 0*2) | (1 << 1*2));

    printf("\n[IR Distance Comparison Start]\n");

    for(;;)
    {
        // LED 1 측정 (PA0)
        GPIOA->ODR |= (1 << 0);
        GPIOA->ODR &= ~(1 << 1);
        for(volatile int i=0; i<100000; i++); // 충분히 대기
        int val1 = Get_ADC_Value();

        // LED 2 측정 (PA1)
        GPIOA->ODR &= ~(1 << 0);
        GPIOA->ODR |= (1 << 1);
        for(volatile int i=0; i<100000; i++);
        int val2 = Get_ADC_Value();

        // 결과 출력
        printf("LED1: %4d | LED2: %4d -> ", val1, val2);
        
        if (val1 > val2 + 150) printf("LEFT is closer\n");
        else if (val2 > val1 + 150) printf("RIGHT is closer\n");
        else printf("SAME distance\n");

        for(volatile int k=0; k<1500000; k++); 
    }
}