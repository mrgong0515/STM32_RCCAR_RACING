# 하드웨어 배선 가이드
## HC-05 블루투스 통신 모듈
## HC-05 블루투스 모듈 (USART1)
- VCC : Nucleo 5V 핀 (Morpho 18)
- GND : Nucleo GND 핀 (Morpho 20)
- TXD : Nucleo PA10 핀 (D2)
- RXD : Nucleo PA9 핀 (D8)

## L298N 모터 드라이버
- IN1 : Nucleo PA0 핀 (A0) - 좌측 전진
- IN2 : Nucleo PA1 핀 (A1) - 좌측 후진
- IN3 : Nucleo PA4 핀 (A2) - 우측 전진
- IN4 : Nucleo PA5 핀 (D13) - 우측 후진
- ENA : Nucleo PB4 핀 (D5) - 좌측 모터 속도
- ENB : Nucleo PB5 핀 (D4) - 우측 모터 속도
