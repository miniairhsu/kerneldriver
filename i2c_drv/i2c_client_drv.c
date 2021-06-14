#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/mod_devicetable.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include "platform.h"

/* device data structure */
struct i2c_data {
    struct i2c_client client;
    dev_t dev;
    u8 *buf;
    u16 value;
    struct cdev cdev;
    struct class *class;

};

/* device data structure */
struct pcdev_private_data
{
    struct pcdev_platform_data pdata;
    char* buffer;
    dev_t dev_num;
    struct cdev cdev;
};


static const struct i2c_device_id i2c_ids[] = {
    {"ds3231", 0},
    {"ds32", 0},
    {}
};




/* driver data structure */
struct pcdrv_private_data
{
    int total_devices;
    dev_t device_num_base;
    struct class *class_pcd;
    struct device *device_pcd;
};

enum pcdev_names 
{
    PCDEVA1X,
    PCDEVB1X,
    PCDEVC1X,
    PCDEVD1X
};
/* configuration item to config device */
struct device_config
{
    int config_item1;
    int config_item2;
};

struct device_config pcdev_config[] = 
{
    [PCDEVA1X] = {.config_item1 = 60, .config_item2 = 21}, 
    [PCDEVB1X] = {.config_item1 = 50, .config_item2 = 22}, 
    [PCDEVC1X] = {.config_item1 = 40, .config_item2 = 23}, 
    [PCDEVD1X] = {.config_item1 = 30, .config_item2 = 24} 
}; 

struct pcdrv_private_data pcdrv_data;
int check_permission(int dev_perm, int acc_mode)
{
    if(dev_perm == RDWR)
        return 0;
    if(dev_perm == RDONLY && ( (acc_mode & FMODE_READ) && !(acc_mode & FMODE_WRITE)) )
        return 0;
    if(dev_perm == WRONLY && ( (acc_mode & FMODE_WRITE) && !(acc_mode & FMODE_READ)) )
        return 0;
    return -EPERM;
}

int pcd_open(struct inode *inode, struct file *filp)
{   
    struct i2c_data *dev = container_of(inode->i_cdev, struct i2c_data, cdev);
    if (dev == NULL) {
        pr_err("Open data fail\n");
        return -1;
    }
    pr_info("File opened\n");
    filp->private_data = dev;
    return 0;
}

int pcd_release(struct inode *inode, struct file *filp)
{
    pr_info("release requsted\r\n");
    return 0;
}

ssize_t pcd_read(struct file* filp, char __user *buff, size_t count, loff_t *f_pos)
{
    struct i2c_data *dev = (struct i2c_data *)filp->private_data;
    struct i2c_adapter *adap = dev->client.adapter;
    struct i2c_msg msg;
    int ret;
    char* temp;
    temp = kmalloc(count, GFP_KERNEL);
    msg.addr = 0x68;
    msg.flags = 0;
    msg.flags |= I2C_M_RD;
    msg.len = count;
    msg.buf = temp;
    ret = i2c_transfer(adap, &msg, 1);  
    if (ret >= 0) {
        ret = copy_to_user(buff, temp, count) ? -EFAULT : count;
    }
    kfree(temp);  
    return ret;
}


ssize_t pcd_write(struct file* filp, const char __user *buff, size_t count, loff_t *f_pos)
{
    struct i2c_data *dev = (struct i2c_data *)filp->private_data;
    struct i2c_adapter *adap = dev->client.adapter;
    struct i2c_msg msg;
    int ret;
    char* temp;
    temp = memdup_user(buff, count);
    msg.addr = 0x68;
    msg.flags = 0;
    msg.len = count;
    msg.buf = temp;
    ret = i2c_transfer(adap, &msg, 1);  
    kfree(temp);  
    return (ret == 1 ? count : ret);
}

loff_t pcd_lseek(struct file *filp, loff_t offset, int whence)
{
    return 0;
}
struct file_operations pcd_fops = {
    .open    = pcd_open, 
    .write   = pcd_write,
    .read    = pcd_read, 
    .llseek  = pcd_lseek, 
    .release = pcd_release,
    .owner   = THIS_MODULE
};
#undef pr_mt
#define pr_fmt(fmt) "%s:" fmt,__func__ 

/* called when matched platform device is found */
static int ds3231_probe(struct i2c_client *client, const struct i2c_device_id *id) {
    struct i2c_data *data;
    int ret;
    data = devm_kzalloc(&client->dev, sizeof(struct i2c_data), GFP_KERNEL);
    data->value = 30;
    data->buf = devm_kzalloc(&client->dev, data->value, GFP_KERNEL);
    i2c_set_clientdata(client, data); //dev_set_drvdata equvilent
    ret = alloc_chrdev_region(&data->dev,0 , 1, "i2c-drv");
    if (ret < 0) {
        pr_err("Device registration fail\n");
        return ret;
    }
    if ((data->class = class_create(THIS_MODULE, "i2cdriver")) == NULL ) {
        pr_err("i2c class create fail\n");
        unregister_chrdev_region(data->dev, 1);
        return -1;
    }

    if(device_create(data->class, NULL, data->dev, NULL, "i2cdrv%d", 0) == NULL) {
        pr_err("device create fail\n");
        class_destroy(data->class);
        unregister_chrdev_region(data->dev, 1);
        return -1;
    }

    cdev_init(&data->cdev, &pcd_fops);

    if (cdev_add(&data->cdev, data->dev, 1) == -1) {
        pr_err("Unable to add device\n");
        device_destroy(data->class, data->dev);
        class_destroy(data->class);
        unregister_chrdev_region(data->dev, 1);
        return -1;
    }
    return 0;
}

