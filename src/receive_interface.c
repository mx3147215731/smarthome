#include <pthread.h>
#include <mqueue.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <wiringPi.h>

#include "receive_interface.h"
#include "control.h"
#include "mq_queue.h"
#include "global.h"
#include "face.h"
#include "myoled.h"
//#include "lrled_gdevice.h"
#include "gdevice.h"
// #include "fan_gdevice.h"
// #include "bled_gdevice.h"
// #include "beep_gdevice.h"
// #include "lock_gdevice.h"

#include "ini.h"
#include "face.h"



#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

/*
接收模块:
对接收到消息做出相应处理
包括 oled 人脸识别 语言播报 GPIO  引脚状态配置

*/

static int oled_fd = -1;
static struct gdevice *pdevhead = NULL;

typedef struct
{
  int msg_len;
  unsigned char *buffer;
  ctrl_info_t *ctrl_info;
} recv_msg_t;


static int handler_gdevice(void *user, const char *section, const char *name, const char *value)
{
    struct gdevice *pdev = NULL;
    if (NULL == pdevhead)
    {
        pdevhead = (struct gdevice *)malloc(sizeof(struct gdevice));
        memset(pdevhead, 0, sizeof(struct gdevice));
        pdevhead->next = NULL;
        strcpy(pdevhead->dev_name, section);
    }
    // printf("section = %s, name = %s, value = %s\n", section, name, value);

    else if (0 != strcmp(section, pdevhead->dev_name)) // 当section对不上的时候，表示到了下一个设备
    {
        // 把新节点(设备)使用头插法插入
        pdev = (struct gdevice *)malloc(sizeof(struct gdevice));
        memset(pdev, 0, sizeof(struct gdevice));
        strcpy(pdev->dev_name, section);
        pdev->next = pdevhead;
        pdevhead = pdev;
    }

    if (NULL != pdevhead)
    {
        if (MATCH(pdevhead->dev_name, "key"))
        {
            sscanf(value, "%x", &pdevhead->key); // 把value(string)的值 转为int类型 16进行格式 传递给  pdevhead->key)
            printf("%d  pdevhead->key = 0x%x\n", __LINE__, pdevhead->key);
        }

        else if (MATCH(pdevhead->dev_name, "gpio_pin"))
        {
            pdevhead->gpio_pin = atoi(value);
        }

        else if (MATCH(pdevhead->dev_name, "gpio_mode"))
        {

            if (strcmp(value, "OUTPUT") == 0)
            {
                pdevhead->gpio_mode = OUTPUT;
            }
            else if (strcmp(value, "INPUT") == 0)
            {
                pdevhead->gpio_mode = INPUT;
            }
            else
            {
                printf("gpio_mode error\n");
            }
        }

        else if (MATCH(pdevhead->dev_name, "gpio_status"))
        {

            if (strcmp(value, "LOW") == 0)
            {
                pdevhead->gpio_mode = LOW;
            }
            else if (strcmp(value, "HIGH") == 0)
            {
                pdevhead->gpio_mode = HIGH;
            }
            else
            {
                printf("gpio_status error\n");
            }
        }

        else if (MATCH(pdevhead->dev_name, "check_face_status"))
        {
            pdevhead->check_face_status = atoi(value);
        }

        else if (MATCH(pdevhead->dev_name, "voice_set_status"))
        {
            pdevhead->voice_set_status = atoi(value);
        }
    }

    return 1;
}



static int receive_init(void)
{
   printf("%s|%s|%d,init enter \n", __FILE__, __func__, __LINE__);
  // pdevhead = add_lrled_to_gdevice_list(pdevhead); // 头插法加入 客厅灯
  // pdevhead = add_bled_to_gdevice_list(pdevhead);  // 加入卧室灯
  // pdevhead = add_fan_to_gdevice_list(pdevhead);   // 加入风扇
  // pdevhead = add_beep_to_gdevice_list(pdevhead);  // 蜂鸣器
  // pdevhead = add_lock_to_gdevice_list(pdevhead);  // 开锁
     if (ini_parse("/etc/gdevice.ini", handler_gdevice, NULL) < 0) {
        printf("Can't load 'gdevice.ini'\n");
        return 1;
    }

    struct gdevice *pdev = NULL;
    pdev = pdevhead;
    while (pdev != NULL)
    {
        // printf("inside %d",__LINE__);

        printf("dev_name:%s\n", pdev->dev_name);
        printf("key:%x\n", pdev->key);
        printf("gpio_pin:%d\n", pdev->gpio_pin);
        printf("gpio_mode:%d\n", pdev->gpio_mode);
        printf("gpio_status:%d\n", pdev->gpio_status);
        printf("check_face_status:%d\n", pdev->check_face_status);
        printf("voice_set_status:%d\n", pdev->voice_set_status);

        pdev = pdev->next;
    }

  // 设备类链表添加
  oled_fd = myoled_init(); // 初始化oled
  face_init();             // 初始化人脸识别

  return oled_fd;
}

