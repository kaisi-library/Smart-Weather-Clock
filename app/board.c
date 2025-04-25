#include "stm32f10x.h"


void board_lowlevel_init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);   // GPIOA
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);   // GPIOB
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);   // GPIOC
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);    // SPI1
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);  // USART1
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);  // USART2

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);      // DMA1

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);    // TIM2
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);    // TIM3
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);     // PWR
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP, ENABLE);     // BKP

    PWR_BackupAccessCmd(ENABLE);                            // Enable backup access
    BKP_DeInit();                                           // De-initialize backup domain

    RCC_LSEConfig(RCC_LSE_ON);                              // Enable LSE
    while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);     // Wait for LSE to be ready
}
