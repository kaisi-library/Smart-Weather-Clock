#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "stm32f10x.h"
#include "main.h"
#include "led.h"
#include "rtc.h"
#include "timer.h"
#include "mpu6050.h"
#include "esp_at.h"
#include "st7735.h"
#include "stfonts.h"
#include "stimage.h"
#include "weather.h"

static uint32_t runms;                                      // Run time in ms
static uint32_t disp_height;                                // Display line height


static const char *wifi_ssid = "open114";
static const char *wifi_password = "12345600";
static const char *weather_url = "https://api.seniverse.com/v3/weather/now.json?key=SPL1rXU8Q4YkyqnAn&location=nanjing&language=en&unit=c";


static void timer_elapsed_callback(void)
{
    runms++;
    if (runms >= 24 * 60 * 60 * 1000)                       // Every day
    {
        runms = 0;                                          // Reset runms to 0
    }
}

static void wifi_init(void)
{
    //显示ESP32初始化信息
    st7735_write_string(0, disp_height, "Init ESP32...", &font_ascii_8x16, ST7735_WHITE, ST7735_BLACK);
    disp_height += font_ascii_8x16.height;
    if(!esp_at_init())                                                                                          // Initialize ESP32
    {
        st7735_write_string(0, disp_height, "Failed!!!", &font_ascii_8x16, ST7735_RED, ST7735_BLACK);
        disp_height += font_ascii_8x16.height;

        while(1);                                                                                               //esp32初始化失败，死循环等待
    }


    st7735_write_string(0, disp_height, "Init WIFI...", &font_ascii_8x16, ST7735_WHITE, ST7735_BLACK);
    disp_height += font_ascii_8x16.height;
    if(!esp_at_wifi_init())                                                                                     // Initialize WiFi
    {
        st7735_write_string(0, disp_height, "Failed!!!", &font_ascii_8x16, ST7735_RED, ST7735_BLACK);
        disp_height += font_ascii_8x16.height;

        while(1);                                                                                               //esp32 wifi初始化失败，死循环等待
    }


    st7735_write_string(0, disp_height, "Connect WIFI...", &font_ascii_8x16, ST7735_WHITE, ST7735_BLACK);
    disp_height += font_ascii_8x16.height;
    if(!esp_at_wifi_connect(wifi_ssid, wifi_password))                                                          // Connect to WiFi
    {
        st7735_write_string(0, disp_height, "Failed!!!", &font_ascii_8x16, ST7735_RED, ST7735_BLACK);
        disp_height += font_ascii_8x16.height;

        while(1);                                                                                               //esp32 wifi connect失败，死循环等待
    }

    st7735_write_string(0, disp_height, "Sync Time...", &font_ascii_8x16, ST7735_WHITE, ST7735_BLACK);
    disp_height += font_ascii_8x16.height;
    if(!esp_at_sntp_init())                                                          //初始化SNTP(Simple Network Time Protocal简单网络时间协议)
    {
        st7735_write_string(0, disp_height, "Failed!!!", &font_ascii_8x16, ST7735_RED, ST7735_BLACK);
        disp_height += font_ascii_8x16.height;

        while(1);                                                                                               //同步时间失败，死循环等待
    }
}