static int ds3231_remove(struct i2c_client *client) 
{
    struct i2c_data *data;
    data = i2c_get_clientdata(client);
    cdev_del(&data->cdev);
    device_destroy(data->class, data->dev);
    class_destroy(data->class);
    unregister_chrdev_region(data->dev, 1);
    return 0;
}


int pcd_platform_driver_probe(struct platform_device *pdev)
{
    int ret;
    struct pcdev_private_data *dev_data;
    /* create in platform device */
    struct pcdev_platform_data *pdata;
    pr_info("A device is detected\r\n");
    /* get platform data */
    //pdata = pded->dev.platform_data
    pdata = (struct pcdev_platform_data *)dev_get_platdata(&pdev->dev);
    if(!pdata) {
        pr_info("No platform data available\r\n");
        ret = -EINVAL;
        return ret;
    }

    /* allocate memory for device private data */
    //dev_data = kzalloc(sizeof(struct pcdev_private_data), GPL_KERNEL));
    dev_data = devm_kzalloc(&pdev->dev, sizeof(struct pcdev_private_data), GFP_KERNEL);
    if(!dev_data) {
        pr_info("Cannot allocate memory\r\n");
        return -ENOMEM;
    }
    /* save device private data pointer in platform device structure 
       so can be used later
       static inline void dev_set_drvdata(struct device *dev, void *data)
       { 
            dev->driver_data = data;
       }
    */
   //pdev->driver_data = dev_data;
    dev_set_drvdata(&pdev->dev, dev_data);
    dev_data->pdata.size = pdata->size;
    dev_data->pdata.perm = pdata->perm;
    dev_data->pdata.serial_number = pdata->serial_number;
    pr_info("Device serial number %s\r\n", dev_data->pdata.serial_number);
    pr_info("Device size %d\r\n", dev_data->pdata.size);
    pr_info("Device permission %d\r\n", dev_data->pdata.perm);
    /* find device config data from platform_device */
    pr_info("driver name = %s\r\n", pdev->id_entry->name);
    pr_info("config item 1 = %d\r\n", pcdev_config[pdev->id_entry->driver_data].config_item1);
    pr_info("config item 2 = %d\r\n", pcdev_config[pdev->id_entry->driver_data].config_item2);
    /*allocate memory for device buffer */
    dev_data->buffer = devm_kzalloc(&pdev->dev, dev_data->pdata.size, GFP_KERNEL);
    if(!dev_data->buffer) {
        pr_info("Cannot allocate memory\r\n");
        return -ENOMEM;
    }
    
    /* get device number */
    dev_data->dev_num = pcdrv_data.device_num_base + pdev->id;

    /* cdev init */
    cdev_init(&dev_data->cdev, &pcd_fops);
    dev_data->cdev.owner = THIS_MODULE;
    ret = cdev_add(&dev_data->cdev, dev_data->dev_num, 1);
    if(ret < 0) {
        pr_err("cdev add fail \r\n");
        return ret;
    }

    /* create device file for detected device */  
    pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd, NULL, dev_data->dev_num, NULL, "pcdev-%d", pdev->id);
    if(IS_ERR(pcdrv_data.device_pcd)) {
        pr_err("device creation fail\n");
        ret = PTR_ERR(pcdrv_data.device_pcd);
        cdev_del(&dev_data->cdev);
        return ret;
    }

    pcdrv_data.total_devices++;
    pr_info("pcd device is probed\n");
    return 0;
}

int pcd_platform_driver_remove(struct platform_device *pdev)
{
    struct pcdev_private_data *dev_data = dev_get_drvdata(&pdev->dev);
    pr_info("pcd device is removed\n");
    /* remove device created */
    device_destroy(pcdrv_data.class_pcd, dev_data->dev_num);
    /* remove cdev entry */
    cdev_del(&dev_data->cdev);
    pcdrv_data.total_devices--;
    return 0;
}


MODULE_DEVICE_TABLE(i2c, i2c_ids);

static struct i2c_driver ds3231_I2c_drv = {
    .driver = {
        .name = "ds32", 
        .owner = THIS_MODULE,
    }, 
    .probe = ds3231_probe,
    .remove = ds3231_remove,
    .id_table = i2c_ids, 
};

/* matching multiple platform device with different names */
struct platform_device_id pcdev_ids[] = {
    /* driver data is linked to pcdev_config */
    [0] = {.name = "pcdev_A1x", .driver_data = PCDEVA1X}, 
    [1] = {.name = "pcdev_B1x", .driver_data = PCDEVB1X}, 
    [2] = {.name = "pcdev_C1x", .driver_data = PCDEVC1X}, 
    [3] = {.name = "pcdev_D1x", .driver_data = PCDEVD1X}, 
};
struct platform_driver pcd_platform_driver = 
{
    .probe  = pcd_platform_driver_probe, 
    .remove =  pcd_platform_driver_remove, 
    .id_table = pcdev_ids, 
    /* id table is used then name field is no longer valid */
    .driver = {
        .name = "pseudo-char-device" //used to match with platform device
    }
};
#define MAX_DEVICES 10
static int __init i2c_client_drv_init(void)
{

    return i2c_add_driver(&ds3231_I2c_drv);
}

static void __exit i2c_client_dev_exit(void)
{
    i2c_del_driver(&ds3231_I2c_drv);
}

module_init(i2c_client_drv_init);
module_exit(i2c_client_dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LEN");
MODULE_DESCRIPTION("An I2C client driver");