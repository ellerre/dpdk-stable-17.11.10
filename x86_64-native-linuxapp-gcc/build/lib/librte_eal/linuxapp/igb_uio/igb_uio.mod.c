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
	{ 0x998b09e7, __VMLINUX_SYMBOL_STR(param_ops_charp) },
	{ 0x5db62b68, __VMLINUX_SYMBOL_STR(pci_unregister_driver) },
	{ 0xccbdf9db, __VMLINUX_SYMBOL_STR(__pci_register_driver) },
	{ 0xe2d5255a, __VMLINUX_SYMBOL_STR(strcmp) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x8ff4079b, __VMLINUX_SYMBOL_STR(pv_irq_ops) },
	{ 0x665550f8, __VMLINUX_SYMBOL_STR(arch_dma_alloc_attrs) },
	{ 0x1fac0f3a, __VMLINUX_SYMBOL_STR(__uio_register_device) },
	{ 0x42c8de35, __VMLINUX_SYMBOL_STR(ioremap_nocache) },
	{ 0x1da5f6d1, __VMLINUX_SYMBOL_STR(sysfs_create_group) },
	{ 0xcbac4b66, __VMLINUX_SYMBOL_STR(dma_ops) },
	{ 0x7ae5ad74, __VMLINUX_SYMBOL_STR(sme_active) },
	{ 0x198787b0, __VMLINUX_SYMBOL_STR(pci_enable_device) },
	{ 0xd7b64983, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0x59acc80d, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x3a4f65e6, __VMLINUX_SYMBOL_STR(pci_msi_unmask_irq) },
	{ 0x4e3ca9a4, __VMLINUX_SYMBOL_STR(pci_intx) },
	{ 0xe94d9890, __VMLINUX_SYMBOL_STR(pci_msi_mask_irq) },
	{ 0x229075d, __VMLINUX_SYMBOL_STR(pci_cfg_access_unlock) },
	{ 0xfd0b70d7, __VMLINUX_SYMBOL_STR(pci_cfg_access_lock) },
	{ 0x300c3da7, __VMLINUX_SYMBOL_STR(irq_get_irq_data) },
	{ 0x8706013e, __VMLINUX_SYMBOL_STR(__dynamic_dev_dbg) },
	{ 0x613df0a6, __VMLINUX_SYMBOL_STR(pci_irq_vector) },
	{ 0x564c4702, __VMLINUX_SYMBOL_STR(dev_notice) },
	{ 0xcb5a4652, __VMLINUX_SYMBOL_STR(dev_err) },
	{ 0x2072ee9b, __VMLINUX_SYMBOL_STR(request_threaded_irq) },
	{ 0xfd21556f, __VMLINUX_SYMBOL_STR(_dev_info) },
	{ 0xe1d20450, __VMLINUX_SYMBOL_STR(pci_alloc_irq_vectors_affinity) },
	{ 0x6f6b29fa, __VMLINUX_SYMBOL_STR(pci_set_master) },
	{ 0x43516a8c, __VMLINUX_SYMBOL_STR(pci_check_and_mask_intx) },
	{ 0x3e20ed34, __VMLINUX_SYMBOL_STR(uio_event_notify) },
	{ 0x4badf802, __VMLINUX_SYMBOL_STR(pci_free_irq_vectors) },
	{ 0xc1514a3b, __VMLINUX_SYMBOL_STR(free_irq) },
	{ 0x85b1995d, __VMLINUX_SYMBOL_STR(pci_clear_master) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0xff1607b3, __VMLINUX_SYMBOL_STR(pci_disable_device) },
	{ 0xedc03953, __VMLINUX_SYMBOL_STR(iounmap) },
	{ 0x73941a94, __VMLINUX_SYMBOL_STR(uio_unregister_device) },
	{ 0x10c2b4ae, __VMLINUX_SYMBOL_STR(sysfs_remove_group) },
	{ 0x28318305, __VMLINUX_SYMBOL_STR(snprintf) },
	{ 0x2ea2c95c, __VMLINUX_SYMBOL_STR(__x86_indirect_thunk_rax) },
	{ 0xdb7305a1, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0xe394fcca, __VMLINUX_SYMBOL_STR(pci_enable_sriov) },
	{ 0x89514cc1, __VMLINUX_SYMBOL_STR(pci_disable_sriov) },
	{ 0x5703d87d, __VMLINUX_SYMBOL_STR(pci_num_vf) },
	{ 0x3c80c06c, __VMLINUX_SYMBOL_STR(kstrtoull) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=uio";


MODULE_INFO(srcversion, "330A891942A68E1201A4E10");
