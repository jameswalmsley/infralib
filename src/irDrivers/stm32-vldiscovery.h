/*
 * stm32-vldiscovery.h
 *
 *  Created on: 21 Apr 2011
 *      Author: James
 */

#ifndef STM32_VLDISCOVERY_H_
#define STM32_VLDISCOVERY_H_

/*
 * GPIO Port/Clock/Pin configuration.
 */
#define IR_GPIO_PORT		GPIOA								///< Port which the IR signal is connected.
#define IR_GPIO_CLK			RCC_APB2Periph_GPIOA				///< Clock of port the IR signal is connected.
#define IR_GPIO_PIN			GPIO_Pin_1							///< Pin which the IR signal is connected.

/*
 * TIMER configuration for Input Capture.
 */
#define IR_TIM				TIM2								///< Timer to use for IR decoding.
#define IR_TIM_CLK			RCC_APB1Periph_TIM2					///< Clock of the timer being used for decoding.
#define IR_TIM_IRQn			TIM2_IRQn							///< IRQ number of the used timer.
#define IR_TIM_IRQHandler	TIM2_IRQHandler						///< Interrupt Handler for the timer used.

/*
 * TIMER configuration for Rx Timeout operation.
 */
#define IR_TO_TIM				TIM7							///< Timer to use for IR rx timeout.
#define IR_TO_TIM_CLK			RCC_APB1Periph_TIM7
#define IR_TO_TIM_IRQn			TIM7_IRQn
#define IR_TO_TIM_IRQHandler	TIM7_IRQHandler

const IR_DRIVER *IR_STM32_Discovery_GetDriver();

#endif /* STM32_VLDISCOVERY_H_ */
