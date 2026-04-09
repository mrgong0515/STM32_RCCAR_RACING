
#include "device_driver.h"
#include <stdio.h>

void Sys_Init(int baud) 
{
    SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2); 
    RCC->CR |= (1 << 0); 
    while(!(RCC->CR & (1 << 1)));
    RCC->CFGR = 0; 

    RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN);

    GPIOA->MODER &= ~((3 << (5*2)) | (3 << (7*2)));
    GPIOA->MODER |=  ((1 << (5*2)) | (1 << (7*2)));

    GPIOB->MODER &= ~((3 << (4*2)) | (3 << (6*2)) | (3 << (8*2)) | (3 << (12*2)));
    GPIOB->MODER |=  ((1 << (4*2)) | (1 << (6*2)) | (1 << (8*2)) | (1 << (12*2)));

    GPIOB->MODER &= ~((3 << (5*2)) | (3 << (9*2)));

    Uart2_Init(baud);      
    Uart1_Init(baud);      
}

void Main(void)
{
    Sys_Init(38400); 
    
    // ✅ 이것만 먼저 테스트
    Uart1_Printf("HELLO\n");
    
    for(;;)
    {
        Uart1_Printf("1P test\n");
        for(volatile int i=0; i<500000; i++);
    }
}
// #include "device_driver.h"
// #include <stdio.h>

// // --- 피니쉬 라인 전용 핀 정의 ---
// #define DIN_HIGH()     (GPIOA->ODR |=  (1 << 7))
// #define DIN_LOW()      (GPIOA->ODR &= ~(1 << 7))
// #define CLK_HIGH()     (GPIOA->ODR |=  (1 << 5))
// #define CLK_LOW()      (GPIOA->ODR &= ~(1 << 5))

// #define TRIG1_HIGH()   (GPIOB->ODR |=  (1 << 8))
// #define TRIG1_LOW()    (GPIOB->ODR &= ~(1 << 8))
// #define TRIG2_HIGH()   (GPIOB->ODR |=  (1 << 4))
// #define TRIG2_LOW()    (GPIOB->ODR &= ~(1 << 4))

// // --- 전역 패턴 데이터 ---
// unsigned char pat_W[8] = {0x7F, 0x02, 0x0C, 0x02, 0x7F, 0x00, 0x00, 0x00};
// unsigned char pat_I[8] = {0x00, 0x41, 0x7F, 0x41, 0x00, 0x00, 0x00, 0x00};
// unsigned char pat_N[8] = {0x7F, 0x04, 0x08, 0x10, 0x7F, 0x00, 0x00, 0x00};

// // --- 함수 선언 ---
// void SendByte_Finish(unsigned char data);
// void MAX7219_SendOne_Finish(int player, int module, unsigned char addr, unsigned char data);
// void MAX7219_Init_All(void);
// void MAX7219_Clear_Finish(int player);
// float Get_Distance_Finish(int player);
// void Display_Winner_Finish(int winner);

// // ---------------------------------------------------------
// // 1. 하드웨어 전송 및 제어 함수
// // ---------------------------------------------------------
// void SendByte_Finish(unsigned char data) {
//     for (int i = 0; i < 8; i++) {
//         if (data & 0x80) DIN_HIGH(); else DIN_LOW();
//         CLK_HIGH(); for(volatile int j=0; j<5; j++);
//         CLK_LOW();  for(volatile int j=0; j<5; j++);
//         data <<= 1;
//     }
// }

// void MAX7219_SendOne_Finish(int player, int module, unsigned char addr, unsigned char data) {
//     if(player == 1) GPIOB->ODR &= ~(1 << 6);  // CS1 LOW (PB6)
//     else           GPIOB->ODR &= ~(1 << 12); // CS2 LOW (PB12)

//     for (int i = 0; i < 4; i++) {
//         if (i == module) { SendByte_Finish(addr); SendByte_Finish(data); }
//         else { SendByte_Finish(0x00); SendByte_Finish(0x00); }
//     }

//     if(player == 1) GPIOB->ODR |= (1 << 6);   // CS1 HIGH
//     else           GPIOB->ODR |= (1 << 12);  // CS2 HIGH
// }

