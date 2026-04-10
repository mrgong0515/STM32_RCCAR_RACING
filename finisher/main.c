/**
 * @file main.c
 * @brief RC카 레이싱 피니셔 보드 메인 애플리케이션
 * @details 2개의 초음파 센서를 이용하여 1P와 2P 차량의 결승선 통과 여부를 감지하고,
 * 통과 순서에 따라 승자를 판별하여 MAX7219 도트 매트릭스 디스플레이에 결과를 출력합니다.
 */

#include "device_driver.h"
#include <stdio.h>

// 초음파 센서 (1P: PA10/PB5, 2P: PA8/PA9)
#define TRIG1_HIGH()   Macro_Set_Bit(GPIOA->ODR, 10)
#define TRIG1_LOW()    Macro_Clear_Bit(GPIOA->ODR, 10)
#define ECHO1_PIN      Macro_Check_Bit_Set(GPIOB->IDR, 5)
#define TRIG2_HIGH()   Macro_Set_Bit(GPIOA->ODR, 8)
#define TRIG2_LOW()    Macro_Clear_Bit(GPIOA->ODR, 8)
#define ECHO2_PIN      Macro_Check_Bit_Set(GPIOA->IDR, 9)

// 도트 매트릭스 출력용 8x8 폰트 데이터 배열
unsigned char font_P[8] = {0x3C, 0x22, 0x22, 0x3C, 0x20, 0x20, 0x20, 0x00};
unsigned char font_1[8] = {0x08, 0x18, 0x28, 0x08, 0x08, 0x08, 0x08, 0x00};
unsigned char font_2[8] = {0x3C, 0x22, 0x02, 0x0C, 0x10, 0x20, 0x3E, 0x00};
unsigned char font_W[8] = {0x22, 0x22, 0x22, 0x22, 0x2A, 0x2A, 0x14, 0x00};
unsigned char font_I[8] = {0x1C, 0x08, 0x08, 0x08, 0x08, 0x08, 0x1C, 0x00};
unsigned char font_N[8] = {0x22, 0x32, 0x2A, 0x26, 0x22, 0x22, 0x22, 0x00};
unsigned char font_blank[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void GPIO_Init_Dual(void);
void MAX7219_Init_Dual(int player);
void MAX7219_SendOne(int player, int module, unsigned char addr, unsigned char data);
void MAX7219_ClearAll(int player);

/**
 * @brief 시스템 클럭 및 하드웨어 주변장치 초기화
 * @details FPU 및 16MHz 클럭을 설정하고 디버깅용 UART와
 * 1P, 2P 초음파 센서 제어를 위한 GPIO 핀(TRIG, ECHO) 모드를 초기화합니다.
 * @param baud UART 디버그 통신에 사용할 보드레이트
 */
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

/**
 * @brief 1번 플레이어(1P) 초음파 센서 거리 측정
 * @details TRIG 핀에 펄스를 인가한 후 ECHO 핀의 High 유지 시간을 측정하여 거리를 계산합니다.
 * @return float 측정된 거리 값 (비정상 응답 시 -1.0 반환)
 */
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

/**
 * @brief 2번 플레이어(2P) 초음파 센서 거리 측정
 * @details TRIG 핀에 펄스를 인가한 후 ECHO 핀의 High 유지 시간을 측정하여 거리를 계산합니다.
 * @return float 측정된 거리 값 (비정상 응답 시 -1.0 반환)
 */
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

/**
 * @brief 특정 도트 매트릭스 모듈에 8x8 글꼴 패턴 출력
 * @details 입력받은 8바이트 폰트 배열을 행(Row) 단위로 전송하여 화면에 문자를 표시합니다.
 * @param player 출력할 플레이어 라인 (1 또는 2)
 * @param module 출력할 개별 모듈의 인덱스 (0 ~ 3)
 * @param pattern 출력할 8x8 형태의 문자 데이터 배열 포인터
 */
void Display_Pattern(int player, int module, unsigned char *pattern) {
    for (int row = 0; row < 8; row++) {
        MAX7219_SendOne(player, module, row + 1, pattern[row]);
    }
}

/**
 * @brief 메인 프로그램 루프
 * @details 하드웨어 초기화 후 평상시 대기 화면을 출력합니다.
 * 이후 무한 루프를 돌며 1P와 2P의 초음파 센서 값을 감시하여,
 * 임계값(Threshold) 이내로 차량이 먼저 진입한 플레이어를 승자로 판별하고 결과를 출력합니다.
 */
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

        if (d1 > 1 && d1 < 550) { winner = 1; break; }
        if (d2 > 1 && d2 < 550) { winner = 2; break; }
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
        Display_Pattern(1, 3, font_N); 
        
        Display_Pattern(2, 0, font_1);
        Display_Pattern(2, 1, font_W);
        Display_Pattern(2, 2, font_I);
        Display_Pattern(2, 3, font_N);
    } 
    else {
        Display_Pattern(1, 0, font_2);
        Display_Pattern(1, 1, font_W);
        Display_Pattern(1, 2, font_I);
        Display_Pattern(1, 3, font_N); 
        
        Display_Pattern(2, 0, font_2);
        Display_Pattern(2, 1, font_W);
        Display_Pattern(2, 2, font_I);
        Display_Pattern(2, 3, font_N);
    }

    while (1);
}