#include "device_driver.h"
#include <stdio.h>

void Motor_GPIO_Init(void)
{
    RCC->AHB1ENR |= (1 << 0); 
    GPIOA->MODER &= ~((3 << 0*2) | (3 << 1*2) | (3 << 4*2) | (3 << 5*2));
    GPIOA->MODER |=  ((1 << 0*2) | (1 << 1*2) | (1 << 4*2) | (1 << 5*2));
}

void Control_Motor_By_Joystick(char joy)
{
    switch(joy)
    {
        case '8': // 전진
            printf(" >> Action: Forward\n");
            GPIOA->ODR = (1 << 0) | (0 << 1) | (1 << 4) | (0 << 5);
            break;
        case '2': // 후진
            printf(" >> Action: Backward\n");
            GPIOA->ODR = (0 << 0) | (1 << 1) | (0 << 4) | (1 << 5);
            break;
        case '4': // 좌회전
            printf(" >> Action: Turn Left\n");
            GPIOA->ODR = (0 << 0) | (0 << 1) | (1 << 4) | (0 << 5);
            break;
        case '6': // 우회전
            printf(" >> Action: Turn Right\n");
            GPIOA->ODR = (1 << 0) | (0 << 1) | (0 << 4) | (0 << 5);
            break;
        case '5': // 중립
        default:
            printf(" >> Action: Stop\n");
            GPIOA->ODR &= ~((1 << 0) | (1 << 1) | (1 << 4) | (1 << 5));
            break;
    }
}

void Sys_Init_Chat(int baud) 
{
    SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2); 
    RCC->CR |= (1 << 0); 
    while(!(RCC->CR & (1 << 1)));
    RCC->CFGR = 0; 

    Uart2_Init(baud);      // PC 디버깅용 (USB)
    Uart1_Init(baud);      // 블루투스 컨트롤러 수신용 (PA9, PA10)
    Motor_GPIO_Init();
    
    setvbuf(stdout, NULL, _IONBF, 0);
}

void Main(void)
{
    Sys_Init_Chat(38400); // 블루투스와 보레이트가 맞는지 꼭 확인! (보통 9600 혹은 38400)
    
    printf("\n==================================\n");
    printf("   Bluetooth Controller Mode      \n");
    printf("   Listening on USART1 (PA9/10)   \n");
    printf("==================================\n");
    
    int state = 0;
    char joy_val = '5';
    char btn_val = '0';

    for(;;)
    {
        // 핵심 변경: USART2가 아니라 USART1(블루투스)에서 데이터를 기다립니다.
        if(Macro_Check_Bit_Set(USART1->SR, 5)) 
        {
            char recv = (char)USART1->DR;

            // 실시간 수신 데이터 테라텀에 표시 (디버깅용)
            if(recv >= 32) printf("%c", recv); 

            // 프로토콜 해석 (S80\n)
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
                printf(" [BT Packet OK] Joy:%c Btn:%c", joy_val, btn_val);
                Control_Motor_By_Joystick(joy_val);
                state = 0;
            }
        }
    }
}