static void receive_final(void)
{
  face_final();
  if (-1 != oled_fd)
  {
    close(oled_fd); // 关闭oled 打开的文件
    oled_fd = -1;   // 复位
  }
}







//  处理设备 --  比如打开灯 和风扇等





static void *handler_device(void *arg)
{
  pthread_detach(pthread_self()); // 和主线程(他的父线程)分离

  recv_msg_t *recv_msg = NULL;
  struct gdevice *cur_gdev = NULL;
  char success_or_failed[20] = "success";
  pthread_t tid = -1;
  int smoke_status = 0;
  double face_result = 0.0; //存放人脸匹配度

  int ret = -1;

  if (NULL != arg) // 有参数
  {
    recv_msg = (recv_msg_t *)arg; // 获取参数
    printf("recv_len = %d\n", recv_msg->msg_len);
    printf("%s|%s|%d, handler: 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n", __FILE__, __func__, __LINE__,
           recv_msg->buffer[0], recv_msg->buffer[1], recv_msg->buffer[2], recv_msg->buffer[3], recv_msg->buffer[4], recv_msg->buffer[5]);
  }

  // need to do something
  if (NULL != recv_msg && NULL != recv_msg->buffer) // if 消息队列非空，并且buffer 里面接收到数据
  {
    
    // recv_msg->buffer[2] -->  第三位 用于存放设备类型
    cur_gdev = find_device_by_key(pdevhead, recv_msg->buffer[2]);
    printf("%s|%s|%d,find success   buffer[2] = 0x%x \n", __FILE__, __func__, __LINE__, recv_msg->buffer[2]);
  }
   
  if (NULL != cur_gdev) // if 能找到的这设备 --> 设备存在
  {
     printf("%s|%s|%d, cur_gdev \n", __FILE__, __func__, __LINE__);
    // BUFFER 的第四个参数  用于 存放开关状态 0 表示开， 1 表示关
    cur_gdev->gpio_status = recv_msg->buffer[3] == 0 ? LOW : HIGH; // 获取状态存入cur_gdev中
    //人脸识别
    if(1 == cur_gdev->check_face_status){
      face_result = face_status(); //得到人脸识别的匹配度
      if(face_result > 0.6){ //匹配成功
      ret = set_gpio_device_status(cur_gdev); // 设置电平 --> 开锁
      recv_msg->buffer[2] = 0x47;  //识别成功的语音播报
      }
      else{
      recv_msg->buffer[2] = 0x46;
      }
    }

    else if( 0 == cur_gdev->check_face_status){
    // printf("%s|%s|%d,Set  before set_gpio_device_status\n",__FILE__,__func__,__LINE__);
    ret = set_gpio_device_status(cur_gdev); // 将获取到的状态真正赋值给引脚
    // printf("%s|%s|%d, after set_gpio_device_status \n",__FILE__,__func__,__LINE__);
    }

 printf("%s|%s|%d, = %d\n", __FILE__, __func__, __LINE__,cur_gdev->voice_set_status);
   
   // 需要语言播报
  if (1 == cur_gdev->voice_set_status) 
  {
    printf("%s|%s|%d,2\n", __FILE__, __func__, __LINE__);
    if (NULL != recv_msg && NULL != recv_msg->ctrl_info && NULL != recv_msg->ctrl_info->ctrl_phead)
    {
     printf("%s|%s|%d,2\n", __FILE__, __func__, __LINE__);
      struct control *pcontrol = recv_msg->ctrl_info->ctrl_phead;
      while (NULL != pcontrol)
      {
        if (strstr(pcontrol->control_name, "voice")) //匹配到语言播报
        {

          if (0x45 == recv_msg->buffer[2] && 0 == recv_msg->buffer[3]) // 语音播报 打开
          {
            smoke_status = 1;
            
          }
          pthread_create(&tid, NULL, pcontrol->set, (void *)recv_msg->buffer); // 新开线程区进行语言播报
          break;
          
        }
        pcontrol = pcontrol->next;
      }
    }
  }

printf("%s|%s|%d,2\n", __FILE__, __func__, __LINE__);
  if (-1 == ret) // 设置失败
  {
    printf("%s|%s|%d,2\n", __FILE__, __func__, __LINE__);
    memset(success_or_failed, '\0', sizeof(success_or_failed));
    strncpy(success_or_failed, "failed", 6);
  }

  printf("%s|%s|%d,2\n", __FILE__, __func__, __LINE__);
  // 配置OLED
  char oled_msg[512];
  memset(oled_msg, 0, sizeof(oled_msg));
  char *change_status = cur_gdev->gpio_status == LOW ? "Open" : "Close";
  sprintf(oled_msg, "%s %s %s!\n", change_status, cur_gdev->dev_name, success_or_failed);
  if(smoke_status == 1)
  {
    memset(oled_msg, 0, sizeof(oled_msg));
   sprintf(oled_msg, "A risk of fire!\n");
  

  }  
  
  myoled_show(oled_msg);
 
  //让门打开5s自动关闭
  if(1 == cur_gdev->check_face_status && 0 == ret && face_result >0.6){
     sleep(5); //开门5s后关门
     cur_gdev->gpio_status = HIGH; //设置高电平(低电平有效)
     ret = set_gpio_device_status(cur_gdev); //关门
  }




  }

  pthread_exit(0);
}


