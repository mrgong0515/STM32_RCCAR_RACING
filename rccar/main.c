#include "device_driver.h"
#include <stdio.h>

// exception.c에 선언된 전역 변수들 참조
extern volatile int RC_Packet_Ready;
extern volatile char RC_Joy_Val;
extern volatile char RC_Btn_Val;

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
            Buzzer_Control(RC_Btn_Val);
            
            // 정상적인 제어 명령을 받았으므로, 타임아웃 타이머를 다시 500ms로 연장(리셋)합니다.
            TIM4_Repeat(500); 
        }
        
        // 500ms 동안 패킷이 들어오지 않아 타이머가 만료되었다면 (통신 단절 상태)
        if(TIM4_Check_Timeout())
        {
            printf(" [Warning] Signal Lost! Force Stop.\n");
            
            // 차량을 강제로 중립/정지 시키고 부저를 끕니다.
            Control_Motor_By_Joystick('5'); 
            Buzzer_Control('0');            
            
            // 정지 상태를 계속 유지하며 감시하기 위해 타임아웃 타이머를 재시작합니다.
            TIM4_Repeat(500);
        }
    }
}