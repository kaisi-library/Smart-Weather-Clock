#include <stdbool.h>
#include <stdint.h>
#include "stm32f10x.h"
#include "timer.h"

static timer_elapsed_callback_t timer_elapsed_callback;


void timer_init(uint32_t period_us)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1;                           // Prescaler value for 1 MHz timer clock (72 MHz / 72 = 1 MHz)
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;             // Count up
    TIM_TimeBaseStructure.TIM_Period = period_us - 1;                       // Set the period for the timer
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;                 // No clock division
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;                        // No repetition
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);                              // Enable update interrupt

    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;                         // Timer2 interrupt
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;               // Preemption priority
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;                      // Sub-priority
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;                         // Enable the interrupt
    NVIC_Init(&NVIC_InitStructure);                                         // Initialize the NVIC

}

void timer_start(void)
{
    TIM_Cmd(TIM2, ENABLE);                                                  // Start the timer
}

void timer_stop(void)
{
    TIM_Cmd(TIM2, DISABLE);                                                 // Stop the timer
}

void timer_elapsed_register(timer_elapsed_callback_t callback)
{
    timer_elapsed_callback = callback;                                      // Register the callback function
}

void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)                      // Check if the update interrupt flag is set
    {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);                         // Clear the interrupt flag

        if (timer_elapsed_callback)                                         // Check if the callback is registered
        {
            timer_elapsed_callback();                                       // Call the callback function
        }
    }
}
