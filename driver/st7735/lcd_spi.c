#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "stm32f10x.h"
#include "lcd_spi.h"


static lcd_spi_send_finish_callback_t lcd_spi_send_finish_callback;


static void lcd_spi_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);  // SPI1_SCK
    GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_SET);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);  // SPI1_MOSI
    GPIO_WriteBit(GPIOA, GPIO_Pin_7, Bit_SET);
}

static void lcd_spi_nvic_init(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel3_IRQn;		//SPI1_TX的DMA通道
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

static void lcd_spi_lowlevel_init(void)
{
    SPI_InitTypeDef SPI_InitStructure;
    SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;		//单线只发送模式
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;								//SPI模式：主模式(时钟发送方)
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;						//数据长度(8bit/16bit)
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;									//SCK起始电平
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;								//SCK第几个边沿采样(移入数据)
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;										//NSS模式：硬件/软件
	//我们计划NSS引脚使用GPIO模拟，外设NSS引脚并不会用到，所以选择SPI_NSS_Soft
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;  // spi clock = 72MHz / 4 = 18MHz
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;					//高位先行
    SPI_Init(SPI1, &SPI_InitStructure);
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);						//SPI_I2S_DMAReq_Tx		启用SPI1外设的DMA请求发送功能
    SPI_Cmd(SPI1, ENABLE);																			//SPI使能
}

void lcd_spi_init(void)
{
    lcd_spi_gpio_init();
    lcd_spi_nvic_init();
    lcd_spi_lowlevel_init();
}

void lcd_spi_write(uint8_t *data, uint16_t length)							//阻塞写入
{
    for (uint16_t i = 0; i < length; i++)
    {
        while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
		//循环，等待SPI_TXE置1退出循环
        SPI_I2S_SendData(SPI1, data[i]);
    }
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);
		//SPI_BSY SPI忙标志位	0：不忙		1：SPI正在通信或者发送缓存非空
		//等待移位寄存器发送完毕，所有数据发送结束
}

void lcd_spi_write_async(const uint8_t *data, uint16_t length)				//异步写入
{
    DMA_DeInit(DMA1_Channel3);
    DMA_InitTypeDef DMA_InitStructure;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&SPI1->DR;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)data;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize = length;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel3, &DMA_InitStructure);
    DMA_ITConfig(DMA1_Channel3, DMA_IT_TC, ENABLE);
    DMA_Cmd(DMA1_Channel3, ENABLE);
}

void lcd_spi_send_finish_register(lcd_spi_send_finish_callback_t callback)
{
    lcd_spi_send_finish_callback = callback;
}

void DMA1_Channel3_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA1_IT_TC3) == SET)
    {
        while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);

        if (lcd_spi_send_finish_callback)
            lcd_spi_send_finish_callback();

        DMA_ClearITPendingBit(DMA1_IT_TC3);
    }
}
