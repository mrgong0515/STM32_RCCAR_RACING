/**
 * @file main.c
 * @brief RC카 레이싱 스타터 보드 메인 애플리케이션
 * @details 조도 센서(ADC)를 통해 1P와 2P 차량의 출발 대기 상태를 확인하고,
 * 카운트다운을 진행하며 부정 출발(False Start) 여부를 실시간으로 감지하여
 * 도트 매트릭스 디스플레이와 부저를 통해 상태를 출력합니다.
 */

#include "device_driver.h"
#include <stdio.h>

#define RCC_AHB1ENR   (*(volatile unsigned int*)0x40023830)
#define GPIOB_MODER   (*(volatile unsigned int*)0x40020400)
#define GPIOB_OTYPER  (*(volatile unsigned int*)0x40020404)
#define GPIOB_ODR     (*(volatile unsigned int*)0x40020414)

void Adc_Init(void);
unsigned int Adc_Read(int channel);
int Check_False_Start_Detail(unsigned int threshold);
int Countdown_State(unsigned int threshold);
void Ready_State(void);

// ---------------------------------
// 패널 번호 기준
// 4번 패널 = 맨 위    = module 0
// 3번 패널 = 그 아래  = module 1
// 2번 패널 = 그 아래  = module 2
// 1번 패널 = 맨 아래  = module 3
// ---------------------------------

unsigned char num_3[8] = {
    0x00, 0x6E, 0x91, 0x91, 0x91, 0x81, 0x42, 0x00
};

unsigned char num_2[8] = {
    0x00, 0x61, 0x91, 0x89, 0x85, 0x83, 0x41, 0x00
};

unsigned char num_1[8] = {
    0x00, 0x01, 0x01, 0xFF, 0x41, 0x21, 0x00, 0x00
};

// 세로 표시용 문자 패턴
// 위에서 아래로
// F            -> False
// S            -> Start
// P            -> Player
// 1 or 2
// 이렇게 보이도록 회전한 비트맵 (가로 비트맵이라서 세로로 회전 필요)

// F : 왼쪽 90도 회전
unsigned char char_F[8] = {
    0xC0,
    0xD0,
    0xD0,
    0xD0,
    0xD0,
    0xD0,
    0xFF,
    0xFF
};

// S : 오른쪽 90도 두 번 돌린 형태(=180도 회전)
unsigned char char_S[8] = {
    0x4C,
    0x92,
    0x92,
    0x92,
    0x92,
    0x92,
    0x92,
    0x64
};

// P : 오른쪽으로 90도 두 번 돌린 형태(=180도 회전)
unsigned char char_P[8] = {
    0x60,
    0x90,
    0x90,
    0x90,
    0x90,
    0x90,
    0x90,
    0xFE
};

// X : 0 ~ 3 패널 전체 X자 표시
unsigned char char_X[8] = {
    0x81,   // 10000001
    0x42,   // 01000010
    0x24,   // 00100100
    0x18,   // 00011000
    0x18,   // 00011000
    0x24,   // 00100100
    0x42,   // 01000010
    0x81    // 10000001
};

/**
 * @brief 출발 대기 상태 시각화
 * @details 대기 상태에서 양쪽 차량이 정위치에 있을 때,
 * 1번과 2번 패널을 0.5초 간격으로 점멸하여 출발 대기 완료를 시각적으로 알립니다.
 */
void Ready_State(void)
{
    for (int i = 0; i < 5; i++) {
        MAX7219_ClearAll();
        MAX7219_FillModule(1);
        MAX7219_FillModule(2);
        TIM2_Delay(500);

        MAX7219_ClearAll();
        TIM2_Delay(500);
    }
}

/**
 * @brief 부정 출발(False Start) 감지
 * @details 조도 센서 값을 임계값(threshold)과 비교하여 차량이 빛을 가리고 있는지 판별합니다.
 * @param threshold 차량 이탈을 판단하는 조도 센서 기준값
 * @return int [0]: 정상 대기, [1]: 1P 부정 출발, [2]: 2P 부정 출발, [3]: 둘 다 부정 출발
 */
