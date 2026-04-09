#include "device_driver.h"
#include <stdio.h>

// --- [매크로 기반 핀 제어] ---
// 초음파 센서 (1P: PA10/PB5, 2P: PA8/PA9)
#define TRIG1_HIGH()   Macro_Set_Bit(GPIOA->ODR, 10)
#define TRIG1_LOW()    Macro_Clear_Bit(GPIOA->ODR, 10)
#define ECHO1_PIN      Macro_Check_Bit_Set(GPIOB->IDR, 5)
#define TRIG2_HIGH()   Macro_Set_Bit(GPIOA->ODR, 8)
#define TRIG2_LOW()    Macro_Clear_Bit(GPIOA->ODR, 8)
#define ECHO2_PIN      Macro_Check_Bit_Set(GPIOA->IDR, 9)

unsigned char font_P[8] = {0x3C, 0x22, 0x22, 0x3C, 0x20, 0x20, 0x20, 0x00};
unsigned char font_1[8] = {0x08, 0x18, 0x28, 0x08, 0x08, 0x08, 0x08, 0x00};
unsigned char font_2[8] = {0x3C, 0x22, 0x02, 0x0C, 0x10, 0x20, 0x3E, 0x00};
unsigned char font_W[8] = {0x22, 0x22, 0x22, 0x22, 0x2A, 0x2A, 0x14, 0x00};
unsigned char font_I[8] = {0x1C, 0x08, 0x08, 0x08, 0x08, 0x08, 0x1C, 0x00};
unsigned char font_N[8] = {0x22, 0x32, 0x2A, 0x26, 0x22, 0x22, 0x22, 0x00};
unsigned char font_blank[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


// --- [함수 선언: max7219.c에 있는 함수들을 쓰겠다고 선언] ---
void GPIO_Init_Dual(void);
void MAX7219_Init_Dual(int player);
void MAX7219_SendOne(int player, int module, unsigned char addr, unsigned char data);
void MAX7219_ClearAll(int player);
// --- 시스템 초기화 ---
void Sys_Init(int baud) 
{
    // FPU 및 기본 클럭 설정
    SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2); 
    RCC->CR |= (1 << 0); 
    while(!(RCC->CR & (1 << 1)));
    RCC->CFGR = 0; 

    // AHB1 클럭 활성화 (구조체 및 표준 비트 명칭 사용)
    RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN);

    Uart2_Init(baud);      
    setvbuf(stdout, NULL, _IONBF, 0);

    // GPIO 모드 설정 (GPIOA->MODER, GPIOB->MODER 직접 사용)
    // 1P: PA10(Output), PB5(Input)
    Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, 10*2); // PA10 -> Output(01)
    Macro_Clear_Area(GPIOB->MODER, 0x3, 5*2);        // PB5 -> Input(00)

    // 2P: PA8(Output), PA9(Input)
    Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, 8*2);  // PA8 -> Output(01)
    Macro_Clear_Area(GPIOA->MODER, 0x3, 9*2);        // PA9 -> Input(00)
}

// --- [함수 1] 1번 센서용 측정 ---
float Get_Distance_1P(void) {
    uint32_t count = 0, timeout = 0;

    TRIG1_LOW(); for(volatile int i=0; i<10; i++);
    TRIG1_HIGH(); for(volatile int i=0; i<50; i++);
    TRIG1_LOW();

    // 레지스터 주소 대신 GPIOB->IDR 사용
    while(Macro_Check_Bit_Clear(GPIOB->IDR, 5)) { 
        if(++timeout > 50000) return -1.0f;
    }
    while(Macro_Check_Bit_Set(GPIOB->IDR, 5)) {   
        count++;
        if(count > 100000) break;
    }
    return count * 0.017f;
}

// --- [함수 2] 2번 센서용 측정 ---
float Get_Distance_2P(void) {
    uint32_t count = 0, timeout = 0;

    TRIG2_LOW(); for(volatile int i=0; i<10; i++);
    TRIG2_HIGH(); for(volatile int i=0; i<50; i++);
    TRIG2_LOW();

    // 레지스터 주소 대신 GPIOA->IDR 사용
    while(Macro_Check_Bit_Clear(GPIOA->IDR, 9)) { 
        if(++timeout > 50000) return -1.0f;
    }
    while(Macro_Check_Bit_Set(GPIOA->IDR, 9)) {   
        count++;
        if(count > 100000) break;
    }
    return count * 0.017f;
}

void Display_Pattern(int player, int module, unsigned char *pattern) {
    for (int row = 0; row < 8; row++) {
        MAX7219_SendOne(player, module, row + 1, pattern[row]);
    }
}

void Main(void) {
    GPIO_Init_Dual(); 
    Sys_Init(38400);
    MAX7219_Init_Dual(1);
    MAX7219_Init_Dual(2);

    // [평상시 대기 화면] 
    // Player 1: "P 1" 표시 (모듈 0에 P, 모듈 1에 1)
    Display_Pattern(1, 0, font_P);
    Display_Pattern(1, 1, font_1);
    // Player 2: "P 2" 표시 (모듈 0에 P, 모듈 1에 2)
    Display_Pattern(2, 0, font_P);
    Display_Pattern(2, 1, font_2);

    printf("\nWait for Race... (Threshold: %d)\n", 350);

    int winner = 0;
    while (1) {
        float d1_f = Get_Distance_1P();
        for(volatile int i=0; i<30000; i++); 
        float d2_f = Get_Distance_2P();

        int d1 = (int)(d1_f * 10.0f);
        int d2 = (int)(d2_f * 10.0f);

        printf("\r1P: %d | 2P: %d   ", d1, d2);

        if (d1 > 1 && d1 < 350) { winner = 1; break; }
        if (d2 > 1 && d2 < 350) { winner = 2; break; }
    }

    // [승자 결정 후 화면 처리]
    printf("\n\n*** WINNER: PLAYER %d ***\n", winner);
    
    // 둘 다 화면을 지우고 승리 메시지 출력
    MAX7219_ClearAll(1);
    MAX7219_ClearAll(2);

    if (winner == 1) {
        Display_Pattern(1, 0, font_1);
        Display_Pattern(1, 1, font_W);
        Display_Pattern(1, 2, font_I);
        Display_Pattern(1, 3, font_N); // N까지 넣으려면 모듈이 더 필요하므로 적절히 배분
        
        Display_Pattern(2, 0, font_1);
        Display_Pattern(2, 1, font_W);
        Display_Pattern(2, 2, font_I);
        Display_Pattern(2, 3, font_N);
    } 
    else {
        Display_Pattern(1, 0, font_2);
        Display_Pattern(1, 1, font_W);
        Display_Pattern(1, 2, font_I);
        Display_Pattern(1, 3, font_N); // N까지 넣으려면 모듈이 더 필요하므로 적절히 배분
        
        Display_Pattern(2, 0, font_2);
        Display_Pattern(2, 1, font_W);
        Display_Pattern(2, 2, font_I);
        Display_Pattern(2, 3, font_N);
    }

    while (1);
}