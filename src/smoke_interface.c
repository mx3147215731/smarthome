
#include <pthread.h>
#include <wiringPi.h>
#include <stdio.h>

#include "smoke_interface.h"
#include "control.h"
#include "mq_queue.h"
#include "global.h"
#include <netinet/tcp.h> // 设置 tcp 心跳 的参数

#define SMOKE_PIN   6 // 烟雾报警模块接的引脚
#define SMOKE_MODE INPUT

static int s_fd = -1;

static int  smoke_init(void)
{
    printf("%s|%s|%d\n",__FILE__,__func__,__LINE__);
    pinMode(SMOKE_PIN, SMOKE_MODE); // 引脚 和 模式配置
    return 0;
}

static void smoke_final(void) 
{
  // do nothing
}

static void* smoke_get(void *arg)
{
  // AA 55 45 00  55 AA -->  45 00 -->触发警报
  int status = HIGH; //低电平有效 -- 默认设置为高电平
  int switch_status = 0;  // 报警开关 -- 默认设置为不开 -- 0
  ssize_t byte_send = -1;
  unsigned char buffer[6] = {0xAA,0x55,0x00,0x00,0x55,0xAA};
  mqd_t mqd = -1;
  ctrl_info_t * ctrl_info = NULL; 

   if(NULL != arg)
        ctrl_info = (ctrl_info_t*)arg;
   if(NULL != ctrl_info)
         mqd = ctrl_info->mqd;      
    
    
    if((mqd_t)-1 == mqd)
    {
       pthread_exit(0);  
    }

  pthread_detach(pthread_self());  // 父子线程分离
  printf("%s thread start.\n",__func__);
  
  while(1)
  {
    status = digitalRead(SMOKE_PIN); // 读取当前引脚状态
    if(LOW == status) //  探测到烟雾 -- 发生报警
    {
     switch_status = 1; // 打开报警器开关
     buffer[2] = 0x45;
     buffer[3] = 0x00;// 低电平触发警报 --
     //蜂鸣器是低电平触发 --> 我们这里把buffer 修改得与beep匹配，方便与他产生联系
     printf("%s|%s|%d,  0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",__FILE__,__func__,__LINE__,buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5]);
       
     byte_send = mq_send(mqd, buffer, 6,0); // 向消息队列里面发送数据 -- 接收到后语言模块会识别播报 - 火灾警报
      if (-1 == byte_send)
      {
        continue;
      }
    }
    else if(HIGH == status && 1 == switch_status) // 未探测到烟雾，并且报警器开关还没关闭 -- 关闭报警器开关
    {
      switch_status = 0; //  关闭报警器开关
      buffer[2] = 0x45;
      buffer[3] = 0x01;//警报结束
      printf("%s|%s|%d,  0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",__FILE__,__func__,__LINE__,buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5]);
       
      byte_send = mq_send(mqd, buffer, 6,0);
      if (-1 == byte_send)
      {
        continue;
      }
    }
    sleep(5);

  }


      
  pthread_exit(0); // 退出线程
}


struct  control smoke_control ={
    .control_name = "smoke",
    .init = smoke_init,
    .final = smoke_final,
    .get = smoke_get,
    .set = NULL, //不需要实现 设置
    .next = NULL
};


struct control *add_smoke_to_ctrl_list(struct control *phead)
{
 return add_interface_to_ctrl_list(phead,&smoke_control);

}
