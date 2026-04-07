# 하드웨어 배선 가이드
## 1. HC-05 블루투스 모듈 (USART1)
- HC-05 VCC : Nucleo 5V 핀 (Morpho 18)
- HC-05 GND : Nucleo GND 핀 (Morpho 20)
- HC-05 TXD : Nucleo PA10 핀 (D2)
- HC-05 RXD : Nucleo PA9 핀 (D8)

## 2. I2C 1602 LCD 모듈 (I2C1)
- LCD VCC : Nucleo 5V 핀
- LCD GND : Nucleo GND 핀
- LCD SDA : Nucleo PB9 핀 (D14)
- LCD SCL : Nucleo PB8 핀 (D15)

## 3. 아날로그 조이스틱 모듈
- 조이스틱 VCC : Nucleo 3.3V 핀
- 조이스틱 GND : Nucleo GND 핀
- 조이스틱 VRx (X축) : Nucleo PA0 핀 (A0)
- 조이스틱 VRy (Y축) : Nucleo PA1 핀 (A1)
- 조이스틱 SW (버튼) : Nucleo PB5 핀 (D4)