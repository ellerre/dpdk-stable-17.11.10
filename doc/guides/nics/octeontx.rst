..  BSD LICENSE
    Copyright (C) Cavium, Inc. 2017.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.
    * Neither the name of Cavium, Inc nor the names of its
    contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

OCTEONTX Poll Mode driver
=========================

The OCTEONTX ETHDEV PMD (**librte_pmd_octeontx**) provides poll mode ethdev
driver support for the inbuilt network device found in the **Cavium OCTEONTX**
SoC family as well as their virtual functions (VF) in SR-IOV context.

More information can be found at `Cavium, Inc Official Website
<http://www.cavium.com/OCTEON-TX_ARM_Processors.html>`_.

Features
--------

Features of the OCTEONTX Ethdev PMD are:

- Packet type information
- Promiscuous mode
- Port hardware statistics
- Jumbo frames
- Link state information
- SR-IOV VF
- Multiple queues for TX
- Lock-free Tx queue
- HW offloaded `ethdev Rx queue` to `eventdev event queue` packet injection

Supported OCTEONTX SoCs
-----------------------

- CN83xx

Unsupported features
--------------------

The features supported by the device and not yet supported by this PMD include:

- Receive Side Scaling (RSS)
- Scattered and gather for TX and RX
- Ingress classification support
- Egress hierarchical scheduling, traffic shaping, and marking

Prerequisites
-------------

See :doc:`../platform/octeontx` for setup information.

Pre-Installation Configuration
------------------------------

Config File Options
~~~~~~~~~~~~~~~~~~~

The following options can be modified in the ``config`` file.
Please note that enabling debugging options may affect system performance.

- ``CONFIG_RTE_LIBRTE_OCTEONTX_PMD`` (default ``y``)

  Toggle compilation of the ``librte_pmd_octeontx`` driver.

- ``CONFIG_RTE_LIBRTE_OCTEONTX_DEBUG_DRIVER`` (default ``n``)

  Toggle display of generic debugging messages

- ``CONFIG_RTE_LIBRTE_OCTEONTX_DEBUG_INIT`` (default ``n``)

  Toggle display of initialization related messages.

- ``CONFIG_RTE_LIBRTE_OCTEONTX_DEBUG_RX`` (default ``n``)

  Toggle display of receive path message

- ``CONFIG_RTE_LIBRTE_OCTEONTX_DEBUG_TX`` (default ``n``)

  Toggle display of transmit path message

- ``CONFIG_RTE_LIBRTE_OCTEONTX_DEBUG_MBOX`` (default ``n``)

  Toggle display of mbox related message


Driver compilation and testing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Refer to the document :ref:`compiling and testing a PMD for a NIC <pmd_build_and_test>`
for details.

To compile the OCTEONTX PMD for Linux arm64 gcc target, run the
following ``make`` command:

.. code-block:: console

   cd <DPDK-source-directory>
   make config T=arm64-thunderx-linuxapp-gcc install

#. Running testpmd:

   Follow instructions available in the document
   :ref:`compiling and testing a PMD for a NIC <pmd_build_and_test>`
   to run testpmd.

   Example output:

   .. code-block:: console

      ./arm64-thunderx-linuxapp-gcc/app/testpmd -c 700 \
                --base-virtaddr=0x100000000000 \
                --mbuf-pool-ops-name="octeontx_fpavf" \
                --vdev='event_octeontx' \
                --vdev='eth_octeontx,nr_port=2' \
                -- --rxq=1 --txq=1 --nb-core=2 --total-num-mbufs=16384 \
                --disable-hw-vlan-filter -i
      .....
      EAL: Detected 24 lcore(s)
      EAL: Probing VFIO support...
      EAL: VFIO support initialized
      .....
      EAL: PCI device 0000:07:00.1 on NUMA socket 0
      EAL:   probe driver: 177d:a04b octeontx_ssovf
      .....
      EAL: PCI device 0001:02:00.7 on NUMA socket 0
      EAL:   probe driver: 177d:a0dd octeontx_pkivf
      .....
      EAL: PCI device 0001:03:01.0 on NUMA socket 0
      EAL:   probe driver: 177d:a049 octeontx_pkovf
      .....
      PMD: octeontx_probe(): created ethdev eth_octeontx for port 0
      PMD: octeontx_probe(): created ethdev eth_octeontx for port 1
      .....
      Configuring Port 0 (socket 0)
      Port 0: 00:0F:B7:11:94:46
      Configuring Port 1 (socket 0)
      Port 1: 00:0F:B7:11:94:47
      .....
      Checking link statuses...
      Port 0 Link Up - speed 40000 Mbps - full-duplex
      Port 1 Link Up - speed 40000 Mbps - full-duplex
      Done
      testpmd>


Initialization
--------------

The octeontx ethdev pmd is exposed as a vdev device which consists of a set
of PKI and PKO PCIe VF devices. On EAL initialization,
PKI/PKO PCIe VF devices will be probed and then the vdev device can be created
from the application code, or from the EAL command line based on
the number of probed/bound PKI/PKO PCIe VF device to DPDK by

* Invoking ``rte_vdev_init("eth_octeontx")`` from the application

* Using ``--vdev="eth_octeontx"`` in the EAL options, which will call
  rte_vdev_init() internally

Device arguments
~~~~~~~~~~~~~~~~
Each ethdev port is mapped to a physical port(LMAC), Application can specify
the number of interesting ports with ``nr_ports`` argument.

Dependency
~~~~~~~~~~
``eth_octeontx`` pmd is depend on ``event_octeontx`` eventdev device and
``octeontx_fpavf`` external mempool handler.

Example:

.. code-block:: console

    ./your_dpdk_application --mbuf-pool-ops-name="octeontx_fpavf" \
                --vdev='event_octeontx' \
                --vdev="eth_octeontx,nr_port=2"

Limitations
-----------

``octeontx_fpavf`` external mempool handler dependency
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
The OCTEONTX SoC family NIC has inbuilt HW assisted external mempool manager.
This driver will only work with ``octeontx_fpavf`` external mempool handler
as it is the most performance effective way for packet allocation and Tx buffer
recycling on OCTEONTX SoC platform.

CRC stripping
~~~~~~~~~~~~~

The OCTEONTX SoC family NICs strip the CRC for every packets coming into the
host interface. So, CRC will be stripped even when the
``rxmode.hw_strip_crc`` member is set to 0 in ``struct rte_eth_conf``.

Maximum packet length
~~~~~~~~~~~~~~~~~~~~~

The OCTEONTX SoC family NICs support a maximum of a 32K jumbo frame. The value
is fixed and cannot be changed. So, even when the ``rxmode.max_rx_pkt_len``
member of ``struct rte_eth_conf`` is set to a value lower than 32k, frames
up to 32k bytes can still reach the host interface.
