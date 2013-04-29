/*
 *	InfraLib - Embedded IR Transceiver Library.
 *
 * IR Signal Capture Driver for STM32-VL-Discovery Board.
 *
 * @author 	James Walmsley
 * @copy	(c) 2011 James Walmsley
 *
 * @license	Apache 2.0
 *
 */

#include <stdlib.h>
#include "../irManager.h"
#include "../irTransmitter.h"
#include "stm32-vldiscovery.h"
#include "stm32f10x.h"

static void RCC_Config();
static void GPIO_Config();
static void NVIC_Config();

static void IR_InitTimeoutTimer(void) {
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

	TIM_TimeBaseStructure.TIM_Period = 0xFFFF;
	TIM_TimeBaseStructure.TIM_Prescaler = (6 - 1);
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(IR_TO_TIM, &TIM_TimeBaseStructure);

	TIM_ClearITPendingBit(IR_TO_TIM, TIM_IT_Update);

	TIM_ITConfig(IR_TO_TIM, TIM_IT_Update, ENABLE);

	//TIM_Cmd(TIM7, ENABLE);
}

static int irInit(void *pParam) {
	TIM_ICInitTypeDef TIM_ICInitStructure;
	uint32_t			preScalerValue = SystemCoreClock/4000000;			// Divider to get timer to operate at 4 Mhz.

	RCC_Config();															// Initialise the Peripheral clocks.
	NVIC_Config();															// Setup the interrupt controller.
	GPIO_Config();															// Initialise the GPIO pins being used.

	TIM_ICInitStructure.TIM_Channel 	= TIM_Channel_2;					// Timer Channel 2 is used for PWM input capture.
	TIM_ICInitStructure.TIM_ICPolarity 	= TIM_ICPolarity_Falling;			// Start from falling edge.
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;			// Direct from Timer Input signal.
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;					// No division of Input capture signal.
	TIM_ICInitStructure.TIM_ICFilter 	= 0x0;								// No filter.

	TIM_PWMIConfig(IR_TIM, &TIM_ICInitStructure);							// Configure Timer for PWM input capture.

	TIM_SelectInputTrigger(IR_TIM, TIM_TS_TI2FP2);							// Select the Timer Input Trigger: TI2FP2.

	TIM_SelectSlaveMode(IR_TIM, TIM_SlaveMode_Reset);						// Select the slave Mode: Reset Mode.

	TIM_SelectMasterSlaveMode(IR_TIM, TIM_MasterSlaveMode_Enable);			// Enable the Master/Slave Mode.

	TIM_Cmd(IR_TIM, ENABLE);												// Enable timer counter.

	IR_TIM->PSC = (preScalerValue-1);										// Pre-scale timer input by 6 times.

	IR_InitTimeoutTimer();
	IR_TX_Init();
	IR_TX_EnableCarrier();


	TIM_ITConfig(IR_TIM, TIM_IT_CC2, ENABLE);								// Enable the timer Capture Compare 2 interrupt.

	return 0;
}

static void RCC_Config() {
	RCC_APB1PeriphClockCmd(IR_TIM_CLK, ENABLE);					///< Enable Timer Clock.
	RCC_APB1PeriphClockCmd(IR_TO_TIM_CLK, ENABLE);
	RCC_APB2PeriphClockCmd(IR_GPIO_CLK, ENABLE);				///< Enable GPIO Clock.
}

static void GPIO_Config() {
	  GPIO_InitTypeDef GPIO_InitStructure;
	  GPIO_InitStructure.GPIO_Pin 	= IR_GPIO_PIN;				///< Pin of IR signal.
	  GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_IN_FLOATING;	///< Floating input mode.
	  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;			///< Max GPIO speed. (10 Mhz > enough).
	  GPIO_Init(IR_GPIO_PORT, &GPIO_InitStructure);				///< Initialise the pin.
}

static void NVIC_Config() {
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel 						= IR_TIM_IRQn;	///<
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority 	= 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority 			= 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd 					= ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel 						= IR_TO_TIM_IRQn;	///<
	NVIC_Init(&NVIC_InitStructure);
}

