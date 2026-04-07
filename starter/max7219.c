#include "device_driver.h"

#define RCC_AHB1ENR    (*(volatile unsigned int*)0x40023830)

#define GPIOA_MODER    (*(volatile unsigned int*)0x40020000)
#define GPIOA_OTYPER   (*(volatile unsigned int*)0x40020004)
#define GPIOA_ODR      (*(volatile unsigned int*)0x40020014)

#define GPIOB_MODER    (*(volatile unsigned int*)0x40020400)
#define GPIOB_OTYPER   (*(volatile unsigned int*)0x40020404)
#define GPIOB_ODR      (*(volatile unsigned int*)0x40020414)

#define DIN_HIGH()   (GPIOA_ODR |=  (1 << 7))
#define DIN_LOW()    (GPIOA_ODR &= ~(1 << 7))

#define CLK_HIGH()   (GPIOA_ODR |=  (1 << 5))
#define CLK_LOW()    (GPIOA_ODR &= ~(1 << 5))

#define CS_HIGH()    (GPIOB_ODR |=  (1 << 6))
#define CS_LOW()     (GPIOB_ODR &= ~(1 << 6))

// DIN = PA7 (D11)
// CLK = PA5 (D13)
// CS  = PB6 (D10)

void Delay(volatile unsigned int count)
{
    while(count--);
}

void GPIO_Init(void)
{
    // GPIOA, GPIOB 클럭 활성화
    RCC_AHB1ENR |= (1 << 0);
    RCC_AHB1ENR |= (1 << 1);

    // PA5, PA7 출력
    GPIOA_MODER &= ~((3 << (5 * 2)) | (3 << (7 * 2)));
    GPIOA_MODER |=  ((1 << (5 * 2)) | (1 << (7 * 2)));

    // PB6 출력
    GPIOB_MODER &= ~(3 << (6 * 2));
    GPIOB_MODER |=  (1 << (6 * 2));

    // Push-pull
    GPIOA_OTYPER &= ~((1 << 5) | (1 << 7));
    GPIOB_OTYPER &= ~(1 << 6);

    DIN_LOW();
    CLK_LOW();
    CS_HIGH();
}

void SendByte(unsigned char data)
{
    for (int i = 0; i < 8; i++)
    {
        if (data & 0x80) DIN_HIGH();
        else             DIN_LOW();

        CLK_HIGH();
        Delay(50);
        CLK_LOW();
        Delay(50);

        data <<= 1;
    }
}

// module:
// 0 = 맨위
// 1 = 위에서 두번째
// 2 = 아래에서 두번째
// 3 = 맨아래
void MAX7219_SendOne(int module, unsigned char addr, unsigned char data)
{
    CS_LOW();

    for (int i = 0; i < 4; i++)
    {
        if (i == module)
        {
            SendByte(addr);
            SendByte(data);
        }
        else
        {
            SendByte(0x00);
            SendByte(0x00);
        }
    }

    CS_HIGH();
    Delay(500);
}

void MAX7219_SendAll(unsigned char addr, unsigned char data)
{
    CS_LOW();

    for (int i = 0; i < 4; i++)
    {
        SendByte(addr);
        SendByte(data);
    }

    CS_HIGH();
    Delay(500);
}

void MAX7219_Init(void)
{
    MAX7219_SendAll(0x0F, 0x00); // display test off
    MAX7219_SendAll(0x0C, 0x01); // normal operation
    MAX7219_SendAll(0x09, 0x00); // decode off
    MAX7219_SendAll(0x0B, 0x07); // 8 rows
    MAX7219_SendAll(0x0A, 0x08); // 밝기 중간
}

void MAX7219_ClearAll(void)
{
    for (int row = 1; row <= 8; row++)
    {
        MAX7219_SendAll(row, 0x00);
    }
}

void MAX7219_FillModule(int module)
{
    for (int row = 1; row <= 8; row++)
    {
        MAX7219_SendOne(module, row, 0xFF);
    }
}

void MAX7219_ShowPattern(int module, unsigned char *pattern)
{
    for (int row = 0; row < 8; row++)
    {
        MAX7219_SendOne(module, row + 1, pattern[row]);
    }
}

void MAX7219_FillModulesRange(int from_module, int to_module)
{
    for (int module = from_module; module <= to_module; module++)
    {
        MAX7219_FillModule(module);
    }
}