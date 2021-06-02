#include "pcd_platform_driver_dt_sysfs.h"

struct device_config pcdev_config[] = 
{
    [PCDEVA1X] = {.config_item1 = 60, .config_item2 = 21}, 
    [PCDEVB1X] = {.config_item1 = 50, .config_item2 = 22}, 
    [PCDEVC1X] = {.config_item1 = 40, .config_item2 = 23}, 
    [PCDEVD1X] = {.config_item1 = 30, .config_item2 = 24} 
}; 

struct pcdrv_private_data pcdrv_data;



struct file_operations pcd_fops = {
    .open    = pcd_open, 
    .write   = pcd_write,
    .read    = pcd_read, 
    .llseek  = pcd_lseek, 
    .release = pcd_release,
    .owner   = THIS_MODULE
};


ssize_t show_max_size(struct device *dev, struct device_attribute *attr, char *buf) 
{
    struct pcdev_private_data *dev_data = dev_get_drvdata(dev->parent); 
    return sprintf(buf, "%d\n", dev_data->pdata.size);
}

ssize_t show_serial_num(struct device *dev, struct device_attribute *attr, char *buf) 
{
    /* access to device private data */
    struct pcdev_private_data *dev_data = dev_get_drvdata(dev->parent); //class device parent instentiate from device tree
    return sprintf(buf, "%s\n", dev_data->pdata.serial_number);
}

ssize_t store_max_size(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    //convert string to long
    long result;
    int ret;
    struct pcdev_private_data *dev_data = dev_get_drvdata(dev->parent);
    ret = kstrtol(buf, 10, &result); //base 10, convert buf to result 
    if(ret)
        return ret;
    dev_data->pdata.size = result;
    dev_data->buffer = krealloc(dev_data->buffer, dev_data->pdata.size, GFP_KERNEL);
    return count;
}
/* create device_attributes and populate its functions*/
static DEVICE_ATTR(max_size, S_IRUGO|S_IWUSR, show_max_size, store_max_size); //create dev_attr_max_size to be populate in sysfs 
static DEVICE_ATTR(serial_num, S_IRUGO, show_serial_num, NULL); //create dev_attr_serial_num to be populated in sysfs 

/* collect attributes */
struct attribute *pcd_attrs[] = {
    &dev_attr_max_size.attr,
    &dev_attr_serial_num.attr,
    NULL
};
struct attribute_group pcd_attr_group = {
    .attrs = pcd_attrs
};

/* create attributes in sysfs dir */
int pcd_sysfs_create_files(struct device *pcd_dev) 
{
    int ret;
//populate attributes on at a time
#if 0
    ret = sysfs_create_file(&pcd_dev->kobj, &dev_attr_max_size.attr);
    if(ret)
        return ret;
    ret = sysfs_create_file(&pcd_dev->kobj, &dev_attr_serial_num.attr);
#endif
    return sysfs_create_group(&pcd_dev->kobj, &pcd_attr_group);
}

struct pcdev_platform_data* pcdev_get_platdata_from_dt(struct device *dev)
{
    //DT node
    struct device_node *dev_node = dev->of_node; //of_node contains device details
    struct pcdev_platform_data *pdata;
    if(!dev_node)
        /* probe is not from DT */
        return NULL;
    pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
    if(!pdata) {
        dev_info(dev, "cannot allocate platform data memory\r\n");
        return ERR_PTR(-ENOMEM);
    }
    //store node information to pdata
    if(of_property_read_string(dev_node, "org,device-serial-num", &pdata->serial_number)) {
        dev_info(dev, "missing serial number property\r\n");
        return ERR_PTR(-EINVAL);
    }

    if(of_property_read_u32(dev_node, "org,size", &pdata->size)) {
        dev_info(dev, "missing property\r\n");
        return ERR_PTR(-EINVAL);
    }

    if(of_property_read_u32(dev_node, "org,perm", &pdata->perm)) {
        dev_info(dev, "missing property\r\n");
        return ERR_PTR(-EINVAL);
    }
    return pdata;
}