static void *receive_get(void *arg) // 接收消息队列里面的 数据
{
  printf("enter receive_get\n");
  //  通过参数 初始化我们 定义的recv_msg_t 结构体
  recv_msg_t *recv_msg = NULL;
  unsigned char *buffer = NULL;
  struct mq_attr attr;
  pthread_t tid = -1;
  ssize_t read_len = -1;

  

  if (NULL != arg)
  {
    recv_msg = (recv_msg_t *)malloc(sizeof(recv_msg_t));
    recv_msg->ctrl_info = (ctrl_info_t *)arg; // 这里实际上就获取到了mqd 和 phead(我们需要操作的struct control 链表 的头节点)
    recv_msg->msg_len = 0;
    recv_msg->buffer = NULL;
  }
  else
    pthread_exit(0);

  if (mq_getattr(recv_msg->ctrl_info->mqd, &attr) == -1)
  { // 获取消息队列失败 -- 异常
    pthread_exit(0);
  }

  // 能获取到消息队列
  recv_msg->buffer = (unsigned char *)malloc(attr.mq_msgsize); // 分配内存
  buffer = (unsigned char *)malloc(attr.mq_msgsize);
  // mq_msgsize -- 每条消息的大小
  memset(recv_msg->buffer, 0, attr.mq_msgsize); // 初始化
  memset(buffer, 0, attr.mq_msgsize);           // 初始化

  pthread_detach(pthread_self()); // 和主线程(他的父线程)分离

  while (1)
  {
    read_len = mq_receive(recv_msg->ctrl_info->mqd, buffer, attr.mq_msgsize, NULL);

    printf("%s|%s|%d, recv: 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n", __FILE__, __func__, __LINE__, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);
    printf("%s|%s|%d: read_len = %ld\n", __FILE__, __func__, __LINE__, read_len);
    if (-1 == read_len)
    { // 接收失败
      if (errno == EAGAIN)
      {
        printf("queue is empty\n");
      }
      else
      {
        break;
      }
    }
    // 以下是接收到正常数据的情况
    else if (buffer[0] == 0xAA && buffer[1] == 0x55 && buffer[4] == 0x55 && buffer[5] == 0xAA)
    {
      recv_msg->msg_len = read_len;
      memcpy(recv_msg->buffer, buffer, read_len);
      //  创建线程去 处理我们的接收到的信号
      pthread_create(&tid, NULL, handler_device, (void *)recv_msg);
    }
  }
  if (NULL != recv_msg)
    free(recv_msg);

  if (NULL != buffer)
    free(buffer);

  pthread_exit(0);
}

struct control receive_control = {
    .control_name = "receive",
    .init = receive_init,
    .final = receive_final,
    .get = receive_get,
    .set = NULL, // 不需要实现 设置
    .next = NULL};

struct control *add_receive_to_ctrl_list(struct control *phead)
{
  // 头插法实现 添加链表节点

  return add_interface_to_ctrl_list(phead, &receive_control);
};
