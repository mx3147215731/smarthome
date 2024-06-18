
#if 0
struct control
{
char control_name[128]; //监听模块名称
int (*init)(void); //初始化函数
void (*final)(void);//结束释放函数
void *(*get)(void *arg);//监听函数，如语音监听
void *(*set)(void *arg); //设置函数，如语音播报
struct control *next;
};
#endif

#include <pthread.h>
#include <stdio.h>
#include "voice_interface.h"
#include "mq_queue.h"
#include "uartTool.h"
#include "global.h"


static int serial_fd = -1; // static 这个 变量只在当前文件有效

static int voice_init(void )
{
  serial_fd = myserialOpen(SERIAL_DEV,BAUD); // 初始化并且打开串口
  printf("%s|%s|%d   serial_fd = %d\n",__FILE__,__func__,__LINE__,serial_fd);

  return serial_fd;
}

static void voice_final(void)
{
  if(-1 != serial_fd) // 打开串口成功
  {
    close(serial_fd); // 关闭我们打开的串口
    serial_fd = -1; // 复位
  }
}
// 接收语言指令
static void* voice_get(void *arg)// mqd 通过arg 传参获得
{
    int len = 0;
    mqd_t mqd = -1;
    ctrl_info_t * ctrl_info = NULL; 
    if(NULL != arg)
        ctrl_info = (ctrl_info_t*)arg;

    unsigned char buffer[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // 初始化 buffer
    if (-1 == serial_fd)
    {
        //打开串口
        serial_fd = voice_init();// 尝试打开串口
        if (-1 == serial_fd){ //还是打开失败
        printf("%s | %s | %d:open serial failed\n", __FILE__, __func__, __LINE__); // 三个宏的含义: 文件名 - main.c,函数名 - pget_voice ,行号 -  138
        pthread_exit(0);   
        }                                                        // 串口打开失败 -->退出
    }
 
     
    mqd = ctrl_info->mqd; 
    

    if((mqd_t)-1 == mqd)
    {
       pthread_exit(0);  
    }

    pthread_detach(pthread_self());// 与父线程分离
    printf("%s thread start\n",__func__);

    while (1)
    {
        len = serialGetstring(serial_fd, buffer); // 通过串口获得语言输入
        printf("%s|%s|%d,  0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",__FILE__,__func__,__LINE__,buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5]);
        printf("%s|%s|%d:len = %d\n",__FILE__,__func__,__LINE__,len);
        if (len > 0)         // 判断是否 接到识别指令
        {
          if(buffer[0] == 0xAA && buffer[1] == 0x55 
            &&buffer[4]==0x55 && buffer[5]==0xAA)
            {
               printf("%s|%s|%d, send: 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",__FILE__,__func__,__LINE__,buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5]);
        
              send_msg(mqd,buffer,len); // 注意获取len长度不能使用strlen() --> 0x00 会识别为截止位-->只能读取到三个字节(但不是我们实际的截止位(0x55 0xAA ))
            }
            memset(buffer,0,sizeof(buffer)); // 复位buffer
        }
    }

    pthread_exit(0);

}
// 语音播报


static void* voice_set(void *arg)
{
  pthread_detach(pthread_self());// 与父线程分离
  unsigned char *buffer = (unsigned char*)arg;
  
  if (-1 == serial_fd)
    {
        //打开串口
        serial_fd = voice_init();// 尝试打开串口
        if (-1 == serial_fd){ //还是打开失败
        printf("%s | %s | %d:open serial failed\n", __FILE__, __func__, __LINE__); // 三个宏的含义: 文件名 - main.c,函数名 - pget_voice ,行号 -  138
        pthread_exit(0);   
        }                                                        // 串口打开失败 -->退出
    }

    if(NULL != buffer){ // 接收到数据
      serialSendstring(serial_fd,buffer,6); // 向串口发送接收到的数据
     // 语言模块识别到串口发送的数据后就，进行相应的语言输出 
    }

 pthread_exit(0);
}

struct  control voice_control ={
    .control_name = "voice",
    .init = voice_init,
    .final = voice_final,
    .get = voice_get,
    .set = voice_set,
    .next = NULL
};


struct control *add_voice_to_ctrl_list(struct control *phead)
{
  //头插法实现 添加链表节点
   return add_interface_to_ctrl_list(phead,&voice_control);

};

