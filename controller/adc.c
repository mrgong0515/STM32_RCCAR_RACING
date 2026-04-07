#include "device_driver.h"

void ADC1_Init(void)
{
    // 1. GPIOA 클럭 활성화 및 핀 설정 (PA0, PA1)
    Macro_Set_Bit(RCC->AHB1ENR, 0); 
    // PA0(IN0, VRx)와 PA1(IN1, VRy)을 아날로그 입력 모드(11)로 설정
    Macro_Write_Block(GPIOA->MODER, 0xF, 0xF, 0);

    // 2. ADC1 장치 클럭 활성화
    Macro_Set_Bit(RCC->APB2ENR, 8); 
    
    // 3. ADC 공통 클럭 분주비 설정 (PCLK2/6 = 16MHz)
    Macro_Write_Block(ADC->CCR, 0x3, 0x2, 16); 
    
    // 4. 채널별 샘플링 시간 설정
    // SMPR2 레지스터의 0~5번 비트를 1로 채웁니다.
    Macro_Write_Block(ADC1->SMPR2, 0x3F, 0x3F, 0); 

    // 5. 시퀀스 길이 설정 (1개 채널씩만 변환하도록 설정)
    Macro_Write_Block(ADC1->SQR1, 0xF, 0x0, 20); 

    // 6. ADC1 장치 켜기
    Macro_Set_Bit(ADC1->CR2, 0); 
}

int ADC1_Read(int channel)
{
    // 1. 변환할 채널 번호를 시퀀스 첫 번째 자리에 등록
    Macro_Write_Block(ADC1->SQR3, 0x1F, channel, 0); 

    // 2. 소프트웨어 변환 시작 트리거 작동
    Macro_Set_Bit(ADC1->CR2, 30); 

    // 3. 변환 완료(EOC) 깃발이 올라올 때까지 대기
    while(Macro_Check_Bit_Clear(ADC1->SR, 1)); 

    // 4. 변환된 12비트(0~4095) 결과값 반환
    return (ADC1->DR & 0xFFFF); 
}