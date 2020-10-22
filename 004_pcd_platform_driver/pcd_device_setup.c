#include <linux/module.h>
#include <linux/platform_device.h>
#include "platform.h"

#undef pr_fmt
#define pr_fmt(fmt) "%s : " fmt, __func__
void pcdev_release(struct device *dev)
{
    pr_info("device release\r\n");
}
/* create 2 platform data */
struct pcdev_platform_data pcdev_pdata[] = {
    [0] = {.size  = 512,  .perm = RDWR, .serial_number = "PCDEV1111"},
    [1] = {.size  = 1024, .perm = RDWR, .serial_number = "PCDEV2222"},
    [2] = {.size  = 512,  .perm = RDONLY, .serial_number = "PCDEV3333"},
    [3] = {.size  = 32,   .perm = WRONLY, .serial_number = "PCDEV4444"}
};

/*create 2 platform devices */
struct platform_device platform_device_1 = {
    .name = "pcdev_A1x", //used to match with platform driver
    .id   = 0,
    .dev  = {
        .platform_data = &pcdev_pdata[0],
        .release       = pcdev_release 
    }
};

struct platform_device platform_device_2 = {
    .name = "pcdev_B1x",
    .id   = 1,
    .dev  = {
        .platform_data = &pcdev_pdata[1], 
        .release       = pcdev_release
    }
};

struct platform_device platform_device_3 = {
    .name = "pcdev_C1x",
    .id   = 2,
    .dev  = {
        .platform_data = &pcdev_pdata[2], 
        .release       = pcdev_release
    }
};

struct platform_device platform_device_4 = {
    .name = "pcdev_D1x",
    .id   = 3,
    .dev  = {
        .platform_data = &pcdev_pdata[3], 
        .release       = pcdev_release
    }
};

struct platform_device *platform_pcdevs[] = 
{
    &platform_device_1,
    &platform_device_2,
    &platform_device_3,
    &platform_device_4
};

static int __init pcdev_platform_init(void)
{
    /* register platform device => sys/devices/platform */
    //platform_device_register(&platform_device_1);
    //platform_device_register(&platform_device_2);
    platform_add_devices(platform_pcdevs, ARRAY_SIZE(platform_pcdevs));
    pr_info("Device setup module loaded \r\n");
    return 0;
}

static void __exit pcdev_platform_exit(void)
{
    platform_device_unregister(&platform_device_1);
    platform_device_unregister(&platform_device_2);
    platform_device_unregister(&platform_device_3);
    platform_device_unregister(&platform_device_4);
    pr_info("Device setup module unloaded \r\n");
}

module_init(pcdev_platform_init);
module_exit(pcdev_platform_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module which registers platform devices");