struct of_device_id org_pcdev_dt_match[];
/* called when matched platform device is found */
int pcd_platform_driver_probe(struct platform_device *pdev)
{
    int ret;
    pr_info("A device is detected\r\n");
    struct device *dev = &pdev->dev;
    struct pcdev_private_data *dev_data;
    struct pcdev_platform_data *pdata;
    int driver_data;
    /* device tree match id */
    struct of_device_id *match;
    dev_info(dev, "A device is detected\r\n");
    /* check if Linux supports device tree CONFIG_OF is yes */
    match = of_match_device(of_match_ptr(org_pcdev_dt_match), dev);
    if (match) {
        pdata = pcdev_get_platdata_from_dt(dev);
        if(IS_ERR(pdata)) {
            return PTR_ERR(pdata);
        }
        driver_data = (int)match->data;
    } else {
         /* get platform data from platform device*/
        pdata = (struct pcdev_platform_data *)dev_get_platdata(dev);
        driver_data = pdev->id_entry->driver_data;
    }
    

    if(!pdata) {
        dev_info(dev, "No platform data available\r\n");
        ret = -EINVAL;
        return ret;
    }
   

    /* allocate memory for device private data */
    dev_data = devm_kzalloc(dev, sizeof(struct pcdev_private_data), GFP_KERNEL);
    if(!dev_data) {
        pr_info("Cannot allocate memory\r\n");
        return -ENOMEM;
    }
    /* save device private data pointer in platform device structure 
       so can be used later
    */
    dev_set_drvdata(dev, dev_data);
    dev_data->pdata.size = pdata->size;
    dev_data->pdata.perm = pdata->perm;
    dev_data->pdata.serial_number = pdata->serial_number;
    pr_info("Device serial number %s\r\n", dev_data->pdata.serial_number);
    pr_info("Device size %d\r\n", dev_data->pdata.size);
    pr_info("Device permission %d\r\n", dev_data->pdata.perm);
    /* find device config data from platform_device */
    pr_info("config item 1 = %d\r\n", pcdev_config[driver_data].config_item1);
    pr_info("config item 2 = %d\r\n", pcdev_config[driver_data].config_item2);
    /*allocate memory for device buffer */
    dev_data->buffer = devm_kzalloc(dev, dev_data->pdata.size, GFP_KERNEL);
    if(!dev_data->buffer) {
        pr_info("Cannot allocate memory\r\n");
        return -ENOMEM;
    }
    
    /* get device number */
    dev_data->dev_num = pcdrv_data.device_num_base + pcdrv_data.total_devices;

    /* cdev init */
    cdev_init(&dev_data->cdev, &pcd_fops);
    dev_data->cdev.owner = THIS_MODULE;
    ret = cdev_add(&dev_data->cdev, dev_data->dev_num, 1);
    if(ret < 0) {
        pr_err("cdev add fail \r\n");
        return ret;
    }
    pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd, dev, dev_data->dev_num, NULL, "pcdev-%d", pcdrv_data.total_devices);
    /* create device file for detected device from platform device driver */
    //pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd, NULL, dev_data->dev_num, NULL, "pcdev-%d", pdev->id);
    if(IS_ERR(pcdrv_data.device_pcd)) {
        pr_err("device creation fail\n");
        ret = PTR_ERR(pcdrv_data.device_pcd);
        cdev_del(&dev_data->cdev);
        return ret;
    }

    pcdrv_data.total_devices++;
    ret = pcd_sysfs_create_files(pcdrv_data.device_pcd);
    if(ret) {
        device_destroy(pcdrv_data.class_pcd, dev_data->dev_num);
        return ret;
    }

    pr_info("pcd device is probed\n");
    return 0;
}



int pcd_platform_driver_remove(struct platform_device *pdev)
{
    pr_info("pcd device is removed\n");
    struct pcdev_private_data *dev_data = dev_get_drvdata(&pdev->dev);
    /* remove device created */
    device_destroy(pcdrv_data.class_pcd, dev_data->dev_num);
    /* remove cdev entry */
    cdev_del(&dev_data->cdev);
    pcdrv_data.total_devices--;
    dev_info(&pdev->dev, "device is removed\r\n");
    return 0;
}



/* matching multiple platform device with different names */
struct platform_device_id pcdev_ids[] = {
    /* driver data is linked to pcdev_config */
    [0] = {.name = "pcdev_A1x", .driver_data = PCDEVA1X}, 
    [1] = {.name = "pcdev_B1x", .driver_data = PCDEVB1X}, 
    [2] = {.name = "pcdev_C1x", .driver_data = PCDEVC1X}, 
    [3] = {.name = "pcdev_D1x", .driver_data = PCDEVD1X}, 
};

struct of_device_id org_pcdev_dt_match[] =
{
    {.compatible = "pcdev-A1x", .data = (void*)PCDEVA1X}, 
    {.compatible = "pcdev-B1x", .data = (void*)PCDEVB1X}, 
    {.compatible = "pcdev-C1x", .data = (void*)PCDEVC1X}, 
    {.compatible = "pcdev-D1x", .data = (void*)PCDEVD1X}, 
    {}
}; 
struct platform_driver pcd_platform_driver = 
{
    .probe  = pcd_platform_driver_probe, 
    .remove =  pcd_platform_driver_remove, 
    .id_table = pcdev_ids, 
    /* id table is used then name field is no longer valid */
    .driver = {
        .name = "pseudo-char-device", //used to match with platform device
        .of_match_table = of_match_ptr(org_pcdev_dt_match) //return null if config_of is not defined
                                                           //return table if defined
    }
};
#define MAX_DEVICES 10
static int __init pcd_plaftform_driver_init(void)
{
    int ret;
    /* dynamic device number allocation */
    ret = alloc_chrdev_region( &pcdrv_data.device_num_base,0 , MAX_DEVICES, "pcdevs");
    if(ret < 0) {
        pr_err("Alloc chr dev fail\r\n");
        return ret;
    }
    /* create device class under sys/class */
    pcdrv_data.class_pcd = class_create(THIS_MODULE, "pcd_class");
    if(IS_ERR(pcdrv_data.class_pcd)) {
        pr_err("Class creation fail\n");
        //convert pointer to error code
        ret = PTR_ERR(pcdrv_data.class_pcd);
        unregister_chrdev_region(pcdrv_data.device_num_base, MAX_DEVICES);
        return ret;
    }
    /* register platform driver */
    platform_driver_register(&pcd_platform_driver);

    pr_info("pcd platform driver loaded\n");
    return 0;
}

static void __exit pcd_plaftform_driver_cleanup(void)
{
    platform_driver_unregister(&pcd_platform_driver);

    /* destory class */
    class_destroy(pcdrv_data.class_pcd);
    
    /* unregister device number */
    unregister_chrdev_region(pcdrv_data.device_num_base, MAX_DEVICES);
    pr_info("pcd platform driver cleanup\n");
}

module_init(pcd_plaftform_driver_init);
module_exit(pcd_plaftform_driver_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LEN");
MODULE_DESCRIPTION("A pseudo char driver multiple");