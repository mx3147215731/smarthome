#include <stdio.h>
#include <wiringPi.h>


#ifndef __GDEVICE_H__
#define __GDEVICE_H__


struct gdevice
{
char dev_name[128]; //设备名称
int key; //key值，用于匹配控制指令的值
int gpio_pin; //控制的gpio引脚  6,7,8,9
int gpio_mode; //输入输出模式  INPUT  OUTPUT  -1
int gpio_status; //高低电平状态  LOW HIGH  -1
int check_face_status; //是否进行人脸检测状态
int voice_set_status; //是否语音语音播报
struct gdevice *next;
};

//向设备列表中 添加 设备

// 使用ini优化后用不到了
// struct gdevice *add_device_to_gdevice_list(struct gdevice *pgdevhead, struct gdevice *gdev); 

// 在设备链表中找到设备 -- 根据 key 值去匹配 --> buffer[2] -- 控制设置号   buffer[3]--对应设备状态
struct gdevice *find_device_by_key(struct gdevice *pgdevhead, int key);

int set_gpio_device_status(struct gdevice *pgdev);

#endif 