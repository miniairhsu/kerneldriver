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

#ifdef RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x7ef3190f, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x9491b702, __VMLINUX_SYMBOL_STR(class_destroy) },
	{ 0x641b7b9c, __VMLINUX_SYMBOL_STR(platform_driver_unregister) },
	{ 0x7952ec47, __VMLINUX_SYMBOL_STR(__platform_driver_register) },
	{ 0x7485e15e, __VMLINUX_SYMBOL_STR(unregister_chrdev_region) },
	{ 0x339b7a65, __VMLINUX_SYMBOL_STR(__class_create) },
	{ 0x29537c9e, __VMLINUX_SYMBOL_STR(alloc_chrdev_region) },
	{ 0xb7f96077, __VMLINUX_SYMBOL_STR(device_create) },
	{ 0x4b1552f8, __VMLINUX_SYMBOL_STR(cdev_add) },
	{ 0x217421db, __VMLINUX_SYMBOL_STR(cdev_init) },
	{ 0x7d78e2b3, __VMLINUX_SYMBOL_STR(of_match_device) },
	{ 0x40e9705f, __VMLINUX_SYMBOL_STR(of_property_read_variable_u32_array) },
	{ 0xab4efeb3, __VMLINUX_SYMBOL_STR(of_property_read_string) },
	{ 0xd80c3e93, __VMLINUX_SYMBOL_STR(devm_kmalloc) },
	{ 0x67496df7, __VMLINUX_SYMBOL_STR(_dev_info) },
	{ 0x9f7a3e33, __VMLINUX_SYMBOL_STR(cdev_del) },
	{ 0x91bda9a9, __VMLINUX_SYMBOL_STR(device_destroy) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x2e5810c6, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr1) },
	{ 0xb1ad28e0, __VMLINUX_SYMBOL_STR(__gnu_mcount_nc) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

