#include "device_driver.h"

void Joystick_Init(void)
{
    // 1. X축, Y축을 위한 ADC 장치 초기화 호출
    ADC1_Init();

    // 2. Z축 버튼(SW)을 위한 PB5 핀 초기화 (Arduino 커넥터 D4)
    // GPIOB 클럭 활성화 (비트 1)
    Macro_Set_Bit(RCC->AHB1ENR, 1); 
    
    // PB5를 입력 모드(00)로 설정 (핀 번호 5 * 2 = 10번 비트 위치)
    Macro_Write_Block(GPIOB->MODER, 0x3, 0x0, 10); 
    
    // PB5에 내부 풀업(Pull-up) 저항 연결 (01)
    Macro_Write_Block(GPIOB->PUPDR, 0x3, 0x1, 10); 
}

int Joystick_Get_X(void)
{
    // X축이 연결된 PA0 핀(채널 0)의 아날로그 값을 읽어 반환
    return ADC1_Read(0); 
}

int Joystick_Get_Y(void)
{
    // Y축이 연결된 PA1 핀(채널 1)의 아날로그 값을 읽어 반환
    return ADC1_Read(1); 
}

int Joystick_Get_SW(void)
{
    // PB5 버튼이 눌렸을 때 GND와 연결되어 0(Low)이 되므로, 이를 반전시켜 1로 반환
    return Macro_Check_Bit_Clear(GPIOB->IDR, 5);
}