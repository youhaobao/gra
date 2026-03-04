#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "QDebug"
#include <stdio.h>
#include "led_control.h"

static int fd;
void led_init(void)
{
    fd = open("/dev/100ask_led",O_RDWR);
    if (fd < 0){
        qDebug()<<"open /dev/100ask_led failed";
    }
}

void led_control(int on)
{
    char buf[2];
    buf[0] = 0;
    if(on)
    {
        buf[1] = 0;
    }
    else
    {
        buf[1] = 1;
    }
        int ret = write(fd,buf,2);
        if(ret<0)
        {
            qDebug() << "写入LED控制命令失败: ";
        }
}
