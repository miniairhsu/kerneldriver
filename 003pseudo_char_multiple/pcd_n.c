#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>

#define NO_OF_DEVICES 4
#define DEV_MEM_SIZE_PCDEV1 1024
#define DEV_MEM_SIZE_PCDEV2 1024
#define DEV_MEM_SIZE_PCDEV3 1024
#define DEV_MEM_SIZE_PCDEV4 1024
char device_buffer_pcdev1[DEV_MEM_SIZE_PCDEV1];
char device_buffer_pcdev2[DEV_MEM_SIZE_PCDEV2];
char device_buffer_pcdev3[DEV_MEM_SIZE_PCDEV3];
char device_buffer_pcdev4[DEV_MEM_SIZE_PCDEV4];

struct pcdev_private_data
{
    char* buffer;
    unsigned size;
    const char *serial_number;
    int perm;
    struct cdev cdev;
};

struct pcdrv_private_data
{
    int total_devices;
    dev_t device_number;
    struct class *class_pcd;
    struct device *device_pcd;
    struct pcdev_private_data pcdev_data[NO_OF_DEVICES];
};

#define RDONLY 0X01
#define WRONLY 0X10
#define RDWR   0x11
struct pcdrv_private_data pcdrv_data = 
{
    .total_devices = NO_OF_DEVICES,
    .pcdev_data = {
        [0] = { 
            .buffer        = device_buffer_pcdev1, 
            .size          = DEV_MEM_SIZE_PCDEV1,
            .serial_number = "PCDEV1", 
            .perm          = RDONLY 
        },

        [1] = { 
            .buffer        = device_buffer_pcdev2, 
            .size          = DEV_MEM_SIZE_PCDEV2,
            .serial_number = "PCDEV2", 
            .perm          = WRONLY 
        },

        [2] = { 
            .buffer        = device_buffer_pcdev3, 
            .size          = DEV_MEM_SIZE_PCDEV3,
            .serial_number = "PCDEV3", 
            .perm          = RDWR 
        },

        [3] = { 
            .buffer        = device_buffer_pcdev4, 
            .size          = DEV_MEM_SIZE_PCDEV4,
            .serial_number = "PCDEV4", 
            .perm          = RDWR 
        }
    }
};


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
    int ret;
    int minor_n;
    struct pcdev_private_data *pcdev_data;
    //find out device file
    minor_n = MINOR(inode->i_rdev);
    pr_info("minor access = %d\r\n", minor_n);
    /* get device private data structure 
       container_of returns addr of containing struct of member
    */
    pcdev_data = container_of(inode->i_cdev, struct pcdev_private_data, cdev);
    /* save pcdev_data to filp for use by read or write in dev/pcdev */
    filp->private_data = pcdev_data;

    ret = check_permission(pcdev_data->perm, filp->f_mode);
    (!ret) ? pr_info("Open is successful\r\n"):pr_info("open is unsuccessful\r\n");
    return ret;
}

int pcd_release(struct inode *inode, struct file *filp)
{
    pr_info("release requsted\r\n");
    return 0;
}

ssize_t pcd_read(struct file* filp, char __user *buff, size_t count, loff_t *f_pos)
{
    //retreat private data from open
    struct pcdev_private_data *pcdev_data = (struct pcdev_private_data *)filp->private_data;
    int max_size = pcdev_data->size;
    pr_info("read requsted %zu\r\n", count);
    pr_info("current file position before reading %zu\r\n", *f_pos);
    //adjust count 
    if((*f_pos + count) > max_size ) {
        count = max_size - *f_pos;
    }
    //copy to user
    if(copy_to_user(buff, pcdev_data->buffer+(*f_pos), count)) {
        return -EFAULT;
    }
    /* update current file position */
    *f_pos += count;
    pr_info("Number of bytes read %zu\r\n", count);
    pr_info("Updated file position = %lld\r\n", *f_pos);
    return count;
}

ssize_t pcd_write(struct file* filp, const char __user *buff, size_t count, loff_t *f_pos)
{
    //retreat private data from open
    struct pcdev_private_data *pcdev_data = (struct pcdev_private_data *)filp->private_data;
    int max_size = pcdev_data->size;
    pr_info("write requsted %zu\r\n", count);
    pr_info("current file position before writing %zu\r\n", *f_pos);
    //adjust count 
    if((*f_pos + count) > max_size ) {
        count = max_size - *f_pos;
    }

    if(!count) {
        pr_err("Write no space left \r\n");
        return -ENOMEM;
    }
    //copy to user
    if(copy_from_user(pcdev_data->buffer+(*f_pos), buff,count)) {
        return -EFAULT;
    }
    /* update current file position */
    *f_pos += count;
    pr_info("Number of bytes write %zu\r\n", count);
    pr_info("Updated file position = %lld\r\n", *f_pos);
    return count;
}

