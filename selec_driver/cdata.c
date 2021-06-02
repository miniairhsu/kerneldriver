/*
 * codes from embedded Linux Primer Second Edition
 */
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/interrupt.h>
#include "cdata_ioctl.h"

#define dbg(fmt,args...) printk("[%s]:%d => "fmt,__FUNCTION__,__LINE__,##args)
#define DBG() printk("[%s]:%d => \n",__FUNCTION__,__LINE__)
#define HELLO_MAJOR 234

static int debug_enable=0;
int wait=0;
module_param(debug_enable, int, 0);
MODULE_PARM_DESC(debug_enable,"Enable module debug mode.");

struct file_operations hello_fops;
struct cmd_struct *cmd_Xfer;

struct cdata_t{
	int index;
	wait_queue_head_t read_wait;
};

static int hello_open(struct inode *inode, struct file *file)
{
	struct cdata_t *cdata;
	
	cdata = (struct cdata_t *)kmalloc(sizeof(struct cdata_t), GFP_KERNEL);
	cdata->index=0;
  	init_waitqueue_head(&cdata->read_wait);
	file->private_data=(void *)cdata;

	printk("hello_open: successful\n");
	return 0;
}

static int hello_release(struct inode *inode, struct file *file)
{
	printk("hello_release: successful\n");
	return 0;
}

static ssize_t hello_read(struct file *file, char *bug, size_t count, loff_t *ptr)
{
	printk("hello_read: returning zero byte\n");
	return 0;
}

static ssize_t hello_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	printk("hello_write: accepting zero bytes\n");
	return 0;
}

static long hello_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct cdata_t *cdata;
	unsigned int event_id;
	cdata=(struct cdata_t *)file->private_data;
	event_id = (unsigned int)arg;
	printk(" hello_ioctl: cmd=%u, arg=%lu\n", cmd, arg);
	switch(cmd)
	{
		case IOCTL_ENABLE:
			printk("set cdata->wait=%d\n", event_id);
			wait=event_id;
		break;

		case IOCTL_TX_CMD:
			copy_to_user((void *)arg, (const void *)cmd_Xfer, sizeof(struct cmd_struct));
		break;
	
	}


	return 0;
}

static unsigned int hello_poll(struct file *file, poll_table *pt)
{
	unsigned int mask = POLLOUT;
	struct cdata_t *cdata;
	
	cdata=(struct cdata_t *)file->private_data;

    printk( "call hello_poll");
	
	poll_wait(file,&cdata->read_wait ,pt);

	if(wait==1)
	{
  		printk(KERN_INFO "make mask readable\n");
		mask = 0;
		mask = POLLIN | POLLRDNORM; //| POLLRDNORM; //readable
		cmd_Xfer->cmd = 1;
		wait = 0;
	} else if(wait == 2) {
		mask = 0;
		mask = POLLIN | POLLRDNORM; //readable
		cmd_Xfer->cmd = 2;
		wait = 0;
	}
	return mask;

}

static int __init hello_init(void)
{
	int ret;
	printk(KERN_INFO "Hello Example Init debug mode is %s\n", 
			debug_enable?"enabled":"disable");

	ret=register_chrdev(HELLO_MAJOR, "cdata", &hello_fops);
	if(ret < 0)
	{
		printk("Error registering hello device\n");
		return ret;
	}
	cmd_Xfer = kmalloc(sizeof(struct cmd_struct), GFP_KERNEL);
	printk("Hello: registered moule successfully!\n");
	return 0;
}

static void __exit hello_exit(void)
{
	printk("Hello Example Exit\n");
	unregister_chrdev(HELLO_MAJOR, "cdata");
}

struct file_operations hello_fops = {
	owner: THIS_MODULE,
	read: hello_read,
	write: hello_write,
	unlocked_ioctl: hello_ioctl,
	open: hello_open,
	poll: hello_poll,
	release: hello_release,
};

module_init(hello_init);
module_exit(hello_exit);

MODULE_AUTHOR("hugh");
MODULE_DESCRIPTION("Example");
MODULE_LICENSE("GPL");