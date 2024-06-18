#ifndef __GLOBAL_H__
#define __GLOBAL_H__


typedef struct 
{
   mqd_t mqd;
   struct control*ctrl_phead;
}ctrl_info_t;

#endif