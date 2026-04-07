#include "device_driver.h"
#include <stdio.h>

void _Invalid_ISR(void)
{
	unsigned int r = Macro_Extract_Area(SCB->ICSR, 0x1ff, 0);
	printf("\nInvalid_Exception: %d!\n", r);
	printf("Invalid_ISR: %d!\n", r - 16);
	for(;;);
}

extern volatile int Key_Pressed;

void EXTI15_10_IRQHandler(void)
{
	Key_Pressed = 1;
	
	EXTI->PR = 0x1 << 13;
	NVIC_ClearPendingIRQ(40);
}

extern volatile int Uart_Data_In;
extern volatile unsigned char Uart_Data;

void USART2_IRQHandler(void)
{
	// 수신된 데이터는 Uart_Data에 저장
	// Uart_Data_In Flag Setting
	if(Macro_Check_Bit_Set(USART2->SR,5)){
		Uart_Data = (unsigned char)(USART2->DR & 0xFF);
		Uart_Data_In = 1;
	}
	// NVIC Pending Clear
	NVIC_ClearPendingIRQ((IRQn_Type)38);
}

// main.c에서 사용할 전역 변수 선언
volatile int RC_Packet_Ready = 0;
volatile char RC_Joy_Val = '5';
volatile char RC_Btn_Val = '0';

void USART1_IRQHandler(void)
{
    static int state = 0;
    static char temp_joy = '5';
    static char temp_btn = '0';

    // 수신 데이터가 있는지 확인
    if(Macro_Check_Bit_Set(USART1->SR, 5))
    {
        char recv = (char)(USART1->DR & 0xFF);

        // 프로토콜 해석 (S80\n 형태)
        if (recv == 'S') {
            state = 1;
        } 
        else if (state == 1) {
            temp_joy = recv;
            state = 2;
        } 
        else if (state == 2) {
            temp_btn = recv;
            state = 3;
        } 
        else if (state == 3 && (recv == '\n' || recv == '\r')) {
            // 패킷 수신이 완료되면 전역 변수에 값을 복사하고 플래그를 세팅합니다.
            RC_Joy_Val = temp_joy;
            RC_Btn_Val = temp_btn;
            RC_Packet_Ready = 1; 
            state = 0;
        }
    }

    // 인터럽트 펜딩 비트 클리어
    NVIC_ClearPendingIRQ((IRQn_Type)37);
}
