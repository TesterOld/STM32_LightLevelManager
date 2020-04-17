#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"

void SetSysClockTo72(void)
{
	ErrorStatus HSEStartUpStatus;
	
	RCC_DeInit();
	RCC_HSEConfig(RCC_HSE_ON);
	HSEStartUpStatus = RCC_WaitForHSEStartUp();
	
	if (HSEStartUpStatus == SUCCESS)
	{
		RCC_HCLKConfig(RCC_SYSCLK_Div1);
		RCC_PCLK2Config(RCC_HCLK_Div1);
		RCC_PCLK1Config(RCC_HCLK_Div2);
		
		RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);
		
		RCC_PLLCmd(ENABLE);
		
		while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);
		
		RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
		
		while (RCC_GetSYSCLKSource() != 0x08);
	}
	else
	{
		while (1)
		{
			// handler if something going wrong
		}
	}
}

void Gpio_Tim_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	
	TIM_TimeBaseInitTypeDef timer;
	TIM_OCInitTypeDef timerPWM;
	GPIO_InitTypeDef gpio_pin;
	
	GPIO_StructInit(&gpio_pin);
	gpio_pin.GPIO_Mode = GPIO_Mode_AF_PP;
	gpio_pin.GPIO_Pin = GPIO_Pin_6;
	gpio_pin.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOB, &gpio_pin);
	
	TIM_TimeBaseStructInit(&timer);
	timer.TIM_Prescaler = 720;
	timer.TIM_Period = 1024;
	timer.TIM_ClockDivision = 0;
	timer.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM4, &timer);
	
	TIM_OCStructInit(&timerPWM);
	timerPWM.TIM_OCMode = TIM_OCMode_PWM1;
	timerPWM.TIM_OutputState = TIM_OutputState_Enable;
	timerPWM.TIM_Pulse = 10;
	timerPWM.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC1Init(TIM4, &timerPWM);
	
	TIM_Cmd(TIM4, ENABLE);
}

void Init_Adc(void)
{
	ADC_InitTypeDef ADC_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	
	// config gpio_pin which equal to ADC1 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	// set clock to frequency needed for ADC (max 14MHz --> 72/6 = 12MHz)
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);
	// enable clock for bus APB2 -> ADC1
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	
	ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_28Cycles5);
	ADC_Init(ADC1, &ADC_InitStructure);
	
	// Enable adc1
	ADC_Cmd(ADC1, ENABLE);
	
	// ADC calibration part
	ADC_ResetCalibration(ADC1);
	while (ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1));
	
	ADC_Cmd(ADC1, ENABLE);
	ADC_SoftwareStartConvCmd(ADC1, ENABLE); // start conversion now in the main loop, we can get value from "ADC_GetConversionValue(ADC1);"
}

int main(void)
{
	int adc_value;
	
	SetSysClockTo72();
	Gpio_Tim_Init();
	Init_Adc();
	
	while (1)
	{
		adc_value = ADC_GetConversionValue(ADC1);
		if (adc_value / 4 <= 1024)
		{
			TIM4->CCR1 = adc_value / 4;// set pwm puls according with ADC value
		}
	}
}
