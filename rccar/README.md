# 하드웨어 배선 가이드
## HC-05 블루투스 모듈 (USART1)
- VCC : Nucleo-64 5V 핀 (CN7 Morpho 18)
- GND : Nucleo-64 GND 핀 (CN7 Morpho 20)
- TXD : Nucleo-64 PA10 핀 (D2)
- RXD : Nucleo-64 PA9 핀 (D8)

## L298N 모터 드라이버
- IN1 : Nucleo-64 PA0 핀 (A0) - 좌측 전진
- IN2 : Nucleo-64 PA1 핀 (A1) - 좌측 후진
- IN3 : Nucleo-64 PA4 핀 (A2) - 우측 전진
- IN4 : Nucleo-64 PA5 핀 (D13) - 우측 후진
- ENA : Nucleo-64 PB4 핀 (D5) - 좌측 모터 속도 조절
- ENB : Nucleo-64 PB5 핀 (D4) - 우측 모터 속도 조절