int Check_False_Start_Detail(unsigned int threshold)
{
    int s1 = (Adc_Read(0) > threshold); // PA0가 밝아지면 1
    int s2 = (Adc_Read(1) > threshold); // PA1이 밝아지면 2

    if (s1 && s2) return 3; // 둘 다 출발
    if (s1) return 1;       // Player 1만 출발
    if (s2) return 2;       // Player 2만 출발
    return 0;               // 둘 다 아직 정지 상태
}

/**
 * @brief 카운트다운 진행 및 부정 출발 모니터링
 * @details 3, 2, 1 숫자를 디스플레이와 부저음으로 출력하며, 
 * 각 단계 사이의 대기 시간 동안 지속적으로 부정 출발을 모니터링합니다.
 * @param threshold 부정 출발을 감지하기 위한 조도 센서 기준값
 * @return int 정상 출발 시 0, 부정 출발 발생 시 해당 위반 상태 코드 반환
 */
int Countdown_State(unsigned int threshold)
{
    int fs = 0;

    // --- 숫자 3 --- //
    MAX7219_ClearAll();
    MAX7219_FillModule(2);
    MAX7219_ShowPattern(3, num_3);
    Buzzer_Count_Short();

    for (int i = 0; i < 10; i++) {
        fs = Check_False_Start_Detail(threshold);
        if (fs > 0) return fs;
        TIM2_Delay(100);
    }

    // --- 숫자 2 --- //
    MAX7219_ClearAll();
    MAX7219_FillModule(1);
    MAX7219_ShowPattern(3, num_2);
    Buzzer_Count_Short();

    for (int i = 0; i < 10; i++) {
        fs = Check_False_Start_Detail(threshold);
        if (fs > 0) return fs;
        TIM2_Delay(100);
    }

    // --- 숫자 1 --- //
    MAX7219_ClearAll();
    MAX7219_FillModule(0);
    MAX7219_ShowPattern(3, num_1);
    Buzzer_Count_Long();

    for (int i = 0; i < 10; i++) {
        fs = Check_False_Start_Detail(threshold);
        if (fs > 0) return fs;
        TIM2_Delay(15);
    }

    // --- 전체 ON (직전 잠시 대기 딜레이) --- //
    MAX7219_FillModulesRange(0, 3);

    for (int i = 0; i < 10; i++) {
        fs = Check_False_Start_Detail(threshold);
        if (fs > 0) return fs;
        TIM2_Delay(100);
    }

    // --- 전체 OFF 및 시작 --- //
    MAX7219_ClearAll();
    Buzzer_Start_Race();

    return 0;       // 정상 출발 완료
}

/**
 * @brief 조도 센서 측정을 위한 ADC 하드웨어 초기화
 * @details PA0, PA1 핀을 아날로그 입력 모드로 설정하고, 
 * 노이즈 방지를 위해 ADC 샘플링 타임을 최대로 설정하여 활성화합니다.
 */
void Adc_Init(void)
{
    // 1. GPIOA 및 ADC1 클럭 활성화
    Macro_Set_Bit(RCC->AHB1ENR, 0);   // GPIOA ON
    Macro_Set_Bit(RCC->APB2ENR, 8);   // ADC1 ON

    // 2. PA0, PA1 아날로그 모드 설정 (11)
    Macro_Write_Block(GPIOA->MODER, 0xf, 0xf, 0); // PA0, PA1 => Analog Mode

    // 3. ADC 설정
    ADC1->CR1 = 0;          // 초기화
    ADC1->CR2 = (1 << 0);   // ADON: ADC 켜기

    // Sampling Time 설정
    ADC1->SMPR2 |= (0x7 << 0) | (0x7 << 3); // CH0, CH1 최대 샘플링 타임
}

/**
 * @brief 지정된 채널의 아날로그-디지털 변환 수행
 * @details 원하는 채널 번호를 선택한 후 소프트웨어 트리거로 변환을 시작하고 결과를 기다립니다.
 * @param channel 읽어올 ADC 채널 번호 (PA0의 경우 0, PA1의 경우 1)
 * @return unsigned int 변환이 완료된 디지털 데이터 (0 ~ 4095)
 */
