#ifndef _LKCAMP_IOCTL_H
#define _LKCAMP_IOCTL_H

#include <linux/ioctl.h>

#define LKCAMP_IOC_MAGIC 'k'
#define LKCAMP_SET_KEY _IOW(LKCAMP_IOC_MAGIC, 1, unsigned char)
#define LKCAMP_GET_KEY _IOR(LKCAMP_IOC_MAGIC, 2, unsigned char)

#endif