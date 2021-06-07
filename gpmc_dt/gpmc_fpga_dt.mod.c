#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(.gnu.linkonce.this_module) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section(__versions) = {
	{ 0xc79d2779, "module_layout" },
	{ 0x8296b546, "class_destroy" },
	{ 0x3bc52a67, "platform_driver_unregister" },
	{ 0x86bedcb9, "__platform_driver_register" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x689c90fe, "__class_create" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x4f58b822, "device_create" },
	{ 0x406681dd, "cdev_add" },
	{ 0xb8c2987b, "cdev_init" },
	{ 0x1fbed5b6, "devm_kmalloc" },
	{ 0x4a453f53, "iowrite32" },
	{ 0xe484e35f, "ioread32" },
	{ 0xc5e74216, "release_resource" },
	{ 0xf3e0e1df, "allocate_resource" },
	{ 0xd68c5a1f, "adjust_resource" },
	{ 0x344fd44f, "pv_ops" },
	{ 0xdbf17652, "_raw_spin_lock" },
	{ 0xbb560189, "_dev_info" },
	{ 0x8b66e8a1, "cdev_del" },
	{ 0xe70dc63b, "device_destroy" },
	{ 0xedc03953, "iounmap" },
	{ 0x1035c7c2, "__release_region" },
	{ 0x85bd1608, "__request_region" },
	{ 0x77358855, "iomem_resource" },
	{ 0x93a219c, "ioremap_nocache" },
	{ 0xc5850110, "printk" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "B4711FB93E1E0AB08451040");
