/**
 * @file adc.c
 * @brief ADC(아날로그-디지털 변환) 제어 드라이버
 * @details STM32F411 MCU의 ADC1 장치를 이용하여 2개의 채널(PA6, PA7)에서 
 * 아날로그 신호를 읽어오는 기능을 제공합니다. (예: 조이스틱 X/Y축 값 읽기 등)
 * 한 번에 하나의 채널을 순차적으로 선택하여 값을 읽는 단일 변환 방식을 사용합니다.
 */

#include "device_driver.h"

/**
 * @brief 2채널(PA6, PA7) 아날로그 입력용 ADC1 초기화
 * @details GPIOA 6번, 7번 핀을 아날로그 입력 모드로 설정하고, 
 * 안정적인 변환을 위해 샘플링 타임을 480 사이클로 넉넉하게 설정합니다.
 */
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

/**
 * @brief 아날로그 값을 읽을 ADC 채널 선택
 * @details 시퀀스 레지스터(SQR3)의 첫 번째 순서에 읽고자 하는 채널 번호를 기입합니다.
 * @param ch 아날로그 값을 읽어올 채널 번호 (PA6의 경우 6, PA7의 경우 7)
 */
void ADC1_Select_Channel(int ch)
{
    Macro_Write_Block(ADC1->SQR3, 0x1F, ch, 0); // SQ1(첫 번째 순서)에 채널 번호 기입
}

/**
 * @brief 선택된 채널의 ADC 변환 시작
 * @details 제어 레지스터의 SWSTART 비트를 세팅하여 아날로그-디지털 변환을 개시합니다.
 */
void ADC1_Start(void)
{
    Macro_Set_Bit(ADC1->CR2, 30); 
}

/**
 * @brief ADC 변환 완료 상태 확인 및 플래그 초기화
 * @details 변환이 완료되었음을 알리는 EOC(End Of Conversion) 플래그를 확인합니다.
 * @return int 변환이 완료되었으면 1, 진행 중이면 0 반환
 */
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

/**
 * @brief 변환이 완료된 디지털 데이터 읽기
 * @details 데이터 레지스터(DR)에서 하위 12비트의 변환 결과를 읽어옵니다.
 * @return int 12비트 해상도로 변환된 아날로그 디지털 값 (0 ~ 4095)
 */
int ADC1_Get_Data(void)
{
    return (int)(ADC1->DR & 0xFFF);
}

/**
 * @brief ADC 장치 비활성화 및 정지
 * @details 진행 중인 변환을 강제로 정지시키고 ADC 하드웨어를 끕니다.
 */
void ADC1_Stop(void)
{
    Macro_Clear_Bit(ADC1->CR2, 30); 
    Macro_Clear_Bit(ADC1->CR2, 0); 
}