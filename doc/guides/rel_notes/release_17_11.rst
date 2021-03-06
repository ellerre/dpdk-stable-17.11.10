DPDK Release 17.11
==================

.. **Read this first.**

   The text in the sections below explains how to update the release notes.

   Use proper spelling, capitalization and punctuation in all sections.

   Variable and config names should be quoted as fixed width text:
   ``LIKE_THIS``.

   Build the docs and view the output file to ensure the changes are correct::

      make doc-guides-html

      xdg-open build/doc/html/guides/rel_notes/release_17_11.html


New Features
------------

.. This section should contain new features added in this release. Sample
   format:

   * **Add a title in the past tense with a full stop.**

     Add a short 1-2 sentence description in the past tense. The description
     should be enough to allow someone scanning the release notes to
     understand the new feature.

     If the feature adds a lot of sub-features you can use a bullet list like
     this:

     * Added feature foo to do something.
     * Enhanced feature bar to do something else.

     Refer to the previous release notes for examples.

     This section is a comment. do not overwrite or remove it.
     Also, make sure to start the actual text at the margin.
     =========================================================

* **Extended port_id range from uint8_t to uint16_t.**

  Increased the ``port_id`` range from 8 bits to 16 bits in order to support
  more than 256 ports in DPDK. All ethdev APIs which have ``port_id`` as
  parameter have been changed.

* **Modified the return type of rte_eth_stats_reset.**

  Changed return type of ``rte_eth_stats_reset`` from ``void`` to ``int`` so
  that the caller can determine whether a device supports the operation or not
  and if the operation was carried out.

* **Added a new driver for Marvell Armada 7k/8k devices.**

  Added the new ``mrvl`` net driver for Marvell Armada 7k/8k devices. See the
  :doc:`../nics/mrvl` NIC guide for more details on this new driver.

* **Updated mlx4 driver.**

  Updated the mlx4 driver including the following changes:

   * Isolated mode (rte_flow) can now be enabled anytime, not only during
     initial device configuration.
   * Flow rules now support up to 4096 priority levels usable at will by
     applications.
   * Enhanced error message to help debugging invalid/unsupported flow rules.
   * Flow rules matching all multicast and promiscuous traffic are now allowed.
   * No more software restrictions on flow rules with the RSS action, their
     configuration is much more flexible.
   * Significantly reduced memory footprint for Rx and Tx queue objects.
   * While supported, UDP RSS is temporarily disabled due to a remaining issue
     with its support in the Linux kernel.
   * The new RSS implementation does not automatically spread traffic according
     to the inner packet of VXLAN frames anymore, only the outer one (like
     other PMDs).
   * Partial (Tx only) support for secondary processes was broken and had to be
     removed.
   * Refactored driver to get rid of dependency on the components provided by
     Mellanox OFED and instead rely on the current and public rdma-core
     package and Linux version from now on.
   * Removed compile-time limitation on number of device instances the PMD
     can support.

* **Updated mlx5 driver.**

  Updated the mlx5 driver including the following changes:

   * Enabled the PMD to run on top of upstream Linux kernel and rdma-core
     libs, removing the dependency on specific Mellanox OFED libraries.
   * Improved PMD latency performance.
   * Improved PMD memory footprint.
   * Added support for vectorized Rx/Tx burst for ARMv8.
   * Added support for secondary process.
   * Added support for flow counters.
   * Added support for Rx hardware timestamp offload.
   * Added support for device removal event.

* **Added SoftNIC PMD.**

  Added a new SoftNIC PMD. This virtual device provides applications with
  software fallback support for traffic management.

* **Added support for NXP DPAA Devices.**

  Added support for NXP's DPAA devices - LS104x series. This includes:

  * DPAA Bus driver
  * DPAA Mempool driver for supporting offloaded packet memory pool
  * DPAA PMD for DPAA devices

  See the :doc:`../nics/dpaa` document for more details of this new driver.

* **Updated support for Cavium OCTEONTX Device.**

  Updated support for Cavium's OCTEONTX device (CN83xx). This includes:

  * OCTEONTX Mempool driver for supporting offloaded packet memory pool
  * OCTEONTX Ethdev PMD
  * OCTEONTX Eventdev-Ethdev Rx adapter

  See the :doc:`../nics/octeontx` document for more details of this new driver.

* **Added PF support to the Netronome NFP PMD.**

  Added PF support to the Netronome NFP PMD. Previously the NFP PMD only
  supported VFs. PF support is just as a basic DPDK port and has no VF
  management yet.

  PF support comes with firmware upload support which allows the PMD to
  independently work from kernel netdev NFP drivers.

  NFP 4000 devices are also now supported along with previous 6000 devices.

* **Updated bnxt PMD.**

  Major enhancements include:

   * Support for Flow API
   * Support for Tx and Rx descriptor status functions

* **Added bus agnostic functions to cryptodev for PMD initialization**

  Added new PMD assist, bus independent, functions
  ``rte_cryptodev_pmd_parse_input_args()``, ``rte_cryptodev_pmd_create()`` and
  ``rte_cryptodev_pmd_destroy()`` for drivers to manage creation and
  destruction of new device instances.

* **Updated QAT crypto PMD.**

  Added several performance enhancements:

  * Removed atomics from the internal queue pair structure.
  * Added coalesce writes to HEAD CSR on response processing.
  * Added coalesce writes to TAIL CSR on request processing.

  In addition support was added for the AES CCM algorithm.

* **Updated the AESNI MB PMD.**

  The AESNI MB PMD has been updated with additional support for:

  * The DES CBC algorithm.
  * The DES DOCSIS BPI algorithm.

  This change requires version 0.47 of the IPSec Multi-buffer library. For
  more details see the :doc:`../cryptodevs/aesni_mb` documentation.

* **Updated the OpenSSL PMD.**

  The OpenSSL PMD has been updated with additional support for:

  * The DES CBC algorithm.
  * The AES CCM algorithm.

* **Added NXP DPAA SEC crypto PMD.**

  A new ``dpaa_sec`` hardware based crypto PMD for NXP DPAA devices has been
  added. See the :doc:`../cryptodevs/dpaa_sec` document for more details.

* **Added MRVL crypto PMD.**

  A new crypto PMD has been added, which provides several ciphering and hashing
  algorithms. All cryptography operations use the MUSDK library crypto API.
  See the :doc:`../cryptodevs/mrvl` document for more details.

* **Add new benchmarking mode to dpdk-test-crypto-perf application.**

  Added a new "PMD cyclecount" benchmark mode to the ``dpdk-test-crypto-perf``
  application to display a detailed breakdown of CPU cycles used by hardware
  acceleration.

* **Added the Security Offload Library.**

  Added an experimental library - ``rte_security``. This provide security APIs
  for protocols like IPSec using inline ipsec offload to ethernet devices or
  full protocol offload with lookaside crypto devices.

  See the :doc:`../prog_guide/rte_security` section of the DPDK Programmers
  Guide document for more information.

* **Updated the DPAA2_SEC crypto driver to support rte_security.**

  Updated the ``dpaa2_sec`` crypto PMD to support ``rte_security`` lookaside
  protocol offload for IPSec.

* **Updated the IXGBE ethernet driver to support rte_security.**

  Updated ixgbe ethernet PMD to support ``rte_security`` inline IPSec offload.

* **Updated ipsec-secgw application to support rte_security.**

  Updated the ``ipsec-secgw`` sample application to support ``rte_security``
  actions for ipsec inline and full protocol offload using lookaside crypto
  offload.

* **Added IOMMU support to libvhost-user**

  Implemented device IOTLB in the Vhost-user backend, and enabled Virtio's
  IOMMU feature. The feature is disabled by default, and can be enabled by
  setting ``RTE_VHOST_USER_IOMMU_SUPPORT`` flag at vhost device registration
  time.

* **Added the Event Ethernet Adapter Library.**

  Added the Event Ethernet Adapter library. This library provides APIs for
  eventdev applications to configure the ethdev for eventdev packet flow.

* **Updated DPAA2 Event PMD for the Event Ethernet Adapter.**

  Added support for the eventdev ethernet adapter for DPAA2.

* **Added Membership library (rte_member).**

  Added a new data structure library called the Membership Library.

  The Membership Library is an extension and generalization of a traditional
  filter (for example Bloom Filter) structure that has multiple usages in a
  wide variety of workloads and applications. In general, the Membership
  Library is a data structure that provides a "set-summary" and responds to
  set-membership queries whether a certain member belongs to a set(s).

  The library provides APIs for DPDK applications to insert a new member,
  delete an existing member, and query the existence of a member in a given
  set, or a group of sets. For the case of a group of sets the library will
  return not only whether the element has been inserted in one of the sets but
  also which set it belongs to.

  See the :doc:`../prog_guide/member_lib` documentation in the Programmers
  Guide, for more information.

* **Added the Generic Segmentation Offload Library.**

  Added the Generic Segmentation Offload (GSO) library to enable
  applications to split large packets (e.g. MTU is 64KB) into small
  ones (e.g. MTU is 1500B). Supported packet types are:

  * TCP/IPv4 packets.
  * VxLAN packets, which must have an outer IPv4 header, and contain
    an inner TCP/IPv4 packet.
  * GRE packets, which must contain an outer IPv4 header, and inner
    TCP/IPv4 headers.

  The GSO library doesn't check if the input packets have correct
  checksums, and doesn't update checksums for output packets.
  Additionally, the GSO library doesn't process IP fragmented packets.

* **Added the Flow Classification Library.**

  Added an experimental Flow Classification library to provide APIs for DPDK
  applications to classify an input packet by matching it against a set of
  flow rules. It uses the ``librte_table`` API to manage the flow rules.


Resolved Issues
---------------

.. This section should contain bug fixes added to the relevant
   sections. Sample format:

   * **code/section Fixed issue in the past tense with a full stop.**

     Add a short 1-2 sentence description of the resolved issue in the past
     tense.

     The title should contain the code/lib section like a commit message.

     Add the entries in alphabetic order in the relevant sections below.

   This section is a comment. do not overwrite or remove it.
   Also, make sure to start the actual text at the margin.
   =========================================================


* **Service core fails to call service callback due to atomic lock**

  In a specific configuration of multi-thread unsafe services and service
  cores, a service core previously did not correctly release the atomic lock
  on the service. This would result in the cores polling the service, but it
  looked like another thread was executing the service callback. The logic for
  atomic locking of the services has been fixed and refactored for readability.


API Changes
-----------

.. This section should contain API changes. Sample format:

   * Add a short 1-2 sentence description of the API change. Use fixed width
     quotes for ``rte_function_names`` or ``rte_struct_names``. Use the past
     tense.

   This section is a comment. do not overwrite or remove it.
   Also, make sure to start the actual text at the margin.
   =========================================================

* **Ethdev device name length increased.**

  The size of internal device name has been increased to 64 characters
  to allow for storing longer bus specific names.

* **Removed the Ethdev RTE_ETH_DEV_DETACHABLE flag.**

  Removed the Ethdev ``RTE_ETH_DEV_DETACHABLE`` flag. This flag is not
  required anymore, with the new hotplug implementation. It has been removed
  from the ether library. Its semantics are now expressed at the bus and PMD
  level.

* **Service cores API updated for usability**

  The service cores API has been changed, removing pointers from the API where
  possible, and instead using integer IDs to identify each service. This
  simplifies application code, aids debugging, and provides better
  encapsulation. A summary of the main changes made is as follows:

  * Services identified by ID not by ``rte_service_spec`` pointer
  * Reduced API surface by using ``set`` functions instead of enable/disable
  * Reworked ``rte_service_register`` to provide the service ID to registrar
  * Reworked start and stop APIs into ``rte_service_runstate_set``
  * Added API to set runstate of service implementation to indicate readiness

