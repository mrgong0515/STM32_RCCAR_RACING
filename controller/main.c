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
    
    // 20ms 주기 타이머 시작
    SysTick_Run(20); 
    
    int x, y, sw, dir;
    char msg[16];
    
    for(;;)
    {
        // 1. 20ms 주기로 조이스틱 데이터 처리 및 송신
        if(SysTick_Check_Timeout())
        {
            x = Joystick_Get_X();
            y = Joystick_Get_Y();
            sw = Joystick_Get_SW();
            
            dir = Get_Joystick_Direction(x, y);
            
            // 패킷 생성: S + {방향} + {버튼} + \n
            // 예: S80\n (전진, 버튼안눌림)
            sprintf(msg, "S%d%d\n", dir, sw);
            
            // 블루투스(Uart1)로 전송
            Uart1_Send_String(msg);
            printf(msg);
            
            // LCD 출력 업데이트 (첫 번째 줄은 고정, 두 번째 줄에 상태 표시)
            LCD_Send_Cmd(0xC0); // 2번째 줄 이동
            sprintf(msg, "DIR:%d BTN:%d    ", dir, sw);
            LCD_Print_String(msg);
        }

        // 2. 차량으로부터 오는 피드백 데이터 수신
        if(Macro_Check_Bit_Set(USART1->SR, 5)) 
        {
            char rx_data = (char)USART1->DR;
            // 수신 데이터를 디버그용 터미널(Uart2)로 전달
            Uart2_Send_Byte(rx_data); 
        }

        // 전송 및 화면 갱신 주기 (너무 빠르면 눈이 아프니 적절히 조절)
        for(volatile int i=0; i<800000; i++); 
    }
}

// 조이스틱 값을 1~9 방향으로 매핑하는 함수
int Get_Joystick_Direction(int x, int y) 
{
    int raw_dx = 0; 
    int raw_dy = 0; 

    // X축 원시 데이터 분석 (데드존 기준 1748 ~ 2348)
    if (x > 2348) raw_dx = 1;
    else if (x < 1748) raw_dx = -1;

    // Y축 원시 데이터 분석
    if (y > 2348) raw_dy = 1;
    else if (y < 1748) raw_dy = -1;

    // 90도 시계 방향 회전 변환 적용
    // 기존의 왼쪽(-1, 0) 입력이 위쪽(0, 1)으로 매핑되도록 축 교환 및 부호 반전
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