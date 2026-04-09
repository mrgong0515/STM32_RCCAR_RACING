/**
 * @file main.c
 * @brief RC카 차량 제어용 메인 수신 애플리케이션
 * @details 블루투스(USART1) 인터럽트를 통해 컨트롤러의 조이스틱 및 버튼 상태 패킷을 수신하고 모터를 제어합니다.
 * 일정 시간 통신이 끊길 경우 차량의 폭주를 막기 위해 강제로 정지시키는 페일세이프(Fail-Safe) 로직이 포함되어 있습니다.
 */

#include "device_driver.h"
#include <stdio.h>

// exception.c에 선언된 전역 변수들 참조
extern volatile int RC_Packet_Ready;
extern volatile char RC_Joy_Val;
extern volatile char RC_Btn_Val;

/**
 * @brief 시스템 클럭 및 하드웨어 주변장치 초기화
 * @details FPU 및 16MHz 클럭을 설정하고 PC 디버깅용 UART2와 블루투스 통신용 UART1을 초기화합니다.
 * 패킷의 비동기 수신을 위해 UART1의 수신 인터럽트를 활성화하며, 모터 제어용 핀을 초기화합니다.
 * @param baud UART 통신에 사용할 보드레이트 (예: 38400)
 */
void Sys_Init(int baud) 
{
    SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2); 
    RCC->CR |= (1 << 0); 
    while(!(RCC->CR & (1 << 1)));
    RCC->CFGR = 0; 

    Uart2_Init(baud);      
    Uart1_Init(baud);      
    
    // USART1 수신 인터럽트 활성화
    Uart1_RX_Interrupt_Enable(1);
    
    Motor_Init();      
    
    setvbuf(stdout, NULL, _IONBF, 0);
}

/**
 * @brief 메인 프로그램 루프
 * @details 통신 단절을 감지하기 위한 500ms 타임아웃 타이머를 운용합니다.
 * 인터럽트를 통해 정상적인 제어 패킷이 수신되면 모터를 구동하고 타임아웃 타이머를 리셋(연장)합니다.
 * 500ms 동안 패킷이 들어오지 않아 타이머가 만료되면 연결이 끊긴 것으로 판단하여 모터를 중립 상태(5)로 강제 전환합니다.
 */
void Main(void)
{
    Sys_Init(38400);
    
    printf("\n==================================\n");
    printf("   Bluetooth Controller Mode      \n");
    printf("   Listening on USART1 (Interrupt)\n");
    printf("==================================\n");

    // 초기화 직후 500ms 타임아웃 감지용 타이머를 시작합니다.
    TIM4_Repeat(500); 

    for(;;)
    {
        // 인터럽트를 통해 정상적인 패킷이 들어왔을 때
        if (RC_Packet_Ready)
        {
            RC_Packet_Ready = 0;

            Control_Motor_By_Joystick(RC_Joy_Val);
            
            // 정상적인 제어 명령을 받았으므로, 타임아웃 타이머를 다시 500ms로 연장(리셋)합니다.
            TIM4_Repeat(500); 
        }
        
        // 500ms 동안 패킷이 들어오지 않아 타이머가 만료되었다면 (통신 단절 상태)
        if(TIM4_Check_Timeout())
        {
            printf(" [Warning] Signal Lost! Force Stop.\n");
            
            // 차량을 강제로 중립/정지 시키고 부저를 끕니다.
            Control_Motor_By_Joystick('5');      
            
            // 정지 상태를 계속 유지하며 감시하기 위해 타임아웃 타이머를 재시작합니다.
            TIM4_Repeat(500);
        }
    }
}