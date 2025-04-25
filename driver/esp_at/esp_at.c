#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "main.h"
#include "esp_usart.h"
#include "esp_at.h"


#define RX_BUFFER_SIZE 4096


#define RX_RESULT_OK    0
#define RX_RESULT_ERROR 1
#define RX_RESULT_FAIL  2


volatile uint8_t rxdata[RX_BUFFER_SIZE];
volatile uint32_t rxlen;
volatile bool rxready;
volatile uint8_t rxresult;

static void on_usart_received(uint8_t data)
{
    // 没有数据请求，不接收数据
    if (!rxready)
    {
        return;
    }

    // 接收数据
    if(rxlen < RX_BUFFER_SIZE)
    {
        rxdata[rxlen++] = data;
    }
    else
    {
        // 数据长度超过缓冲区大小，丢弃数据
        rxresult = RX_RESULT_FAIL;
        rxready = false;
        return;
    }

    // 检查数据接收完毕
    if(data == '\n')
    {
        // 判断接收换行符是否为\r\n
        if(rxlen > 1 && rxdata[rxlen - 2] == '\r')
        {
            //收到OK
            if(rxlen >= 4 && rxdata[rxlen - 4] == 'O' && rxdata[rxlen - 3] == 'K')
            {
                rxresult = RX_RESULT_OK;
                rxready = false;
            }
        }

        //收到ERROR
        else if(rxlen >= 7 &&
                rxdata[rxlen - 7] == 'E' && rxdata[rxlen - 6] == 'R'
                && rxdata[rxlen - 5] == 'R' && rxdata[rxlen - 4] == 'O'
                && rxdata[rxlen - 3] == 'R' )
        {
            rxresult = RX_RESULT_ERROR;
            rxready = false;
        }

    }
}

bool esp_at_init(void)
{
    rxready = false;

    esp_usart_init();
    esp_usart_receive_register(on_usart_received);

    return esp_at_reset();
}

bool esp_at_send_command(const char *cmd, const char **response, uint32_t *length, uint32_t timeout)
{
    // 清空接收缓冲区
    rxlen = 0;
    rxready = true;
    rxresult = RX_RESULT_FAIL;

    // 发送AT命令
    esp_usart_write_string(cmd);
    esp_usart_write_string("\r\n");

    // 判断超时
    while(rxready && timeout -- )
    {
        delay_ms(1);
    }

    rxready = false;                        // 关闭接收

    if(response)
    {
        *response = (const char *)rxdata;   // 返回接收数据
    }

    if(length)
    {
        *length = rxlen;                    // 返回接收数据长度
    }

    // 判断接收结果
    return rxresult == RX_RESULT_OK;
}

bool esp_at_send_data(const uint8_t *data, uint32_t length)
{
    esp_usart_write_data((uint8_t *)data, length);

    return true;
}

bool esp_at_reset(void)
{
    //复位esp32
    if(!esp_at_send_command("AT+RESTORE", NULL, NULL, 1000))
    {
        return false;
    }
    //等待esp32复位完成
    delay_ms(2000);
    //关闭回显
    if(!esp_at_send_command("ATE0", NULL, NULL, 1000))
    {
        return false;
    }
    //关闭存储
    if(!esp_at_send_command("AT+SYSSTORE=0", NULL, NULL, 1000))
    {
        return false;
    }
    return true;
}

bool esp_at_wifi_init(void)
{
    //设置为station模式
    if(!esp_at_send_command("AT+CWMODE=1", NULL, NULL, 1000))
    {
        return false;
    }

    return true;
}

bool esp_at_wifi_connect(const char *ssid, const char *password)
{
    //连接wifi
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"", ssid, password);

    if(!esp_at_send_command(cmd, NULL, NULL, 10000))
    {
        return false;
    }

    return true;
}

bool esp_at_get_http(const char *url, const char **response, uint32_t *length, uint32_t timeout)
{
    char cmd[128];

    //获取http资源
    snprintf(cmd, sizeof(cmd), "AT+HTTPCGET=\"%s\"", url);
    if(!esp_at_send_command(cmd, response, length, 10000))
    {
        return false;
    }
    return true;
}

bool esp_at_sntp_init(void)
{
    //设置SNTP模式
    if(!esp_at_send_command("AT+CIPSNTPCFG=1,8,\"cn.ntp.org.cn\",\"ntp.sjtu.edu.cn\"", NULL, NULL, 1000))
    {
        return false;
    }

    //查询SNTP时间
    if(!esp_at_send_command("AT+CIPSNTPTIME?", NULL, NULL, 1000))
    {
        return false;
    }
    return true;
}

bool esp_at_get_time(uint32_t *timestamp)
{
    const char *response;
    uint32_t length;

    //获取本地时间戳
    if(!esp_at_send_command("AT+SYSTIMESTAMP?", &response, &length, 1000))
    {
        return false;
    }

    char *sts =strstr(response, "+SYSTIMESTAMP:") + strlen("+SYSTIMESTAMP:");

    *timestamp = atoi(sts);

    return true;
}

bool esp_at_wifi_get_ip(char ip[16])
{
    const char *response;
    if (!esp_at_send_command("AT+CIPSTA?", &response, NULL, 1000))
    {
        return false;
    }

    //返回类型
    //  +CIPSTA:ip:<"ip">
    //  +CIPSTA:gateway:<"gateway">
    //  +CIPSTA:netmask:<"netmask">
    //  +CIPSTA:ip6ll:<"ipv6 addr">
    //  +CIPSTA:ip6gl:<"ipv6 addr">
    //  OK

    // 解析ip地址
    const char *pip = strstr(response, "+CIPSTA:ip:") + strlen("+CIPSTA:ip:");              //定位到ip地址
    if (pip)
    {
        for (int i = 0; i < 16; i++)
        {
            if (pip[i] == '\r')                 //找到第一个回车符
            {
                ip[i] = '\0';
                break;
            }
            ip[i] = pip[i];                     //复制ip地址
        }
        return true;                            //成功获取ip地址
    }

    return false;                               //没有找到ip地址
}

//xx:xx:xx:xx:xx:xx
bool esp_at_wifi_get_mac(char mac[18])
{
    const char *response;
    if (!esp_at_send_command("AT+CIPSTAMAC?", &response, NULL, 1000))
    {
        return false;
    }

    //返回类型
    //  +CIPSTAMAC:<mac>
    //  OK

    // 解析mac地址
    const char *pmac = strstr(response, "+CIPSTAMAC:mac:") + strlen("+CIPSTAMAC:mac:");     //定位到mac地址
    if (pmac)
    {
        strncpy(mac, pmac, 18);                 //复制mac地址
        return true;                            //成功获取mac地址
    }

    return false;                               //没有找到mac地址
}

