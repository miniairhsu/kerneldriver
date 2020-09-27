#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>

#define DEV_MEM_SIZE 512
char device_buffer[DEV_MEM_SIZE];
dev_t device_number;
struct cdev pcd_cdev;
int pcd_open(struct inode *inode, struct file *filp)
{
    pr_info("open requsted\r\n");
    return 0;
}

int pcd_release(struct inode *inode, struct file *filp)
{
    pr_info("release requsted\r\n");
    return 0;
}

ssize_t pcd_read(struct file* filp, char __user *buff, size_t count, loff_t *f_pos)
{
    pr_info("read requsted %zu\r\n", count);
    pr_info("current file position before reading %zu\r\n", *f_pos);
    //adjust count 
    if((*f_pos + count) > DEV_MEM_SIZE ) {
        count = DEV_MEM_SIZE - *f_pos;
    }
    //copy to user
    if(copy_to_user(buff, &device_buffer[*f_pos], count)) {
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
    pr_info("write requsted %zu\r\n", count);
    pr_info("current file position before write %zu\r\n", *f_pos);
    //adjust count 
    if((*f_pos + count) > DEV_MEM_SIZE ) {
        count = DEV_MEM_SIZE - *f_pos;
    }
    if(!count)
        return -ENOMEM;
    //copy to user
    if(copy_from_user(&device_buffer[*f_pos], buff, count)) {
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
    pr_info("lseek requsted\r\n");
    pr_info("lseek current file position %lld\r\n", filp->f_pos);
    switch(whence) {
        case SEEK_SET:
            if(offset > DEV_MEM_SIZE || offset < 0)
                return -EINVAL;
            filp->f_pos = offset;
            break;
        case SEEK_CUR:
            temp = filp->f_pos + offset;
            if(temp > DEV_MEM_SIZE || temp < 0)
                return -EINVAL;
            filp->f_pos = temp;
            break;
        case SEEK_END:
            temp = DEV_MEM_SIZE + offset;
            if(temp > DEV_MEM_SIZE || temp < 0)
                return -EINVAL;
            filp->f_pos = temp;
            break;
        default:
            return -EINVAL;
    }
     pr_info("lseek new file position %lld\r\n", filp->f_pos);
    return filp->f_pos;
}
struct file_operations pcd_fops = {
    .open    = pcd_open, 
    .write   = pcd_write,
    .read    = pcd_read, 
    .llseek  = pcd_lseek, 
    .release = pcd_release,
    .owner   = THIS_MODULE
};

struct class *class_pcd;
struct device *device_pcd;
#undef pr_mt
#define pr_fmt(fmt) "%s:" fmt,__func__ 
static int __init pcd_module_init(void)
{
    int ret;
    ret = alloc_chrdev_region(&device_number, 0, 1, "pcd_devices");
    if(ret < 0) {
        pr_err("Alloc chr dev fail\r\n");
        goto out;
    }
    pr_info("device number major:minor = %d:%d\r\n", MAJOR(device_number), MINOR(device_number));
    //init cdev
    cdev_init(&pcd_cdev, &pcd_fops);
    pcd_cdev.owner = THIS_MODULE;
    //register with VFS
    ret = cdev_add(&pcd_cdev, device_number, 1);
    if(ret < 0) {
        pr_err("cdev add fail\r\n");
        goto unreg_chrdev;
    }
    //create device class under sys/class
    class_pcd = class_create(THIS_MODULE, "pcd_class");
    if(IS_ERR(class_pcd)) {
        pr_err("Class creation fail\n");
        //convert pointer to error code
        ret = PTR_ERR(class_pcd);
        goto cdev_del;
    }

    //device file populate device information undr sys/class/pcd_class
    device_pcd = device_create(class_pcd, NULL, device_number, NULL, "pcd");
    if(IS_ERR(device_pcd)) {
         pr_err("device creation fail\n");
         ret = PTR_ERR(device_pcd);
         goto class_del;
    }
    pr_info("Module init done\r\n");
    return 0;
class_del:
    class_destroy(class_pcd);
cdev_del:
    cdev_del(&pcd_cdev);
unreg_chrdev:
    unregister_chrdev_region(device_number, 1);
out:
    pr_err("Module insertion fail\r\n");
    return ret;
}

static void __exit pcd_module_cleanup(void)
{
    device_destroy(class_pcd, device_number);
    class_destroy(class_pcd);
    cdev_del(&pcd_cdev);
    unregister_chrdev_region(device_number, 1);
    pr_info("Modeul unloaded\r\n");
}

module_init(pcd_module_init);
module_exit(pcd_module_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LEN");
MODULE_DESCRIPTION("A pseudo char driver");