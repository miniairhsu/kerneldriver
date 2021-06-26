#include <linux/module.h>

static int __init helloworld_init(void)
{
    pr_info("Hello world\r\n");
    return 0;
}

static void __exit helloworld_exit(void)
{
    pr_info("Good bye world\r\n");
}

module_init(helloworld_init);
module_exit(helloworld_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ME");
MODULE_DESCRIPTION("Simple hello world module");
MODULE_INFO(board, "Beaglebone black REV A5");