int main(void)
{
    board_lowlevel_init();

    led_init();
    rtc_init();                                                                                         // Initialize RTC
    timer_init(1000);                                                                                   // Initialize timer with 1 ms period
    timer_elapsed_register(timer_elapsed_callback);                                                     // Register timer elapsed callback
    timer_start();                                                                                      // Start timer

    mpu6050_init();                                                                                     // Initialize MPU6050

    st7735_init();
    st7735_fill_screen(ST7735_BLACK);

    //显示开机内容
    st7735_write_string(0, 0, "Initializing...", &font_ascii_8x16, ST7735_WHITE, ST7735_BLACK);         // Display initializing message
    disp_height += font_ascii_8x16.height;                                                              // Update line height
    //等待ESP32模块初始化完成
    delay_ms(1000);                                                                                     // Delay for 1 second

    st7735_write_string(0, disp_height, "Wait ESP32...", &font_ascii_8x16, ST7735_WHITE, ST7735_BLACK); // Display initializing message
    disp_height += font_ascii_8x16.height;                                                              // Update line height
    //等待ESP32模块初始化完成
    delay_ms(1500);                                                                                     // Delay for 1.5 seconds

    wifi_init();                                                                                        // Initialize WiFi

    st7735_write_string(0, disp_height, "Ready", &font_ascii_8x16, ST7735_GREEN, ST7735_BLACK);
    disp_height += font_ascii_8x16.height;
    delay_ms(500);                                                                                      // Delay for 500 ms

    st7735_fill_screen(ST7735_BLACK);                                                                   // Clear screen

    runms = 0;                                                                                          // Reset runms to 0
    uint32_t last_rumms = runms;
    bool weather_ok = false;
    bool sntp_ok = false;
    bool ip_ok = false;
    bool mac_ok = false;
    char str[64];
    while(1)
    {
        if(runms == last_rumms)                                                                         // 避免忙等
        {
            continue;
        }
        last_rumms = runms;                                                                             // 更新上次运行时间

        //更新时间信息：100 ms 显示一次
        if(last_rumms % 100 == 0)
        {
            rtc_date_t date;
            rtc_get_date(&date);                                                                        // 获取当前日期和时间

            snprintf(str, sizeof(str), "%02d-%02d-%02d", date.year % 100, date.month, date.day);
            st7735_write_string(0, 0, str, &font_ascii_8x16, ST7735_WHITE, ST7735_BLACK);
            snprintf(str, sizeof(str), "%02d%s%02d", date.hour, date.second % 2 ? " " : ":", date.minute);
            st7735_write_string(0, 62, str, &font_time_24x48, ST7735_CYAN, ST7735_BLACK);
        }

        //联网同步时间：1 hour 显示一次
        if(!sntp_ok || last_rumms % (60 * 60 * 1000) == 0)
        {
            uint32_t ts;
            sntp_ok = esp_at_get_time(&ts);                                                             //获取当前时间戳
            if (!sntp_ok)                                                                               //判断是否获取时间戳成功
            {
                snprintf(str, sizeof(str), "Failed to get time");
                st7735_write_string(0, 130, str, &font_ascii_8x16, ST7735_RED, ST7735_BLACK);
                delay_ms(1000);                                                                         //延时1s，显示获取时间戳成功
                snprintf(str, sizeof(str), "                ");                                         //清除显示的时间戳
                st7735_write_string(0, 130, str, &font_ascii_8x16, ST7735_BLACK, ST7735_BLACK);
            }
            else if (ts == 0)                                                                           //判断获取的时间戳是否为0
            {
                snprintf(str, sizeof(str), "Failed to get time");
                st7735_write_string(0, 130, str, &font_ascii_8x16, ST7735_RED, ST7735_BLACK);
                delay_ms(1000);                                                                         //延时1s，显示获取时间戳成功
                snprintf(str, sizeof(str), "                ");                                         //清除显示的时间戳
                st7735_write_string(0, 130, str, &font_ascii_8x16, ST7735_BLACK, ST7735_BLACK);
            }
            else if (ts == 0xFFFFFFFF)                                                                  //判断获取的时间戳是否为0xFFFFFFFF
            {
                snprintf(str, sizeof(str), "Failed to get time");
                st7735_write_string(0, 130, str, &font_ascii_8x16, ST7735_RED, ST7735_BLACK);
                delay_ms(1000);                                                                         //延时1s，显示获取时间戳成功
                snprintf(str, sizeof(str), "                ");                                         //清除显示的时间戳
                st7735_write_string(0, 130, str, &font_ascii_8x16, ST7735_BLACK, ST7735_BLACK);

            }
            else                                                                                        //获取时间戳成功
            {
                snprintf(str, sizeof(str), "Time obtained");
                st7735_write_string(0, 130, str, &font_ascii_8x16, ST7735_RED, ST7735_BLACK);
                delay_ms(1000);                                                                         //延时1s，显示获取时间戳成功
                snprintf(str, sizeof(str), "                ");                                         //清除显示的时间戳
                st7735_write_string(0, 130, str, &font_ascii_8x16, ST7735_BLACK, ST7735_BLACK);
            }

            rtc_set_timestamp(ts + 8 * 60 *60);                                                         //设置时区为东八区,即北京时间
        }

        //获取天气信息：10 minutes 更新一次
        if(!weather_ok || last_rumms % (10 * 60 * 1000) == 0)
        {
            const char *response;
            weather_ok = esp_at_get_http(weather_url, &response, NULL, 10000);                          // 从网址获取天气数据
            if (!weather_ok)                                                                            //判断是否获取天气数据成功
            {
                snprintf(str, sizeof(str), "Failed to get weather");
                st7735_write_string(0, 130, str, &font_ascii_8x16, ST7735_RED, ST7735_BLACK);
                delay_ms(1000);                                                                         //延时1s，显示获取时间戳成功
                snprintf(str, sizeof(str), "                ");                                         //清除显示的时间戳
                st7735_write_string(0, 130, str, &font_ascii_8x16, ST7735_BLACK, ST7735_BLACK);
                continue;
            }
            else if (response == NULL)                                                                  //判断获取的天气数据是否为NULL
            {
                snprintf(str, sizeof(str), "Failed to get weather");
                st7735_write_string(0, 130, str, &font_ascii_8x16, ST7735_RED, ST7735_BLACK);
                delay_ms(1000);                                                                           //延时10ms，显示获取时间戳成功
                snprintf(str, sizeof(str), "                ");                                         //清除显示的时间戳
                st7735_write_string(0, 130, str, &font_ascii_8x16, ST7735_BLACK, ST7735_BLACK);
                continue;
            }
            else if (strlen(response) == 0)                                                              //判断获取的天气数据是否为空
            {
                snprintf(str, sizeof(str), "Failed to get weather");
                st7735_write_string(0, 130, str, &font_ascii_8x16, ST7735_RED, ST7735_BLACK);
                delay_ms(1000);                                                                         //延时1s，显示获取时间戳成功
                snprintf(str, sizeof(str), "                ");                                         //清除显示的时间戳
                st7735_write_string(0, 130, str, &font_ascii_8x16, ST7735_BLACK, ST7735_BLACK);
                continue;
            }
            else                                                                                         //获取天气数据成功
            {
                snprintf(str, sizeof(str), "Weather obtained");
                st7735_write_string(0, 130, str, &font_ascii_8x16, ST7735_RED, ST7735_BLACK);
                delay_ms(1000);                                                                         //延时1s，显示获取时间戳成功
                snprintf(str, sizeof(str), "                ");                                         //清除显示的时间戳
                st7735_write_string(0, 130, str, &font_ascii_8x16, ST7735_BLACK, ST7735_BLACK);
            }

            weather_t weather;
            weather_parse(response, &weather);                                                         // 解析天气数据

            //snprintf(str, sizeof(str), "%s", "nanjing");                                             //显示城市名称
            //st7735_write_string(0, 64, str, &font_ascii_8x16, ST7735_WHITE, ST7735_BLACK);
            // snprintf(str, sizeof(str), "%s", weather.weather);                                      //显示天气情况
            // st7735_write_string(0, 16, str, &font_ascii_8x16, ST7735_YELLOW, ST7735_BLACK);

            const st_image_t *img = NULL;
            if (strcmp(weather.weather, "Cloudy") == 0) {
                img = &icon_weather_duoyun;
            } else if (strcmp(weather.weather, "Wind") == 0) {
                img = &icon_weather_feng;
            } else if (strcmp(weather.weather, "Clear") == 0) {
                img = &icon_weather_qing;
            } else if (strcmp(weather.weather, "Snow") == 0) {
                img = &icon_weather_xue;
            } else if (strcmp(weather.weather, "Overcast") == 0) {
                img = &icon_weather_yin;
            } else if (strcmp(weather.weather, "Rain") == 0) {
                img = &icon_weather_yu;
            }
            if (img != NULL) {
                st7735_draw_image(0, 16, img->width, img->height, img->data);
            } else {
                snprintf(str, sizeof(str), "%s", weather.weather);
                st7735_write_string(0, 16, str, &font_ascii_8x16, ST7735_YELLOW, ST7735_BLACK);
            }


            snprintf(str, sizeof(str), "%sC", weather.temperature);                                     //显示温度
            st7735_write_string(78, 0, str, &font_temper_16x32, ST7735_BLUE, ST7735_BLACK);
        }

        //获取环境温度信息：1 seconds 更新一次
        if(last_rumms % (1 * 1000) == 0)
        {
            float temperature = mpu6050_read_temper();                                                  //获取当前温度
            snprintf(str, sizeof(str), "%5.1fC", temperature);                                          //格式化温度数据
            st7735_write_string(78, 32, str, &font_ascii_8x16, ST7735_GREEN, ST7735_BLACK);
        }

        //获取网络信息：30 seconds 更新一次
        if(!ip_ok || !mac_ok || last_rumms % (30 * 1000) == 0)
        {   
            snprintf(str, sizeof(str), "ssid:");
            st7735_write_string(0, 159 - 48, str, &font_ascii_8x16, ST7735_WHITE, ST7735_BLACK);    
            st7735_write_string(49, 159 - 48, wifi_ssid, &font_ascii_8x16, ST7735_WHITE, ST7735_BLACK); //显示wifi名称

            char ip[16];
            ip_ok = esp_at_wifi_get_ip(ip);                                                             //获取当前ip地址
            st7735_write_string(0, 159 - 16, ip, &font_ascii_8x16, ST7735_WHITE, ST7735_BLACK);         //显示ip地址

//            char mac[18];
//            mac_ok = esp_at_wifi_get_mac(mac);                                                        //获取当前mac地址
//            st7735_write_string(0, 159 - 32, mac, &font_ascii_8x16, ST7735_WHITE, ST7735_BLACK);      //显示mac地址
       }
    }


}

