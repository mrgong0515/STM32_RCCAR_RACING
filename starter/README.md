# 하드웨어 배선 가이드
## MAX7219 아두이노 8X32 도트 매트릭스 모듈 [SZH-EKAD-115]
- VCC   : Nucleo-64 5V 핀 
- GND   : Nucleo-64 GND 핀 
- DIN   : Nucleo-64 PA7 핀 (D11)
- CS    : Nucleo-64 PB6 핀 (D12)
- CLK   : Nucleo-64 PA5 핀 (D13)

## 피에조 부저
- (+)   : Nucleo-64 PB0 핀 (A3) 
- (-)   : Nucleo-64 GND 핀

## 택트 스위치
- SIG   : Nucleo-64 PC13 핀 (CN7 Morpho 23)
- GND   : Nucleo-64 GND 핀

## 조도센서
- VCC   : Nucleo-64 3.3V 핀
- GND   : Nucleo-64 GND 핀
- ADC1  : Nucleo-64 PA0 핀 (A0) (10kΩ 저항 연결)
- ADC2  : Nucleo-64 PA1 핀 (A1) (10kΩ 저항 연결)