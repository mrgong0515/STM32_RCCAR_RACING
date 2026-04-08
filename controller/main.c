#include "device_driver.h"
#include <stdio.h>

int Get_Joystick_Direction(int x, int y);

void Sys_Init(int baud) 
{
    // FPU 및 클럭 설정
    SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2); 
    RCC->CR |= (1 << 0); 
    while(!(RCC->CR & (1 << 1)));
    RCC->CFGR = 0; 

    // 하드웨어 초기화
    Uart2_Init(baud);      // 디버그용
    Uart1_Init(baud);      // HC-05 블루투스용
    I2C1_Init();           // LCD 통신용
    LCD_Init();            // LCD 모듈 초기화
    Joystick_Init();       // 조이스틱(ADC 및 PB5) 초기화
    
    setvbuf(stdout, NULL, _IONBF, 0);
}

void Main(void)
{
    Sys_Init(38400); 
    
    LCD_Send_Cmd(0x01); // 화면 초기화
    TIM2_Delay(2);
    LCD_Print_String("RC Controller");
    
    // 20ms 주기 타이머 시작 (ADC 읽기 주기 유지)
    SysTick_Run(20); 
    
    int x, y, sw, dir;
    int prev_dir = -1; 
    int prev_sw = -1;  
    int heartbeat_cnt = 0; // 주기적 전송을 위한 카운터 변수 추가
    char msg[16];
    
    for(;;)
    {
        if(SysTick_Check_Timeout())
        {
            x = Joystick_Get_X();
            y = Joystick_Get_Y();
            sw = Joystick_Get_SW();
            
            dir = Get_Joystick_Direction(x, y);
            
            // 상태가 변했거나, 상태가 변하지 않았어도 10주기(200ms)가 경과했다면 전송
            if(dir != prev_dir || sw != prev_sw || heartbeat_cnt >= 10)
            {
                sprintf(msg, "S%d%d\n", dir, sw);
                Uart1_Send_String(msg);
                
                // LCD 업데이트는 상태가 실제로 변했을 때만 수행하여 불필요한 깜빡임 방지
                if(dir != prev_dir || sw != prev_sw) 
                {
                    LCD_Send_Cmd(0xC0); 
                    sprintf(msg, "DIR:%d BTN:%d    ", dir, sw);
                    LCD_Print_String(msg);
                }
                
                prev_dir = dir;
                prev_sw = sw;
                heartbeat_cnt = 0; // 데이터 전송 후 카운터 리셋
            }
            else
            {
                heartbeat_cnt++; // 상태가 변하지 않았다면 카운터 증가
            }
        }

        if(Macro_Check_Bit_Set(USART1->SR, 5)) 
        {
            char rx_data = (char)USART1->DR;
            Uart2_Send_Byte(rx_data); 
        }
    }
}

int Get_Joystick_Direction(int x, int y) 
{
    int raw_dx = 0; 
    int raw_dy = 0; 

    // X축 원시 데이    터 분석
    if (x > 3000) raw_dx = 1;
    else if (x < 1000) raw_dx = -1;

    // Y축 원시 데이터 분석
    if (y > 3000) raw_dy = 1;
    else if (y < 1000) raw_dy = -1;

    // 방향 맞추기
    int dx = raw_dy;
    int dy = -raw_dx;

    // 숫자 키패드 방식 매핑 (789 / 456 / 123)
    if (dy == 1) { // 전진 라인
        if (dx == -1) return 7;
        if (dx == 1) return 9;
        return 8;
    } 
    else if (dy == -1) { // 후진 라인
        if (dx == -1) return 1;
        if (dx == 1) return 3;
        return 2;
    } 
    else { // 중립 라인
        if (dx == -1) return 4;
        if (dx == 1) return 6;
        return 5;
    }
}