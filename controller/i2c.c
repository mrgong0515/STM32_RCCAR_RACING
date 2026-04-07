/**
 * @file i2c.c
 * @brief STM32F411 I2C1 하드웨어 제어 드라이버
 */

#include "device_driver.h"

/* freg = 5000 ~ 100000 */

#define SC16IS752_I2CADDR									0x9A
#define SC16IS752_I2CADDR_WR								(SC16IS752_I2CADDR|0x0)
#define SC16IS752_I2CADDR_RD								(SC16IS752_I2CADDR|0x1)

/**
 * @brief I2C1 하드웨어 장치를 표준 모드(100KHz)로 초기화합니다.
 */
void I2C1_Init(void)
{
    // 1. GPIOB 클럭 켜기
    Macro_Set_Bit(RCC->AHB1ENR, 1);

    // 2. PB8(SCL), PB9(SDA) 핀을 대체 기능(AF) 모드 및
    // 오픈 드레인(Open-Drain)으로 설정
    Macro_Write_Block(GPIOB->MODER, 0x3, 0x2, 16);
    Macro_Write_Block(GPIOB->MODER, 0x3, 0x2, 18);
    Macro_Set_Bit(GPIOB->OTYPER, 8);
    Macro_Set_Bit(GPIOB->OTYPER, 9);
    Macro_Write_Block(GPIOB->PUPDR, 0x3, 0x1, 16);
    Macro_Write_Block(GPIOB->PUPDR, 0x3, 0x1, 18);

    // 3. PB8, PB9의 대체 기능을 AF4(I2C1)로 설정
    Macro_Write_Block(GPIOB->AFR[1], 0xf, 0x4, 0);
    Macro_Write_Block(GPIOB->AFR[1], 0xf, 0x4, 4);
    
    // 4. I2C1 장치 클럭 켜기 (APB1 버스)
    Macro_Set_Bit(RCC->APB1ENR, 21);

    // 5. I2C1 소프트웨어 리셋 수행 (안정화를 위함)
    Macro_Set_Bit(RCC->APB1RSTR, 21);
    Macro_Clear_Bit(RCC->APB1RSTR, 21);

    // 6. I2C1 타이밍 및 속도 레지스터 설정 
    // (Standard Mode, 100KHz 권장)
    Macro_Write_Block(I2C1->CR2, 0x3f, (PCLK1 / 1000000), 0);
    I2C1->CCR = PCLK1 / (100000 * 2);
    I2C1->TRISE = (PCLK1 / 1000000) + 1;

    // 7. I2C1 장치 활성화
    Macro_Set_Bit(I2C1->CR1, 0);
}

/**
 * @brief 지정된 I2C 슬레이브 주소로 1바이트의 데이터를 전송합니다.
 * @param slave_addr 통신할 슬레이브 장치의 8비트 주소 (쓰기 모드)
 * @param data 전송할 1바이트 데이터
 */
void I2C1_Write_Byte(unsigned char slave_addr, unsigned char data)
{
    // 1. 시작 신호(Start) 발생 및 확인 (SR1 SB 비트)
    Macro_Set_Bit(I2C1->CR1, 8);
    while(!Macro_Check_Bit_Set(I2C1->SR1, 0));

    // 2. 슬레이브 주소(쓰기 모드) 전송 및 응답(ACK) 확인 (SR1 ADDR 비트)
    I2C1->DR = slave_addr;
    while(!Macro_Check_Bit_Set(I2C1->SR1, 1));

    // 3. ADDR 깃발 클리어 (SR1, SR2 순차적 읽기)
    (void)I2C1->SR2;

    // 4. 데이터 전송 버퍼 비어있음(TXE) 확인 후 데이터 전송
    while(!Macro_Check_Bit_Set(I2C1->SR1, 7));
    I2C1->DR = data;

    // 5. 바이트 전송 완료(BTF) 확인
    while(!Macro_Check_Bit_Set(I2C1->SR1, 2));

    // 6. 정지 신호(Stop) 발생
    Macro_Set_Bit(I2C1->CR1, 9);
}