__IO uint16_t IC2Value 	= 0;
__IO uint16_t IC1Value 	= 0;

static uint8_t bTimerStarted = 0;

//---- TIMER ISR for Input Capture.
void IR_TIM_IRQHandler(void)
{
	/* Clear TIM3 Capture compare interrupt pending bit */
	TIM_ClearITPendingBit(TIM2, TIM_IT_CC2);
	/*int i;*/

	// If the first capture start the timeout timer:
	// Start the Timeout Timer
	if(!bTimerStarted) {
		TIM_Cmd(IR_TO_TIM, ENABLE);
	}

	/* Get the Input Capture value */
	IC1Value = TIM_GetCapture1(TIM2);
	IC2Value = TIM_GetCapture2(TIM2);		// Get First capture value.

	// Convert to Microseconds.
	IR_Event(IC2Value/4, IC1Value/4, 0);	// Divide by 4, as each period is 25 ns ... 4 ticks per ns.
}

//---- TIMER ISR for Signal Timeout.
static uint16_t bFirstRun = 1;
void IR_TO_TIM_IRQHandler(void) {
	TIM_ClearITPendingBit(TIM7, TIM_IT_Update);

	/* Get the Input Capture value */
	IC1Value = TIM_GetCapture1(TIM2);
	IC2Value = TIM_GetCapture2(TIM2);	// Get First capture value.

	bTimerStarted = 0;

	if(!bFirstRun) {
		IR_Event(0,IC1Value/4, 1);		// Signal IR receive state machine that transmission has ended. (Including the last pulse length).
	} else {
		bFirstRun = 0;
	}
	TIM_Cmd(TIM7, DISABLE);
}

static uint16_t i = 0;
static int bFirstUpdate = 1;
static int bStop = 0;
static int bEndSignal = 0;
static int bFirstRun2 = 1;

static uint32_t bActive = 0;

static IR_WAVEFORM *pTxWaveform = NULL;

void TIM4_IRQHandler(void) {
	TIM_ClearITPendingBit(TIM4, TIM_IT_Update);

	if(bFirstRun2) {
		bFirstRun2 = 0;
		bStop = 1;
	}

	if(bEndSignal) {
		TIM4->ARR 	= 0xFFFF;
		TIM4->CCR2 	= 0;
		bStop = 1;
		bEndSignal = 0;
		return;
	}

	if(bStop) {
		TIM_Cmd(TIM4, DISABLE);
		i = 0;
		bFirstUpdate = 1;
		bActive = 0;
		bStop = 0;
		return;
	}

	if(!bFirstUpdate) {
		TIM4->ARR 	= pTxWaveform->arrWavePoints[i].usPeriodUs * 4;
		TIM4->CCR2 	= pTxWaveform->arrWavePoints[i].usDutyCycleUs * 4;
		i++;
	}

	if(bFirstUpdate) {
		bFirstUpdate = 0;
	}

	if(i == (pTxWaveform->usWavePoints)) {
		bEndSignal = 1;
	}
}

int irTransmit(IR_WAVEFORM *poWaveform, uint32_t ulCarrierFrequency, void *pParam) {
	pTxWaveform = poWaveform;
	bActive = 1;

	IR_TX_Init();

	TIM_ClearFlag(TIM4, TIM_FLAG_Update);
	TIM_Cmd(TIM4, ENABLE);					// Start transfer.

	while(bActive);							// Poll on completion.

	return 0;
}


static const IR_DRIVER g_irDriver = {
	.pfnInit			= irInit,
	.pfnClose			= NULL,
	.pfnTransmit		= irTransmit,
	.szpName			= "STM32-VL-Discovery IR Driver",
	.szpVersionString 	= "v1.0.0 (C)2011 James Walmsley",
};

const IR_DRIVER *IR_STM32_Discovery_GetDriver() {
	return &g_irDriver;
}