* **The following changes have been made in the mempool library**

  * Moved ``flags`` datatype from ``int`` to ``unsigned int`` for
    ``rte_mempool``.
  * Removed ``__rte_unused int flag`` param from ``rte_mempool_generic_put``
    and ``rte_mempool_generic_get`` API.
  * Added ``flags`` param in ``rte_mempool_xmem_size`` and
    ``rte_mempool_xmem_usage``.
  * ``rte_mem_phy2mch`` was used in Xen dom0 to obtain the physical address;
    remove this API as Xen dom0 support was removed.

* **Added IOVA aliases related to physical address handling.**

  Some data types, structure members and functions related to physical address
  handling are deprecated and have new aliases with IOVA wording. For example:

  * ``phys_addr_t`` can be often replaced by ``rte_iova_t`` of same size.
  * ``RTE_BAD_PHYS_ADDR`` is often replaced by ``RTE_BAD_IOVA`` of same value.
  * ``rte_memseg.phys_addr`` is aliased with ``rte_memseg.iova_addr``.
  * ``rte_mem_virt2phy()`` can often be replaced by ``rte_mem_virt2iova``.
  * ``rte_malloc_virt2phy`` is aliased with ``rte_malloc_virt2iova``.
  * ``rte_memzone.phys_addr`` is aliased with ``rte_memzone.iova``.
  * ``rte_mempool_objhdr.physaddr`` is aliased with
    ``rte_mempool_objhdr.iova``.
  * ``rte_mempool_memhdr.phys_addr`` is aliased with
    ``rte_mempool_memhdr.iova``.
  * ``rte_mempool_virt2phy()`` can be replaced by ``rte_mempool_virt2iova()``.
  * ``rte_mempool_populate_phys*()`` are aliased with
    ``rte_mempool_populate_iova*()``
  * ``rte_mbuf.buf_physaddr`` is aliased with ``rte_mbuf.buf_iova``.
  * ``rte_mbuf_data_dma_addr*()`` are aliased with ``rte_mbuf_data_iova*()``.
  * ``rte_pktmbuf_mtophys*`` are aliased with ``rte_pktmbuf_iova*()``.

* **PCI bus API moved outside of the EAL**

  The PCI bus previously implemented within the EAL has been moved.
  A first part has been added as an RTE library providing PCI helpers to
  parse device locations or other such utilities.
  A second part consisting of the actual bus driver has been moved to its
  proper subdirectory, without changing its functionalities.

  As such, several PCI-related functions are not exposed by the EAL anymore:

  * ``rte_pci_detach``
  * ``rte_pci_dump``
  * ``rte_pci_ioport_map``
  * ``rte_pci_ioport_read``
  * ``rte_pci_ioport_unmap``
  * ``rte_pci_ioport_write``
  * ``rte_pci_map_device``
  * ``rte_pci_probe``
  * ``rte_pci_probe_one``
  * ``rte_pci_read_config``
  * ``rte_pci_register``
  * ``rte_pci_scan``
  * ``rte_pci_unmap_device``
  * ``rte_pci_unregister``
  * ``rte_pci_write_config``

  These functions are made available either as part of ``librte_pci`` or
  ``librte_bus_pci``.

* **Moved vdev bus APIs outside of the EAL**

  Moved the following APIs from ``librte_eal`` to ``librte_bus_vdev``:

  * ``rte_vdev_init``
  * ``rte_vdev_register``
  * ``rte_vdev_uninit``
  * ``rte_vdev_unregister``

* **Add return value to stats_get dev op API**

  The ``stats_get`` dev op API return value has been changed to be int.
  In this way PMDs can return an error value in case of failure at stats
  getting process time.

* **Modified the rte_cryptodev_allocate_driver function.**

  Modified the ``rte_cryptodev_allocate_driver()`` function in the cryptodev
  library. An extra parameter ``struct cryptodev_driver *crypto_drv`` has been
  added.

* **Removed virtual device bus specific functions from librte_cryptodev.**

  The functions ``rte_cryptodev_vdev_parse_init_params()`` and
  ``rte_cryptodev_vdev_pmd_init()`` have been removed from librte_cryptodev
  and have been replaced by non bus specific functions
  ``rte_cryptodev_pmd_parse_input_args()`` and ``rte_cryptodev_pmd_create()``.

  The ``rte_cryptodev_create_vdev()`` function was removed to avoid the
  dependency on vdev in librte_cryptodev; instead, users can call
  ``rte_vdev_init()`` directly.

* **Removed PCI device bus specific functions from librte_cryptodev.**

  The functions ``rte_cryptodev_pci_generic_probe()`` and
  ``rte_cryptodev_pci_generic_remove()`` have been removed from librte_cryptodev
  and have been replaced by non bus specific functions
  ``rte_cryptodev_pmd_create()`` and ``rte_cryptodev_pmd_destroy()``.

* **Removed deprecated functions to manage log level or type.**

  The functions ``rte_set_log_level()``, ``rte_get_log_level()``,
  ``rte_set_log_type()`` and ``rte_get_log_type()`` have been removed.

  They are respectively replaced by ``rte_log_set_global_level()``,
  ``rte_log_get_global_level()``, ``rte_log_set_level()`` and
  ``rte_log_get_level()``.

* **Removed mbuf flags PKT_RX_VLAN_PKT and PKT_RX_QINQ_PKT.**

  The ``mbuf`` flags ``PKT_RX_VLAN_PKT`` and ``PKT_RX_QINQ_PKT`` have
  been removed since their behavior was not properly described.

* **Added mbuf flags PKT_RX_VLAN and PKT_RX_QINQ.**

  Two ``mbuf`` flags have been added to indicate that the VLAN
  identifier has been saved in the ``mbuf`` structure. For instance:

  - If VLAN is not stripped and TCI is saved: ``PKT_RX_VLAN``
  - If VLAN is stripped and TCI is saved: ``PKT_RX_VLAN | PKT_RX_VLAN_STRIPPED``

* **Modified the vlan_offload_set_t function prototype in the ethdev library.**

  Modified the ``vlan_offload_set_t`` function prototype in the ethdev
  library.  The return value has been changed from ``void`` to ``int`` so the
  caller can determine whether the backing device supports the operation or if
  the operation was successfully performed.


ABI Changes
-----------

.. This section should contain ABI changes. Sample format:

   * Add a short 1-2 sentence description of the ABI change that was announced
     in the previous releases and made in this release. Use fixed width quotes
     for ``rte_function_names`` or ``rte_struct_names``. Use the past tense.

   This section is a comment. do not overwrite or remove it.
   Also, make sure to start the actual text at the margin.
   =========================================================

* **Extended port_id range.**

  The size of the field ``port_id`` in the ``rte_eth_dev_data`` structure
  has changed, as described in the `New Features` section above.

* **New parameter added to rte_eth_dev.**

  A new parameter ``security_ctx`` has been added to ``rte_eth_dev`` to
  support security operations like IPSec inline.

* **New parameter added to rte_cryptodev.**

  A new parameter ``security_ctx`` has been added to ``rte_cryptodev`` to
  support security operations like lookaside crypto.


Removed Items
-------------

.. This section should contain removed items in this release. Sample format:

   * Add a short 1-2 sentence description of the removed item in the past
     tense.

   This section is a comment. do not overwrite or remove it.
   Also, make sure to start the actual text at the margin.
   =========================================================

* Xen dom0 in EAL has been removed, as well as the xenvirt PMD and vhost_xen.

* The crypto performance unit tests have been removed,
  replaced by the ``dpdk-test-crypto-perf`` application.


Shared Library Versions
-----------------------

.. Update any library version updated in this release and prepend with a ``+``
   sign, like this:

     librte_acl.so.2
   + librte_cfgfile.so.2
     librte_cmdline.so.2

   This section is a comment. do not overwrite or remove it.
   =========================================================


The libraries prepended with a plus sign were incremented in this version.

.. code-block:: diff

     librte_acl.so.2
   + librte_bitratestats.so.2
   + librte_bus_dpaa.so.1
   + librte_bus_fslmc.so.1
   + librte_bus_pci.so.1
   + librte_bus_vdev.so.1
     librte_cfgfile.so.2
     librte_cmdline.so.2
   + librte_cryptodev.so.4
     librte_distributor.so.1
   + librte_eal.so.6
   + librte_ethdev.so.8
   + librte_eventdev.so.3
   + librte_flow_classify.so.1
     librte_gro.so.1
   + librte_gso.so.1
     librte_hash.so.2
     librte_ip_frag.so.1
     librte_jobstats.so.1
     librte_kni.so.2
     librte_kvargs.so.1
     librte_latencystats.so.1
     librte_lpm.so.2
     librte_mbuf.so.3
   + librte_mempool.so.3
     librte_meter.so.1
     librte_metrics.so.1
     librte_net.so.1
   + librte_pci.so.1
   + librte_pdump.so.2
     librte_pipeline.so.3
   + librte_pmd_bnxt.so.2
   + librte_pmd_bond.so.2
   + librte_pmd_i40e.so.2
   + librte_pmd_ixgbe.so.2
     librte_pmd_ring.so.2
   + librte_pmd_softnic.so.1
   + librte_pmd_vhost.so.2
     librte_port.so.3
     librte_power.so.1
     librte_reorder.so.1
     librte_ring.so.1
     librte_sched.so.1
   + librte_security.so.1
   + librte_table.so.3
     librte_timer.so.1
     librte_vhost.so.3


Tested Platforms
----------------

.. This section should contain a list of platforms that were tested with this
   release.

   The format is:

   * <vendor> platform with <vendor> <type of devices> combinations

     * List of CPU
     * List of OS
     * List of devices
     * Other relevant details...

   This section is a comment. do not overwrite or remove it.
   Also, make sure to start the actual text at the margin.
   =========================================================

* Intel(R) platforms with Intel(R) NICs combinations

   * CPU

     * Intel(R) Atom(TM) CPU C2758 @ 2.40GHz
     * Intel(R) Xeon(R) CPU D-1540 @ 2.00GHz
     * Intel(R) Xeon(R) CPU D-1541 @ 2.10GHz
     * Intel(R) Xeon(R) CPU E5-4667 v3 @ 2.00GHz
     * Intel(R) Xeon(R) CPU E5-2680 v2 @ 2.80GHz
     * Intel(R) Xeon(R) CPU E5-2699 v3 @ 2.30GHz
     * Intel(R) Xeon(R) CPU E5-2695 v4 @ 2.10GHz
     * Intel(R) Xeon(R) CPU E5-2658 v2 @ 2.40GHz
     * Intel(R) Xeon(R) CPU E5-2658 v3 @ 2.20GHz

   * OS:

     * CentOS 7.2
     * Fedora 25
     * Fedora 26
     * FreeBSD 11
     * Red Hat Enterprise Linux Server release 7.3
     * SUSE Enterprise Linux 12
     * Wind River Linux 8
     * Ubuntu 16.04
     * Ubuntu 16.10

   * NICs:

     * Intel(R) 82599ES 10 Gigabit Ethernet Controller

       * Firmware version: 0x61bf0001
       * Device id (pf/vf): 8086:10fb / 8086:10ed
       * Driver version: 5.2.3 (ixgbe)

     * Intel(R) Corporation Ethernet Connection X552/X557-AT 10GBASE-T

       * Firmware version: 0x800003e7
       * Device id (pf/vf): 8086:15ad / 8086:15a8
       * Driver version: 4.4.6 (ixgbe)

     * Intel(R) Ethernet Converged Network Adapter X710-DA4 (4x10G)

       * Firmware version: 6.01 0x80003205
       * Device id (pf/vf): 8086:1572 / 8086:154c
       * Driver version: 2.1.26 (i40e)

     * Intel(R) Ethernet Converged Network Adapter X710-DA2 (2x10G)

       * Firmware version: 6.01 0x80003204
       * Device id (pf/vf): 8086:1572 / 8086:154c
       * Driver version: 2.1.26 (i40e)

     * Intel(R) Ethernet Converged Network Adapter XXV710-DA2 (2x25G)

       * Firmware version: 6.01 0x80003221
       * Device id (pf/vf): 8086:158b
       * Driver version: 2.1.26 (i40e)

     * Intel(R) Ethernet Converged Network Adapter XL710-QDA2 (2X40G)

       * Firmware version: 6.01 0x8000321c
       * Device id (pf/vf): 8086:1583 / 8086:154c
       * Driver version: 2.1.26 (i40e)

     * Intel(R) Corporation I350 Gigabit Network Connection

       * Firmware version: 1.63, 0x80000dda
       * Device id (pf/vf): 8086:1521 / 8086:1520
       * Driver version: 5.3.0-k (igb)

