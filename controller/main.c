#include "device_driver.h"
#include <stdio.h>

// 조이스틱 판단 기준 (중앙 2048 기준)
#define JOY_CENTER 2048
#define THRESHOLD  1200  

void Sys_Init_Controller(int baud) 
{
    SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2); 
    RCC->CR |= (1 << 0); 
    while(!(RCC->CR & (1 << 1)));
    RCC->CFGR = 0; 

    Uart2_Init(baud);      
    Uart1_Init(baud);      
    ADC1_2CH_Init(); // PA6(X), PA7(Y) 초기화

    setvbuf(stdout, NULL, _IONBF, 0);
}

void Main(void)
{
    Sys_Init_Controller(38400); 
    
    printf("\n==========================================\n");
    printf("   RC Car Controller & Monitor Ready      \n");
    printf("   Format: [X:xxxx, Y:yyyy] -> Direction  \n");
    printf("==========================================\n");

    int x_val, y_val;
    char dir_char = ' '; // 현재 방향 저장용

    for(;;)
    {
        // 1. 조이스틱 ADC 값 읽기
        ADC1_Select_Channel(6);
        ADC1_Start();
        while(!ADC1_Get_Status());
        x_val = ADC1_Get_Data();

        ADC1_Select_Channel(7);
        ADC1_Start();
        while(!ADC1_Get_Status());
        y_val = ADC1_Get_Data();

        // 2. 방향 판별 (중첩되지 않게 초기화)
        dir_char = ' '; 

        // X축 판별 (좌/우)
        if (x_val > (JOY_CENTER + THRESHOLD))      dir_char = 'R';
        else if (x_val < (JOY_CENTER - THRESHOLD)) dir_char = 'L';
        
        // Y축 판별 (상/하) - X축보다 우선순위를 두거나 별도로 전송 가능
        if (y_val > (JOY_CENTER + THRESHOLD))      dir_char = 'U';
        else if (y_val < (JOY_CENTER - THRESHOLD)) dir_char = 'D';

        // 3. 블루투스로 문자 전송 (방향이 있을 때만)
        if (dir_char != ' ') {
            Uart1_Send_Byte(dir_char);
        }

        // 4. 테라텀 모니터링 출력
        // \r을 사용해 한 줄에서 값이 계속 업데이트되게 합니다.
        printf("\r[X:%4d, Y:%4d] Send: %c    ", x_val, y_val, dir_char);

        // 5. 블루투스 수신 데이터 확인
        if(Macro_Check_Bit_Set(USART1->SR, 5)) 
        {
            char bt_data = (char)USART1->DR;
            // 수신 데이터가 있으면 줄을 바꿔서 출력
            printf("\n[BT RX]: %c\n", bt_data); 
        }

        // 전송 및 화면 갱신 주기 (너무 빠르면 눈이 아프니 적절히 조절)
        for(volatile int i=0; i<800000; i++); 
    }
}