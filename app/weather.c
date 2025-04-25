#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "weather.h"


// 示例输入：
//  {
//    "results": [
//        {
//          "location": {
//            "id": "WTSQQYHVQ973",
//            "name": "南京",
//            "country": "CN",
//            "path": "南京,南京,江苏,中国",
//            "timezone": "Asia/Shanghai",
//            "timezone_offset": "+08:00"
//          },
//          "now": {
//            "text": "晴",
//            "code": "0",
//            "temperature": "24"
//          },
//          "last_update": "2025-04-20T12:00:12+08:00"
//        }
//      ]
//    }

// OK


bool weather_parse(const char *data, weather_t *weather)
{
    char *p = strstr(data, "\"text\":\"");          // 查找天气描述的起始位置
    if (p == NULL)                                  // 如果没有找到，返回false
    {
        return false;
    }

    p += strlen("\"text\":\"");                     // 移动到天气描述的起始位置     

    char *q = strchr(p, '\"');                      // 查找天气描述的结束位置
    if (q == NULL)                                  // 如果没有找到，返回false    
    {
        return false;
    }

    int len = q - p;                                // 计算天气描述的长度 
    if (len >= sizeof(weather->weather))            // 如果长度超过了数组的大小，截断
    {
        len = sizeof(weather->weather) - 1;         // 保留一个字符用于结束符
    }

    strncpy(weather->weather, p, len);              // 复制天气描述到结构体中  
    weather->weather[len] = '\0';                   // 添加结束符

    p = strstr(data, "\"temperature\":\"");         // 查找温度的起始位置
    if (p == NULL)                                  // 如果没有找到，返回false
    {
        return false;
    }

    p += strlen("\"temperature\":\"");              // 移动到温度的起始位置
    q = strchr(p, '\"');                            // 查找温度的结束位置
    if (q == NULL)
    {
        return false;
    }

    len = q - p;                                    // 计算温度的长度
    if (len >= sizeof(weather->temperature))        // 如果长度超过了数组的大小，截断
    {
        len = sizeof(weather->temperature) - 1;     // 保留一个字符用于结束符
    }

    strncpy(weather->temperature, p, len);          // 复制温度到结构体中
    weather->temperature[len] = '\0';               // 添加结束符

    return true;
}