* Intel(R) platforms with Mellanox(R) NICs combinations

   * Platform details:

     * Intel(R) Xeon(R) CPU E5-2697A v4 @ 2.60GHz
     * Intel(R) Xeon(R) CPU E5-2697 v3 @ 2.60GHz
     * Intel(R) Xeon(R) CPU E5-2680 v2 @ 2.80GHz
     * Intel(R) Xeon(R) CPU E5-2650 v4 @ 2.20GHz
     * Intel(R) Xeon(R) CPU E5-2640 @ 2.50GHz
     * Intel(R) Xeon(R) CPU E5-2620 v4 @ 2.10GHz

   * OS:

     * Red Hat Enterprise Linux Server release 7.3 (Maipo)
     * Red Hat Enterprise Linux Server release 7.2 (Maipo)
     * Ubuntu 16.10
     * Ubuntu 16.04
     * Ubuntu 14.04

   * MLNX_OFED: 4.2-1.0.0.0

   * NICs:

     * Mellanox(R) ConnectX(R)-3 Pro 40G MCX354A-FCC_Ax (2x40G)

       * Host interface: PCI Express 3.0 x8
       * Device ID: 15b3:1007
       * Firmware version: 2.42.5000

     * Mellanox(R) ConnectX(R)-4 10G MCX4111A-XCAT (1x10G)

       * Host interface: PCI Express 3.0 x8
       * Device ID: 15b3:1013
       * Firmware version: 12.21.1000

     * Mellanox(R) ConnectX(R)-4 10G MCX4121A-XCAT (2x10G)

       * Host interface: PCI Express 3.0 x8
       * Device ID: 15b3:1013
       * Firmware version: 12.21.1000

     * Mellanox(R) ConnectX(R)-4 25G MCX4111A-ACAT (1x25G)

       * Host interface: PCI Express 3.0 x8
       * Device ID: 15b3:1013
       * Firmware version: 12.21.1000

     * Mellanox(R) ConnectX(R)-4 25G MCX4121A-ACAT (2x25G)

       * Host interface: PCI Express 3.0 x8
       * Device ID: 15b3:1013
       * Firmware version: 12.21.1000

     * Mellanox(R) ConnectX(R)-4 40G MCX4131A-BCAT/MCX413A-BCAT (1x40G)

       * Host interface: PCI Express 3.0 x8
       * Device ID: 15b3:1013
       * Firmware version: 12.21.1000

     * Mellanox(R) ConnectX(R)-4 40G MCX415A-BCAT (1x40G)

       * Host interface: PCI Express 3.0 x16
       * Device ID: 15b3:1013
       * Firmware version: 12.21.1000

     * Mellanox(R) ConnectX(R)-4 50G MCX4131A-GCAT/MCX413A-GCAT (1x50G)

       * Host interface: PCI Express 3.0 x8
       * Device ID: 15b3:1013
       * Firmware version: 12.21.1000

     * Mellanox(R) ConnectX(R)-4 50G MCX414A-BCAT (2x50G)

       * Host interface: PCI Express 3.0 x8
       * Device ID: 15b3:1013
       * Firmware version: 12.21.1000

     * Mellanox(R) ConnectX(R)-4 50G MCX415A-GCAT/MCX416A-BCAT/MCX416A-GCAT
       (2x50G)

       * Host interface: PCI Express 3.0 x16
       * Device ID: 15b3:1013
       * Firmware version: 12.21.1000

     * Mellanox(R) ConnectX(R)-4 50G MCX415A-CCAT (1x100G)

       * Host interface: PCI Express 3.0 x16
       * Device ID: 15b3:1013
       * Firmware version: 12.21.1000

     * Mellanox(R) ConnectX(R)-4 100G MCX416A-CCAT (2x100G)

       * Host interface: PCI Express 3.0 x16
       * Device ID: 15b3:1013
       * Firmware version: 12.21.1000

     * Mellanox(R) ConnectX(R)-4 Lx 10G MCX4121A-XCAT (2x10G)

       * Host interface: PCI Express 3.0 x8
       * Device ID: 15b3:1015
       * Firmware version: 14.21.1000

     * Mellanox(R) ConnectX(R)-4 Lx 25G MCX4121A-ACAT (2x25G)

       * Host interface: PCI Express 3.0 x8
       * Device ID: 15b3:1015
       * Firmware version: 14.21.1000

     * Mellanox(R) ConnectX(R)-5 100G MCX556A-ECAT (2x100G)

       * Host interface: PCI Express 3.0 x16
       * Device ID: 15b3:1017
       * Firmware version: 16.21.1000

     * Mellanox(R) ConnectX-5 Ex EN 100G MCX516A-CDAT (2x100G)

       * Host interface: PCI Express 4.0 x16
       * Device ID: 15b3:1019
       * Firmware version: 16.21.1000

* ARM platforms with Mellanox(R) NICs combinations

   * Platform details:

     * Qualcomm ARM 1.1 2500MHz

   * OS:

     * Ubuntu 16.04

   * MLNX_OFED: 4.2-1.0.0.0

   * NICs:

     * Mellanox(R) ConnectX(R)-4 Lx 25G MCX4121A-ACAT (2x25G)

       * Host interface: PCI Express 3.0 x8
       * Device ID: 15b3:1015
       * Firmware version: 14.21.1000

     * Mellanox(R) ConnectX(R)-5 100G MCX556A-ECAT (2x100G)

       * Host interface: PCI Express 3.0 x16
       * Device ID: 15b3:1017
       * Firmware version: 16.21.1000

Fixes in 17.11 LTS Release
--------------------------

17.11.1
~~~~~~~