unsigned int Adc_Read(int channel)
{
    ADC1->SQR3 = channel;               // 읽을 채널 선택
    ADC1->CR2 |= (1 << 30);             // SWSTART: 변환 시작
    while (!(ADC1->SR & (1 << 1)));     // EOC 대기
    return ADC1->DR;                    // 결과값 반환
}

/**
 * @brief 시스템 클럭 및 통신용 주변장치 초기화
 * @details FPU 및 시스템 클럭을 설정하고 상태 디버깅용 UART 포트를 초기화합니다.
 * @param baud UART 통신에 사용할 속도(Baudrate)
 */
void Sys_Init(int baud)
{
    // FPU 및 클럭 설정
    SCB->CPACR |= (0x3 << 10 * 2) | (0x3 << 11 * 2);
    RCC->CR |= (1 << 0);
    while (!(RCC->CR & (1 << 1)));
    RCC->CFGR = 0;

    // 하드웨어 초기화
    Uart2_Init(baud);                   // 디버그용
    Uart1_Init(baud);                   // HC-05 블루투스용
    setvbuf(stdout, NULL, _IONBF, 0);
}

/**
 * @brief 메인 프로그램 루프
 * @details 센서 값을 읽어 차량 정위치를 확인하고, 버튼 입력에 따라 카운트다운을 시작합니다.
 * 부정 출발 감지 결과에 맞춰 화면에 경고 문구(FSP1, FSP2 등)를 출력합니다.
 */
void Main(void)
{
    unsigned int sensor1, sensor2;
    unsigned int threshold = 2000;

    // 시스템 초기화
    Clock_Init();
    GPIO_Init();
    Key_Poll_Init();
    Buzzer_Init();
    MAX7219_Init();
    MAX7219_ClearAll();
    Sys_Init(38400);
    Adc_Init();

    printf("Racing Track Start System Initialized...\n");

    while (1) {
        sensor1 = Adc_Read(0);      // PA0
        sensor2 = Adc_Read(1);      // PA1

        printf("S1: %d | S2: %d\n", sensor1, sensor2);

        // 1. 차량 정위치 확인 (조도 센서 둘 다 어두움)
        if (sensor1 < threshold && sensor2 < threshold) {

            // 차량 감지 시 "대기" 상태
            Ready_State();

            // 2. 버튼 눌림 확인
            if (Key_Get_Pressed()) {
                printf(">> [START] Button Clicked! <<\n");

                // 버튼에서 손을 뗄 때까지 대기
                Key_Wait_Key_Released();

                // 3. 카운트다운 및 레이스 시작 (출발)
                int result = Countdown_State(threshold);

                if (result > 0) {
                    // 1. 부저 3번
                    for (int i = 0; i < 3; i++) {
                        Buzzer_Count_Short();
                    }

                    // 2. 패널에 상황에 맞는 메시지 출력
                    MAX7219_ClearAll();

                    // 2-1. P1 부정 출발
                    if(result == 1) {
                    printf("FSP1 Detected!\n");
                    MAX7219_ShowPattern(0, char_F);
                    MAX7219_ShowPattern(1, char_S);
                    MAX7219_ShowPattern(2, char_P);
                    MAX7219_ShowPattern(3, num_1);
                    }

                    // 2-2. P2 부정 출발
                    else if(result == 2) {
                    printf("FSP2 Detected!\n");
                    MAX7219_ShowPattern(0, char_F);
                    MAX7219_ShowPattern(1, char_S);
                    MAX7219_ShowPattern(2, char_P);
                    MAX7219_ShowPattern(3, num_2);
                    }
                    
                    // 2-3. P1, P2 모두 부정 출발
                    else if(result >= 3) {
                        printf("Both FS Detected!\n");
                        for(int i = 0; i < 4; i++) {
                            MAX7219_ShowPattern(i, char_X);
                            }
                    }

                    TIM2_Delay(3000);       // 3초간 경고 유지
                }

                // 카운트다운 직후 차량이 바로 출발하므로 잠시 대기
                for (volatile int i = 0; i < 2000000; i++);
            }
        }
        else {
            // 차량이 없으면 클리어
            MAX7219_ClearAll();
        }

        // TeraTerm 출력 속도 조절
        for (volatile int i = 0; i < 500000; i++);
    }
}