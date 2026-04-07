#include "device_driver.h"
#include <stdio.h>

void Sys_Init(int baud) 
{
    SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2); 
    RCC->CR |= (1 << 0); 
    while(!(RCC->CR & (1 << 1)));
    RCC->CFGR = 0; 

    Uart2_Init(baud);      // PC 디버깅용 (USB)
    Uart1_Init(baud);      // 블루투스 컨트롤러 수신용 (PA9, PA10)
    
    Motor_Init(); 
    
    setvbuf(stdout, NULL, _IONBF, 0);
}

void Main(void)
{
    Sys_Init(38400);
    
    printf("\n==================================\n");
    printf("   Bluetooth Controller Mode      \n");
    printf("   Listening on USART1 (PA9/10)   \n");
    printf("==================================\n");
    
    int state = 0;
    char joy_val = '5';
    char btn_val = '0';

    for(;;)
    {
        // USART1(블루투스)에서 데이터를 수신합니다.
        if(Macro_Check_Bit_Set(USART1->SR, 5)) 
        {
            char recv = (char)USART1->DR;

            // 실시간 수신 데이터 테라텀에 표시 (디버깅용)
            if(recv >= 32) printf("%c", recv); 

            // 프로토콜 해석 (S50\n)
            if (recv == 'S') {
                state = 1;
            } 
            else if (state == 1) {
                joy_val = recv;
                state = 2;
            } 
            else if (state == 2) {
                btn_val = recv;
                state = 3;
            } 
            else if (state == 3 && (recv == '\n' || recv == '\r')) {
                printf(" [BT Packet OK] Joy:%c Btn:%c\n", joy_val, btn_val);
                
                // 분리된 모터 드라이버의 함수를 호출하여 PWM 속도 및 방향 제어
                Control_Motor_By_Joystick(joy_val);
                
                state = 0;
            }
        }
    }
}