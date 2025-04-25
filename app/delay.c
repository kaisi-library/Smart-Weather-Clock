#include <stdint.h>
#include "stm32f10x.h"

void delay_us(uint16_t us)
{
   SysTick->LOAD = (SystemCoreClock / 1000000) * us - 1; // Load the SysTick counter value
   SysTick->VAL = 0; // Clear the current value of the SysTick counter 
   SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk; // Start the SysTick timer
   while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0); // Wait until the COUNTFLAG is set
   SysTick->CTRL = 0; // Disable the SysTick timer
}

void delay_ms(uint16_t ms)
{
   for (uint16_t i = 0; i < ms; i++)
   {
      delay_us(1000); // Call the delay_us function to create a 1ms delay
   }
}
