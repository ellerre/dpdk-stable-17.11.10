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
	{ 0xaf381eb0, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x75f8d3b8, __VMLINUX_SYMBOL_STR(up_read) },
	{ 0x79aa04a2, __VMLINUX_SYMBOL_STR(get_random_bytes) },
	{ 0xec6667d0, __VMLINUX_SYMBOL_STR(netif_carrier_on) },
	{ 0x8568288a, __VMLINUX_SYMBOL_STR(netif_carrier_off) },
	{ 0x4e536271, __VMLINUX_SYMBOL_STR(__dynamic_pr_debug) },
	{ 0x778b8af3, __VMLINUX_SYMBOL_STR(mutex_unlock) },
	{ 0x6479ba64, __VMLINUX_SYMBOL_STR(__put_net) },
	{ 0x478a608c, __VMLINUX_SYMBOL_STR(kthread_create_on_node) },
	{ 0x15ba50a6, __VMLINUX_SYMBOL_STR(jiffies) },
	{ 0xb98c4938, __VMLINUX_SYMBOL_STR(down_read) },
	{ 0xe2d5255a, __VMLINUX_SYMBOL_STR(strcmp) },
	{ 0x140d2b6f, __VMLINUX_SYMBOL_STR(kthread_bind) },
	{ 0xb45caf3e, __VMLINUX_SYMBOL_STR(__netdev_alloc_skb) },
	{ 0xa6682fdd, __VMLINUX_SYMBOL_STR(__init_waitqueue_head) },
	{ 0x998b09e7, __VMLINUX_SYMBOL_STR(param_ops_charp) },
	{ 0x42d5399d, __VMLINUX_SYMBOL_STR(misc_register) },
	{ 0xfb578fc5, __VMLINUX_SYMBOL_STR(memset) },
	{ 0xeed397e6, __VMLINUX_SYMBOL_STR(netif_rx_ni) },
	{ 0x8686510b, __VMLINUX_SYMBOL_STR(unregister_pernet_subsys) },
	{ 0x49e2a6e1, __VMLINUX_SYMBOL_STR(netif_tx_wake_queue) },
	{ 0x9b65a65f, __VMLINUX_SYMBOL_STR(current_task) },
	{ 0x8a3b82f5, __VMLINUX_SYMBOL_STR(__mutex_init) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xaf4a03a5, __VMLINUX_SYMBOL_STR(kthread_stop) },
	{ 0x5a5a2271, __VMLINUX_SYMBOL_STR(__cpu_online_mask) },
	{ 0x93f23a42, __VMLINUX_SYMBOL_STR(free_netdev) },
	{ 0xa1c76e0a, __VMLINUX_SYMBOL_STR(_cond_resched) },
	{ 0x9166fada, __VMLINUX_SYMBOL_STR(strncpy) },
	{ 0x79b83733, __VMLINUX_SYMBOL_STR(register_netdev) },
	{ 0x5a921311, __VMLINUX_SYMBOL_STR(strncmp) },
	{ 0xa8406756, __VMLINUX_SYMBOL_STR(skb_push) },
	{ 0x9c079d54, __VMLINUX_SYMBOL_STR(mutex_lock) },
	{ 0x69d38b8b, __VMLINUX_SYMBOL_STR(up_write) },
	{ 0x84a77c31, __VMLINUX_SYMBOL_STR(down_write) },
	{ 0xfe487975, __VMLINUX_SYMBOL_STR(init_wait_entry) },
	{ 0x7cd8d75e, __VMLINUX_SYMBOL_STR(page_offset_base) },
	{ 0xa916b694, __VMLINUX_SYMBOL_STR(strnlen) },
	{ 0xdb7305a1, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0x8ddd8aad, __VMLINUX_SYMBOL_STR(schedule_timeout) },
	{ 0xf2c07f3a, __VMLINUX_SYMBOL_STR(alloc_netdev_mqs) },
	{ 0x2ea2c95c, __VMLINUX_SYMBOL_STR(__x86_indirect_thunk_rax) },
	{ 0x54c76f7e, __VMLINUX_SYMBOL_STR(eth_type_trans) },
	{ 0x7f24de73, __VMLINUX_SYMBOL_STR(jiffies_to_usecs) },
	{ 0xd39f7192, __VMLINUX_SYMBOL_STR(wake_up_process) },
	{ 0xec9ac8d5, __VMLINUX_SYMBOL_STR(register_pernet_subsys) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
	{ 0xcbd4898c, __VMLINUX_SYMBOL_STR(fortify_panic) },
	{ 0x645c2cc8, __VMLINUX_SYMBOL_STR(ether_setup) },
	{ 0xfe768495, __VMLINUX_SYMBOL_STR(__wake_up) },
	{ 0xb3f7646e, __VMLINUX_SYMBOL_STR(kthread_should_stop) },
	{ 0x237a015a, __VMLINUX_SYMBOL_STR(prepare_to_wait_event) },
	{ 0x54496b4, __VMLINUX_SYMBOL_STR(schedule_timeout_interruptible) },
	{ 0x69acdf38, __VMLINUX_SYMBOL_STR(memcpy) },
	{ 0xd9f4f53c, __VMLINUX_SYMBOL_STR(dev_trans_start) },
	{ 0xd4fa5c30, __VMLINUX_SYMBOL_STR(finish_wait) },
	{ 0x32984638, __VMLINUX_SYMBOL_STR(unregister_netdev) },
	{ 0xdc4caa9f, __VMLINUX_SYMBOL_STR(consume_skb) },
	{ 0xe2d5ab6, __VMLINUX_SYMBOL_STR(skb_put) },
	{ 0x362ef408, __VMLINUX_SYMBOL_STR(_copy_from_user) },
	{ 0xc3da4af4, __VMLINUX_SYMBOL_STR(misc_deregister) },
	{ 0xf95fc8e8, __VMLINUX_SYMBOL_STR(__init_rwsem) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "C1E8E203446B0B083BEE97D");
