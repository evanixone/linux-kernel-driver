#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
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
__used __section("__versions") = {
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0x9c6febfc, "add_uevent_var" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0x92997ed8, "_printk" },
	{ 0xca9f75e, "i2c_transfer_buffer_flags" },
	{ 0x3ea1b6e4, "__stack_chk_fail" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0x5e3bec43, "i2c_smbus_read_byte" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0x51a910c0, "arm_copy_to_user" },
	{ 0xae353d77, "arm_copy_from_user" },
	{ 0x5f754e5a, "memset" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0xe66b7d3b, "cdev_init" },
	{ 0x1341b5e, "cdev_add" },
	{ 0x7b7fcf8d, "__class_create" },
	{ 0xeed31c02, "device_create" },
	{ 0x54743cc0, "i2c_get_adapter" },
	{ 0x1cd942e8, "i2c_new_client_device" },
	{ 0x9dedf58e, "i2c_unregister_device" },
	{ 0xf425bc, "i2c_register_driver" },
	{ 0xd7a9ae63, "i2c_put_adapter" },
	{ 0xf9a482f9, "msleep" },
	{ 0xff0cc455, "i2c_del_driver" },
	{ 0xd910fabf, "device_destroy" },
	{ 0x25775ba2, "class_destroy" },
	{ 0xf1ff7262, "cdev_del" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x78a319e7, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "5C2226CBA2305A2ABBDA9BA");
