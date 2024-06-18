
#include "gdevice.h"

#if 0 
struct gdevice *add_device_to_gdevice_list(struct gdevice *pgdevhead, struct gdevice *gdev)
{

    if (NULL == pgdevhead)
    {
        pgdevhead = gdev; // 直接传入我们的 voice_control
    }

    else // 头结点非空 - 链表有数据
    {
        gdev->next = pgdevhead; // 把新的节点的next指向头结点
        pgdevhead = gdev;       // 让心节点成为头结点
    }

    return pgdevhead;
}
#endif

// 有点写的个性化
struct gdevice *find_device_by_key(struct gdevice *pgdevhead, int key)
{
    struct gdevice *p = NULL;
    printf("%s|%s|%d, cur_gdev1 \n", __FILE__, __func__, __LINE__);

    if (NULL != pgdevhead) // 链表非空就去查找
    {
        printf("%s|%s|%d, cur_gdev2\n", __FILE__, __func__, __LINE__);
        p = pgdevhead;
        while (NULL != p)
        {
            printf("%s|%s|%d, key =%x p->key = %x\n", __FILE__, __func__, __LINE__,key,p->key);
            if (key == p->key){
                return p; //  找到 就返回
                printf("%s|%s|%d, cur_gdev2\n", __FILE__, __func__, __LINE__);
            }
            p = p->next;
        }
    }
    return NULL; // 返回空 --> 链表为空 or 查找失败
}



int set_gpio_device_status(struct gdevice *pgdev)
{
    printf("%s|%s|%d,begin set_gpio_device_status\n",__FILE__,__func__,__LINE__);
    if(NULL == pgdev)
    {
        return -1;
    }
    
    if(-1 !=  pgdev->gpio_pin) // 如果引脚没有配置
    {
        if(-1 != pgdev->gpio_mode) // 如果引脚模式没有配置 -- 就去配置
        { 
             printf("%s|%s|%d,before pinMode\n",__FILE__,__func__,__LINE__);
            pinMode(pgdev->gpio_pin,pgdev->gpio_mode);// 配置引脚的输入输出
             printf("%s|%s|%d,after pinMode\n",__FILE__,__func__,__LINE__);
        }

        if(-1 != pgdev->gpio_status)// 如果引脚状态没有配置 -- 就去配置
        {  
            printf("%s|%s|%d,before digitalWrite\n",__FILE__,__func__,__LINE__);
        
            digitalWrite(pgdev->gpio_pin,pgdev->gpio_status); // 配置为输出的时候，输出的电平
             printf("%s|%s|%d,after digitalWrite\n",__FILE__,__func__,__LINE__);
        }

    }
   
  return 0;
}