* app/procinfo: add compilation option in config
* app/testpmd: fix crash of txonly with multiple segments
* app/testpmd: fix flow director filter
* app/testpmd: fix flowgen forwarding offload flags
* app/testpmd: fix invalid Rx queue number setting
* app/testpmd: fix invalid Tx queue number setting
* app/testpmd: fix port configuration print
* app/testpmd: fix port id allocation
* app/testpmd: fix port index in RSS forward config
* app/testpmd: fix port topology in RSS forward config
* app/testpmd: fix port validation
* app/testpmd: remove xenvirt again
* bus/dpaa: fix ARM big endian build
* bus/dpaa: fix build when assert enabled
* bus/dpaa: fix default IOVA mode
* bus/fslmc: fix build with latest glibc
* bus/fslmc: fix the cplusplus macro closure
* bus/pci: fix interrupt handler type
* bus/pci: forbid IOVA mode if IOMMU address width too small
* bus/vdev: continue probing after a device failure
* cmdline: avoid garbage in unused fields of parsed result
* cmdline: fix dynamic tokens parsing
* cryptodev: add missing CPU flag string
* cryptodev: fix function prototype
* cryptodev: fix session pointer cast
* crypto/dpaa2_sec: fix enum conversion for GCM
* crypto: fix pedantic compilation
* crypto/qat: fix allocation check and leak
* crypto/qat: fix null auth algo overwrite
* crypto/qat: fix out-of-bounds access
* crypto/qat: fix parameter type
* crypto/scheduler: fix strncpy
* doc: fix format in OpenSSL installation guide
* doc: fix lists of supported crypto algorithms
* drivers: change the deprecated memseg physaddr to IOVA
* eal/arm64: remove the braces in memory barrier macros
* eal/ppc64: revert arch-specific TSC freq query
* eal/ppc: remove the braces in memory barrier macros
* ethdev: fix link autonegotiation value
* ethdev: fix missing imissed counter in xstats
* ethdev: fix port data reset timing
* ethdev: fix port id allocation
* eventdev: fix doxygen comments
* eventdev: set error code in port link/unlink functions
* event/octeontx: fix Rx adapter port id mapping
* event/sw: fix debug logging config option
* event/sw: fix queue memory leak and multi-link bug
* examples/bond: check mbuf allocation
* examples/bond: fix vdev name
* examples/ip_pipeline: fix timer period unit
* examples/ipsec-secgw: fix corner case for SPI value
* examples/ipsec-secgw: fix missing ingress flow attribute
* examples/ipsec-secgw: fix SPI byte order in flow item
* examples/ipsec-secgw: fix usage of incorrect port
* examples/l3fwd-power: fix frequency detection
* examples/l3fwd-power: fix Rx without interrupt
* examples/vhost: fix sending ARP packet to self
* examples/vhost: fix startup check
* flow_classify: fix ISO C in exported header
* igb_uio: allow multi-process access
* keepalive: fix state alignment
* kni: fix build dependency
* kni: fix build with kernel 4.15
* lib: fix missing includes in exported headers
* log: fix memory leak in regexp level set
* lpm: fix ARM big endian build
* malloc: fix end for bounded elements
* malloc: protect stats with lock
* mbuf: fix NULL freeing when debug enabled
* mbuf: fix performance of freeing with non atomic refcnt
* member: fix ISO C in exported header
* member: fix memory leak on error
* mempool: fix first memory area notification
* mempool: fix physical contiguous check
* mempool/octeontx: fix improper memory barrier
* mempool/octeontx: fix memory area registration
* mempool/octeontx: fix natural alignment being optimized out
* memzone: fix leak on allocation error
* mk: fix external build
* mk: remove TILE-Gx machine type
* mk: support renamed Makefile in external project
* net/bnxt: fix check for ether type
* net/bnxt: fix double increment of idx during Tx ring alloc
* net/bnxt: fix duplicate filter pattern creation error
* net/bnxt: fix duplicate pattern for 5tuple filter
* net/bnxt: fix group info usage
* net/bnxt: fix link speed setting with autoneg off
* net/bnxt: fix number of pools for RSS
* net/bnxt: fix return code in MAC address set
* net/bnxt: fix Rx checksum flags
* net/bnxt: fix size of Tx ring in HW
* net/bnxt: free the aggregation ring
* net/bnxt: parse checksum offload flags
* net/bonding: check error of MAC address setting
* net/bonding: fix activated slave in 8023ad mode
* net/bonding: fix bonding in 8023ad mode
* net/bonding: fix setting slave MAC addresses
* net/dpaa: fix FW version code
* net/dpaa: fix potential memory leak
* net/dpaa: fix the mbuf packet type if zero
* net/dpaa: fix uninitialized and unused variables
* net/e1000: fix null pointer check
* net/e1000: fix VF Rx interrupt enabling
* net/ena: do not set Tx L4 offloads in Rx path
* net/enic: fix crash due to static max number of queues
* net/enic: fix L4 Rx ptype comparison
* net/failsafe: fix invalid free
* net/failsafe: fix Rx safe check compiler hint
* net: fix ESP header byte ordering definition
* net/fm10k: fix logical port delete
* net/i40e: add debug logs when writing global registers
* net/i40e: add FDIR NVGRE parameter check
* net/i40e: check multi-driver option parsing
* net/i40e: exclude LLDP packet count
* net/i40e: fix ARM big endian build
* net/i40e: fix FDIR input set conflict
* net/i40e: fix FDIR rule confiliction issue
* net/i40e: fix flag for MAC address write
* net/i40e: fix flow director Rx resource defect
* net/i40e: fix interrupt conflict with multi-driver
* net/i40e: fix ISO C in exported header
* net/i40e: fix memory leak
* net/i40e: fix multiple DDP packages conflict
* net/i40e: fix multiple driver support
* net/i40e: fix packet type for X722
* net/i40e: fix port segmentation fault when restart
* net/i40e: fix Rx interrupt
* net/i40e: fix setting MAC address of VF
* net/i40e: fix setting of MAC address on i40evf
* net/i40e: fix VF reset stats crash
* net/i40e: fix VF Rx interrupt enabling
* net/i40e: fix VLAN offload setting
* net/i40e: fix VLAN offload setting issue
* net/i40e: fix VSI MAC filter on primary address change
* net/i40e: warn when writing global registers
* net/igb: fix Tx queue number assignment
* net/ixgbe: fix ARM big endian build
* net/ixgbe: fix max queue number for VF
* net/ixgbe: fix parsing FDIR NVGRE issue
* net/ixgbe: fix reset error handling
* net/ixgbe: fix the failure of number of Tx queue check
* net/ixgbe: fix tunnel filter fail problem
* net/ixgbe: fix VF Rx interrupt enabling
* net/ixgbe: fix wrong PBA setting
* net/mlx4: fix drop flow resources leak
* net/mlx4: fix Rx offload non-fragmented indication
* net/mlx4: fix Tx packet drop application report
* net/mlx4: fix unnecessary include
* net/mlx4: revert workaround for broken Verbs
* net/mlx5: cleanup allocation of ethtool stats
* net/mlx5: fix calculation of flow ID flag
* net/mlx5: fix deadlock of link status alarm
* net/mlx5: fix flow item validation
* net/mlx5: fix flow priority on queue action
* net/mlx5: fix flow RSS configuration
* net/mlx5: fix handling link status event
* net/mlx5: fix HW checksum offload for outer IP
* net/mlx5: fix link state on device start
* net/mlx5: fix memory region boundary checks
* net/mlx5: fix memory region cache last index
* net/mlx5: fix memory region cache lookup
* net/mlx5: fix memory region lookup
* net/mlx5: fix Memory Region registration
* net/mlx5: fix missing attribute size for drop action
* net/mlx5: fix missing RSS capability
* net/mlx5: fix overflow of Memory Region cache
* net/mlx5: fix overwriting bit-fields in SW Rx queue
* net/mlx5: fix port stop by verify flows are still present
* net/mlx5: fix return value of start operation
* net/mlx5: fix RSS key configuration
* net/mlx5: fix secondary process verification
* net/mlx5: fix Tx checksum offloads
* net/mlx5: fix UAR remapping on non configured queues
* net/mlx5: fix un-supported RSS hash fields use
* net/mlx5: fix VLAN configuration after port stop
* net/mlx5: remove parser/flow drop queue
* net/mlx5: use PCI address as port name
* net/mrvl: fix HIF objects allocation
* net/mrvl: fix multiple probe
* net/mrvl: fix oversize bpool handling
* net/mrvl: fix shadow queue tail and size calculations
* net/mrvl: keep shadow Txqs inside PMD Txq
* net/nfp: fix CRC strip check behaviour
* net/nfp: fix jumbo settings
* net/nfp: fix MTU settings
* net/octeontx: add channel to port id mapping
* net/pcap: fix the NUMA id display in logs
* net/qede/base: fix VF LRO tunnel configuration
* net/qede: check tunnel L3 header
* net/qede: fix clearing of queue stats
* net/qede: fix few log messages
* net/qede: fix MTU set and max Rx length
* net/qede: fix to enable LRO over tunnels
* net/qede: fix to reject config with no Rx queue
* net/qede: fix tunnel header size in Tx BD configuration
* net/qede: replace config option with run-time arg
* net/sfc: do not hold management event queue lock while MCDI
* net/sfc: fix DMA memory leak after kvarg processing failure
* net/sfc: fix flow RSS check in error handling
* net/sfc: fix incorrect bitwise ORing of L3/L4 packet types
* net/sfc: fix initialization of flow structure
* net/sfc: fix label name to be consistent
* net/sfc: fix main MAC address handling
* net/sfc: fix multicast address list copy memory leak
* net/sfc: stop periodic DMA if MAC stats upload fails
* net/szedata2: fix check of mmap return value
* net/tap: fix cleanup on allocation failure
* net/tap: remove unused kernel version definitions
* net/thunderx: fix multi segment Tx function return
* net/virtio: fix incorrect cast
* net/virtio: fix memory leak when reinitializing device
* net/virtio: fix queue flushing with vector Rx enabled
* net/virtio: fix Rx and Tx handler selection for ARM32
* net/virtio: fix typo in LRO support
* net/virtio: fix vector Rx flushing
* net/virtio-user: fix crash as features change
* pdump: fix error check when creating/canceling thread
* pmdinfogen: fix cross compilation for ARM big endian
* security: fix device operation type
* security: fix enum start value
* security: fix pedantic compilation
* service: fix lcore role after delete
* service: fix number mapped cores count
* service: fix possible mem leak on initialize
* service: fix service core launch
* test/bitmap: fix memory leak
* test/crypto: fix missing include
* test/eventdev: use CPU event type
* test/memzone: fix freeing test
* test/memzone: fix NULL freeing
* test/memzone: fix wrong test
* test: register test as failed if setup failed
* test/reorder: fix memory leak
* test/ring: fix memory leak
* test/ring_perf: fix memory leak
* test/table: fix memory leak
* test/table: fix uninitialized parameter
* test/timer_perf: fix memory leak
* timer: fix reset on service cores
* usertools/devbind: fix kernel module reporting
* vfio: fix enabled check on error
* vhost: fix crash
* vhost: fix dequeue zero copy with virtio1
* vhost: fix error code check when creating thread
* vhost: fix IOTLB pool out-of-memory handling
* vhost: fix mbuf free
* vhost: protect active rings from async ring changes
* vhost: remove pending IOTLB entry if miss request failed

17.11.2
~~~~~~~

* examples/vhost: move to safe GPA translation API
* examples/vhost_scsi: move to safe GPA translation API
* vhost: add support for non-contiguous indirect descs tables (fixes CVE-2018-1059)
* vhost: check all range is mapped when translating GPAs (fixes CVE-2018-1059)
* vhost: deprecate unsafe GPA translation API (fixes CVE-2018-1059)
* vhost: ensure all range is mapped when translating QVAs (fixes CVE-2018-1059)
* vhost: fix indirect descriptors table translation size (fixes CVE-2018-1059)
* vhost: handle virtually non-contiguous buffers in Rx (fixes CVE-2018-1059)
* vhost: handle virtually non-contiguous buffers in Rx-mrg (fixes CVE-2018-1059)
* vhost: handle virtually non-contiguous buffers in Tx (fixes CVE-2018-1059)
* vhost: introduce safe API for GPA translation (fixes CVE-2018-1059)

17.11.3
~~~~~~~

