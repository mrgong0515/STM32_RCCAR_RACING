/**
 * @file exception.c
 * @brief RC카 수신부 인터럽트 서비스 루틴(ISR) 및 예외 처리
 * @details 외부 인터럽트 및 UART 통신(USART1, USART2) 수신 인터럽트를 처리하여
 * 컨트롤러로부터 전달되는 제어 패킷을 파싱하고 메인 루프에 상태를 전달합니다.
 */

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

/**
 * @brief USART2 수신 인터럽트 서비스 루틴 (디버깅용)
 * @details PC와 연결된 USART2 통신 라인을 통해 데이터가 수신될 때 호출됩니다.
 * 수신된 1바이트 데이터를 전역 변수에 저장하고 데이터 수신 플래그를 세팅합니다.
 */
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

/**
 * @brief USART1 수신 인터럽트 서비스 루틴 (블루투스 제어 명령 수신)
 * @details 컨트롤러로부터 전송되는 조이스틱 및 버튼 상태 패킷(예: S80\n)을 
 * 상태 머신(State Machine) 방식으로 한 바이트씩 해석합니다. 
 * 완전한 패킷이 수신되면 전역 변수에 값을 복사하고 패킷 준비 완료 플래그를 세팅합니다.
 */
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
