#include <stdio.h>

#include "mq_queue.h"

#define QUEUE_NAME  "/mq_queue"

mqd_t msg_queue_create(void) 
{
   //创建消息队列
    mqd_t mqd=-1;

    struct mq_attr attr;
    attr.mq_flags=0;
    attr.mq_maxmsg=10;
    attr.mq_msgsize=256;
    attr.mq_curmsgs=0;
    // mqd_t mq_open(const char *name, int oflag,mode_t mode, struct mq_attr attr );
    // 打开文件 QUEUE_NAME,没有就创建，打开方式为可读可写，给到0666的权限
    mqd = mq_open(QUEUE_NAME,O_CREAT | O_RDWR,0666,&attr);
    
    printf("%s|%s|%d    ,mqd = %d\n",__FILE__,__func__,__LINE__,mqd);

    return mqd;
}

int send_msg(mqd_t mqd,void *msg,int msg_len)
{
    int byte_send = -1;
    byte_send = mq_send(mqd,(char *)msg,msg_len,0);

    return byte_send; // 根据返回值确定，是否成功发送
}

void msq_queue_final(mqd_t mqd)
{
    if(-1 != mqd)
       mq_close(mqd);
    mq_unlink(QUEUE_NAME);
    mqd = -1;
}