* app/crypto-perf: check minimum lcore number
* app/crypto-perf: fix excess crypto device error
* app/crypto-perf: fix IOVA translation
* app/crypto-perf: fix parameters copy
* app/crypto-perf: use strcpy for allocated string
* app/procinfo: fix strncpy usage in args parsing
* app/testpmd: fix burst stats reporting
* app/testpmd: fix command token
* app/testpmd: fix empty list of RSS queues for flow
* app/testpmd: fix forward ports Rx flush
* app/testpmd: fix forward ports update
* app/testpmd: fix removed device link status asking
* app/testpmd: fix slave port detection
* app/testpmd: fix synchronic port hotplug
* app/testpmd: fix valid ports prints
* bus/dpaa: fix resource leak
* bus/fslmc: fix find device start condition
* bus/pci: fix find device implementation
* bus/vdev: fix finding device by name
* cryptodev: fix supported size check
* crypto/dpaa2_sec: fix HMAC supported digest sizes
* crypto/scheduler: fix 64-bit mask of workers cores
* crypto/scheduler: fix memory leak
* crypto/scheduler: fix multicore rings re-use
* crypto/scheduler: fix possible duplicated ring names
* crypto/scheduler: set null pointer after freeing
* crypto/zuc: batch ops with same transform
* crypto/zuc: do not set default op status
* doc: add timestamp offload to mlx5 features
* doc: fix NFP NIC guide grammar
* drivers/net: fix link autoneg value for virtual PMDs
* eal/ppc: remove braces in SMP memory barrier macro
* ethdev: fix port accessing after release
* ethdev: fix queue start
* event/dpaa2: remove link from info structure
* examples/exception_path: limit core count to 64
* examples/l2fwd-crypto: fix the default aead assignments
* examples/performance-thread: fix return type of threads
* examples/quota_watermark: fix return type of threads
* hash: fix missing spinlock unlock in add key
* ip_frag: fix double free of chained mbufs
* kni: fix build on CentOS 7.4
* kni: fix build on RHEL 7.5
* mbuf: fix Tx checksum offload API doc
* mbuf: improve tunnel Tx offloads API doc
* mem: do not use physical addresses in IOVA as VA mode
* mempool: fix leak when no objects are populated
* mempool: fix virtual address population
* mk: fix make defconfig on FreeBSD
* net: add IPv6 header fields macros
* net/bnx2x: do not cast function pointers as a policy
* net/bnx2x: fix for PCI FLR after ungraceful exit
* net/bnx2x: fix KR2 device check
* net/bnx2x: fix memzone name overrun
* net/bnxt: avoid invalid vnic id in set L2 Rx mask
* net/bnxt: fix endianness of flag
* net/bnxt: fix license header
* net/bnxt: fix LRO disable
* net/bnxt: fix Rx checksum flags
* net/bnxt: fix Rx checksum flags for tunnel frames
* net/bnxt: fix Rx drop setting
* net/bnxt: fix Rx mbuf and agg ring leak in dev stop
* net/bnxt: fix usage of vnic id
* net/bnxt: free memory allocated for VF filters
* net/bnxt: set padding flags in Rx descriptor
* net/bonding: clear started state if start fails
* net/bonding: export mode 4 slave info routine
* net/bonding: fix primary slave port id storage type
* net/bonding: fix setting VLAN ID on slave ports
* net/bonding: fix slave activation simultaneously
* net/bonding: free mempool used in mode 6
* net/dpaa2: fix xstats
* net/dpaa: fix oob access
* net/enic: allocate stats DMA buffer upfront during probe
* net/enic: fix crash on MTU update with non-setup queues
* net/failsafe: fix duplicate event registration
* net/failsafe: fix probe cleanup
* net/failsafe: fix removed sub-device cleanup
* net/i40e: fix DDP profile DEL operation
* net/i40e: fix failing to disable FDIR Tx queue
* net/i40e: fix intr callback unregister by adding retry
* net/i40e: fix link status update
* net/i40e: fix link update no wait
* net/i40e: fix shifts of signed values
* net/ixgbe: enable vector PMD for icc 32 bits
* net/ixgbe: fix busy wait during checking link status
* net/ixgbe: fix DCB configuration
* net/ixgbe: fix intr callback unregister by adding retry
* net/ixgbe: fix too many interrupts
* net/liquidio: fix link state fetching during start
* net/mlx4: avoid constant recreations in function
* net/mlx4: fix a typo in header file
* net/mlx4: fix broadcast Rx
* net/mlx4: fix removal detection of stopped port
* net/mlx4: fix RSS resource leak in case of error
* net/mlx4: fix Rx resource leak in case of error
* net/mlx4: fix single port configuration
* net/mlx4: fix UDP flow rule limitation enforcement
* net/mlx4: store RSS hash result in mbufs
* net/mlx5: add data-plane debug message macro
* net/mlx5: add missing function documentation
* net/mlx5: add packet type index for TCP ack
* net/mlx5: change device reference for secondary process
* net/mlx5: change non failing function return values
* net/mlx5: change pkt burst select function prototype
* net/mlx5: change tunnel flow priority
* net/mlx5: enforce RSS key length limitation
* net/mlx5: fix allocation when no memory on device NUMA node
* net/mlx5: fix build with clang on ARM
* net/mlx5: fix calculation of Tx TSO inline room size
* net/mlx5: fix close after start failure
* net/mlx5: fix count in xstats
* net/mlx5: fix CRC strip capability query
* net/mlx5: fix disabling Tx packet inlining
* net/mlx5: fix double free on error handling
* net/mlx5: fix ethtool link setting call order
* net/mlx5: fix existing file removal
* net/mlx5: fix flow creation with a single target queue
* net/mlx5: fix flow director conversion
* net/mlx5: fix flow director drop rule deletion crash
* net/mlx5: fix flow director mask
* net/mlx5: fix flow director rule deletion crash
* net/mlx5: fix flow validation
* net/mlx5: fix icc build
* net/mlx5: fix invalid flow item check
* net/mlx5: fix IPv6 header fields
* net/mlx5: fix link status behavior
* net/mlx5: fix link status initialization
* net/mlx5: fix link status to use wait to complete
* net/mlx5: fix probe return value polarity
* net/mlx5: fix reception of multiple MAC addresses
* net/mlx5: fix resource leak in case of error
* net/mlx5: fix RSS flow action bounds check
* net/mlx5: fix RSS key length query
* net/mlx5: fix secondary process mempool registration
* net/mlx5: fix socket connection return value
* net/mlx5: fix sriov flag
* net/mlx5: fix synchronization on polling Rx completions
* net/mlx5: improve flow error explanation
* net/mlx5: map UAR address around huge pages
* net/mlx5: mark parameters with unused attribute
* net/mlx5: name parameters in function prototypes
* net/mlx5: normalize function prototypes
* net/mlx5: prefix all functions with mlx5
* net/mlx5: refuse empty VLAN flow specification
* net/mlx5: remove 32-bit support
* net/mlx5: remove assert un-accessible from secondary process
* net/mlx5: remove control path locks
* net/mlx5: remove excessive data prefetch
* net/mlx5: remove get priv internal function
* net/mlx5: remove kernel version check
* net/mlx5: remove useless empty lines
* net/mlx5: setup RSS regardless of queue count
* net/mlx5: split L3/L4 in flow director
* net/mlx5: standardize on negative errno values
* net/mlx5: use dynamic logging
* net/mlx5: use port id in PMD log
* net/mlx5: warn for unsuccessful memory registration
* net/mlx: control netdevices through ioctl only
* net/mrvl: fix crash when port is closed without starting
* net/mrvl: fix Rx descriptors number
* net/nfp: fix assigning port id in mbuf
* net/nfp: fix barrier location
* net/nfp: fix link speed capabilities
* net/nfp: fix mbufs releasing when stop or close
* net/octeontx: fix null pointer dereference
* net/octeontx: fix uninitialized speed variable
* net/octeontx: fix uninitialized variable in port open
* net/qede/base: fix to support OVLAN mode
* net/qede: fix alloc from socket 0
* net/qede: fix device stop to remove primary MAC
* net/qede: fix L2-handles used for RSS hash update
* net/qede: fix memory alloc for multiple port reconfig
* net/qede: fix missing loop index in Tx SG mode
* net/qede: fix multicast filtering
* net/qede: fix to prevent overwriting packet type
* net/qede: fix unicast filter routine return code
* net/qede: fix VF port creation sequence
* net/sfc: add missing defines for SAL annotation
* net/sfc: add missing Rx fini on RSS setup fail path
* net/sfc/base: fix comparison always true warning
* net/sfc: fix mbuf data alignment calculation
* net/sfc: fix type of opaque pointer in perf profile handler
* net/sfc: ignore spec bits not covered by mask
* net/sfc: process RSS settings on Rx configure step
* net/szedata2: fix format string for PCI address
* net/szedata2: fix total stats
* net/tap: fix icc build
* net/vhost: fix crash when creating vdev dynamically
* net/vhost: fix invalid state
* net/vhost: initialise device as inactive
* net/vmxnet3: set the queue shared buffer at start
* nfp: allow for non-root user
* nfp: restore the unlink operation
* nfp: unlink the appropriate lock file
* pci: remove duplicated symbol from map file
* test/distributor: fix return type of thread function
* test: fix memory flags test for low NUMA nodes number
* test/mempool: fix autotest retry
* test/pipeline: fix return type of stub miss
* test/pipeline: fix type of table entry parameter
* test/reorder: fix freeing mbuf twice
* vfio: do not needlessly check for IOVA mode
* vhost: check cmsg not null
* vhost: fix compilation issue when vhost debug enabled
* vhost: fix dead lock on closing in server mode
* vhost: fix device cleanup at stop
* vhost: fix message payload union in setting ring address
* vhost: fix offset while mmaping log base address
* vhost: fix realloc failure
* vhost: fix ring index returned to master on stop

17.11.4
~~~~~~~

* app/crypto-perf: fix auth IV offset
* app/testpmd: fix buffer leak in TM command
* app/testpmd: fix DCB config
* app/testpmd: fix VLAN TCI mask set error for FDIR
* bitrate: add sanity check on parameters
* bus/dpaa: fix buffer offset setting in FMAN
* bus/dpaa: fix build
* bus/dpaa: fix phandle support for Linux 4.16
* bus/pci: use IOVAs check when setting IOVA mode
* crypto/qat: fix checks for 3GPP algo bit params
* doc: fix bonding command in testpmd
* doc: update qede management firmware guide
* eal: fix bitmap documentation
* eal: fix return codes on thread naming failure
* eal/linux: fix invalid syntax in interrupts
* eal/linux: fix uninitialized value
* ethdev: fix a doxygen comment for port allocation
* ethdev: fix queue statistics mapping documentation
* eventdev: add event buffer flush in Rx adapter
* eventdev: fix internal port logic in Rx adapter
* eventdev: fix missing update to Rx adaper WRR position
* eventdev: fix port in Rx adapter internal function
* eventdev: fix Rx SW adapter stop
* event: fix ring init failure handling
* event/octeontx: remove unnecessary port start and stop
* examples/exception_path: fix out-of-bounds read
* examples: fix strncpy error for GCC8
* examples/flow_filtering: add flow director config for i40e
* examples/ipsec-secgw: fix bypass rule processing
* examples/ipsec-secgw: fix IPv4 checksum at Tx
* examples/l2fwd-crypto: check return value on IV size check
* examples/l2fwd-crypto: fix digest with AEAD algo
* examples/l2fwd-crypto: skip device not supporting operation
* examples/l3fwd: remove useless include
* hash: fix a multi-writer race condition
* hash: fix doxygen of return values
* hash: fix key slot size accuracy
* hash: fix multiwriter lock memory allocation
* kni: fix build on RHEL 7.5
* kni: fix build with gcc 8.1
* kni: fix crash with null name
* maintainers: claim maintainership for ARM v7 and v8
* maintainers: update for Mellanox PMDs
* mem: add function for checking memsegs IOVAs addresses
* mem: fix max DMA maskbit size
* mem: use address hint for mapping hugepages
* metrics: add check for invalid key
* metrics: disallow null as metric name
* metrics: do not fail silently when uninitialised
* mk: fix cross build
* mk: fix permissions when using make install
* mk: remove unnecessary test rules
* mk: update targets for classified tests
* net/bnx2x: fix FW command timeout during stop
* net/bnx2x: fix poll link status
* net/bnx2x: fix to set device link status
* net/bnxt: add missing ids in xstats
* net/bnxt: check access denied for HWRM commands
* net/bnxt: check for invalid vNIC id
* net/bnxt: fix filter freeing
* net/bnxt: fix HW Tx checksum offload check
* net/bnxt: fix lock release on NVM write failure
* net/bnxt: fix memory leaks in NVM commands
* net/bnxt: fix RETA size
* net/bnxt: fix Rx ring count limitation
* net/bnxt: fix set MTU
* net/bnxt: fix to move a flow to a different queue
* net/bnxt: use correct flags during VLAN configuration
* net/bonding: always update bonding link status
* net/bonding: do not clear active slave count
* net/bonding: fix MAC address reset
* net/bonding: fix race condition
* net/cxgbe: fix init failure due to new flash parts
* net/cxgbe: fix Rx channel map and queue type
* net/dpaa2: remove loop for unused pool entries
* net/ena: change memory type
* net/ena: check pointer before memset
* net/ena: fix GENMASK_ULL macro
* net/ena: fix SIGFPE with 0 Rx queue
* net/ena: set link speed as none
* net/enic: add devarg to specify ingress VLAN rewrite mode
* net/enic: do not overwrite admin Tx queue limit
* net/i40e: fix check of flow director programming status
* net/i40e: fix link speed
* net/i40e: fix packet type parsing with DDP
* net/i40e: fix setting TPID with AQ command
* net/i40e: fix shifts of 32-bit value
* net/i40e: revert fix of flow director check
* net/i40e: workaround performance degradation
* net/ixgbe: add support for VLAN in IP mode FDIR
* net/ixgbe: fix mask bits register set error for FDIR
* net/ixgbe: fix tunnel id format error for FDIR
* net/ixgbe: fix tunnel type set error for FDIR
* net/mlx4: check RSS queues number limitation
* net/mlx4: fix minor resource leak during init
* net/mlx5: add missing sanity checks for Tx completion queue
* net/mlx5: fix assert for Tx completion queue count
* net/mlx5: fix build with old kernels
* net/mlx5: fix compilation for rdma-core v19
* net/mlx5: fix crash in device probe
* net/mlx5: fix error number handling
* net/mlx5: fix flow search on FDIR deletion
* net/mlx5: fix queue rollback when starting device
* net/mlx5: fix return value when deleting fdir filter
* net/mlx5: fix Rx buffer replenishment threshold
* net/mlx5: fix secondary process resource leakage
* net/mlx5: fix TCI mask filter
* net/mlx5: preserve allmulticast flag for flow isolation mode
* net/mlx5: preserve promiscuous flag for flow isolation mode
* net/mvpp2: check pointer before using it
* net/nfp: check hugepages IOVAs based on DMA mask
* net/nfp: fix field initialization in Tx descriptor
* net/nfp: support IOVA VA mode
* net/octeontx: fix stop clearing Rx/Tx functions
* net/pcap: fix multiple queues
* net/qede/base: fix GRC attention callback
* net/qede/base: fix to clear HW indication
* net/qede: fix default extended VLAN offload config
* net/qede: fix for devargs
* net/qede: fix incorrect link status update
* net/qede: fix interrupt handler unregister
* net/qede: fix legacy interrupt mode
* net/qede: fix link change event notification
* net/qede: fix MAC address removal failure message
* net/qede: fix ntuple filter configuration
* net/qede: fix unicast MAC address handling in VF
* net/qede: fix VF MTU update
* net/qede: remove primary MAC removal
* net/sfc: cut non VLAN ID bits from TCI
* net/sfc: fix assert in set multicast address list
* net/sfc: handle unknown L3 packet class in EF10 event parser
* net/tap: fix zeroed flow mask configurations
* net/thunderx: avoid sq door bell write on zero packet
* net/thunderx: fix build with gcc optimization on
* ring: fix sign conversion warning
* security: fix crash on destroy null session
* test/crypto: fix device id when stopping port
* test: fix code on report
* test: fix EAL flags autotest on FreeBSD
* test: fix result printing
* test: fix uninitialized port configuration
* test/flow_classify: fix return types
* test/hash: fix multiwriter with non consecutive cores
* test/hash: fix potential memory leak
* test: improve filtering
* test: make autotest runner python 2/3 compliant
* test: print autotest categories
* vfio: fix PCI address comparison
* vhost: fix missing increment of log cache count
* vhost: flush IOTLB cache on new mem table handling
* vhost: improve dirty pages logging performance
* vhost: release locks on RARP packet failure
* vhost: retranslate vring addr when memory table changes

