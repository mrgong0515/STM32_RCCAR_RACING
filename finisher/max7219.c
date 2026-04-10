/**
 * @file max7219.c
 * @brief MAX7219 도트 매트릭스 디스플레이 제어 드라이버
 * @details 소프트웨어 비트뱅잉(Bit-banging) 방식으로 SPI 통신을 구현하여 
 * 2명의 플레이어(CS1, CS2)용 8x8 도트 매트릭스 모듈을 독립적으로 제어합니다.
 */

#include "device_driver.h"
#include "stm32f411xe.h" 

// --- [핀 조작 매크로] ---
#define DIN_HIGH()   (GPIOA->ODR |=  (1 << 7))
#define DIN_LOW()    (GPIOA->ODR &= ~(1 << 7))
#define CLK_HIGH()   (GPIOA->ODR |=  (1 << 5))
#define CLK_LOW()    (GPIOA->ODR &= ~(1 << 5))

#define CS1_HIGH()   (GPIOB->ODR |=  (1 << 6))
#define CS1_LOW()    (GPIOB->ODR &= ~(1 << 6))
#define CS2_HIGH()   (GPIOB->ODR |=  (1 << 12))
#define CS2_LOW()    (GPIOB->ODR &= ~(1 << 12))

/**
 * @brief 소프트웨어 기반 마이크로초(us) 미세 지연
 * @details 클럭 신호 생성 등 짧은 타이밍 조절을 위해 사용되는 빈 반복문 딜레이입니다.
 * @param count 지연할 사이클 수
 */
void Delay_uS(volatile unsigned int count) {
    for(; count > 0; count--);
}

/**
 * @brief MAX7219 통신을 위한 GPIO 핀 초기화
 * @details 공통으로 사용되는 데이터 핀(DIN: PA7)과 클럭 핀(CLK: PA5), 
 * 그리고 플레이어별 칩 선택 핀(CS1: PB6, CS2: PB12)을 출력 모드로 설정합니다.
 */
void GPIO_Init_Dual(void) {
    // GPIOA, GPIOB 클럭 활성화
    RCC->AHB1ENR |= (1 << 0) | (1 << 1);

    // PA5(CLK), PA7(DIN) 출력 설정
    GPIOA->MODER &= ~((3 << (5 * 2)) | (3 << (7 * 2)));
    GPIOA->MODER |=  ((1 << (5 * 2)) | (1 << (7 * 2)));

    // PB6(CS1), PB12(CS2) 출력 설정
    GPIOB->MODER &= ~((3 << (6 * 2)) | (3 << (12 * 2)));
    GPIOB->MODER |=  ((1 << (6 * 2)) | (1 << (12 * 2)));

    DIN_LOW(); 
    CLK_LOW();
    CS1_HIGH(); 
    CS2_HIGH();
}

/**
 * @brief 1바이트 데이터 직렬 전송 (소프트웨어 SPI)
 * @details MSB(최상위 비트)부터 1비트씩 클럭 신호에 맞추어 데이터를 전송합니다.
 * @param data 전송할 8비트 데이터
 */
void SendByte(unsigned char data) {
    for (int i = 0; i < 8; i++) {
        if (data & 0x80) DIN_HIGH();
        else             DIN_LOW();

        Delay_uS(5);
        CLK_HIGH();
        Delay_uS(5);
        CLK_LOW();
        
        data <<= 1;
    }
}

/**
 * @brief 특정 플레이어의 개별 디스플레이 모듈에 데이터 전송
 * @details 데이지 체인으로 연결된 여러 모듈 중 하나에만 데이터를 보내고, 
 * 나머지 모듈에는 빈 데이터(0x00)를 보내어 상태를 유지하도록 합니다.
 * @param player 플레이어 번호 (1 또는 2)
 * @param module 제어할 모듈의 인덱스 (0 ~ 3)
 * @param addr 제어 레지스터 주소
 * @param data 전송할 데이터
 */
void MAX7219_SendOne(int player, int module, unsigned char addr, unsigned char data) {
    if(player == 1)      CS1_LOW();
    else if(player == 2) CS2_LOW();
    else return;

    for (int i = 0; i < 4; i++) {
        if (i == module) {
            SendByte(addr);
            SendByte(data);
        } else {
            SendByte(0x00);
            SendByte(0x00);
        }
    }

    if(player == 1)      CS1_HIGH();
    else if(player == 2) CS2_HIGH();
    
    Delay_uS(10);
}

/**
 * @brief 특정 플레이어의 모든 디스플레이 모듈에 동일한 데이터 전송
 * @details 연결된 4개의 모듈 전체에 한 번에 같은 설정이나 화면을 적용합니다.
 * @param player 플레이어 번호 (1 또는 2)
 * @param addr 제어 레지스터 주소
 * @param data 전송할 데이터
 */
void MAX7219_SendAll(int player, unsigned char addr, unsigned char data) {
    if(player == 1)      CS1_LOW();
    else if(player == 2) CS2_LOW();
    else return;

    for (int i = 0; i < 4; i++) {
        SendByte(addr);
        SendByte(data);
    }

    if(player == 1)      CS1_HIGH();
    else if(player == 2) CS2_HIGH();
}

/**
 * @brief MAX7219 디스플레이 초기 설정
 * @details 디스플레이의 정상 동작 모드, 스캔 범위, 밝기 등을 설정하고 화면을 지웁니다.
 * @param player 초기화할 플레이어 번호 (1 또는 2)
 */
void MAX7219_Init_Dual(int player) {
    MAX7219_SendAll(player, 0x0F, 0x00); // Display Test Off
    MAX7219_SendAll(player, 0x0C, 0x01); // Normal Operation
    MAX7219_SendAll(player, 0x09, 0x00); // No Decode
    MAX7219_SendAll(player, 0x0B, 0x07); // 8 rows
    MAX7219_SendAll(player, 0x0A, 0x01); // 밝기 최소 (안전용)
    
    for (int row = 1; row <= 8; row++) {
        MAX7219_SendAll(player, row, 0x00); // Clear All
    }
}

/**
 * @brief 특정 플레이어의 디스플레이 전체 화면 지우기
 * @details 모든 행(Row)의 데이터를 0으로 만들어 매트릭스의 모든 LED를 끕니다.
 * @param player 화면을 지울 플레이어 번호 (1 또는 2)
 */
void MAX7219_ClearAll(int player) {
    for (int row = 1; row <= 8; row++) {
        MAX7219_SendAll(player, row, 0x00);
        Delay_uS(10); // 통신 사이의 아주 미세한 휴식 (안정성)
    }
}