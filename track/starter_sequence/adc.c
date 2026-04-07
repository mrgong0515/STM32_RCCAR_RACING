#include "device_driver.h"

// 1. 초기화: PA6, PA7 두 채널을 모두 사용할 준비를 합니다.
void ADC1_2CH_Init(void)
{
    // GPIOA 전원 ON 및 PA6, PA7 아날로그 모드 설정
    Macro_Set_Bit(RCC->AHB1ENR, 0); 
    Macro_Write_Block(GPIOA->MODER, 0xF, 0xF, 12);  // PA6(12,13), PA7(14,15) = Analog

    // ADC1 전원 ON 및 샘플링 타임 설정
    Macro_Set_Bit(RCC->APB2ENR, 8); 
    Macro_Write_Block(ADC1->SMPR2, 0x3F, 0x3F, 18); // CH6, CH7 모두 480 Cycles

    // 변환 시퀀스 개수 = 1개 (한 번에 하나씩 채널을 바꿔가며 읽을 예정)
    Macro_Write_Block(ADC1->SQR1, 0xF, 0x0, 20); 

    // ADC 클럭 설정 및 ON
    Macro_Write_Block(ADC->CCR, 0x3, 0x2, 16); 
    Macro_Set_Bit(ADC1->CR2, 0); 
}

// 2. 채널 선택 함수: 읽기 전에 채널 번호를 지정합니다. (6 또는 7)
void ADC1_Select_Channel(int ch)
{
    Macro_Write_Block(ADC1->SQR3, 0x1F, ch, 0); // SQ1(첫 번째 순서)에 채널 번호 기입
}

// 3. 변환 시작
void ADC1_Start(void)
{
    Macro_Set_Bit(ADC1->CR2, 30); 
}

// 4. 상태 확인 (EOC 플래그 체크 및 클리어)
int ADC1_Get_Status(void)
{
    int r = Macro_Check_Bit_Set(ADC1->SR, 1);
    if(r)
    {
        Macro_Clear_Bit(ADC1->SR, 1);
        Macro_Clear_Bit(ADC1->SR, 4); // STRT 비트도 같이 클리어
    }
    return r;
}

// 5. 데이터 읽기
int ADC1_Get_Data(void)
{
    return (int)(ADC1->DR & 0xFFF);
}

// 6. ADC 정지
void ADC1_Stop(void)
{
    Macro_Clear_Bit(ADC1->CR2, 30); 
    Macro_Clear_Bit(ADC1->CR2, 0); 
}