17.11.5
~~~~~~~

* acl: forbid rule with priority zero
* app/pdump: fix port id storage size
* app/procinfo: fix sprintf overrun
* app/test-crypto-perf: fix check for auth key
* app/test-crypto-perf: fix check for cipher IV
* app/test-crypto-perf: fix double allocation of memory
* app/testpmd: check Rx VLAN offload flag to print VLAN TCI
* app/testpmd: fix csum parse-tunnel command invocation
* app/testpmd: fix duplicate exit
* app/testpmd: fix L4 length for UDP checksum
* app/testpmd: fix memory leak for TM object
* app/testpmd: fix metering and policing commands
* app/testpmd: fix physical port socket initialization
* app/testpmd: fix printf format in event callback
* app/testpmd: fix RED byte stats
* app/testpmd: fix shaper profile parameters
* app/testpmd: fix vdev socket initialization
* app/testpmd: optimize mbuf pool allocation
* app/testpmd: reserve NUMA node per port and per ring
* build: enable ARM NEON flag when __aarch64__ defined
* bus/dpaa: fix build with gcc 9.0
* bus/dpaa: fix inconsistent struct alignment
* bus/pci: compare kernel driver instead of interrupt handler
* bus/pci: fix allocation of device path
* bus/pci: fix config r/w access
* bus/pci: replace strncpy by strlcpy
* crypto/aesni_mb: fix possible array overrun
* crypto/mvsam: update hash digest sizes
* crypto/scheduler: fix build with gcc 8.2
* devtools: provide more generic grep in git check
* doc: add cross-compilation in sample apps guide
* doc: add VFIO in ENA guide
* doc: clarify L3 Tx checksum prerequisite
* doc: clarify L4 Tx checksum prerequisite
* doc: clarify TSO Tx offload prerequisite
* doc: fix formatting in IP reassembly app guide
* doc: fix missing CCM to QAT feature list
* doc: fix NUMA library name in Linux guide
* doc: fix spelling in PMD guides
* doc: fix style and syntax in flow API guide
* doc: fix typo in testpmd guide
* doc: fix typos in the flow API guide
* doc: fix wrong usage of bind command
* eal/arm64: fix instrinsic for GCC < 4.9
* eal: declare trace buffer at top of own block
* eal: explicit cast in constant byte swap
* eal: explicit cast in rwlock functions
* eal: explicit cast of builtin for bsf32
* eal: explicit cast of core id when getting index
* eal: explicit cast of strlcpy return
* eal: fix build
* eal: fix build with gcc 9.0
* eal: fix build with -O1
* eal: fix casts in random functions
* eal: introduce rte version of fls
* eal/linux: fix memory leak of logid
* eal/linux: handle UIO read failure in interrupt handler
* eal: support strlcpy function
* eal: use correct data type for bitmap slab operations
* eal/x86: fix type of variable in memcpy function
* eal/x86: remove unused memcpy file
* efd: fix write unlock during ring creation
* ethdev: explicit cast of buffered Tx number
* ethdev: explicit cast of queue count return
* ethdev: fix doxygen comment to be with structure
* ethdev: fix queue start and stop
* ethdev: fix type and scope of variables in Rx burst
* eventdev: fix eth Rx adapter hotplug incompatibility
* eventdev: fix unlock in Rx adapter
* event/sw: fix cq index check for unlink usecases
* examples/flow_filtering: remove VLAN item
* examples/ipv4_multicast: fix leak of cloned packets
* examples/vhost: remove unnecessary constant
* fix dpdk.org URLs
* gro: fix overflow of TCP payload calculation
* hash: explicit casts for truncation in CRC32c
* hash: move stack declaration at top of CRC32c function
* igb_uio: fix refcount if open returns error
* ip_frag: fix overflow in key comparison
* ip_frag: use key length for key comparison
* kni: fix build on Linux < 3.14
* kni: fix build on Linux 4.19
* kni: fix kernel FIFO synchronization
* kni: fix possible uninitialized variable
* kvargs: fix processing a null list
* latency: fix timestamp marking and latency calculation
* mbuf: avoid implicit demotion in 64-bit arithmetic
* mbuf: avoid integer promotion in prepend/adj/chain
* mbuf: explicit cast of headroom on reset
* mbuf: explicit cast of size on detach
* mbuf: explicit casts of reference counter
* mbuf: fix reference counter integer promotion
* mbuf: fix Tx offload mask
* mbuf: fix type of private size in detach
* mbuf: fix type of variables in linearize function
* mem: fix memory initialization time
* mem: fix undefined behavior in NUMA-aware mapping
* mk: disable gcc AVX512F support
* net/bnx2x: fix call to link handling periodic function
* net/bnx2x: fix logging to include device name
* net/bnx2x: fix to add PHY lock
* net/bnx2x: fix to disable further interrupts
* net/bnx2x: fix VF link state update
* net/bnxt: fix registration of VF async event completion ring
* net/bnxt: fix uninitialized pointer access in Tx
* net/bnxt: set MAC filtering as outer for non tunnel frames
* net/bnxt: set VLAN strip mode before default VNIC cfg
* net/bonding: do not ignore RSS key on device config
* net/bonding: fix crash when stopping mode 4 port
* net/bonding: fix possible silent failure in config
* net/bonding: fix Rx slave fairness
* net/bonding: stop and deactivate slaves on stop
* net/bonding: support matching QinQ ethertype
* net/bonding: use evenly distributed default RSS RETA
* net/e1000/base: fix uninitialized variable
* net/e1000: do not error out if Rx drop enable is set
* net/ena: fix passing RSS hash to mbuf
* net/enic: do not use non-standard integer types
* net/enic: fix flow API memory leak
* net/enic: set Rx VLAN offload flag for non-stripped packets
* net: explicit cast in L4 checksum
* net: explicit cast of IP checksum to 16-bit
* net: explicit cast of multicast bit clearing
* net: explicit cast of protocol in IPv6 checksum
* net/failsafe: add checks for deferred queue setup
* net/failsafe: fix crash on slave queue release
* net/failsafe: remove not supported multicast MAC filter
* net: fix build with pedantic
* net: fix Intel prepare function for IP checksum offload
* net/i40e/base: correct global reset timeout calculation
* net/i40e/base: fix comment referencing internal data
* net/i40e/base: gracefully clean the resources
* net/i40e/base: properly clean resources
* net/i40e: enable loopback function for X722 MAC
* net/i40e: fix link status update
* net/i40e: fix offload not supported mask
* net/i40e: fix send admin queue command before init
* net/i40e: fix X710 Rx after reading some registers
* net/i40e: keep promiscuous on if allmulticast is enabled
* net/i40e: update Tx offload mask
* net/igb: update Tx offload mask
* net/ixgbe: fix busy polling while fiber link update
* net/ixgbe: fix maximum wait time in comment
* net/ixgbe: stop link setup alarm handler before device start
* net/ixgbe: update Tx offload mask
* net/ixgbevf: fix link state
* net/ixgbe: wait longer for link after fiber MAC setup
* net/mlx4: fix possible uninitialized variable
* net/mlx5: add Bluefield device id
* net/mlx5: disable ConnectX-4 Lx Multi Packet Send by default
* net/mlx5: fix initialization of struct members
* net/mlx5: fix interrupt completion queue index wrapping
* net/mlx5: fix multi-chunk mempool support
* net/mlx5: make vectorized Tx threshold configurable
* net/mlx5: optimize Rx buffer replenishment threshold
* net: move stack variable at top of VLAN strip function
* net/mvpp2: fix array initialization
* net/nfp: fix mbuf flags with checksum good
* net/nfp: fix memcpy out of source range
* net/nfp: fix misuse of strlcpy
* net/nfp: fix RSS
* net/nfp: replace strncpy by strlcpy
* net/octeontx: fix failures when available ports > queues
* net/octeontx: fix mbuf corruption with large private sizes
* net/octeontx: fix packet corruption on Tx
* net/qede/base: fix MFW FLR flow
* net/qede/base: fix to handle stag update event
* net/qede: fix crash when configure fails
* net/qede: fix ethernet type in HW registers
* net/qede: fix flow director for IPv6 filter
* net/qede: fix Rx buffer size calculation
* net/qede: fix strncpy
* net/qede: fix Tx offload mask
* net/qede: fix Tx tunnel offload support mask
* net/qede: replace strncpy by strlcpy
* net/sfc: allow to query RSS key and HF in isolated mode
* net/sfc: allow to query RSS key and HF when RSS is disabled
* net/sfc/base: avoid usage of too big arrays on stack
* net/sfc/base: check size of memory to read sensors data to
* net/sfc/base: fix a typo in unicast filter insertion comment
* net/sfc/base: fix build because of no declaration
* net/sfc/base: fix ID retrieval in v3 licensing
* net/sfc/base: fix MAC Tx stats for less or equal to 64 bytes
* net/sfc/base: fix out of bounds read when dereferencing sdup
* net/sfc/base: fix PreFAST warnings because of unused return
* net/sfc/base: fix SAL annotation for input buffers
* net/sfc/base: make last byte of module information available
* net/sfc/base: prevent access to the NIC config before probe
* net/sfc/base: properly align on line continuation
* net/sfc/base: remove Falcon-specific concurrency check
* net/sfc: do not skip RSS configuration step on reconfigure
* net/sfc: fix an Rx queue double release possibility
* net/sfc: fix a Tx queue double release possibility
* net/sfc: make sure that stats name is nul-terminated
* net/sfc: receive prepared packets even in Rx exception case
* net/softnic: fix undefined dev info fields
* net/tap: fix file descriptor check
* net/thunderx: fix Tx desc corruption in scatter-gather mode
* net/vhost: fix parameters string
* net/virtio: fix PCI config error handling
* net/virtio: fix unchecked return value
* net/virtio: register/unregister intr handler on start/stop
* net/virtio-user: check negotiated features before set
* net/virtio-user: do not reset owner when driver resets
* net/virtio-user: fix typo in error message
* pci: fix parsing of address without function number
* ring: remove signed type flip-flopping
* ring: remove useless variables
* spinlock/x86: move stack declaration before code
* table: fix casting cuckoo hash function
* test/crypto: fix number of queue pairs
* test/event: check burst mode capability
* test/hash: fix bucket size in perf test
* test/hash: fix build
* test: release ring resources after PMD perf test
* test/reorder: fix out of bound access
* usertools: check for lspci dependency
* vfio: do not needlessly setup device in secondary process
* vhost: fix corner case for enqueue operation
* vhost: remove unneeded null pointer check

