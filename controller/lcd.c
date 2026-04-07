/**
 * @file lcd.c
 * @brief I2C 통신 기반 1602 LCD 제어 모듈 드라이버
 */

#include "device_driver.h"

#define LCD_I2C_ADDR 0x4E 

/**
 * @brief HD44780 LCD 모듈을 4비트 통신 모드로 초기화합니다.
 */
void LCD_Init(void)
{
    TIM2_Delay(50);

    I2C1_Write_Byte(LCD_I2C_ADDR, 0x30 | 0x0C);
    I2C1_Write_Byte(LCD_I2C_ADDR, 0x30 | 0x08);
    TIM2_Delay(5);

    I2C1_Write_Byte(LCD_I2C_ADDR, 0x30 | 0x0C);
    I2C1_Write_Byte(LCD_I2C_ADDR, 0x30 | 0x08);
    TIM2_Delay(1);

    I2C1_Write_Byte(LCD_I2C_ADDR, 0x30 | 0x0C);
    I2C1_Write_Byte(LCD_I2C_ADDR, 0x30 | 0x08);
    TIM2_Delay(1);

    I2C1_Write_Byte(LCD_I2C_ADDR, 0x20 | 0x0C);
    I2C1_Write_Byte(LCD_I2C_ADDR, 0x20 | 0x08);
    TIM2_Delay(1);

    // 4. 이제부터 완벽한 4비트 모드이므로 쪼개기 함수(LCD_Send_Cmd) 정상 사용 가능
    LCD_Send_Cmd(0x28);
    LCD_Send_Cmd(0x0C);
    
    // 화면 지우기(0x01)는 내부 처리 시간이 오래 걸리므로 반드시 2ms 이상의 대기 시간이 필요합니다.
    LCD_Send_Cmd(0x01);
    TIM2_Delay(2); 
    
    LCD_Send_Cmd(0x06);
}

/**
 * @brief LCD에 8비트 제어 명령어를 전송합니다.
 * @param cmd 전송할 명령어 데이터
 */
void LCD_Send_Cmd(unsigned char cmd)
{
    unsigned char high = cmd & 0xF0;
    unsigned char low = (cmd << 4) & 0xF0;

    I2C1_Write_Byte(LCD_I2C_ADDR, (high | 0x0C));
    I2C1_Write_Byte(LCD_I2C_ADDR, (high | 0x08));
    I2C1_Write_Byte(LCD_I2C_ADDR, (low | 0x0C));
    I2C1_Write_Byte(LCD_I2C_ADDR, (low | 0x08));
}

/**
 * @brief LCD에 8비트 문자 데이터를 전송하여 화면에 출력합니다.
 * @param data 출력할 문자 데이터 (ASCII)
 */
void LCD_Send_Data(unsigned char data)
{
    unsigned char high = data & 0xF0;
    unsigned char low = (data << 4) & 0xF0;

    I2C1_Write_Byte(LCD_I2C_ADDR, (high | 0x0D));
    I2C1_Write_Byte(LCD_I2C_ADDR, (high | 0x09));
    I2C1_Write_Byte(LCD_I2C_ADDR, (low | 0x0D));
    I2C1_Write_Byte(LCD_I2C_ADDR, (low | 0x09));
}

/**
 * @brief 널 문자로 끝나는 문자열을 LCD에 연속으로 출력합니다.
 * @param str 출력할 문자열 포인터
 */
void LCD_Print_String(char *str)
{
    while (*str != '\0') {
        LCD_Send_Data(*str++);
    }
}