loff_t pcd_lseek(struct file *filp, loff_t offset, int whence)
{
    loff_t temp;
    //retreat private data from open
    struct pcdev_private_data *pcdev_data = (struct pcdev_private_data *)filp->private_data;
    int max_size = pcdev_data->size;
    pr_info("lseek requsted\r\n");
    pr_info("lseek current file position %lld\r\n", filp->f_pos);
    switch(whence) {
        case SEEK_SET:
            if(offset > max_size || offset < 0)
                return -EINVAL;
            filp->f_pos = offset;
            break;
        case SEEK_CUR:
            temp = filp->f_pos + offset;
            if(temp > max_size || temp < 0)
                return -EINVAL;
            filp->f_pos = temp;
            break;
        case SEEK_END:
            temp = max_size + offset;
            if(temp > max_size || temp < 0)
                return -EINVAL;
            filp->f_pos = temp;
            break;
        default:
            return -EINVAL;
    }
    pr_info("lseek new file position %lld\r\n", filp->f_pos);
    return filp->f_pos;
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
static int __init pcd_module_init(void)
{
    int ret;
    int i;
    ret = alloc_chrdev_region(&pcdrv_data.device_number, 0, NO_OF_DEVICES, "pcd_devices");
    if(ret < 0) {
        pr_err("Alloc chr dev fail\r\n");
        goto out;
    }
    //create device class under sys/class
    pcdrv_data.class_pcd = class_create(THIS_MODULE, "pcd_class");
    if(IS_ERR(pcdrv_data.class_pcd)) {
        pr_err("Class creation fail\n");
        //convert pointer to error code
        ret = PTR_ERR(pcdrv_data.class_pcd);
        goto unreg_chrdev;
    }
    for(i = 0; i < NO_OF_DEVICES; i++) {
        pr_info("device number major:minor = %d:%d\r\n", MAJOR(pcdrv_data.device_number+i), MINOR(pcdrv_data.device_number+i));
         //init cdev
        cdev_init(&pcdrv_data.pcdev_data[i].cdev, &pcd_fops);
        pcdrv_data.pcdev_data[i].cdev.owner = THIS_MODULE;
        //register with VFS
        ret = cdev_add(&pcdrv_data.pcdev_data[i].cdev, pcdrv_data.device_number+i, 1);
        if(ret < 0) {
            pr_err("cdev add fail\r\n");
            goto cdev_del;
        }
       

        //device file populate device information 
        pcdrv_data.device_pcd = device_create( pcdrv_data.class_pcd, NULL, pcdrv_data.device_number+i, NULL, "pcdevn-%d", i+1);
        if(IS_ERR(pcdrv_data.device_pcd)) {
            pr_err("device creation fail\n");
            ret = PTR_ERR(pcdrv_data.device_pcd);
            goto class_del;
        }
    }
    pr_info("Module init done\r\n");
    return 0;
cdev_del:
class_del:
    for(;i>=0;i--) {
        device_destroy(pcdrv_data.class_pcd, pcdrv_data.device_number+i);
        cdev_del(&pcdrv_data.pcdev_data[i].cdev);
    }
    class_destroy(pcdrv_data.class_pcd);
    
unreg_chrdev:
    unregister_chrdev_region(pcdrv_data.device_number, NO_OF_DEVICES);
out:
    pr_err("Module insertion fail\r\n");
    return ret;
}

static void __exit pcd_module_cleanup(void)
{
    int i;
    for(i = 0;i < NO_OF_DEVICES;i++) {
        device_destroy(pcdrv_data.class_pcd, pcdrv_data.device_number+i);
        cdev_del(&pcdrv_data.pcdev_data[i].cdev);
    }
    class_destroy(pcdrv_data.class_pcd);
    unregister_chrdev_region(pcdrv_data.device_number, NO_OF_DEVICES);
    pr_info("Modeul unloaded\r\n");
}

module_init(pcd_module_init);
module_exit(pcd_module_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LEN");
MODULE_DESCRIPTION("A pseudo char driver multiple");