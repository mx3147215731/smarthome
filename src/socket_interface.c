#include <pthread.h>

#include "socket_interface.h"
#include "control.h"
#include "socket.h"
#include "mq_queue.h"
#include "global.h"
#include <netinet/tcp.h> // 设置 tcp 心跳 的参数

static int s_fd = -1;

static int  tcpsocket_init(void)
{
    s_fd = socket_init(IPADDR,IPPORT);
    //return s_fd;
    return -1;
}

static void tcpsocket_final(void) 
{
  close(s_fd);
  s_fd = -1;
}

static void* tcpsocket_get(void *arg)
{
  
    int c_fd = -1;
    unsigned char buffer[BUF_SIZE];
    int ret = -1;
    struct sockaddr_in c_addr;
    mqd_t mqd = -1;
    ctrl_info_t * ctrl_info = NULL; 
    int keepalive = 1;    // 开启TCP_KEEPALIVE选项
    int keepidle = 10;    // 设置探测时间间隔为10秒
    int keepinterval = 5; // 设置探测包发送间隔为5秒
    int keepcount = 3;    // 设置探测包发送次数为3次

   

    pthread_detach(pthread_self()); // 和主线程(他的父线程)分离

    printf("%s|%s|%d   s_fd = %d\n", __FILE__, __func__, __LINE__, s_fd);
   
    if(-1 == s_fd) // 判断是否初始化成功
    {
     s_fd = tcpsocket_init();
     if(-1 == s_fd)
     {
      printf("tcpsocket_init error\n");
      pthread_exit(0);
     }

    }

     if(NULL != arg)
        ctrl_info = (ctrl_info_t*)arg;
     if(NULL != ctrl_info)
         mqd = ctrl_info->mqd;      
    
    
    if((mqd_t)-1 == mqd)
    {
       pthread_exit(0);  
    }

    memset(&c_addr, 0, sizeof(struct sockaddr_in));

    int clen = sizeof(struct sockaddr_in);

    printf("%s thread start\n", __func__);
    while (1) // 一直等待接收
    {
        c_fd = accept(s_fd, (struct sockaddr *)&c_addr, &clen); // 获得新的客户端 描述符

       if (c_fd == -1)
        {
            continue;
        }

        ret = setsockopt(c_fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive,sizeof(keepalive));
        if(-1 == ret){
          perror("setsockopt");
          break;
        }
       
        ret = setsockopt(c_fd, IPPROTO_TCP, TCP_KEEPIDLE, (void *)&keepidle, sizeof(keepidle));
          if(-1 == ret){
          perror("setsockopt");
          break;
        }
             ret = setsockopt(c_fd, IPPROTO_TCP, TCP_KEEPINTVL, &keepinterval,
                         sizeof(keepinterval));
          if(-1 == ret){
          perror("setsockopt");
          break;
        }
         ret = setsockopt(c_fd, IPPROTO_TCP, TCP_KEEPCNT, &keepcount,
                         sizeof(keepcount)); 
             if(-1 == ret){
          perror("setsockopt");
          break;
        }

        // 打印调试信息
        printf("%s | %s | %d: Access a connection from %s:%d\n", __FILE__, __func__, __LINE__, inet_ntoa(c_addr.sin_addr), ntohs(c_addr.sin_port));

    

        while (1)
        {
            memset(buffer, 0, BUF_SIZE);
            ret = recv(c_fd, buffer, BUF_SIZE, 0); // 等待接收
            // 将接收到数据打印出来
               printf("%s|%s|%d,  0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",__FILE__,__func__,__LINE__,buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5]);
       
            if (ret > 0)
            {
            if(buffer[0] == 0xAA && buffer[1] == 0x55 
                 &&buffer[4]==0x55 && buffer[5]==0xAA)
            {
                  printf("%s|%s|%d,  send: 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",__FILE__,__func__,__LINE__,buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5]);
       
              send_msg(mqd,buffer,ret); 
            }
                
            }
            else if (0 == ret || -1 == ret) // 没读到，or 读到空
            {
                break;
            }
        }

      
    }

    pthread_exit(0);

}


struct  control tcpsocket_control ={
    .control_name = "tcpsocket",
    .init = tcpsocket_init,
    .final = tcpsocket_final,
    .get = tcpsocket_get,
    .set = NULL, //不需要实现 设置
    .next = NULL
};


struct control *add_tcpsocket_to_ctrl_list(struct control *phead)
{
  //头插法实现 添加链表节点

   return add_interface_to_ctrl_list(phead,&tcpsocket_control);

};












