#include <stdbool.h>
#include <stdint.h>
#include "stm32f10x.h"
#include "main.h"

#define SCL_PORT    GPIOB
#define SCL_PIN     GPIO_Pin_6
#define SDA_PORT    GPIOB
#define SDA_PIN     GPIO_Pin_7

#define SCL_HIGH()  GPIO_SetBits(SCL_PORT, SCL_PIN)
#define SCL_LOW()   GPIO_ResetBits(SCL_PORT, SCL_PIN)
#define SDA_HIGH()  GPIO_SetBits(SDA_PORT, SDA_PIN)
#define SDA_LOW()   GPIO_ResetBits(SDA_PORT, SDA_PIN)
#define SDA_READ()  GPIO_ReadInputDataBit(SDA_PORT, SDA_PIN)
#define DELAY()     i2c_delay()

static void i2c_delay(void)
{
    delay_ms(5);
}

static void i2c_start(void)
{
    SDA_HIGH();
    SCL_HIGH();
    DELAY();
    SDA_LOW();
    DELAY();
    SCL_LOW();
}

static void i2c_stop(void)
{
    SDA_LOW();
    DELAY();
    SCL_HIGH();
    DELAY();
    SDA_HIGH();
    DELAY();
}

static bool i2c_write_byte(uint8_t data)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        if (data & 0x80)
            SDA_HIGH();
        else
            SDA_LOW();
        DELAY();
        SCL_HIGH();
        DELAY();
        SCL_LOW();
        data <<= 1;
    }
    SDA_HIGH();						//主机释放SDA控制权，从机把数据放到SDA
    DELAY();
    SCL_HIGH();						//SCL置高电平，主机接收从机发出的应答位
    DELAY();
    if (SDA_READ())				    //读取SDA，判断应答位
									//0：接收到应答		1：未接收到应答
    {
        SCL_LOW();
				return false;		//读取到SDA数据为0，未接收到应答
    }
    SCL_LOW();
    DELAY();
    return true;					//读取到SDA数据为1，接收到应答
}

static uint8_t i2c_read_byte(bool ack)
{
    uint8_t data = 0;
    SDA_HIGH();						//主机释放SDA控制权，从机把数据放到SDA
    for (uint8_t i = 0; i < 8; i++)
    {
        data <<= 1;
        SCL_HIGH();				    //SCL置高电平，主机接收从机发出的数据
        DELAY();
			if (SDA_READ())			//读取SDA的数据
									//读取到数据为1时，将data最低位置1
            data |= 0x01;
        SCL_LOW();
        DELAY();
    }
    if (ack)
				SDA_LOW();			//SDA置0，发送应答为0，继续接收数据
    else
        SDA_HIGH();				    //SDA置1，发送应答为1，停止接收数据
    DELAY();
    SCL_HIGH();						//SCL置1，从机接收主机发送的应答数据
    DELAY();
    SCL_LOW();
    SDA_HIGH();						//终止信号
    DELAY();
    return data;
}

static void i2c_io_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Pin = SCL_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SCL_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Pin = SDA_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SDA_PORT, &GPIO_InitStructure);
}

void swi2c_init(void)
{
    i2c_io_init();
}

bool swi2c_write(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t length)
{
    i2c_start();															//起始信号
    if (!i2c_write_byte(addr << 1))						                    //i2c_write_byte(addr << 1)	写地址	左移一位
																			//MPU6050地址：	1101 000 	+ r/w
																			//指定从机地址		addr			+	0(写命令)
    {
        i2c_stop();
        return false;													    //i2c_write_byte函数返回false，表示从机无应答，直接停止放回false
    }
    i2c_write_byte(reg);											        //指定寄存器地址
    for (uint16_t i = 0; i < length; i++)			                        //写数据
    {
        i2c_write_byte(data[i]);
    }
    i2c_stop();																//终止信号
    return true;
}

bool swi2c_read(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t length)
{
    i2c_start();
    if (!i2c_write_byte(addr << 1))						                    //指定从机地址		addr			+	0(写命令)
																			//if语句判断从机有无应答，无应答直接结束，返回值为false
    {
        i2c_stop();
        return false;
    }
    i2c_write_byte(reg);											        //指定寄存器地址
    i2c_start();															//重复起始信号
    i2c_write_byte((addr << 1) | 0x01);				                        //指定从机地址		addr			+	1(读命令)
    for (uint16_t i = 0; i < length; i++)
    {
        data[i] = i2c_read_byte(i == length - 1 ? false : true);
    }
    i2c_stop();																//终止信号
    return true;
}
