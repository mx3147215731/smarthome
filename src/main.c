#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <wiringPi.h>

#include "control.h"
#include "mq_queue.h"
#include "voice_interface.h"
#include "socket_interface.h"
#include "smoke_interface.h"
#include "receive_interface.h"
#include "global.h"

// msg_queue_create



int main() {
    pthread_t thread_id;
    struct control *control_phead = NULL;
    struct control *pointer = NULL;
    ctrl_info_t *ctrl_info = NULL;
    ctrl_info = (ctrl_info_t *)malloc(sizeof(ctrl_info_t));
    ctrl_info->ctrl_phead = NULL;
    ctrl_info->mqd = -1;

    int node_num = 0; // 统计节点数
    if(-1 == wiringPiSetup())// 初始化 wiringPi 库
    {
        perror("wiringPi Init");
        return -1;
    }

    // 创建消息队列
    ctrl_info->mqd = msg_queue_create();
    if(-1 == ctrl_info->mqd)// 创建消息队列失败
    {
        printf("%s|%s|%d, mqd= %d\n",__FILE__,__func__,__LINE__,ctrl_info->mqd);
        return -1;
    }
    
    //  头插法插入 ， so 头一直在变化
    ctrl_info->ctrl_phead = add_voice_to_ctrl_list(ctrl_info->ctrl_phead);
    ctrl_info->ctrl_phead = add_tcpsocket_to_ctrl_list(ctrl_info->ctrl_phead);
    ctrl_info->ctrl_phead = add_smoke_to_ctrl_list(ctrl_info->ctrl_phead);
    ctrl_info->ctrl_phead = add_receive_to_ctrl_list(ctrl_info->ctrl_phead);
  
  

    pointer = ctrl_info->ctrl_phead;

    while(NULL!=pointer) // 对所有控制结构体初始化，并且统计节点数
    {
        if(NULL != pointer->init)
        {
             printf("%s|%s|%d   control_name = %s\n",__FILE__,__func__,__LINE__,pointer->control_name);
             pointer->init();
        }
        pointer = pointer->next;
        node_num++; // 统计节点数
    }

    // 根据节点的总数 --> 创建对应数目的线程
    pthread_t *tid = (pthread_t *)malloc(sizeof(int) *node_num);
    pointer = ctrl_info->ctrl_phead;

    for(int i=0;i<node_num;++i)//遍历所有节点
    {
       if(NULL != pointer->get){
          printf("%s|%s|%d   control_name = %s\n",__FILE__,__func__,__LINE__,pointer->control_name);
          pthread_create(&tid[i],NULL,(void *)pointer->get,(void *)ctrl_info); // 传入这个结构体参数，方便同时调用多组线程里面的API
       }
        pointer = pointer->next;
    }
    
     for(int i=0;i<node_num;++i)
     {
     pthread_join(tid[i],NULL);
     }

     for(int i=0;i<node_num;++i)
     {
      if(NULL != pointer->final)
          pointer->final(); // 接打开的使用接口关闭
      pointer = pointer->next;
     }
     
     msq_queue_final(ctrl_info->mqd);

     if(NULL != ctrl_info)
          free(ctrl_info); // 这个是malloc 堆区申请的内存 -->  需要手动的释放

     if(NULL != tid)
          free(tid);


     return 0;
}