// void MAX7219_Init_All(void) {
//     for(int p=1; p<=2; p++) {
//         MAX7219_SendOne_Finish(p, 0, 0x0C, 0x01);
//         MAX7219_SendOne_Finish(p, 0, 0x09, 0x00);
//         MAX7219_SendOne_Finish(p, 0, 0x0B, 0x07);
//         MAX7219_SendOne_Finish(p, 0, 0x0A, 0x01); 
//         MAX7219_Clear_Finish(p);
//     }
// }

// void MAX7219_Clear_Finish(int player) {
//     for (int i = 1; i <= 8; i++) MAX7219_SendOne_Finish(player, i, i, 0x00);
// }

// // ---------------------------------------------------------
// // 2. 센서 및 판정 함수
// // ---------------------------------------------------------
// float Get_Distance_Finish(int player) {
//     int echo_pin = (player == 1) ? 9 : 5;
//     uint32_t count = 0;
//     uint32_t timeout = 0;

//     if(player == 1) { TRIG1_LOW(); for(volatile int i=0; i<10; i++); TRIG1_HIGH(); for(volatile int i=0; i<50; i++); TRIG1_LOW(); }
//     else           { TRIG2_LOW(); for(volatile int i=0; i<10; i++); TRIG2_HIGH(); for(volatile int i=0; i<50; i++); TRIG2_LOW(); }

//     while(!(GPIOB->IDR & (1 << echo_pin))) {
//         if(++timeout > 100000) return 999.0f; // Timeout 증가
//     }
//     while(GPIOB->IDR & (1 << echo_pin)) {
//         count++;
//         if(count > 100000) break; 
//     }
//     return count * 0.017f;
// }

// void Display_Winner_Finish(int winner) {
//     MAX7219_Clear_Finish(1); MAX7219_Clear_Finish(2);
//     MAX7219_SendOne_Finish(winner, 0, 1, pat_W[0]);
//     MAX7219_SendOne_Finish(winner, 1, 1, pat_I[0]);
//     MAX7219_SendOne_Finish(winner, 2, 1, pat_N[0]);
//     printf("\n\n🏆 PLAYER %d WIN! 🏆\n", winner);
// }

// // ---------------------------------------------------------
// // 3. 컨트롤러와 동일한 규격의 초기화 루틴
// // ---------------------------------------------------------
// void Sys_Init(int baud) 
// {
//     SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2); 
//     RCC->CR |= (1 << 0); 
//     while(!(RCC->CR & (1 << 1)));
//     RCC->CFGR = 0; 

//     RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN);

//     // ✅ GPIO 핀 설정을 UART Init보다 먼저!
//     GPIOA->MODER &= ~((3 << (5*2)) | (3 << (7*2)));
//     GPIOA->MODER |=  ((1 << (5*2)) | (1 << (7*2)));

//     GPIOB->MODER &= ~((3 << (4*2)) | (3 << (6*2)) | (3 << (8*2)) | (3 << (12*2)));
//     GPIOB->MODER |=  ((1 << (4*2)) | (1 << (6*2)) | (1 << (8*2)) | (1 << (12*2)));

//     GPIOB->MODER &= ~((3 << (5*2)) | (3 << (9*2)));

//     // ✅ UART는 GPIO 설정 다음에
//     Uart2_Init(baud);      
//     Uart1_Init(baud);      
    
//     setvbuf(stdout, NULL, _IONBF, 0);
// }
// void Main(void)
// {
//     Sys_Init(38400); 
    
//     printf("\n--- Finish Line Monitor Start ---\n");  // 이걸로 변경
//     MAX7219_Init_All();

//     float d1, d2;
//     float threshold = 12.0f; 

//     for(;;)
//     {
//         d1 = Get_Distance_Finish(1);
//         for(volatile int i=0; i<10000; i++); 
//         d2 = Get_Distance_Finish(2);

//         printf("\r1P: %4.1f cm | 2P: %4.1f cm    ", (double)d1, (double)d2);
//     }
// }