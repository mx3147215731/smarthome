#include <stdio.h>
#include "control.h"

struct control *add_interface_to_ctrl_list(struct control *phead,struct control * control_interface)
{
  //头插法实现 添加链表节点

  if(NULL == phead)
  {
    phead = control_interface; // 直接传入我们的 voice_control
  }

  else// 头结点非空 - 链表有数据
  {
   control_interface->next = phead; //把新的节点的next指向头结点
   phead = control_interface; // 让心节点成为头结点
  }

  return phead;

}