17.11.6
~~~~~~~

* app/testpmd: expand RED queue thresholds to 64 bits
* app/testpmd: fix quit to stop all ports before close
* crypto/qat: fix block size error handling
* devtools: fix wrong headline lowercase for arm
* doc: add dependency for PDF in contributing guide
* doc: add missing loopback option in testpmd guide
* doc: fix a parameter name in testpmd guide
* doc: fix garbage text in generated HTML guides
* doc: fix references in power management guide
* drivers/crypto: fix PMDs memory leak
* drivers: fix sprintf with snprintf
* drivers/net: fix several Tx prepare functions
* eal: check string parameter lengths
* eal: fix core number validation
* eal: fix out of bound access when no CPU available
* efd: fix tail queue leak
* ethdev: clear ethdev data upon detach
* ethdev: fix errno to have positive value
* ethdev: fix typo in queue setup error log
* eventdev: fix xstats documentation typo
* examples/bond: fix crash when there is no active slave
* examples/flow_filtering: fix example documentation
* examples/ipsec-secgw: fix outbound codepath for single SA
* examples/ipsec-secgw: make local variables static
* examples/tep_term: remove unused constant
* examples/vhost: fix path allocation failure handling
* gro: check invalid TCP header length
* gso: fix VxLAN/GRE tunnel checks
* kni: fix build for dev_open in Linux 5.0
* kni: fix build for igb_ndo_bridge_setlink in Linux 5.0
* kni: fix build on RHEL 8
* kni: fix build on RHEL8 for arm and Power9
* mk: fix scope of disabling AVX512F support
* net/af_packet: fix setting MTU decrements sockaddr twice
* net/bnx2x: cleanup info logs
* net/bonding: fix possible null pointer reference
* net/dpaa2: fix device init for secondary process
* net/dpaa: fix secondary process
* net/ena: add supported RSS offloads types
* net/ena: fix dev init with multi-process
* net/ena: fix errno to positive value
* net/ena: update completion queue after cleanup
* net/enic: remove useless include
* net: fix underflow for checksum of invalid IPv4 packets
* net/fm10k: fix internal switch initial status
* net/i40e: fix get RSS conf
* net/i40e: fix getting RSS configuration
* net/i40e: fix queue region DCB configure
* net/i40e: fix statistics inconsistency
* net/i40e: fix using recovery mode firmware
* net/i40e: fix VF overwrite PF RSS LUT for X722
* net/i40e: remove redundant reset of queue number
* net/i40e: revert fix offload not supported mask
* net/igb: fix LSC interrupt when using MSI-X
* net/ixgbe: fix over using multicast table for VF
* net/ixgbe: fix overwriting RSS RETA
* net/mlx5: fix validation of Rx queue number
* net/qede: fix performance bottleneck in Rx path
* net/qede: remove prefetch in Tx path
* net/sfc/base: fix Tx descriptor max number check
* net/sfc: discard last seen VLAN TCI if Tx packet is dropped
* net/sfc: fix datapath name references in logs
* net/sfc: fix port ID log
* net/sfc: fix Rx packets counter
* net/sfc: fix typo in preprocessor check
* net/sfc: fix VF error/missed stats mapping
* net/sfc: pass HW Tx queue index on creation
* net/tap: add buffer overflow checks before checksum
* net/tap: fix possible uninitialized variable access
* net/virtio: add barrier before reading the flags
* net/virtio: fix resuming port with Rx vector path
* net/virtio-user: fix used ring in cvq handling
* test/crypto: fix misleading trace message
* test/memzone: fix typo
* test/memzone: handle previously allocated memzones
* timer: fix race condition
* vfio: fix error message
* vhost: fix access for indirect descriptors
* vhost: fix crash after mmap failure
* vhost: fix error handling when mem table gets updated
* vhost: fix memory leak on realloc failure
* vhost: fix payload size of reply
* vhost: fix possible out of bound access in vector filling
* vhost: fix race condition when adding fd in the fdset

17.11.7 Release Notes
---------------------

17.11.7 Fixes
~~~~~~~~~~~~~

* app/crypto-perf: check range of socket id
* app/test: fix build with musl libc
* app/test: fix sprintf with strlcat
* app/testpmd: fix a typo in log message
* app/testpmd: fix stdout flush after printing stats
* app/testpmd: fix typo in comment
* app/testpmd: remove unused field from port struct
* app/testpmd: remove useless casts on statistics
* app/testpmd: revert fixed flag for exact link speed
* app/testpmd: set fixed flag for exact link speed
* bitrate: fix unchecked return value
* build: fix crash by disabling AVX512 with binutils 2.31
* bus/dpaa: fix Rx discard register mask
* bus/fslmc: fix build with musl libc
* bus/fslmc: fix warning with GCC 9
* bus/fslmc: remove unused include of error.h
* bus/vdev: fix debug message on probing
* cfgfile: replace strcat with strlcat
* cryptodev: fix driver name comparison
* crypto/dpaa2_sec: fix session clearing
* crypto/qat: fix not included algs for zero counter
* doc: fix a minor typo in testpmd guide
* doc: fix broken link in LPM guide
* doc: fix examples in bonding guide
* doc: fix interactive commands in testpmd guide
* doc: fix link in Linux getting started guide
* doc: fix links to doxygen and sphinx sites
* doc: fix typo in mlx5 guide
* doc: remove reference to rte.doc.mk in programmers guide
* drivers/net: do not use private ethdev data
* drivers/net: fix possible overflow using strlcat
* drivers/net: fix shifting 32-bit signed variable 31 times
* eal: fix check when retrieving current CPU affinity
* eal: fix typo in comment of vector function
* eal: improve musl compatibility of string functions
* eal/linux: fix log levels for pagemap reading failure
* eal/ppc: fix global memory barrier
* eal: remove dead code in core list parsing
* eal: support strlcat function
* eal: tighten permissions on shared memory files
* ethdev: fix a typo
* event/sw: fix enqueue checks in self-test
* examples/ethtool: fix two typos
* examples/ipsec-secgw: fix AES-CTR block size
* examples/ipsec-secgw: fix build error log
* examples/ipsec-secgw: fix debug logs
* examples/l2fwd-cat: fix build on FreeBSD
* examples/vhost_scsi: fix null-check for parameter
* examples/vm_power_manager: fix PMD specific code
* hash: fix doc about thread/process safety
* igb_uio: fix build on Linux 5.3 for fall through
* kni: fix build on RHEL8
* kni: fix build with Linux 5.1
* maintainers: update for IBM POWER
* malloc: fix documentation of realloc function
* mbuf: fix a typo
* mk: disable warnings for packed mem config data structure
* mk: fix AVX512 disabled warning on non x86
* mk: fix build of shared library with libbsd
* net/bnx2x: fix DMAE timeout
* net/bnx2x: fix memory leak
* net/bnx2x: fix optic module verification
* net/bnx2x: fix race for periodic flags
* net/bnx2x: fix ramrod timeout
* net/bnx2x: fix segfaults due to stale interrupt status
* net/bnxt: fix Rx VLAN offload flags
* net/bnxt: support IOVA VA mode
* net/bonding: avoid warning for invalid port
* net/bonding: fix buffer length when printing strings
* net/bonding: fix LACP negotiation
* net/bonding: fix port id types
* net/bonding: fix queue index types
* net/bonding: fix reset active slave
* net/cxgbe: fix missing checksum flags and packet type
* net/cxgbe: update Chelsio T5/T6 NIC device ids
* net/enic: check for unsupported flow item types
* net/enic: fix flow director SCTP matching
* net/enic: fix SCTP match for flow API
* net: fix Tx VLAN flag for offload emulation
* net/fm10k: fix VLAN strip offload flag
* net/i40e: fix dereference before null check in mbuf release
* net/i40e: fix link speed for X722
* net/i40e: fix time sync for 25G
* net/ixgbe: fix crash on remove
* net/ixgbe: fix warning with GCC 9
* net/kni: fix return value check
* net/mlx5: check Tx queue size overflow
* net/mlx5: fix comments mixing Rx and Tx
* net/mlx5: fix hex dump of error completion
* net/mlx5: fix instruction hotspot on replenishing Rx buffer
* net/mlx5: fix max number of queues for NEON Tx
* net/mlx5: fix packet inline on Tx queue wraparound
* net/mlx5: fix release of Rx queue object
* net/mlx5: fix sync when handling Tx completions
* net/nfp: fix RSS query
* net/null: add MAC address setting fake operation
* net/octeontx: fix vdev name
* net/qede: fix Tx packet prepare for tunnel packets
* net/qede: support IOVA VA mode
* net/ring: avoid hard-coded length
* net/ring: check length of ring name
* net/ring: fix return value check
* net/ring: use calloc style where appropriate
* net/sfc: log port ID as 16-bit unsigned integer on panic
* net/sfc: remove control path logging from Rx queue count
* net/virtio: add barrier in interrupt enable
* net/virtio: fix buffer leak on VLAN insert
* net/virtio: fix dangling pointer on failure
* net/virtio: fix duplicate naming of include guard
* net/virtio: remove forward declaration
* net/virtio: remove useless condition
* power: fix frequency list buffer validation
* ring: fix an error message
* ring: fix namesize macro documentation block
* spinlock: reimplement with atomic one-way barrier
* test/bonding: assign non-zero MAC to null devices
* test/distributor: replace sprintf with strlcpy
* test/hash: replace sprintf with snprintf
* test/rwlock: add missing inttypes include
* test/rwlock: benchmark on all available cores
* test/spinlock: amortize the cost of getting time
* test/spinlock: remove delay for correct benchmarking
* test/virtual_pmd: add MAC address setting fake op
* version: 17.11.7-rc1
* vhost: fix device leak on connection add failure
* vhost: fix potential use-after-free for memory region
* vhost: fix potential use-after-free for zero copy mbuf
* vhost: fix silent queue enabling with legacy guests
* vhost: restore mbuf first when freeing zmbuf

17.11.7 Validation
~~~~~~~~~~~~~~~~~~

* Mellanox(R) Testing

   * Basic functionality

      * Send and receive multiple types of traffic
      * testpmd xstats counter test
      * testpmd timestamp test
      * Changing/checking link status through testpmd
      * RTE flow and flow_director tests
      * Some RSS tests
      * VLAN stripping and insertion tests
      * Checksum and TSO tests
      * ptype tests
      * Port interrupt testing
      * Multi-process testing

   * OFED versions tested

      * rdma-core v25.0
      * MLNX_OFED_LINUX-4.6-1.0.1.1

   * Tested NICs

      * ConnectX-4 Lx (fw 14.25.6406).
      * ConnectX-5 (fw 16.25.6406).

   * OS tested

      * RHEL7.4 (kernel 5.3.0-rc4).
      * RHEL7.4 (kernel 3.10.0-693.el7.x86_64).

