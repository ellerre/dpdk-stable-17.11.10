 ..  BSD LICENSE
     Copyright(c) 2015 Intel Corporation. All rights reserved.
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
     * Neither the name of Intel Corporation nor the names of its
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

Compiling the Sample Applications
=================================

This section explains how to compile the DPDK sample applications.

To compile all the sample applications
--------------------------------------

Set the path to DPDK source code if its not set:

    .. code-block:: console

        export RTE_SDK=/path/to/rte_sdk

Go to DPDK source:

    .. code-block:: console

        cd $RTE_SDK

Build DPDK:

   .. code-block:: console

        make defconfig
        make

Build the sample applications:

   .. code-block:: console

       export RTE_TARGET=build
       make -C examples

For other possible ``RTE_TARGET`` values and additional information on
compiling see
:ref:`Compiling DPDK on Linux <linux_gsg_compiling_dpdk>` or
:ref:`Compiling DPDK on FreeBSD <building_from_source>`.
Applications are output to: ``$RTE_SDK/examples/app-dir/build`` or
``$RTE_SDK/examples/app-dir/$RTE_TARGET``.


In the example above the compiled application is written to the ``build`` subdirectory.
To have the applications written to a different location,
the ``O=/path/to/build/directory`` option may be specified in the make command.

    .. code-block:: console

       make O=/tmp

To build the applications for debugging use the ``DEBUG`` option.
This option adds some extra flags, disables compiler optimizations and
sets verbose output.

    .. code-block:: console

       make DEBUG=1


To compile a single application
-------------------------------

Set the path to DPDK source code:

    .. code-block:: console

        export RTE_SDK=/path/to/rte_sdk

Go to DPDK source:

    .. code-block:: console

       cd $RTE_SDK

Build DPDK:

    .. code-block:: console

        make defconfig
        make

Go to the sample application directory. Unless otherwise specified the sample
applications are located in ``$RTE_SDK/examples/``.


Build the application:

    .. code-block:: console

        export RTE_TARGET=build
        make

To cross compile the sample application(s)
------------------------------------------

For cross compiling the sample application(s), please append 'CROSS=$(CROSS_COMPILER_PREFIX)' to the 'make' command.
In example of AARCH64 cross compiling:

    .. code-block:: console

        export RTE_TARGET=build
        export RTE_SDK=/path/to/rte_sdk
        make -C examples CROSS=aarch64-linux-gnu-
               or
        make CROSS=aarch64-linux-gnu-
