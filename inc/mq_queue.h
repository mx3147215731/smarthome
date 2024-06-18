#ifndef __MQ_QUEUE_H__
#define __MQ_QUEUE_H__

#include <mqueue.h>
#include <errno.h> //用于进行错误判断 
#include <string.h>
#include <unistd.h>

mqd_t msg_queue_create(void);
void msq_queue_final(mqd_t mqd);
int send_msg(mqd_t mqd,void *msg,int msg_len);

#endif