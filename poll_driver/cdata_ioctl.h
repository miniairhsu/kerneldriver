#ifndef _CDATA_IOCTL_H_
#define _CDATA_IOCTL_H_

#include <linux/ioctl.h>

#define IOCTL_EMPTY  _IO(0xD0, 1)
#define IOCTL_SYNC  _IO(0xD1, 2)
#define IOCTL_NAME _IOW(0XDD, 3, int)
#define IOCTL_WRITE _IOW(0XDE, 4, int)
#define IOCTL_ENABLE _IOW(0XDF, 5, int)
#define IOCTL_TX_CMD _IOW(0XA1, 6, int)
#define IOCTL_DISABLE _IOW(0XE1, 7, int)

#define POLLEVENT1 0x4000
#define POLLEVENT2 0x8000

struct cmd_struct{
    int cmd;
};
#endif