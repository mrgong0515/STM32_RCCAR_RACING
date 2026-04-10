/**
 * @file key.c
 * @brief 외부 버튼(Key) 입력 제어 드라이버
 * @details 폴링(Polling) 및 인터럽트(Interrupt) 방식을 통해 
 * 피니셔 보드의 사용자 버튼(PC13) 입력을 처리하는 기능을 제공합니다.
 */

#include "device_driver.h"

void Key_Poll_Init(void)
{
	Macro_Set_Bit(RCC->AHB1ENR, 2); 
	Macro_Write_Block(GPIOC->MODER, 0x3, 0x0, 26);
}

int Key_Get_Pressed(void)
{
	return Macro_Check_Bit_Clear(GPIOC->IDR, 13);	
}

void Key_Wait_Key_Pressed(void)
{
	while(!Macro_Check_Bit_Clear(GPIOC->IDR, 13));
}

void Key_Wait_Key_Released(void)
{
	while(!Macro_Check_Bit_Set(GPIOC->IDR, 13));
}

/**
 * @brief 외부 버튼(PC13) 인터럽트 활성화 및 비활성화 제어
 * @details PC13 핀을 하강 에지(Falling trigger) 방식의 외부 인터럽트(EXTI13)로 설정하고,
 * 시스템 구성 컨트롤러(SYSCFG)와 NVIC 상에서 해당 IRQ(40번, EXTI15_10)를 제어합니다.
 * @param en 1을 입력하면 인터럽트 활성화, 0을 입력하면 비활성화
 */
void Key_ISR_Enable(int en)
{
	if(en)
	{
		Macro_Set_Bit(RCC->AHB1ENR, 2); 
		Macro_Write_Block(GPIOC->MODER, 0x3, 0x0, 26);

		Macro_Set_Bit(RCC->APB2ENR, 14); 
		Macro_Write_Block(SYSCFG->EXTICR[3], 0xf, 0x2, 4);

		Macro_Set_Bit(EXTI->FTSR, 13);
		EXTI->PR = 0x1 << 13;
		
		NVIC_ClearPendingIRQ((IRQn_Type)40);
		Macro_Set_Bit(EXTI->IMR, 13);
		NVIC_EnableIRQ((IRQn_Type)40);
	}

	else
	{
		NVIC_DisableIRQ((IRQn_Type)40);
	}
}