* Red Hat(R) Testing

   * Functionality

      * PF
      * VF
      * vhost single/multi queues and cross-NUMA
      * vhostclient reconnect
      * vhost live migration with single/multi queues and cross-NUMA
      * OVS PVP

* Intel(R) Testing

   * Basic Intel(R) NIC(ixgbe and i40e) testing

      * vlan
      * vxlan
      * Jumbo frames
      * Generic filter
      * Flow director
      * PF and VF
      * Intel NIC single core/NIC performance

   * Basic cryptodev and virtio testing

      * cryptodev
      * vhost/virtio basic loopback, PVP and performance test

17.11.7 Known Issues
~~~~~~~~~~~~~~~~~~~~

* DPDK 17.11.7 contains fixes up to DPDK v19.08. Issues identified/fixed in DPDK master branch after DPDK v19.08 may be present in DPDK 17.11.7

Fixes skipped and status unresolved
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



17.11.8 Release Notes
---------------------

17.11.8 Fixes
~~~~~~~~~~~~~

* vhost: add number of fds to vhost-user messages
* vhost: fix possible denial of service by leaking FDs - CVE-2019-14818
* vhost: fix possible denial of service on SET_VRING_NUM - CVE-2019-14818
* vhost: validate virtqueue size

17.11.8 Validation
~~~~~~~~~~~~~~~~~~

* Tested with two testpmd instances, one with Vhost PMD, the other with Virtio-user
  PMD. Intialization goes well, and packets flow.

17.11.9 Release Notes
---------------------

17.11.9 Fixes
~~~~~~~~~~~~~

* vhost: fix vring requests validation broken if no FD

17.11.9 Validation
~~~~~~~~~~~~~~~~~~

* virtio/vhost regression tests by Intel:
  * http://doc.dpdk.org/dts/test_plans/virtio_pvp_regression_test_plan.html
  * http://doc.dpdk.org/dts/test_plans/vhost_dequeue_zero_copy_test_plan.html
  * http://doc.dpdk.org/dts/test_plans/vm2vm_virtio_pmd_test_plan.html

17.11.10 Release Notes
----------------------

17.11.10 Fixes
~~~~~~~~~~~~~~

* app/crypto-perf: fix input of AEAD decrypt
* app/proc_info: fix string copying to use strlcpy
* app/testpmd: block xstats for hidden ports
* app/testpmd: fix crash on port reset
* app/testpmd: fix help for loop topology option
* app/testpmd: fix Tx checksum when TSO enabled
* app/testpmd: use better randomness for Tx split
* bus/pci: align next mapping address on page boundary
* bus/pci: fix Intel IOMMU sysfs access check
* bus/pci: remove useless link dependency on ethdev
* cryptodev: fix checks related to device id
* cryptodev: fix initialization on multi-process
* cryptodev: fix missing device id range checking
* crypto/dpaa2_sec: fix length retrieved from hardware
* crypto/openssl: use local copy for session contexts
* crypto/qat: fix digest length in XCBC capability
* crypto/qat: fix null auth issues when using vfio_pci
* doc: fix a common typo in NIC guides
* doc: fix AESNI-GCM limitations in crypto guide
* doc: fix description of versioning macros
* doc: fix format in virtio guide
* doc: fix tap guide
* doc: fix typo in l2fwd-crypto guide
* doc/guides: clean repeated words
* doc: robustify PDF build
* drivers/crypto: remove some invalid comments
* eal: remove dead code on NUMA node detection
* ethdev: fix endian annotation for SPI item
* ethdev: fix include of ethernet header file
* ethdev: fix typos for ENOTSUP
* ethdev: limit maximum number of queues
* ethdev: remove redundant device info cleanup before get
* event/dpaa2: fix default queue configuration
* event/octeontx: fix partial Rx packet handling
* event/sw: fix xstats reset value
* examples/ipsec-secgw: fix default configuration
* examples/ipsec-secgw: fix GCM IV length
* examples/ipsec-secgw: fix SHA256-HMAC digest length
* examples/l3fwd-power: fix Rx interrupt disabling
* examples/multi_process: fix client crash with sparse ports
* examples/vm_power: fix build without i40e
* examples/vm_power: fix type of cmdline token in cli
* kni: fix build with kernel 5.3
* lib/distributor: fix deadlock on aarch64
* lib: fix doxygen typos
* lib: fix log typos
* maintainers: update for stable branches
* malloc: fix realloc copy size
* malloc: fix realloc padded element size
* malloc: set pad to 0 on free
* mempool/dpaa2: report error on endless loop in mbuf release
* mk: remove library search path from binary
* net/af_packet: fix stale sockets
* net/af_packet: improve Tx statistics accuracy
* net/bnxt: cleanup comments
* net/bnxt: enforce IO barrier for doorbell command
* net/bnxt: expose some missing counters in port stats
* net/bnxt: fix async link handling and update
* net/bnxt: fix coding style
* net/bnxt: fix crash in secondary process
* net/bnxt: fix crash in xstats get
* net/bnxt: fix dereference before null check
* net/bnxt: fix log message level
* net/bnxt: fix mbuf free when clearing Tx queue
* net/bnxt: fix memory leak
* net/bnxt: fix multicast filter programming
* net/bnxt: fix Rx queue count
* net/bnxt: fix setting default MAC address
* net/bnxt: get default HWRM command timeout from FW
* net/bnxt: move macro definitions to header file
* net/bnxt: remove commented out code
* net/bnxt: remove duplicate barrier
* net/bnxt: remove redundant header file inclusion
* net/bnxt: remove unnecessary variable assignment
* net/bnxt: return error if setting link up fails
* net/bonding: fix LACP fast queue Rx handler
* net/bonding: fix link speed update in broadcast mode
* net/bonding: fix OOB access in other aggregator modes
* net/bonding: fix port ID check
* net/bonding: fix selection logic
* net/bonding: fix slave id types
* net/bonding: fix unicast packets filtering
* net/bonding: use non deprecated PCI API
* net/cxgbe: fix prefetch for non-coalesced Tx packets
* net/dpaa2: add retry and timeout in packet enqueue API
* net/dpaa2: fix possible use of uninitialized vars
* net/dpaa2: set port in mbuf
* net/e1000: fix link status
* net/fm10k: fix mbuf free in vector Rx
* net/fm10k: fix stats crash in multi-process
* net/i40e: downgrade error log
* net/i40e: fix address of first segment
* net/i40e: remove compiler barrier from NEON Rx
* net/i40e: remove memory barrier from NEON Rx
* net/igb: fix global variable multiple definitions
* net/igb: fix PHY status if PHY reset is not blocked
* net/ixgbe: enable new PF host mbox version
* net/ixgbe: fix address of first segment
* net/ixgbe: fix link status
* net/ixgbe: fix link status
* net/ixgbe: fix queue interrupt for X552/557
* net/ixgbe: fix VF RSS offloads configuration
* net/ixgbe: fix X553 speed capability
* net/ixgbe: remove memory barrier from NEON Rx
* net/ixgbe: remove redundant assignment
* net/ixgbe: support packet type with NEON
* net/mlx4: fix build on ppc64
* net/mlx5: fix Rx CQ doorbell synchronization on aarch64
* net/mlx: fix build with make and recent gcc
* net/mlx: fix debug build with icc
* net/qede: fix setting MTU
* net/qede: fix setting VLAN strip mode
* net/qede: limit Rx ring index read for debug
* net/sfc: fix adapter lock usage on rule creation
* net/tap: fix blocked Rx packets
* net/vhost: fix redundant queue state event
* net/virtio: fix descriptor addressed in Tx
* net/virtio: fix mbuf data and packet length mismatch
* net/virtio: fix Tx checksum offloads
* net/virtio: get all pending Rx packets in vectorized paths
* net/virtio: init MTU in case no control channel
* net/virtio: reject deferred Rx start
* net/virtio: reject deferred Tx start
* net/virtio-user: fix setting filters
* power: fix socket indicator value
* power: handle frequency increase with turbo disabled
* ring: enforce reading tail before slots
* security: fix doxygen fields
* service: use log for error messages
* test/bonding: fix LSC related cases
* test: fix global variable multiple definitions
* test/interrupt: account for race with callback
* test/lpm: fix measured cycles for delete
* test/mbuf: fix forged mbuf in clone test
* test: optimise fd closing in forks
* test/service: fix wait for service core
* test/test: fix test app defining unused variable
* usertools: fix pmdinfo with python 3 and pyelftools>=0.24
* version: 17.11.10-rc1
* vfio: fix truncated BAR offset for 32-bit
* vhost: convert buffer addresses to GPA for logging
* vhost: fix IPv4 checksum
* vhost: fix slave request fd leak
* vhost: fix virtqueue not accessible
* vhost: fix vring address handling during live migration
* vhost: fix vring memory partially mapped
* vhost: forbid reallocation when running
* vhost: prevent zero copy mode if IOMMU is on
* vhost: protect vring access done by application
* vhost: translate incoming log address to GPA
* vhost: un-inline dirty pages logging functions

17.11.10 Validation
~~~~~~~~~~~~~~~~~~~

* Red Hat(R) Testing

   * RHEL 7.8
   * Functionality

      * PF assignment
      * VF assignment
      * vhost single/multi queues and cross-NUMA
      * vhostclient reconnect
      * vhost live migration with single/multi queues and cross-NUMA
      * OVS PVP

* Intel(R) Testing

   * Basic Intel(R) NIC(ixgbe and i40e) testing

      * PF (i40e)
      * PF (ixgbe)
      * VF
      * Compile Testing
      * Intel NIC single core/NIC performance

   * Basic cryptodev and virtio testing

      * cryptodev
      * vhost/virtio basic loopback, PVP and performance test

* Mellanox(R) Testing

   * Basic functionality with testpmd

      * Tx/Rx
      * xstats
      * Timestamps
      * Link status
      * RTE flow and flow_director
      * RSS
      * VLAN stripping and insertion
      * Checksum/TSO
      * ptype
      * Multi-process

   * ConnectX-5

      * RHEL 7.4
      * Kernel 3.10.0-693.el7.x86_64
      * Driver MLNX_OFED_LINUX-4.7-3.2.9.0
      * fw 16.26.4012

   * ConnectX-4 Lx

      * RHEL 7.4
      * Kernel 3.10.0-693.el7.x86_64
      * Driver MLNX_OFED_LINUX-4.7-3.2.9.0
      * fw 14.26.4012

* Intel(R) Testing with Open vSwitch

   * OVS testing with OVS branches 2.10 and 2.19 with VSPERF

   * Tested NICs

      * i40e (X710)
      * ixgbe (82599ES)

   * Functionality

      * P2P
      * PVP
      * Hotplug
      * Multiqueue
      * Vhostuserclient reconnect
      * Vhost cross-NUMA awareness
      * Jumbo frames
      * Rate limiting
      * QoS policer

17.11.10 Known Issues
~~~~~~~~~~~~~~~~~~~~~

* MLX4/5 and rte_flow: count actions and rules with vlan items are not supported, and MLNX_OFED_LINUX 4.6-1.0.1.1 is not compatible
* DPDK 17.11.10 contains fixes up to DPDK v19.11. Issues identified/fixed in DPDK master branch after DPDK v19.11 may be present in DPDK 17.11.10
