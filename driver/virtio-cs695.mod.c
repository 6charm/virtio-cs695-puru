#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

#ifdef CONFIG_UNWINDER_ORC
#include <asm/orc_header.h>
ORC_HEADER;
#endif

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
	{ 0xb8f77011, "misc_deregister" },
	{ 0x95534a9f, "__asan_report_load8_noabort" },
	{ 0x6f6699f1, "nonseekable_open" },
	{ 0x27891904, "virtqueue_get_buf" },
	{ 0x8111a21e, "complete" },
	{ 0x54654294, "__asan_report_load2_noabort" },
	{ 0x7915a480, "__asan_report_store2_noabort" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x281a8d31, "unregister_virtio_driver" },
	{ 0x4c03a563, "random_kmalloc_seed" },
	{ 0x55474962, "kmalloc_caches" },
	{ 0xc45cbd20, "kmalloc_trace" },
	{ 0x29e6fd4e, "misc_register" },
	{ 0xb823ac8b, "__asan_report_store8_noabort" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0xef5f773a, "__init_swait_queue_head" },
	{ 0x43babd19, "sg_init_one" },
	{ 0x36f25fcb, "virtqueue_add_sgs" },
	{ 0x5da0f427, "virtqueue_kick" },
	{ 0x6ca75d22, "wait_for_completion" },
	{ 0x6b10bee1, "_copy_to_user" },
	{ 0x8f28a146, "__asan_report_store4_noabort" },
	{ 0x98ec031b, "__asan_unregister_globals" },
	{ 0xf5869226, "__asan_register_globals" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xdd36cb49, "register_virtio_driver" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0x122c3a7e, "_printk" },
	{ 0x64a2c724, "virtio_reset_device" },
	{ 0x40d5b7b6, "virtqueue_detach_unused_buf" },
	{ 0x65487097, "__x86_indirect_thunk_rax" },
	{ 0x37a0cba, "kfree" },
	{ 0x5a9546b6, "module_layout" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("virtio:d0000002Av*");

MODULE_INFO(srcversion, "A71A6217A452714A6AAA543");
