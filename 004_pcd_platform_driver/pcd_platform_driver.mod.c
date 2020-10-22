#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
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
__used
__attribute__((section("__versions"))) = {
	{ 0x31b425c9, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x3aa104d0, __VMLINUX_SYMBOL_STR(class_destroy) },
	{ 0x97dd967c, __VMLINUX_SYMBOL_STR(platform_driver_unregister) },
	{ 0x4f5dc9a3, __VMLINUX_SYMBOL_STR(__platform_driver_register) },
	{ 0x7485e15e, __VMLINUX_SYMBOL_STR(unregister_chrdev_region) },
	{ 0x577101b8, __VMLINUX_SYMBOL_STR(__class_create) },
	{ 0x29537c9e, __VMLINUX_SYMBOL_STR(alloc_chrdev_region) },
	{ 0xbbd8f690, __VMLINUX_SYMBOL_STR(device_destroy) },
	{ 0xada9f7e6, __VMLINUX_SYMBOL_STR(cdev_del) },
	{ 0x49271ffb, __VMLINUX_SYMBOL_STR(device_create) },
	{ 0x6d12a90a, __VMLINUX_SYMBOL_STR(cdev_add) },
	{ 0x59e5c3a1, __VMLINUX_SYMBOL_STR(cdev_init) },
	{ 0xc5e3c349, __VMLINUX_SYMBOL_STR(devm_kmalloc) },
	{ 0x2e5810c6, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr1) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xefd6cf06, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr0) },
	{ 0xb1ad28e0, __VMLINUX_SYMBOL_STR(__gnu_mcount_nc) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

