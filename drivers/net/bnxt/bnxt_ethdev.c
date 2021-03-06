/*-
 *   BSD LICENSE
 *
 *   Copyright(c) Broadcom Limited.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Broadcom Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <inttypes.h>
#include <stdbool.h>

#include <rte_dev.h>
#include <rte_ethdev.h>
#include <rte_ethdev_pci.h>
#include <rte_malloc.h>
#include <rte_cycles.h>

#include "bnxt.h"
#include "bnxt_filter.h"
#include "bnxt_hwrm.h"
#include "bnxt_irq.h"
#include "bnxt_ring.h"
#include "bnxt_rxq.h"
#include "bnxt_rxr.h"
#include "bnxt_stats.h"
#include "bnxt_txq.h"
#include "bnxt_txr.h"
#include "bnxt_vnic.h"
#include "hsi_struct_def_dpdk.h"
#include "bnxt_nvm_defs.h"

#define DRV_MODULE_NAME		"bnxt"
static const char bnxt_version[] =
	"Broadcom Cumulus driver " DRV_MODULE_NAME "\n";

static const struct rte_pci_id bnxt_pci_id_map[] = {
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM,
			 BROADCOM_DEV_ID_STRATUS_NIC_VF) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_STRATUS_NIC) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_57414_VF) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_57301) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_57302) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_57304_PF) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_57304_VF) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_NS2) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_57402) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_57404) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_57406_PF) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_57406_VF) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_57402_MF) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_57407_RJ45) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_57404_MF) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_57406_MF) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_57407_SFP) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_57407_MF) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_5741X_VF) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_5731X_VF) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_57314) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_57417_MF) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_57311) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_57312) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_57412) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_57414) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_57416_RJ45) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_57417_RJ45) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_57412_MF) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_57317_RJ45) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_57417_SFP) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_57416_SFP) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_57317_SFP) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_57414_MF) },
	{ RTE_PCI_DEVICE(PCI_VENDOR_ID_BROADCOM, BROADCOM_DEV_ID_57416_MF) },
	{ .vendor_id = 0, /* sentinel */ },
};

#define BNXT_ETH_RSS_SUPPORT (	\
	ETH_RSS_IPV4 |		\
	ETH_RSS_NONFRAG_IPV4_TCP |	\
	ETH_RSS_NONFRAG_IPV4_UDP |	\
	ETH_RSS_IPV6 |		\
	ETH_RSS_NONFRAG_IPV6_TCP |	\
	ETH_RSS_NONFRAG_IPV6_UDP)

static int bnxt_vlan_offload_set_op(struct rte_eth_dev *dev, int mask);
static void bnxt_print_link_info(struct rte_eth_dev *eth_dev);

/***********************/

/*
 * High level utility functions
 */

static void bnxt_free_mem(struct bnxt *bp)
{
	bnxt_free_filter_mem(bp);
	bnxt_free_vnic_attributes(bp);
	bnxt_free_vnic_mem(bp);

	bnxt_free_stats(bp);
	bnxt_free_tx_rings(bp);
	bnxt_free_rx_rings(bp);
	bnxt_free_def_cp_ring(bp);

	rte_free(bp->grp_info);
	bp->grp_info = NULL;
}

static int bnxt_alloc_mem(struct bnxt *bp)
{
	int rc;

	/* Default completion ring */
	rc = bnxt_init_def_ring_struct(bp, SOCKET_ID_ANY);
	if (rc)
		goto alloc_mem_err;

	rc = bnxt_alloc_rings(bp, 0, NULL, NULL,
			      bp->def_cp_ring, "def_cp");
	if (rc)
		goto alloc_mem_err;

	rc = bnxt_alloc_vnic_mem(bp);
	if (rc)
		goto alloc_mem_err;

	rc = bnxt_alloc_vnic_attributes(bp);
	if (rc)
		goto alloc_mem_err;

	rc = bnxt_alloc_filter_mem(bp);
	if (rc)
		goto alloc_mem_err;

	return 0;

alloc_mem_err:
	bnxt_free_mem(bp);
	return rc;
}

static int bnxt_init_chip(struct bnxt *bp)
{
	unsigned int i, rss_idx, fw_idx;
	struct rte_eth_link new;
	struct rte_pci_device *pci_dev = RTE_ETH_DEV_TO_PCI(bp->eth_dev);
	struct rte_eth_conf *dev_conf = &bp->eth_dev->data->dev_conf;
	struct rte_intr_handle *intr_handle = &pci_dev->intr_handle;
	uint64_t rx_offloads = dev_conf->rxmode.offloads;
	uint32_t intr_vector = 0;
	uint32_t queue_id, base = BNXT_MISC_VEC_ID;
	uint32_t vec = BNXT_MISC_VEC_ID;
	int rc;

	/* disable uio/vfio intr/eventfd mapping */
	rte_intr_disable(intr_handle);

	if (bp->eth_dev->data->mtu > ETHER_MTU) {
		bp->eth_dev->data->dev_conf.rxmode.jumbo_frame = 1;
		bp->flags |= BNXT_FLAG_JUMBO;
	} else {
		bp->eth_dev->data->dev_conf.rxmode.jumbo_frame = 0;
		bp->flags &= ~BNXT_FLAG_JUMBO;
	}

	rc = bnxt_alloc_all_hwrm_stat_ctxs(bp);
	if (rc) {
		RTE_LOG(ERR, PMD, "HWRM stat ctx alloc failure rc: %x\n", rc);
		goto err_out;
	}

	rc = bnxt_alloc_hwrm_rings(bp);
	if (rc) {
		RTE_LOG(ERR, PMD, "HWRM ring alloc failure rc: %x\n", rc);
		goto err_out;
	}

	rc = bnxt_alloc_all_hwrm_ring_grps(bp);
	if (rc) {
		RTE_LOG(ERR, PMD, "HWRM ring grp alloc failure: %x\n", rc);
		goto err_out;
	}

	rc = bnxt_mq_rx_configure(bp);
	if (rc) {
		RTE_LOG(ERR, PMD, "MQ mode configure failure rc: %x\n", rc);
		goto err_out;
	}

	/* VNIC configuration */
	for (i = 0; i < bp->nr_vnics; i++) {
		struct bnxt_vnic_info *vnic = &bp->vnic_info[i];
		uint32_t size = sizeof(*vnic->fw_grp_ids) * bp->max_ring_grps;

		vnic->fw_grp_ids = rte_zmalloc("vnic_fw_grp_ids", size, 0);
		if (!vnic->fw_grp_ids) {
			RTE_LOG(ERR, PMD,
				"Failed to alloc %d bytes for group ids\n",
				size);
			rc = -ENOMEM;
			goto err_out;
		}
		memset(vnic->fw_grp_ids, -1, size);

		rc = bnxt_hwrm_vnic_alloc(bp, vnic);
		if (rc) {
			RTE_LOG(ERR, PMD, "HWRM vnic %d alloc failure rc: %x\n",
				i, rc);
			goto err_out;
		}

		rc = bnxt_hwrm_vnic_ctx_alloc(bp, vnic);
		if (rc) {
			RTE_LOG(ERR, PMD,
				"HWRM vnic %d ctx alloc failure rc: %x\n",
				i, rc);
			goto err_out;
		}

		/*
		 * Firmware sets pf pair in default vnic cfg. If the VLAN strip
		 * setting is not available at this time, it will not be
		 * configured correctly in the CFA.
		 */
		if (rx_offloads & DEV_RX_OFFLOAD_VLAN_STRIP)
			vnic->vlan_strip = true;
		else
			vnic->vlan_strip = false;

		rc = bnxt_hwrm_vnic_cfg(bp, vnic);
		if (rc) {
			RTE_LOG(ERR, PMD, "HWRM vnic %d cfg failure rc: %x\n",
				i, rc);
			goto err_out;
		}

		rc = bnxt_set_hwrm_vnic_filters(bp, vnic);
		if (rc) {
			RTE_LOG(ERR, PMD,
				"HWRM vnic %d filter failure rc: %x\n",
				i, rc);
			goto err_out;
		}
		if (vnic->rss_table && vnic->hash_type) {
			/*
			 * Fill the RSS hash & redirection table with
			 * ring group ids for all VNICs
			 */
			for (rss_idx = 0, fw_idx = 0;
			     rss_idx < HW_HASH_INDEX_SIZE;
			     rss_idx++, fw_idx++) {
				if (vnic->fw_grp_ids[fw_idx] ==
				    INVALID_HW_RING_ID)
					fw_idx = 0;
				vnic->rss_table[rss_idx] =
						vnic->fw_grp_ids[fw_idx];
			}
			rc = bnxt_hwrm_vnic_rss_cfg(bp, vnic);
			if (rc) {
				RTE_LOG(ERR, PMD,
					"HWRM vnic %d set RSS failure rc: %x\n",
					i, rc);
				goto err_out;
			}
		}

		bnxt_hwrm_vnic_plcmode_cfg(bp, vnic);

		if (bp->eth_dev->data->dev_conf.rxmode.enable_lro)
			bnxt_hwrm_vnic_tpa_cfg(bp, vnic, 1);
		else
			bnxt_hwrm_vnic_tpa_cfg(bp, vnic, 0);
	}
	rc = bnxt_hwrm_cfa_l2_set_rx_mask(bp, &bp->vnic_info[0], 0, NULL);
	if (rc) {
		RTE_LOG(ERR, PMD,
			"HWRM cfa l2 rx mask failure rc: %x\n", rc);
		goto err_out;
	}

	/* check and configure queue intr-vector mapping */
	if ((rte_intr_cap_multiple(intr_handle) ||
	     !RTE_ETH_DEV_SRIOV(bp->eth_dev).active) &&
	    bp->eth_dev->data->dev_conf.intr_conf.rxq != 0) {
		intr_vector = bp->eth_dev->data->nb_rx_queues;
		RTE_LOG(INFO, PMD, "%s(): intr_vector = %d\n", __func__,
			intr_vector);
		if (intr_vector > bp->rx_cp_nr_rings) {
			RTE_LOG(ERR, PMD, "At most %d intr queues supported",
					bp->rx_cp_nr_rings);
			return -ENOTSUP;
		}
		if (rte_intr_efd_enable(intr_handle, intr_vector))
			return -1;
	}

	if (rte_intr_dp_is_en(intr_handle) && !intr_handle->intr_vec) {
		intr_handle->intr_vec =
			rte_zmalloc("intr_vec",
				    bp->eth_dev->data->nb_rx_queues *
				    sizeof(int), 0);
		if (intr_handle->intr_vec == NULL) {
			RTE_LOG(ERR, PMD, "Failed to allocate %d rx_queues"
				" intr_vec", bp->eth_dev->data->nb_rx_queues);
			return -ENOMEM;
		}
		RTE_LOG(DEBUG, PMD, "%s(): intr_handle->intr_vec = %p "
			"intr_handle->nb_efd = %d intr_handle->max_intr = %d\n",
			 __func__, intr_handle->intr_vec, intr_handle->nb_efd,
			intr_handle->max_intr);
	}

	for (queue_id = 0; queue_id < bp->eth_dev->data->nb_rx_queues;
	     queue_id++) {
		intr_handle->intr_vec[queue_id] = vec;
		if (vec < base + intr_handle->nb_efd - 1)
			vec++;
	}

	/* enable uio/vfio intr/eventfd mapping */
	rte_intr_enable(intr_handle);

	rc = bnxt_get_hwrm_link_config(bp, &new);
	if (rc) {
		RTE_LOG(ERR, PMD, "HWRM Get link config failure rc: %x\n", rc);
		goto err_out;
	}

	if (!bp->link_info.link_up) {
		rc = bnxt_set_hwrm_link_config(bp, true);
		if (rc) {
			RTE_LOG(ERR, PMD,
				"HWRM link config failure rc: %x\n", rc);
			goto err_out;
		}
	}
	bnxt_print_link_info(bp->eth_dev);

	return 0;

err_out:
	bnxt_free_all_hwrm_resources(bp);

	return rc;
}

static int bnxt_shutdown_nic(struct bnxt *bp)
{
	bnxt_free_all_hwrm_resources(bp);
	bnxt_free_all_filters(bp);
	bnxt_free_all_vnics(bp);
	return 0;
}

static int bnxt_init_nic(struct bnxt *bp)
{
	int rc;

	rc = bnxt_init_ring_grps(bp);
	if (rc)
		return rc;

	bnxt_init_vnics(bp);
	bnxt_init_filters(bp);

	return 0;
}

/*
 * Device configuration and status function
 */

static void bnxt_dev_info_get_op(struct rte_eth_dev *eth_dev,
				  struct rte_eth_dev_info *dev_info)
{
	struct rte_pci_device *pdev = RTE_DEV_TO_PCI(eth_dev->device);
	struct bnxt *bp = (struct bnxt *)eth_dev->data->dev_private;
	uint16_t max_vnics, i, j, vpool, vrxq;
	unsigned int max_rx_rings;

	dev_info->pci_dev = RTE_ETH_DEV_TO_PCI(eth_dev);

	/* MAC Specifics */
	dev_info->max_mac_addrs = bp->max_l2_ctx;
	dev_info->max_hash_mac_addrs = 0;

	/* PF/VF specifics */
	if (BNXT_PF(bp))
		dev_info->max_vfs = pdev->max_vfs;
	max_rx_rings = RTE_MIN(bp->max_vnics, RTE_MIN(bp->max_l2_ctx,
						RTE_MIN(bp->max_rsscos_ctx,
						bp->max_stat_ctx)));
	/* For the sake of symmetry, max_rx_queues = max_tx_queues */
	dev_info->max_rx_queues = max_rx_rings;
	dev_info->max_tx_queues = max_rx_rings;
	dev_info->reta_size = HW_HASH_INDEX_SIZE;
	dev_info->hash_key_size = 40;
	max_vnics = bp->max_vnics;

	/* Fast path specifics */
	dev_info->min_rx_bufsize = 1;
	dev_info->max_rx_pktlen = BNXT_MAX_MTU + ETHER_HDR_LEN + ETHER_CRC_LEN
				  + VLAN_TAG_SIZE;
	dev_info->rx_offload_capa = DEV_RX_OFFLOAD_VLAN_STRIP |
					DEV_RX_OFFLOAD_IPV4_CKSUM |
					DEV_RX_OFFLOAD_UDP_CKSUM |
					DEV_RX_OFFLOAD_TCP_CKSUM |
					DEV_RX_OFFLOAD_OUTER_IPV4_CKSUM;
	dev_info->tx_offload_capa = DEV_TX_OFFLOAD_VLAN_INSERT |
					DEV_TX_OFFLOAD_IPV4_CKSUM |
					DEV_TX_OFFLOAD_TCP_CKSUM |
					DEV_TX_OFFLOAD_UDP_CKSUM |
					DEV_TX_OFFLOAD_TCP_TSO |
					DEV_TX_OFFLOAD_OUTER_IPV4_CKSUM |
					DEV_TX_OFFLOAD_VXLAN_TNL_TSO |
					DEV_TX_OFFLOAD_GRE_TNL_TSO |
					DEV_TX_OFFLOAD_IPIP_TNL_TSO |
					DEV_TX_OFFLOAD_GENEVE_TNL_TSO;

	/* *INDENT-OFF* */
	dev_info->default_rxconf = (struct rte_eth_rxconf) {
		.rx_thresh = {
			.pthresh = 8,
			.hthresh = 8,
			.wthresh = 0,
		},
		.rx_free_thresh = 32,
		/* If no descriptors available, pkts are dropped by default */
		.rx_drop_en = 1,
	};

	dev_info->default_txconf = (struct rte_eth_txconf) {
		.tx_thresh = {
			.pthresh = 32,
			.hthresh = 0,
			.wthresh = 0,
		},
		.tx_free_thresh = 32,
		.tx_rs_thresh = 32,
		.txq_flags = ETH_TXQ_FLAGS_NOMULTSEGS |
			     ETH_TXQ_FLAGS_NOOFFLOADS,
	};
	eth_dev->data->dev_conf.intr_conf.lsc = 1;

	eth_dev->data->dev_conf.intr_conf.rxq = 1;

	/* *INDENT-ON* */

	/*
	 * TODO: default_rxconf, default_txconf, rx_desc_lim, and tx_desc_lim
	 *       need further investigation.
	 */

	/* VMDq resources */
	vpool = 64; /* ETH_64_POOLS */
	vrxq = 128; /* ETH_VMDQ_DCB_NUM_QUEUES */
	for (i = 0; i < 4; vpool >>= 1, i++) {
		if (max_vnics > vpool) {
			for (j = 0; j < 5; vrxq >>= 1, j++) {
				if (dev_info->max_rx_queues > vrxq) {
					if (vpool > vrxq)
						vpool = vrxq;
					goto found;
				}
			}
			/* Not enough resources to support VMDq */
			break;
		}
	}
	/* Not enough resources to support VMDq */
	vpool = 0;
	vrxq = 0;
found:
	dev_info->max_vmdq_pools = vpool;
	dev_info->vmdq_queue_num = vrxq;

	dev_info->vmdq_pool_base = 0;
	dev_info->vmdq_queue_base = 0;
}

/* Configure the device based on the configuration provided */
static int bnxt_dev_configure_op(struct rte_eth_dev *eth_dev)
{
	struct bnxt *bp = (struct bnxt *)eth_dev->data->dev_private;

	bp->rx_queues = (void *)eth_dev->data->rx_queues;
	bp->tx_queues = (void *)eth_dev->data->tx_queues;

	/* Inherit new configurations */
	bp->rx_nr_rings = eth_dev->data->nb_rx_queues;
	bp->tx_nr_rings = eth_dev->data->nb_tx_queues;
	bp->rx_cp_nr_rings = bp->rx_nr_rings;
	bp->tx_cp_nr_rings = bp->tx_nr_rings;

	if (eth_dev->data->dev_conf.rxmode.jumbo_frame)
		eth_dev->data->mtu =
				eth_dev->data->dev_conf.rxmode.max_rx_pkt_len -
				ETHER_HDR_LEN - ETHER_CRC_LEN - VLAN_TAG_SIZE;
	return 0;
}

static void bnxt_print_link_info(struct rte_eth_dev *eth_dev)
{
	struct rte_eth_link *link = &eth_dev->data->dev_link;

	if (link->link_status)
		RTE_LOG(INFO, PMD, "Port %d Link Up - speed %u Mbps - %s\n",
			eth_dev->data->port_id,
			(uint32_t)link->link_speed,
			(link->link_duplex == ETH_LINK_FULL_DUPLEX) ?
			("full-duplex") : ("half-duplex\n"));
	else
		RTE_LOG(INFO, PMD, "Port %d Link Down\n",
			eth_dev->data->port_id);
}

static int bnxt_dev_lsc_intr_setup(struct rte_eth_dev *eth_dev)
{
	bnxt_print_link_info(eth_dev);
	return 0;
}

static int bnxt_dev_start_op(struct rte_eth_dev *eth_dev)
{
	struct bnxt *bp = (struct bnxt *)eth_dev->data->dev_private;
	int vlan_mask = 0;
	int rc;

	if (bp->rx_cp_nr_rings > RTE_ETHDEV_QUEUE_STAT_CNTRS) {
		RTE_LOG(ERR, PMD,
			"RxQ cnt %d > CONFIG_RTE_ETHDEV_QUEUE_STAT_CNTRS %d\n",
			bp->rx_cp_nr_rings, RTE_ETHDEV_QUEUE_STAT_CNTRS);
	}
	bp->dev_stopped = 0;

	bnxt_enable_int(bp);

	rc = bnxt_init_chip(bp);
	if (rc)
		goto error;

	bnxt_link_update_op(eth_dev, 1);

	if (eth_dev->data->dev_conf.rxmode.hw_vlan_filter)
		vlan_mask |= ETH_VLAN_FILTER_MASK;
	if (eth_dev->data->dev_conf.rxmode.hw_vlan_strip)
		vlan_mask |= ETH_VLAN_STRIP_MASK;
	rc = bnxt_vlan_offload_set_op(eth_dev, vlan_mask);
	if (rc)
		goto error;

	return 0;

error:
	bnxt_shutdown_nic(bp);
	bnxt_free_tx_mbufs(bp);
	bnxt_free_rx_mbufs(bp);
	return rc;
}

static int bnxt_dev_set_link_up_op(struct rte_eth_dev *eth_dev)
{
	struct bnxt *bp = (struct bnxt *)eth_dev->data->dev_private;
	int rc = 0;

	if (!bp->link_info.link_up)
		rc = bnxt_set_hwrm_link_config(bp, true);
	if (!rc)
		eth_dev->data->dev_link.link_status = 1;

	bnxt_print_link_info(eth_dev);
	return rc;
}

static int bnxt_dev_set_link_down_op(struct rte_eth_dev *eth_dev)
{
	struct bnxt *bp = (struct bnxt *)eth_dev->data->dev_private;

	eth_dev->data->dev_link.link_status = 0;
	bnxt_set_hwrm_link_config(bp, false);
	bp->link_info.link_up = 0;

	return 0;
}

/* Unload the driver, release resources */
static void bnxt_dev_stop_op(struct rte_eth_dev *eth_dev)
{
	struct bnxt *bp = (struct bnxt *)eth_dev->data->dev_private;
	struct rte_intr_handle *intr_handle
		= &bp->pdev->intr_handle;

	if (bp->eth_dev->data->dev_started) {
		/* TBD: STOP HW queues DMA */
		eth_dev->data->dev_link.link_status = 0;
	}
	bnxt_dev_set_link_down_op(eth_dev);
	/* Wait for link to be reset and the async notification to process. */
	rte_delay_ms(BNXT_LINK_WAIT_INTERVAL * 2);

	/* Clean queue intr-vector mapping */
	rte_intr_efd_disable(intr_handle);
	if (intr_handle->intr_vec != NULL) {
		rte_free(intr_handle->intr_vec);
		intr_handle->intr_vec = NULL;
	}

	bnxt_hwrm_port_clr_stats(bp);
	bnxt_free_tx_mbufs(bp);
	bnxt_free_rx_mbufs(bp);
	/* Process any remaining notifications in default completion queue */
	bnxt_int_handler(eth_dev);
	bnxt_shutdown_nic(bp);
	bp->dev_stopped = 1;
}

static void bnxt_dev_close_op(struct rte_eth_dev *eth_dev)
{
	struct bnxt *bp = (struct bnxt *)eth_dev->data->dev_private;

	if (bp->dev_stopped == 0)
		bnxt_dev_stop_op(eth_dev);

	bnxt_free_mem(bp);
	if (eth_dev->data->mac_addrs != NULL) {
		rte_free(eth_dev->data->mac_addrs);
		eth_dev->data->mac_addrs = NULL;
	}
	if (bp->grp_info != NULL) {
		rte_free(bp->grp_info);
		bp->grp_info = NULL;
	}
}

static void bnxt_mac_addr_remove_op(struct rte_eth_dev *eth_dev,
				    uint32_t index)
{
	struct bnxt *bp = (struct bnxt *)eth_dev->data->dev_private;
	uint64_t pool_mask = eth_dev->data->mac_pool_sel[index];
	struct bnxt_vnic_info *vnic;
	struct bnxt_filter_info *filter, *temp_filter;
	uint32_t pool = RTE_MIN(MAX_FF_POOLS, ETH_64_POOLS);
	uint32_t i;

	/*
	 * Loop through all VNICs from the specified filter flow pools to
	 * remove the corresponding MAC addr filter
	 */
	for (i = 0; i < pool; i++) {
		if (!(pool_mask & (1ULL << i)))
			continue;

		STAILQ_FOREACH(vnic, &bp->ff_pool[i], next) {
			filter = STAILQ_FIRST(&vnic->filter);
			while (filter) {
				temp_filter = STAILQ_NEXT(filter, next);
				if (filter->mac_index == index) {
					STAILQ_REMOVE(&vnic->filter, filter,
						      bnxt_filter_info, next);
					bnxt_hwrm_clear_l2_filter(bp, filter);
					filter->mac_index = INVALID_MAC_INDEX;
					memset(&filter->l2_addr, 0,
					       ETHER_ADDR_LEN);
					STAILQ_INSERT_TAIL(
							&bp->free_filter_list,
							filter, next);
				}
				filter = temp_filter;
			}
		}
	}
}

static int bnxt_mac_addr_add_op(struct rte_eth_dev *eth_dev,
				struct ether_addr *mac_addr,
				uint32_t index, uint32_t pool)
{
	struct bnxt *bp = (struct bnxt *)eth_dev->data->dev_private;
	struct bnxt_vnic_info *vnic = STAILQ_FIRST(&bp->ff_pool[pool]);
	struct bnxt_filter_info *filter;

	if (BNXT_VF(bp)) {
		RTE_LOG(ERR, PMD, "Cannot add MAC address to a VF interface\n");
		return -ENOTSUP;
	}

	if (!vnic) {
		RTE_LOG(ERR, PMD, "VNIC not found for pool %d!\n", pool);
		return -EINVAL;
	}
	/* Attach requested MAC address to the new l2_filter */
	STAILQ_FOREACH(filter, &vnic->filter, next) {
		if (filter->mac_index == index) {
			RTE_LOG(DEBUG, PMD,
				"MAC addr already existed for pool %d\n", pool);
			return 0;
		}
	}
	filter = bnxt_alloc_filter(bp);
	if (!filter) {
		RTE_LOG(ERR, PMD, "L2 filter alloc failed\n");
		return -ENODEV;
	}
	STAILQ_INSERT_TAIL(&vnic->filter, filter, next);
	filter->mac_index = index;
	memcpy(filter->l2_addr, mac_addr, ETHER_ADDR_LEN);
	return bnxt_hwrm_set_l2_filter(bp, vnic->fw_vnic_id, filter);
}

int bnxt_link_update_op(struct rte_eth_dev *eth_dev, int wait_to_complete)
{
	int rc = 0;
	struct bnxt *bp = (struct bnxt *)eth_dev->data->dev_private;
	struct rte_eth_link new;
	unsigned int cnt = BNXT_LINK_WAIT_CNT;

	memset(&new, 0, sizeof(new));
	do {
		/* Retrieve link info from hardware */
		rc = bnxt_get_hwrm_link_config(bp, &new);
		if (rc) {
			new.link_speed = ETH_LINK_SPEED_100M;
			new.link_duplex = ETH_LINK_FULL_DUPLEX;
			RTE_LOG(ERR, PMD,
				"Failed to retrieve link rc = 0x%x!\n", rc);
			goto out;
		}
		rte_delay_ms(BNXT_LINK_WAIT_INTERVAL);

		if (!wait_to_complete)
			break;
	} while (!new.link_status && cnt--);

out:
	/* Timed out or success */
	if (new.link_status != eth_dev->data->dev_link.link_status ||
	new.link_speed != eth_dev->data->dev_link.link_speed) {
		memcpy(&eth_dev->data->dev_link, &new,
			sizeof(struct rte_eth_link));
		bnxt_print_link_info(eth_dev);
	}

	return rc;
}

static void bnxt_promiscuous_enable_op(struct rte_eth_dev *eth_dev)
{
	struct bnxt *bp = (struct bnxt *)eth_dev->data->dev_private;
	struct bnxt_vnic_info *vnic;

	if (bp->vnic_info == NULL)
		return;

	vnic = &bp->vnic_info[0];

	vnic->flags |= BNXT_VNIC_INFO_PROMISC;
	bnxt_hwrm_cfa_l2_set_rx_mask(bp, vnic, 0, NULL);
}

static void bnxt_promiscuous_disable_op(struct rte_eth_dev *eth_dev)
{
	struct bnxt *bp = (struct bnxt *)eth_dev->data->dev_private;
	struct bnxt_vnic_info *vnic;

	if (bp->vnic_info == NULL)
		return;

	vnic = &bp->vnic_info[0];

	vnic->flags &= ~BNXT_VNIC_INFO_PROMISC;
	bnxt_hwrm_cfa_l2_set_rx_mask(bp, vnic, 0, NULL);
}

static void bnxt_allmulticast_enable_op(struct rte_eth_dev *eth_dev)
{
	struct bnxt *bp = (struct bnxt *)eth_dev->data->dev_private;
	struct bnxt_vnic_info *vnic;

	if (bp->vnic_info == NULL)
		return;

	vnic = &bp->vnic_info[0];

	vnic->flags |= BNXT_VNIC_INFO_ALLMULTI;
	bnxt_hwrm_cfa_l2_set_rx_mask(bp, vnic, 0, NULL);
}

static void bnxt_allmulticast_disable_op(struct rte_eth_dev *eth_dev)
{
	struct bnxt *bp = (struct bnxt *)eth_dev->data->dev_private;
	struct bnxt_vnic_info *vnic;

	if (bp->vnic_info == NULL)
		return;

	vnic = &bp->vnic_info[0];

	vnic->flags &= ~BNXT_VNIC_INFO_ALLMULTI;
	bnxt_hwrm_cfa_l2_set_rx_mask(bp, vnic, 0, NULL);
}

static int bnxt_reta_update_op(struct rte_eth_dev *eth_dev,
			    struct rte_eth_rss_reta_entry64 *reta_conf,
			    uint16_t reta_size)
{
	struct bnxt *bp = (struct bnxt *)eth_dev->data->dev_private;
	struct rte_eth_conf *dev_conf = &bp->eth_dev->data->dev_conf;
	struct bnxt_vnic_info *vnic;
	int i;

	if (!(dev_conf->rxmode.mq_mode & ETH_MQ_RX_RSS_FLAG))
		return -EINVAL;

	if (reta_size != HW_HASH_INDEX_SIZE) {
		RTE_LOG(ERR, PMD, "The configured hash table lookup size "
			"(%d) must equal the size supported by the hardware "
			"(%d)\n", reta_size, HW_HASH_INDEX_SIZE);
		return -EINVAL;
	}
	/* Update the RSS VNIC(s) */
	for (i = 0; i < MAX_FF_POOLS; i++) {
		STAILQ_FOREACH(vnic, &bp->ff_pool[i], next) {
			memcpy(vnic->rss_table, reta_conf, reta_size);

			bnxt_hwrm_vnic_rss_cfg(bp, vnic);
		}
	}
	return 0;
}

static int bnxt_reta_query_op(struct rte_eth_dev *eth_dev,
			      struct rte_eth_rss_reta_entry64 *reta_conf,
			      uint16_t reta_size)
{
	struct bnxt *bp = (struct bnxt *)eth_dev->data->dev_private;
	struct bnxt_vnic_info *vnic = &bp->vnic_info[0];
	struct rte_intr_handle *intr_handle
		= &bp->pdev->intr_handle;

	/* Retrieve from the default VNIC */
	if (!vnic)
		return -EINVAL;
	if (!vnic->rss_table)
		return -EINVAL;

	if (reta_size != HW_HASH_INDEX_SIZE) {
		RTE_LOG(ERR, PMD, "The configured hash table lookup size "
			"(%d) must equal the size supported by the hardware "
			"(%d)\n", reta_size, HW_HASH_INDEX_SIZE);
		return -EINVAL;
	}
	/* EW - need to revisit here copying from u64 to u16 */
	memcpy(reta_conf, vnic->rss_table, reta_size);

	if (rte_intr_allow_others(intr_handle)) {
		if (eth_dev->data->dev_conf.intr_conf.lsc != 0)
			bnxt_dev_lsc_intr_setup(eth_dev);
	}

	return 0;
}

static int bnxt_rss_hash_update_op(struct rte_eth_dev *eth_dev,
				   struct rte_eth_rss_conf *rss_conf)
{
	struct bnxt *bp = (struct bnxt *)eth_dev->data->dev_private;
	struct rte_eth_conf *dev_conf = &bp->eth_dev->data->dev_conf;
	struct bnxt_vnic_info *vnic;
	uint16_t hash_type = 0;
	int i;

	/*
	 * If RSS enablement were different than dev_configure,
	 * then return -EINVAL
	 */
	if (dev_conf->rxmode.mq_mode & ETH_MQ_RX_RSS_FLAG) {
		if (!rss_conf->rss_hf)
			RTE_LOG(ERR, PMD, "Hash type NONE\n");
	} else {
		if (rss_conf->rss_hf & BNXT_ETH_RSS_SUPPORT)
			return -EINVAL;
	}

	bp->flags |= BNXT_FLAG_UPDATE_HASH;
	memcpy(&bp->rss_conf, rss_conf, sizeof(*rss_conf));

	if (rss_conf->rss_hf & ETH_RSS_IPV4)
		hash_type |= HWRM_VNIC_RSS_CFG_INPUT_HASH_TYPE_IPV4;
	if (rss_conf->rss_hf & ETH_RSS_NONFRAG_IPV4_TCP)
		hash_type |= HWRM_VNIC_RSS_CFG_INPUT_HASH_TYPE_TCP_IPV4;
	if (rss_conf->rss_hf & ETH_RSS_NONFRAG_IPV4_UDP)
		hash_type |= HWRM_VNIC_RSS_CFG_INPUT_HASH_TYPE_UDP_IPV4;
	if (rss_conf->rss_hf & ETH_RSS_IPV6)
		hash_type |= HWRM_VNIC_RSS_CFG_INPUT_HASH_TYPE_IPV6;
	if (rss_conf->rss_hf & ETH_RSS_NONFRAG_IPV6_TCP)
		hash_type |= HWRM_VNIC_RSS_CFG_INPUT_HASH_TYPE_TCP_IPV6;
	if (rss_conf->rss_hf & ETH_RSS_NONFRAG_IPV6_UDP)
		hash_type |= HWRM_VNIC_RSS_CFG_INPUT_HASH_TYPE_UDP_IPV6;

	/* Update the RSS VNIC(s) */
	for (i = 0; i < MAX_FF_POOLS; i++) {
		STAILQ_FOREACH(vnic, &bp->ff_pool[i], next) {
			vnic->hash_type = hash_type;

			/*
			 * Use the supplied key if the key length is
			 * acceptable and the rss_key is not NULL
			 */
			if (rss_conf->rss_key &&
			    rss_conf->rss_key_len <= HW_HASH_KEY_SIZE)
				memcpy(vnic->rss_hash_key, rss_conf->rss_key,
				       rss_conf->rss_key_len);

			bnxt_hwrm_vnic_rss_cfg(bp, vnic);
		}
	}
	return 0;
}

static int bnxt_rss_hash_conf_get_op(struct rte_eth_dev *eth_dev,
				     struct rte_eth_rss_conf *rss_conf)
{
	struct bnxt *bp = (struct bnxt *)eth_dev->data->dev_private;
	struct bnxt_vnic_info *vnic = &bp->vnic_info[0];
	int len;
	uint32_t hash_types;

	/* RSS configuration is the same for all VNICs */
	if (vnic && vnic->rss_hash_key) {
		if (rss_conf->rss_key) {
			len = rss_conf->rss_key_len <= HW_HASH_KEY_SIZE ?
			      rss_conf->rss_key_len : HW_HASH_KEY_SIZE;
			memcpy(rss_conf->rss_key, vnic->rss_hash_key, len);
		}

		hash_types = vnic->hash_type;
		rss_conf->rss_hf = 0;
		if (hash_types & HWRM_VNIC_RSS_CFG_INPUT_HASH_TYPE_IPV4) {
			rss_conf->rss_hf |= ETH_RSS_IPV4;
			hash_types &= ~HWRM_VNIC_RSS_CFG_INPUT_HASH_TYPE_IPV4;
		}
		if (hash_types & HWRM_VNIC_RSS_CFG_INPUT_HASH_TYPE_TCP_IPV4) {
			rss_conf->rss_hf |= ETH_RSS_NONFRAG_IPV4_TCP;
			hash_types &=
				~HWRM_VNIC_RSS_CFG_INPUT_HASH_TYPE_TCP_IPV4;
		}
		if (hash_types & HWRM_VNIC_RSS_CFG_INPUT_HASH_TYPE_UDP_IPV4) {
			rss_conf->rss_hf |= ETH_RSS_NONFRAG_IPV4_UDP;
			hash_types &=
				~HWRM_VNIC_RSS_CFG_INPUT_HASH_TYPE_UDP_IPV4;
		}
		if (hash_types & HWRM_VNIC_RSS_CFG_INPUT_HASH_TYPE_IPV6) {
			rss_conf->rss_hf |= ETH_RSS_IPV6;
			hash_types &= ~HWRM_VNIC_RSS_CFG_INPUT_HASH_TYPE_IPV6;
		}
		if (hash_types & HWRM_VNIC_RSS_CFG_INPUT_HASH_TYPE_TCP_IPV6) {
			rss_conf->rss_hf |= ETH_RSS_NONFRAG_IPV6_TCP;
			hash_types &=
				~HWRM_VNIC_RSS_CFG_INPUT_HASH_TYPE_TCP_IPV6;
		}
		if (hash_types & HWRM_VNIC_RSS_CFG_INPUT_HASH_TYPE_UDP_IPV6) {
			rss_conf->rss_hf |= ETH_RSS_NONFRAG_IPV6_UDP;
			hash_types &=
				~HWRM_VNIC_RSS_CFG_INPUT_HASH_TYPE_UDP_IPV6;
		}
		if (hash_types) {
			RTE_LOG(ERR, PMD,
				"Unknwon RSS config from firmware (%08x), RSS disabled",
				vnic->hash_type);
			return -ENOTSUP;
		}
	} else {
		rss_conf->rss_hf = 0;
	}
	return 0;
}

static int bnxt_flow_ctrl_get_op(struct rte_eth_dev *dev,
			       struct rte_eth_fc_conf *fc_conf)
{
	struct bnxt *bp = (struct bnxt *)dev->data->dev_private;
	struct rte_eth_link link_info;
	int rc;

	rc = bnxt_get_hwrm_link_config(bp, &link_info);
	if (rc)
		return rc;

	memset(fc_conf, 0, sizeof(*fc_conf));
	if (bp->link_info.auto_pause)
		fc_conf->autoneg = 1;
	switch (bp->link_info.pause) {
	case 0:
		fc_conf->mode = RTE_FC_NONE;
		break;
	case HWRM_PORT_PHY_QCFG_OUTPUT_PAUSE_TX:
		fc_conf->mode = RTE_FC_TX_PAUSE;
		break;
	case HWRM_PORT_PHY_QCFG_OUTPUT_PAUSE_RX:
		fc_conf->mode = RTE_FC_RX_PAUSE;
		break;
	case (HWRM_PORT_PHY_QCFG_OUTPUT_PAUSE_TX |
			HWRM_PORT_PHY_QCFG_OUTPUT_PAUSE_RX):
		fc_conf->mode = RTE_FC_FULL;
		break;
	}
	return 0;
}

static int bnxt_flow_ctrl_set_op(struct rte_eth_dev *dev,
			       struct rte_eth_fc_conf *fc_conf)
{
	struct bnxt *bp = (struct bnxt *)dev->data->dev_private;

	if (BNXT_NPAR_PF(bp) || BNXT_VF(bp)) {
		RTE_LOG(ERR, PMD, "Flow Control Settings cannot be modified\n");
		return -ENOTSUP;
	}

	switch (fc_conf->mode) {
	case RTE_FC_NONE:
		bp->link_info.auto_pause = 0;
		bp->link_info.force_pause = 0;
		break;
	case RTE_FC_RX_PAUSE:
		if (fc_conf->autoneg) {
			bp->link_info.auto_pause =
					HWRM_PORT_PHY_CFG_INPUT_AUTO_PAUSE_RX;
			bp->link_info.force_pause = 0;
		} else {
			bp->link_info.auto_pause = 0;
			bp->link_info.force_pause =
					HWRM_PORT_PHY_CFG_INPUT_FORCE_PAUSE_RX;
		}
		break;
	case RTE_FC_TX_PAUSE:
		if (fc_conf->autoneg) {
			bp->link_info.auto_pause =
					HWRM_PORT_PHY_CFG_INPUT_AUTO_PAUSE_TX;
			bp->link_info.force_pause = 0;
		} else {
			bp->link_info.auto_pause = 0;
			bp->link_info.force_pause =
					HWRM_PORT_PHY_CFG_INPUT_FORCE_PAUSE_TX;
		}
		break;
	case RTE_FC_FULL:
		if (fc_conf->autoneg) {
			bp->link_info.auto_pause =
					HWRM_PORT_PHY_CFG_INPUT_AUTO_PAUSE_TX |
					HWRM_PORT_PHY_CFG_INPUT_AUTO_PAUSE_RX;
			bp->link_info.force_pause = 0;
		} else {
			bp->link_info.auto_pause = 0;
			bp->link_info.force_pause =
					HWRM_PORT_PHY_CFG_INPUT_FORCE_PAUSE_TX |
					HWRM_PORT_PHY_CFG_INPUT_FORCE_PAUSE_RX;
		}
		break;
	}
	return bnxt_set_hwrm_link_config(bp, true);
}

/* Add UDP tunneling port */
static int
bnxt_udp_tunnel_port_add_op(struct rte_eth_dev *eth_dev,
			 struct rte_eth_udp_tunnel *udp_tunnel)
{
	struct bnxt *bp = (struct bnxt *)eth_dev->data->dev_private;
	uint16_t tunnel_type = 0;
	int rc = 0;

	switch (udp_tunnel->prot_type) {
	case RTE_TUNNEL_TYPE_VXLAN:
		if (bp->vxlan_port_cnt) {
			RTE_LOG(ERR, PMD, "Tunnel Port %d already programmed\n",
				udp_tunnel->udp_port);
			if (bp->vxlan_port != udp_tunnel->udp_port) {
				RTE_LOG(ERR, PMD, "Only one port allowed\n");
				return -ENOSPC;
			}
			bp->vxlan_port_cnt++;
			return 0;
		}
		tunnel_type =
			HWRM_TUNNEL_DST_PORT_ALLOC_INPUT_TUNNEL_TYPE_VXLAN;
		bp->vxlan_port_cnt++;
		break;
	case RTE_TUNNEL_TYPE_GENEVE:
		if (bp->geneve_port_cnt) {
			RTE_LOG(ERR, PMD, "Tunnel Port %d already programmed\n",
				udp_tunnel->udp_port);
			if (bp->geneve_port != udp_tunnel->udp_port) {
				RTE_LOG(ERR, PMD, "Only one port allowed\n");
				return -ENOSPC;
			}
			bp->geneve_port_cnt++;
			return 0;
		}
		tunnel_type =
			HWRM_TUNNEL_DST_PORT_ALLOC_INPUT_TUNNEL_TYPE_GENEVE;
		bp->geneve_port_cnt++;
		break;
	default:
		RTE_LOG(ERR, PMD, "Tunnel type is not supported\n");
		return -ENOTSUP;
	}
	rc = bnxt_hwrm_tunnel_dst_port_alloc(bp, udp_tunnel->udp_port,
					     tunnel_type);
	return rc;
}

static int
bnxt_udp_tunnel_port_del_op(struct rte_eth_dev *eth_dev,
			 struct rte_eth_udp_tunnel *udp_tunnel)
{
	struct bnxt *bp = (struct bnxt *)eth_dev->data->dev_private;
	uint16_t tunnel_type = 0;
	uint16_t port = 0;
	int rc = 0;

	switch (udp_tunnel->prot_type) {
	case RTE_TUNNEL_TYPE_VXLAN:
		if (!bp->vxlan_port_cnt) {
			RTE_LOG(ERR, PMD, "No Tunnel port configured yet\n");
			return -EINVAL;
		}
		if (bp->vxlan_port != udp_tunnel->udp_port) {
			RTE_LOG(ERR, PMD, "Req Port: %d. Configured port: %d\n",
				udp_tunnel->udp_port, bp->vxlan_port);
			return -EINVAL;
		}
		if (--bp->vxlan_port_cnt)
			return 0;

		tunnel_type =
			HWRM_TUNNEL_DST_PORT_FREE_INPUT_TUNNEL_TYPE_VXLAN;
		port = bp->vxlan_fw_dst_port_id;
		break;
	case RTE_TUNNEL_TYPE_GENEVE:
		if (!bp->geneve_port_cnt) {
			RTE_LOG(ERR, PMD, "No Tunnel port configured yet\n");
			return -EINVAL;
		}
		if (bp->geneve_port != udp_tunnel->udp_port) {
			RTE_LOG(ERR, PMD, "Req Port: %d. Configured port: %d\n",
				udp_tunnel->udp_port, bp->geneve_port);
			return -EINVAL;
		}
		if (--bp->geneve_port_cnt)
			return 0;

		tunnel_type =
			HWRM_TUNNEL_DST_PORT_FREE_INPUT_TUNNEL_TYPE_GENEVE;
		port = bp->geneve_fw_dst_port_id;
		break;
	default:
		RTE_LOG(ERR, PMD, "Tunnel type is not supported\n");
		return -ENOTSUP;
	}

	rc = bnxt_hwrm_tunnel_dst_port_free(bp, port, tunnel_type);
	if (!rc) {
		if (tunnel_type ==
		    HWRM_TUNNEL_DST_PORT_FREE_INPUT_TUNNEL_TYPE_VXLAN)
			bp->vxlan_port = 0;
		if (tunnel_type ==
		    HWRM_TUNNEL_DST_PORT_FREE_INPUT_TUNNEL_TYPE_GENEVE)
			bp->geneve_port = 0;
	}
	return rc;
}

static int bnxt_del_vlan_filter(struct bnxt *bp, uint16_t vlan_id)
{
	struct bnxt_filter_info *filter, *temp_filter, *new_filter;
	struct bnxt_vnic_info *vnic;
	unsigned int i;
	int rc = 0;
	uint32_t chk = HWRM_CFA_L2_FILTER_ALLOC_INPUT_ENABLES_L2_OVLAN;

	/* Cycle through all VNICs */
	for (i = 0; i < bp->nr_vnics; i++) {
		/*
		 * For each VNIC and each associated filter(s)
		 * if VLAN exists && VLAN matches vlan_id
		 *      remove the MAC+VLAN filter
		 *      add a new MAC only filter
		 * else
		 *      VLAN filter doesn't exist, just skip and continue
		 */
		STAILQ_FOREACH(vnic, &bp->ff_pool[i], next) {
			filter = STAILQ_FIRST(&vnic->filter);
			while (filter) {
				temp_filter = STAILQ_NEXT(filter, next);

				if (filter->enables & chk &&
				    filter->l2_ovlan == vlan_id) {
					/* Must delete the filter */
					STAILQ_REMOVE(&vnic->filter, filter,
						      bnxt_filter_info, next);
					bnxt_hwrm_clear_l2_filter(bp, filter);
					STAILQ_INSERT_TAIL(
							&bp->free_filter_list,
							filter, next);

					/*
					 * Need to examine to see if the MAC
					 * filter already existed or not before
					 * allocating a new one
					 */

					new_filter = bnxt_alloc_filter(bp);
					if (!new_filter) {
						RTE_LOG(ERR, PMD,
							"MAC/VLAN filter alloc failed\n");
						rc = -ENOMEM;
						goto exit;
					}
					STAILQ_INSERT_TAIL(&vnic->filter,
							   new_filter, next);
					/* Inherit MAC from previous filter */
					new_filter->mac_index =
							filter->mac_index;
					memcpy(new_filter->l2_addr,
					       filter->l2_addr, ETHER_ADDR_LEN);
					/* MAC only filter */
					rc = bnxt_hwrm_set_l2_filter(bp,
							vnic->fw_vnic_id,
							new_filter);
					if (rc)
						goto exit;
					RTE_LOG(INFO, PMD,
						"Del Vlan filter for %d\n",
						vlan_id);
				}
				filter = temp_filter;
			}
		}
	}
exit:
	return rc;
}

static int bnxt_add_vlan_filter(struct bnxt *bp, uint16_t vlan_id)
{
	struct bnxt_filter_info *filter, *temp_filter, *new_filter;
	struct bnxt_vnic_info *vnic;
	unsigned int i;
	int rc = 0;
	uint32_t en = HWRM_CFA_L2_FILTER_ALLOC_INPUT_ENABLES_L2_IVLAN |
		HWRM_CFA_L2_FILTER_ALLOC_INPUT_ENABLES_L2_IVLAN_MASK;
	uint32_t chk = HWRM_CFA_L2_FILTER_ALLOC_INPUT_ENABLES_L2_IVLAN;

	/* Cycle through all VNICs */
	for (i = 0; i < bp->nr_vnics; i++) {
		/*
		 * For each VNIC and each associated filter(s)
		 * if VLAN exists:
		 *   if VLAN matches vlan_id
		 *      VLAN filter already exists, just skip and continue
		 *   else
		 *      add a new MAC+VLAN filter
		 * else
		 *   Remove the old MAC only filter
		 *    Add a new MAC+VLAN filter
		 */
		STAILQ_FOREACH(vnic, &bp->ff_pool[i], next) {
			filter = STAILQ_FIRST(&vnic->filter);
			while (filter) {
				temp_filter = STAILQ_NEXT(filter, next);

				if (filter->enables & chk) {
					if (filter->l2_ovlan == vlan_id)
						goto cont;
				} else {
					/* Must delete the MAC filter */
					STAILQ_REMOVE(&vnic->filter, filter,
						      bnxt_filter_info, next);
					bnxt_hwrm_clear_l2_filter(bp, filter);
					filter->l2_ovlan = 0;
					STAILQ_INSERT_TAIL(
							&bp->free_filter_list,
							filter, next);
				}
				new_filter = bnxt_alloc_filter(bp);
				if (!new_filter) {
					RTE_LOG(ERR, PMD,
						"MAC/VLAN filter alloc failed\n");
					rc = -ENOMEM;
					goto exit;
				}
				STAILQ_INSERT_TAIL(&vnic->filter, new_filter,
						   next);
				/* Inherit MAC from the previous filter */
				new_filter->mac_index = filter->mac_index;
				memcpy(new_filter->l2_addr, filter->l2_addr,
				       ETHER_ADDR_LEN);
				/* MAC + VLAN ID filter */
				new_filter->l2_ivlan = vlan_id;
				new_filter->l2_ivlan_mask = 0xF000;
				new_filter->enables |= en;
				rc = bnxt_hwrm_set_l2_filter(bp,
							     vnic->fw_vnic_id,
							     new_filter);
				if (rc)
					goto exit;
				RTE_LOG(INFO, PMD,
					"Added Vlan filter for %d\n", vlan_id);
cont:
				filter = temp_filter;
			}
		}
	}
exit:
	return rc;
}

static int bnxt_vlan_filter_set_op(struct rte_eth_dev *eth_dev,
				   uint16_t vlan_id, int on)
{
	struct bnxt *bp = (struct bnxt *)eth_dev->data->dev_private;

	/* These operations apply to ALL existing MAC/VLAN filters */
	if (on)
		return bnxt_add_vlan_filter(bp, vlan_id);
	else
		return bnxt_del_vlan_filter(bp, vlan_id);
}

static int
bnxt_vlan_offload_set_op(struct rte_eth_dev *dev, int mask)
{
	struct bnxt *bp = (struct bnxt *)dev->data->dev_private;
	unsigned int i;

	if (mask & ETH_VLAN_FILTER_MASK) {
		if (!dev->data->dev_conf.rxmode.hw_vlan_filter) {
			/* Remove any VLAN filters programmed */
			for (i = 0; i < 4095; i++)
				bnxt_del_vlan_filter(bp, i);
		}
		RTE_LOG(INFO, PMD, "VLAN Filtering: %d\n",
			dev->data->dev_conf.rxmode.hw_vlan_filter);
	}

	if (mask & ETH_VLAN_STRIP_MASK) {
		/* Enable or disable VLAN stripping */
		for (i = 0; i < bp->nr_vnics; i++) {
			struct bnxt_vnic_info *vnic = &bp->vnic_info[i];
			if (dev->data->dev_conf.rxmode.hw_vlan_strip)
				vnic->vlan_strip = true;
			else
				vnic->vlan_strip = false;
			bnxt_hwrm_vnic_cfg(bp, vnic);
		}
		RTE_LOG(INFO, PMD, "VLAN Strip Offload: %d\n",
			dev->data->dev_conf.rxmode.hw_vlan_strip);
	}

	if (mask & ETH_VLAN_EXTEND_MASK)
		RTE_LOG(ERR, PMD, "Extend VLAN Not supported\n");

	return 0;
}

static void
bnxt_set_default_mac_addr_op(struct rte_eth_dev *dev, struct ether_addr *addr)
{
	struct bnxt *bp = (struct bnxt *)dev->data->dev_private;
	/* Default Filter is tied to VNIC 0 */
	struct bnxt_vnic_info *vnic = &bp->vnic_info[0];
	struct bnxt_filter_info *filter;
	int rc;

	if (BNXT_VF(bp))
		return;

	memcpy(bp->mac_addr, addr, sizeof(bp->mac_addr));
	memcpy(&dev->data->mac_addrs[0], bp->mac_addr, ETHER_ADDR_LEN);

	STAILQ_FOREACH(filter, &vnic->filter, next) {
		/* Default Filter is at Index 0 */
		if (filter->mac_index != 0)
			continue;
		rc = bnxt_hwrm_clear_l2_filter(bp, filter);
		if (rc)
			break;
		memcpy(filter->l2_addr, addr, ETHER_ADDR_LEN);
		memset(filter->l2_addr_mask, 0xff, ETHER_ADDR_LEN);
		filter->flags |= HWRM_CFA_L2_FILTER_ALLOC_INPUT_FLAGS_PATH_RX;
		filter->enables |=
			HWRM_CFA_L2_FILTER_ALLOC_INPUT_ENABLES_L2_ADDR |
			HWRM_CFA_L2_FILTER_ALLOC_INPUT_ENABLES_L2_ADDR_MASK;
		rc = bnxt_hwrm_set_l2_filter(bp, vnic->fw_vnic_id, filter);
		if (rc) {
			memcpy(filter->l2_addr, bp->mac_addr,
			       ETHER_ADDR_LEN);
			break;
		}
		filter->mac_index = 0;
		RTE_LOG(DEBUG, PMD, "Set MAC addr\n");
	}
}

static int
bnxt_dev_set_mc_addr_list_op(struct rte_eth_dev *eth_dev,
			  struct ether_addr *mc_addr_set,
			  uint32_t nb_mc_addr)
{
	struct bnxt *bp = (struct bnxt *)eth_dev->data->dev_private;
	char *mc_addr_list = (char *)mc_addr_set;
	struct bnxt_vnic_info *vnic;
	uint32_t off = 0, i = 0;

	vnic = &bp->vnic_info[0];

	if (nb_mc_addr > BNXT_MAX_MC_ADDRS) {
		vnic->flags |= BNXT_VNIC_INFO_ALLMULTI;
		goto allmulti;
	}

	/* TODO Check for Duplicate mcast addresses */
	vnic->flags &= ~BNXT_VNIC_INFO_ALLMULTI;
	for (i = 0; i < nb_mc_addr; i++) {
		memcpy(vnic->mc_list + off, &mc_addr_list[i], ETHER_ADDR_LEN);
		off += ETHER_ADDR_LEN;
	}

	vnic->mc_addr_cnt = i;
	if (vnic->mc_addr_cnt)
		vnic->flags |= BNXT_VNIC_INFO_MCAST;
	else
		vnic->flags &= ~BNXT_VNIC_INFO_MCAST;

allmulti:
	return bnxt_hwrm_cfa_l2_set_rx_mask(bp, vnic, 0, NULL);
}

static int
bnxt_fw_version_get(struct rte_eth_dev *dev, char *fw_version, size_t fw_size)
{
	struct bnxt *bp = (struct bnxt *)dev->data->dev_private;
	uint8_t fw_major = (bp->fw_ver >> 24) & 0xff;
	uint8_t fw_minor = (bp->fw_ver >> 16) & 0xff;
	uint8_t fw_updt = (bp->fw_ver >> 8) & 0xff;
	int ret;

	ret = snprintf(fw_version, fw_size, "%d.%d.%d",
			fw_major, fw_minor, fw_updt);

	ret += 1; /* add the size of '\0' */
	if (fw_size < (uint32_t)ret)
		return ret;
	else
		return 0;
}

static void
bnxt_rxq_info_get_op(struct rte_eth_dev *dev, uint16_t queue_id,
	struct rte_eth_rxq_info *qinfo)
{
	struct bnxt_rx_queue *rxq;

	rxq = dev->data->rx_queues[queue_id];

	qinfo->mp = rxq->mb_pool;
	qinfo->scattered_rx = dev->data->scattered_rx;
	qinfo->nb_desc = rxq->nb_rx_desc;

	qinfo->conf.rx_free_thresh = rxq->rx_free_thresh;
	qinfo->conf.rx_drop_en = 0;
	qinfo->conf.rx_deferred_start = 0;
}

static void
bnxt_txq_info_get_op(struct rte_eth_dev *dev, uint16_t queue_id,
	struct rte_eth_txq_info *qinfo)
{
	struct bnxt_tx_queue *txq;

	txq = dev->data->tx_queues[queue_id];

	qinfo->nb_desc = txq->nb_tx_desc;

	qinfo->conf.tx_thresh.pthresh = txq->pthresh;
	qinfo->conf.tx_thresh.hthresh = txq->hthresh;
	qinfo->conf.tx_thresh.wthresh = txq->wthresh;

	qinfo->conf.tx_free_thresh = txq->tx_free_thresh;
	qinfo->conf.tx_rs_thresh = 0;
	qinfo->conf.txq_flags = txq->txq_flags;
	qinfo->conf.tx_deferred_start = txq->tx_deferred_start;
}

static int bnxt_mtu_set_op(struct rte_eth_dev *eth_dev, uint16_t new_mtu)
{
	struct bnxt *bp = eth_dev->data->dev_private;
	struct rte_eth_dev_info dev_info;
	uint32_t max_dev_mtu;
	uint32_t rc = 0;
	uint32_t i;

	bnxt_dev_info_get_op(eth_dev, &dev_info);
	max_dev_mtu = dev_info.max_rx_pktlen -
		      ETHER_HDR_LEN - ETHER_CRC_LEN - VLAN_TAG_SIZE * 2;

	if (new_mtu < ETHER_MIN_MTU || new_mtu > max_dev_mtu) {
		RTE_LOG(ERR, PMD, "MTU requested must be within (%d, %d)\n",
			ETHER_MIN_MTU, max_dev_mtu);
		return -EINVAL;
	}


	if (new_mtu > ETHER_MTU) {
		bp->flags |= BNXT_FLAG_JUMBO;
		eth_dev->data->dev_conf.rxmode.jumbo_frame = 1;
	} else {
		eth_dev->data->dev_conf.rxmode.jumbo_frame = 0;
		bp->flags &= ~BNXT_FLAG_JUMBO;
	}

	eth_dev->data->dev_conf.rxmode.max_rx_pkt_len =
		new_mtu + ETHER_HDR_LEN + ETHER_CRC_LEN + VLAN_TAG_SIZE * 2;

	eth_dev->data->mtu = new_mtu;
	RTE_LOG(INFO, PMD, "New MTU is %d\n", eth_dev->data->mtu);

	for (i = 0; i < bp->nr_vnics; i++) {
		struct bnxt_vnic_info *vnic = &bp->vnic_info[i];
		uint16_t size = 0;

		vnic->mru = bp->eth_dev->data->mtu + ETHER_HDR_LEN +
					ETHER_CRC_LEN + VLAN_TAG_SIZE * 2;
		rc = bnxt_hwrm_vnic_cfg(bp, vnic);
		if (rc)
			break;

		size = rte_pktmbuf_data_room_size(bp->rx_queues[0]->mb_pool);
		size -= RTE_PKTMBUF_HEADROOM;

		if (size < new_mtu) {
			rc = bnxt_hwrm_vnic_plcmode_cfg(bp, vnic);
			if (rc)
				return rc;
		}
	}

	return rc;
}

static int
bnxt_vlan_pvid_set_op(struct rte_eth_dev *dev, uint16_t pvid, int on)
{
	struct bnxt *bp = (struct bnxt *)dev->data->dev_private;
	uint16_t vlan = bp->vlan;
	int rc;

	if (BNXT_NPAR_PF(bp) || BNXT_VF(bp)) {
		RTE_LOG(ERR, PMD,
			"PVID cannot be modified for this function\n");
		return -ENOTSUP;
	}
	bp->vlan = on ? pvid : 0;

	rc = bnxt_hwrm_set_default_vlan(bp, 0, 0);
	if (rc)
		bp->vlan = vlan;
	return rc;
}

static int
bnxt_dev_led_on_op(struct rte_eth_dev *dev)
{
	struct bnxt *bp = (struct bnxt *)dev->data->dev_private;

	return bnxt_hwrm_port_led_cfg(bp, true);
}

static int
bnxt_dev_led_off_op(struct rte_eth_dev *dev)
{
	struct bnxt *bp = (struct bnxt *)dev->data->dev_private;

	return bnxt_hwrm_port_led_cfg(bp, false);
}

static uint32_t
bnxt_rx_queue_count_op(struct rte_eth_dev *dev, uint16_t rx_queue_id)
{
	uint32_t desc = 0, raw_cons = 0, cons;
	struct bnxt_cp_ring_info *cpr;
	struct bnxt_rx_queue *rxq;
	struct rx_pkt_cmpl *rxcmp;

	rxq = dev->data->rx_queues[rx_queue_id];
	cpr = rxq->cp_ring;
	raw_cons = cpr->cp_raw_cons;

	while (1) {
		cons = RING_CMP(cpr->cp_ring_struct, raw_cons);
		rte_prefetch0(&cpr->cp_desc_ring[cons]);
		rxcmp = (struct rx_pkt_cmpl *)&cpr->cp_desc_ring[cons];

		if (!CMP_VALID(rxcmp, raw_cons, cpr->cp_ring_struct)) {
			break;
		} else {
			raw_cons++;
			desc++;
		}
	}

	return desc;
}

static int
bnxt_rx_descriptor_status_op(void *rx_queue, uint16_t offset)
{
	struct bnxt_rx_queue *rxq = (struct bnxt_rx_queue *)rx_queue;
	struct bnxt_rx_ring_info *rxr;
	struct bnxt_cp_ring_info *cpr;
	struct bnxt_sw_rx_bd *rx_buf;
	struct rx_pkt_cmpl *rxcmp;
	uint32_t cons, cp_cons;

	if (!rxq)
		return -EINVAL;

	cpr = rxq->cp_ring;
	rxr = rxq->rx_ring;

	if (offset >= rxq->nb_rx_desc)
		return -EINVAL;

	cons = RING_CMP(cpr->cp_ring_struct, offset);
	cp_cons = cpr->cp_raw_cons;
	rxcmp = (struct rx_pkt_cmpl *)&cpr->cp_desc_ring[cons];

	if (cons > cp_cons) {
		if (CMPL_VALID(rxcmp, cpr->valid))
			return RTE_ETH_RX_DESC_DONE;
	} else {
		if (CMPL_VALID(rxcmp, !cpr->valid))
			return RTE_ETH_RX_DESC_DONE;
	}
	rx_buf = &rxr->rx_buf_ring[cons];
	if (rx_buf->mbuf == NULL)
		return RTE_ETH_RX_DESC_UNAVAIL;


	return RTE_ETH_RX_DESC_AVAIL;
}

static int
bnxt_tx_descriptor_status_op(void *tx_queue, uint16_t offset)
{
	struct bnxt_tx_queue *txq = (struct bnxt_tx_queue *)tx_queue;
	struct bnxt_tx_ring_info *txr;
	struct bnxt_cp_ring_info *cpr;
	struct bnxt_sw_tx_bd *tx_buf;
	struct tx_pkt_cmpl *txcmp;
	uint32_t cons, cp_cons;

	if (!txq)
		return -EINVAL;

	cpr = txq->cp_ring;
	txr = txq->tx_ring;

	if (offset >= txq->nb_tx_desc)
		return -EINVAL;

	cons = RING_CMP(cpr->cp_ring_struct, offset);
	txcmp = (struct tx_pkt_cmpl *)&cpr->cp_desc_ring[cons];
	cp_cons = cpr->cp_raw_cons;

	if (cons > cp_cons) {
		if (CMPL_VALID(txcmp, cpr->valid))
			return RTE_ETH_TX_DESC_UNAVAIL;
	} else {
		if (CMPL_VALID(txcmp, !cpr->valid))
			return RTE_ETH_TX_DESC_UNAVAIL;
	}
	tx_buf = &txr->tx_buf_ring[cons];
	if (tx_buf->mbuf == NULL)
		return RTE_ETH_TX_DESC_DONE;

	return RTE_ETH_TX_DESC_FULL;
}

static struct bnxt_filter_info *
bnxt_match_and_validate_ether_filter(struct bnxt *bp,
				struct rte_eth_ethertype_filter *efilter,
				struct bnxt_vnic_info *vnic0,
				struct bnxt_vnic_info *vnic,
				int *ret)
{
	struct bnxt_filter_info *mfilter = NULL;
	int match = 0;
	*ret = 0;

	if (efilter->ether_type == ETHER_TYPE_IPv4 ||
		efilter->ether_type == ETHER_TYPE_IPv6) {
		RTE_LOG(ERR, PMD, "invalid ether_type(0x%04x) in"
			" ethertype filter.", efilter->ether_type);
		*ret = -EINVAL;
		goto exit;
	}
	if (efilter->queue >= bp->rx_nr_rings) {
		RTE_LOG(ERR, PMD, "Invalid queue %d\n", efilter->queue);
		*ret = -EINVAL;
		goto exit;
	}

	vnic0 = STAILQ_FIRST(&bp->ff_pool[0]);
	vnic = STAILQ_FIRST(&bp->ff_pool[efilter->queue]);
	if (vnic == NULL) {
		RTE_LOG(ERR, PMD, "Invalid queue %d\n", efilter->queue);
		*ret = -EINVAL;
		goto exit;
	}

	if (efilter->flags & RTE_ETHTYPE_FLAGS_DROP) {
		STAILQ_FOREACH(mfilter, &vnic0->filter, next) {
			if ((!memcmp(efilter->mac_addr.addr_bytes,
				     mfilter->l2_addr, ETHER_ADDR_LEN) &&
			     mfilter->flags ==
			     HWRM_CFA_NTUPLE_FILTER_ALLOC_INPUT_FLAGS_DROP &&
			     mfilter->ethertype == efilter->ether_type)) {
				match = 1;
				break;
			}
		}
	} else {
		STAILQ_FOREACH(mfilter, &vnic->filter, next)
			if ((!memcmp(efilter->mac_addr.addr_bytes,
				     mfilter->l2_addr, ETHER_ADDR_LEN) &&
			     mfilter->ethertype == efilter->ether_type &&
			     mfilter->flags ==
			     HWRM_CFA_L2_FILTER_CFG_INPUT_FLAGS_PATH_RX)) {
				match = 1;
				break;
			}
	}

	if (match)
		*ret = -EEXIST;

exit:
	return mfilter;
}

static int
bnxt_ethertype_filter(struct rte_eth_dev *dev,
			enum rte_filter_op filter_op,
			void *arg)
{
	struct bnxt *bp = (struct bnxt *)dev->data->dev_private;
	struct rte_eth_ethertype_filter *efilter =
			(struct rte_eth_ethertype_filter *)arg;
	struct bnxt_filter_info *bfilter, *filter1;
	struct bnxt_vnic_info *vnic, *vnic0;
	int ret;

	if (filter_op == RTE_ETH_FILTER_NOP)
		return 0;

	if (arg == NULL) {
		RTE_LOG(ERR, PMD, "arg shouldn't be NULL for operation %u.",
			    filter_op);
		return -EINVAL;
	}

	vnic0 = STAILQ_FIRST(&bp->ff_pool[0]);
	vnic = STAILQ_FIRST(&bp->ff_pool[efilter->queue]);

	switch (filter_op) {
	case RTE_ETH_FILTER_ADD:
		bnxt_match_and_validate_ether_filter(bp, efilter,
							vnic0, vnic, &ret);
		if (ret < 0)
			return ret;

		bfilter = bnxt_get_unused_filter(bp);
		if (bfilter == NULL) {
			RTE_LOG(ERR, PMD,
				"Not enough resources for a new filter.\n");
			return -ENOMEM;
		}
		bfilter->filter_type = HWRM_CFA_NTUPLE_FILTER;
		memcpy(bfilter->l2_addr, efilter->mac_addr.addr_bytes,
		       ETHER_ADDR_LEN);
		memcpy(bfilter->dst_macaddr, efilter->mac_addr.addr_bytes,
		       ETHER_ADDR_LEN);
		bfilter->enables |= NTUPLE_FLTR_ALLOC_INPUT_EN_DST_MACADDR;
		bfilter->ethertype = efilter->ether_type;
		bfilter->enables |= NTUPLE_FLTR_ALLOC_INPUT_EN_ETHERTYPE;

		filter1 = bnxt_get_l2_filter(bp, bfilter, vnic0);
		if (filter1 == NULL) {
			ret = -1;
			goto cleanup;
		}
		bfilter->enables |=
			HWRM_CFA_NTUPLE_FILTER_ALLOC_INPUT_ENABLES_L2_FILTER_ID;
		bfilter->fw_l2_filter_id = filter1->fw_l2_filter_id;

		bfilter->dst_id = vnic->fw_vnic_id;

		if (efilter->flags & RTE_ETHTYPE_FLAGS_DROP) {
			bfilter->flags =
				HWRM_CFA_NTUPLE_FILTER_ALLOC_INPUT_FLAGS_DROP;
		}

		ret = bnxt_hwrm_set_ntuple_filter(bp, bfilter->dst_id, bfilter);
		if (ret)
			goto cleanup;
		STAILQ_INSERT_TAIL(&vnic->filter, bfilter, next);
		break;
	case RTE_ETH_FILTER_DELETE:
		filter1 = bnxt_match_and_validate_ether_filter(bp, efilter,
							vnic0, vnic, &ret);
		if (ret == -EEXIST) {
			ret = bnxt_hwrm_clear_ntuple_filter(bp, filter1);

			STAILQ_REMOVE(&vnic->filter, filter1, bnxt_filter_info,
				      next);
			bnxt_free_filter(bp, filter1);
		} else if (ret == 0) {
			RTE_LOG(ERR, PMD, "No matching filter found\n");
		}
		break;
	default:
		RTE_LOG(ERR, PMD, "unsupported operation %u.", filter_op);
		ret = -EINVAL;
		goto error;
	}
	return ret;
cleanup:
	bnxt_free_filter(bp, bfilter);
error:
	return ret;
}

static inline int
parse_ntuple_filter(struct bnxt *bp,
		    struct rte_eth_ntuple_filter *nfilter,
		    struct bnxt_filter_info *bfilter)
{
	uint32_t en = 0;

	if (nfilter->queue >= bp->rx_nr_rings) {
		RTE_LOG(ERR, PMD, "Invalid queue %d\n", nfilter->queue);
		return -EINVAL;
	}

	switch (nfilter->dst_port_mask) {
	case UINT16_MAX:
		bfilter->dst_port_mask = -1;
		bfilter->dst_port = nfilter->dst_port;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_DST_PORT |
			NTUPLE_FLTR_ALLOC_INPUT_EN_DST_PORT_MASK;
		break;
	default:
		RTE_LOG(ERR, PMD, "invalid dst_port mask.");
		return -EINVAL;
	}

	bfilter->ip_addr_type = NTUPLE_FLTR_ALLOC_INPUT_IP_ADDR_TYPE_IPV4;
	en |= NTUPLE_FLTR_ALLOC_IN_EN_IP_PROTO;

	switch (nfilter->proto_mask) {
	case UINT8_MAX:
		if (nfilter->proto == 17) /* IPPROTO_UDP */
			bfilter->ip_protocol = 17;
		else if (nfilter->proto == 6) /* IPPROTO_TCP */
			bfilter->ip_protocol = 6;
		else
			return -EINVAL;
		en |= NTUPLE_FLTR_ALLOC_IN_EN_IP_PROTO;
		break;
	default:
		RTE_LOG(ERR, PMD, "invalid protocol mask.");
		return -EINVAL;
	}

	switch (nfilter->dst_ip_mask) {
	case UINT32_MAX:
		bfilter->dst_ipaddr_mask[0] = -1;
		bfilter->dst_ipaddr[0] = nfilter->dst_ip;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_DST_IPADDR |
			NTUPLE_FLTR_ALLOC_INPUT_EN_DST_IPADDR_MASK;
		break;
	default:
		RTE_LOG(ERR, PMD, "invalid dst_ip mask.");
		return -EINVAL;
	}

	switch (nfilter->src_ip_mask) {
	case UINT32_MAX:
		bfilter->src_ipaddr_mask[0] = -1;
		bfilter->src_ipaddr[0] = nfilter->src_ip;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_SRC_IPADDR |
			NTUPLE_FLTR_ALLOC_INPUT_EN_SRC_IPADDR_MASK;
		break;
	default:
		RTE_LOG(ERR, PMD, "invalid src_ip mask.");
		return -EINVAL;
	}

	switch (nfilter->src_port_mask) {
	case UINT16_MAX:
		bfilter->src_port_mask = -1;
		bfilter->src_port = nfilter->src_port;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_SRC_PORT |
			NTUPLE_FLTR_ALLOC_INPUT_EN_SRC_PORT_MASK;
		break;
	default:
		RTE_LOG(ERR, PMD, "invalid src_port mask.");
		return -EINVAL;
	}

	bfilter->enables = en;
	return 0;
}

static struct bnxt_filter_info*
bnxt_match_ntuple_filter(struct bnxt *bp,
			 struct bnxt_filter_info *bfilter,
			 struct bnxt_vnic_info **mvnic)
{
	struct bnxt_filter_info *mfilter = NULL;
	int i;

	for (i = bp->nr_vnics - 1; i >= 0; i--) {
		struct bnxt_vnic_info *vnic = &bp->vnic_info[i];
		STAILQ_FOREACH(mfilter, &vnic->filter, next) {
			if (bfilter->src_ipaddr[0] == mfilter->src_ipaddr[0] &&
			    bfilter->src_ipaddr_mask[0] ==
			    mfilter->src_ipaddr_mask[0] &&
			    bfilter->src_port == mfilter->src_port &&
			    bfilter->src_port_mask == mfilter->src_port_mask &&
			    bfilter->dst_ipaddr[0] == mfilter->dst_ipaddr[0] &&
			    bfilter->dst_ipaddr_mask[0] ==
			    mfilter->dst_ipaddr_mask[0] &&
			    bfilter->dst_port == mfilter->dst_port &&
			    bfilter->dst_port_mask == mfilter->dst_port_mask &&
			    bfilter->flags == mfilter->flags &&
			    bfilter->enables == mfilter->enables) {
				if (mvnic)
					*mvnic = vnic;
				return mfilter;
			}
		}
	}
	return NULL;
}

static int
bnxt_cfg_ntuple_filter(struct bnxt *bp,
		       struct rte_eth_ntuple_filter *nfilter,
		       enum rte_filter_op filter_op)
{
	struct bnxt_filter_info *bfilter, *mfilter, *filter1;
	struct bnxt_vnic_info *vnic, *vnic0, *mvnic;
	int ret;

	if (nfilter->flags != RTE_5TUPLE_FLAGS) {
		RTE_LOG(ERR, PMD, "only 5tuple is supported.");
		return -EINVAL;
	}

	if (nfilter->flags & RTE_NTUPLE_FLAGS_TCP_FLAG) {
		RTE_LOG(ERR, PMD, "Ntuple filter: TCP flags not supported\n");
		return -EINVAL;
	}

	bfilter = bnxt_get_unused_filter(bp);
	if (bfilter == NULL) {
		RTE_LOG(ERR, PMD,
			"Not enough resources for a new filter.\n");
		return -ENOMEM;
	}
	ret = parse_ntuple_filter(bp, nfilter, bfilter);
	if (ret < 0)
		goto free_filter;

	vnic = STAILQ_FIRST(&bp->ff_pool[nfilter->queue]);
	vnic0 = STAILQ_FIRST(&bp->ff_pool[0]);
	filter1 = STAILQ_FIRST(&vnic0->filter);
	if (filter1 == NULL) {
		ret = -1;
		goto free_filter;
	}

	bfilter->dst_id = vnic->fw_vnic_id;
	bfilter->fw_l2_filter_id = filter1->fw_l2_filter_id;
	bfilter->enables |=
		HWRM_CFA_NTUPLE_FILTER_ALLOC_INPUT_ENABLES_L2_FILTER_ID;
	bfilter->ethertype = 0x800;
	bfilter->enables |= NTUPLE_FLTR_ALLOC_INPUT_EN_ETHERTYPE;

	mfilter = bnxt_match_ntuple_filter(bp, bfilter, &mvnic);

	if (mfilter != NULL && filter_op == RTE_ETH_FILTER_ADD &&
	    bfilter->dst_id == mfilter->dst_id) {
		RTE_LOG(ERR, PMD, "filter exists.\n");
		ret = -EEXIST;
		goto free_filter;
	} else if (mfilter != NULL && filter_op == RTE_ETH_FILTER_ADD &&
		   bfilter->dst_id != mfilter->dst_id) {
		mfilter->dst_id = vnic->fw_vnic_id;
		ret = bnxt_hwrm_set_ntuple_filter(bp, mfilter->dst_id, mfilter);
		STAILQ_REMOVE(&mvnic->filter, mfilter, bnxt_filter_info, next);
		STAILQ_INSERT_TAIL(&vnic->filter, mfilter, next);
		RTE_LOG(ERR, PMD, "filter with matching pattern exists.\n");
		RTE_LOG(ERR, PMD, " Updated it to the new destination queue\n");
		goto free_filter;
	}
	if (mfilter == NULL && filter_op == RTE_ETH_FILTER_DELETE) {
		RTE_LOG(ERR, PMD, "filter doesn't exist.");
		ret = -ENOENT;
		goto free_filter;
	}

	if (filter_op == RTE_ETH_FILTER_ADD) {
		bfilter->filter_type = HWRM_CFA_NTUPLE_FILTER;
		ret = bnxt_hwrm_set_ntuple_filter(bp, bfilter->dst_id, bfilter);
		if (ret)
			goto free_filter;
		STAILQ_INSERT_TAIL(&vnic->filter, bfilter, next);
	} else {
		if (mfilter == NULL) {
			/* This should not happen. But for Coverity! */
			ret = -ENOENT;
			goto free_filter;
		}
		ret = bnxt_hwrm_clear_ntuple_filter(bp, mfilter);

		STAILQ_REMOVE(&vnic->filter, mfilter, bnxt_filter_info, next);
		bnxt_free_filter(bp, mfilter);
		mfilter->fw_l2_filter_id = -1;
		bnxt_free_filter(bp, bfilter);
		bfilter->fw_l2_filter_id = -1;
	}

	return 0;
free_filter:
	bfilter->fw_l2_filter_id = -1;
	bnxt_free_filter(bp, bfilter);
	return ret;
}

static int
bnxt_ntuple_filter(struct rte_eth_dev *dev,
			enum rte_filter_op filter_op,
			void *arg)
{
	struct bnxt *bp = (struct bnxt *)dev->data->dev_private;
	int ret;

	if (filter_op == RTE_ETH_FILTER_NOP)
		return 0;

	if (arg == NULL) {
		RTE_LOG(ERR, PMD, "arg shouldn't be NULL for operation %u.",
			    filter_op);
		return -EINVAL;
	}

	switch (filter_op) {
	case RTE_ETH_FILTER_ADD:
		ret = bnxt_cfg_ntuple_filter(bp,
			(struct rte_eth_ntuple_filter *)arg,
			filter_op);
		break;
	case RTE_ETH_FILTER_DELETE:
		ret = bnxt_cfg_ntuple_filter(bp,
			(struct rte_eth_ntuple_filter *)arg,
			filter_op);
		break;
	default:
		RTE_LOG(ERR, PMD, "unsupported operation %u.", filter_op);
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int
bnxt_parse_fdir_filter(struct bnxt *bp,
		       struct rte_eth_fdir_filter *fdir,
		       struct bnxt_filter_info *filter)
{
	enum rte_fdir_mode fdir_mode =
		bp->eth_dev->data->dev_conf.fdir_conf.mode;
	struct bnxt_vnic_info *vnic0, *vnic;
	struct bnxt_filter_info *filter1;
	uint32_t en = 0;
	int i;

	if (fdir_mode == RTE_FDIR_MODE_PERFECT_TUNNEL)
		return -EINVAL;

	filter->l2_ovlan = fdir->input.flow_ext.vlan_tci;
	en |= EM_FLOW_ALLOC_INPUT_EN_OVLAN_VID;

	switch (fdir->input.flow_type) {
	case RTE_ETH_FLOW_IPV4:
	case RTE_ETH_FLOW_NONFRAG_IPV4_OTHER:
		/* FALLTHROUGH */
		filter->src_ipaddr[0] = fdir->input.flow.ip4_flow.src_ip;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_SRC_IPADDR;
		filter->dst_ipaddr[0] = fdir->input.flow.ip4_flow.dst_ip;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_DST_IPADDR;
		filter->ip_protocol = fdir->input.flow.ip4_flow.proto;
		en |= NTUPLE_FLTR_ALLOC_IN_EN_IP_PROTO;
		filter->ip_addr_type =
			NTUPLE_FLTR_ALLOC_INPUT_IP_ADDR_TYPE_IPV4;
		filter->src_ipaddr_mask[0] = 0xffffffff;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_SRC_IPADDR_MASK;
		filter->dst_ipaddr_mask[0] = 0xffffffff;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_DST_IPADDR_MASK;
		filter->ethertype = 0x800;
		filter->enables |= NTUPLE_FLTR_ALLOC_INPUT_EN_ETHERTYPE;
		break;
	case RTE_ETH_FLOW_NONFRAG_IPV4_TCP:
		filter->src_port = fdir->input.flow.tcp4_flow.src_port;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_SRC_PORT;
		filter->dst_port = fdir->input.flow.tcp4_flow.dst_port;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_DST_PORT;
		filter->dst_port_mask = 0xffff;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_DST_PORT_MASK;
		filter->src_port_mask = 0xffff;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_SRC_PORT_MASK;
		filter->src_ipaddr[0] = fdir->input.flow.tcp4_flow.ip.src_ip;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_SRC_IPADDR;
		filter->dst_ipaddr[0] = fdir->input.flow.tcp4_flow.ip.dst_ip;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_DST_IPADDR;
		filter->ip_protocol = 6;
		en |= NTUPLE_FLTR_ALLOC_IN_EN_IP_PROTO;
		filter->ip_addr_type =
			NTUPLE_FLTR_ALLOC_INPUT_IP_ADDR_TYPE_IPV4;
		filter->src_ipaddr_mask[0] = 0xffffffff;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_SRC_IPADDR_MASK;
		filter->dst_ipaddr_mask[0] = 0xffffffff;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_DST_IPADDR_MASK;
		filter->ethertype = 0x800;
		filter->enables |= NTUPLE_FLTR_ALLOC_INPUT_EN_ETHERTYPE;
		break;
	case RTE_ETH_FLOW_NONFRAG_IPV4_UDP:
		filter->src_port = fdir->input.flow.udp4_flow.src_port;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_SRC_PORT;
		filter->dst_port = fdir->input.flow.udp4_flow.dst_port;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_DST_PORT;
		filter->dst_port_mask = 0xffff;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_DST_PORT_MASK;
		filter->src_port_mask = 0xffff;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_SRC_PORT_MASK;
		filter->src_ipaddr[0] = fdir->input.flow.udp4_flow.ip.src_ip;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_SRC_IPADDR;
		filter->dst_ipaddr[0] = fdir->input.flow.udp4_flow.ip.dst_ip;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_DST_IPADDR;
		filter->ip_protocol = 17;
		en |= NTUPLE_FLTR_ALLOC_IN_EN_IP_PROTO;
		filter->ip_addr_type =
			NTUPLE_FLTR_ALLOC_INPUT_IP_ADDR_TYPE_IPV4;
		filter->src_ipaddr_mask[0] = 0xffffffff;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_SRC_IPADDR_MASK;
		filter->dst_ipaddr_mask[0] = 0xffffffff;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_DST_IPADDR_MASK;
		filter->ethertype = 0x800;
		filter->enables |= NTUPLE_FLTR_ALLOC_INPUT_EN_ETHERTYPE;
		break;
	case RTE_ETH_FLOW_IPV6:
	case RTE_ETH_FLOW_NONFRAG_IPV6_OTHER:
		/* FALLTHROUGH */
		filter->ip_addr_type =
			NTUPLE_FLTR_ALLOC_INPUT_IP_ADDR_TYPE_IPV6;
		filter->ip_protocol = fdir->input.flow.ipv6_flow.proto;
		en |= NTUPLE_FLTR_ALLOC_IN_EN_IP_PROTO;
		rte_memcpy(filter->src_ipaddr,
			   fdir->input.flow.ipv6_flow.src_ip, 16);
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_SRC_IPADDR;
		rte_memcpy(filter->dst_ipaddr,
			   fdir->input.flow.ipv6_flow.dst_ip, 16);
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_DST_IPADDR;
		memset(filter->dst_ipaddr_mask, 0xff, 16);
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_DST_IPADDR_MASK;
		memset(filter->src_ipaddr_mask, 0xff, 16);
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_SRC_IPADDR_MASK;
		filter->ethertype = 0x86dd;
		filter->enables |= NTUPLE_FLTR_ALLOC_INPUT_EN_ETHERTYPE;
		break;
	case RTE_ETH_FLOW_NONFRAG_IPV6_TCP:
		filter->src_port = fdir->input.flow.tcp6_flow.src_port;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_SRC_PORT;
		filter->dst_port = fdir->input.flow.tcp6_flow.dst_port;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_DST_PORT;
		filter->dst_port_mask = 0xffff;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_DST_PORT_MASK;
		filter->src_port_mask = 0xffff;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_SRC_PORT_MASK;
		filter->ip_addr_type =
			NTUPLE_FLTR_ALLOC_INPUT_IP_ADDR_TYPE_IPV6;
		filter->ip_protocol = fdir->input.flow.tcp6_flow.ip.proto;
		en |= NTUPLE_FLTR_ALLOC_IN_EN_IP_PROTO;
		rte_memcpy(filter->src_ipaddr,
			   fdir->input.flow.tcp6_flow.ip.src_ip, 16);
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_SRC_IPADDR;
		rte_memcpy(filter->dst_ipaddr,
			   fdir->input.flow.tcp6_flow.ip.dst_ip, 16);
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_DST_IPADDR;
		memset(filter->dst_ipaddr_mask, 0xff, 16);
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_DST_IPADDR_MASK;
		memset(filter->src_ipaddr_mask, 0xff, 16);
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_SRC_IPADDR_MASK;
		filter->ethertype = 0x86dd;
		filter->enables |= NTUPLE_FLTR_ALLOC_INPUT_EN_ETHERTYPE;
		break;
	case RTE_ETH_FLOW_NONFRAG_IPV6_UDP:
		filter->src_port = fdir->input.flow.udp6_flow.src_port;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_SRC_PORT;
		filter->dst_port = fdir->input.flow.udp6_flow.dst_port;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_DST_PORT;
		filter->dst_port_mask = 0xffff;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_DST_PORT_MASK;
		filter->src_port_mask = 0xffff;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_SRC_PORT_MASK;
		filter->ip_addr_type =
			NTUPLE_FLTR_ALLOC_INPUT_IP_ADDR_TYPE_IPV6;
		filter->ip_protocol = fdir->input.flow.udp6_flow.ip.proto;
		en |= NTUPLE_FLTR_ALLOC_IN_EN_IP_PROTO;
		rte_memcpy(filter->src_ipaddr,
			   fdir->input.flow.udp6_flow.ip.src_ip, 16);
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_SRC_IPADDR;
		rte_memcpy(filter->dst_ipaddr,
			   fdir->input.flow.udp6_flow.ip.dst_ip, 16);
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_DST_IPADDR;
		memset(filter->dst_ipaddr_mask, 0xff, 16);
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_DST_IPADDR_MASK;
		memset(filter->src_ipaddr_mask, 0xff, 16);
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_SRC_IPADDR_MASK;
		filter->ethertype = 0x86dd;
		filter->enables |= NTUPLE_FLTR_ALLOC_INPUT_EN_ETHERTYPE;
		break;
	case RTE_ETH_FLOW_L2_PAYLOAD:
		filter->ethertype = fdir->input.flow.l2_flow.ether_type;
		en |= NTUPLE_FLTR_ALLOC_INPUT_EN_ETHERTYPE;
		break;
	case RTE_ETH_FLOW_VXLAN:
		if (fdir->action.behavior == RTE_ETH_FDIR_REJECT)
			return -EINVAL;
		filter->vni = fdir->input.flow.tunnel_flow.tunnel_id;
		filter->tunnel_type =
			CFA_NTUPLE_FILTER_ALLOC_REQ_TUNNEL_TYPE_VXLAN;
		en |= HWRM_CFA_NTUPLE_FILTER_ALLOC_INPUT_ENABLES_TUNNEL_TYPE;
		break;
	case RTE_ETH_FLOW_NVGRE:
		if (fdir->action.behavior == RTE_ETH_FDIR_REJECT)
			return -EINVAL;
		filter->vni = fdir->input.flow.tunnel_flow.tunnel_id;
		filter->tunnel_type =
			CFA_NTUPLE_FILTER_ALLOC_REQ_TUNNEL_TYPE_NVGRE;
		en |= HWRM_CFA_NTUPLE_FILTER_ALLOC_INPUT_ENABLES_TUNNEL_TYPE;
		break;
	case RTE_ETH_FLOW_UNKNOWN:
	case RTE_ETH_FLOW_RAW:
	case RTE_ETH_FLOW_FRAG_IPV4:
	case RTE_ETH_FLOW_NONFRAG_IPV4_SCTP:
	case RTE_ETH_FLOW_FRAG_IPV6:
	case RTE_ETH_FLOW_NONFRAG_IPV6_SCTP:
	case RTE_ETH_FLOW_IPV6_EX:
	case RTE_ETH_FLOW_IPV6_TCP_EX:
	case RTE_ETH_FLOW_IPV6_UDP_EX:
	case RTE_ETH_FLOW_GENEVE:
		/* FALLTHROUGH */
	default:
		return -EINVAL;
	}

	vnic0 = STAILQ_FIRST(&bp->ff_pool[0]);
	vnic = STAILQ_FIRST(&bp->ff_pool[fdir->action.rx_queue]);
	if (vnic == NULL) {
		RTE_LOG(ERR, PMD, "Invalid queue %d\n", fdir->action.rx_queue);
		return -EINVAL;
	}

	if (fdir_mode == RTE_FDIR_MODE_PERFECT_MAC_VLAN) {
		rte_memcpy(filter->dst_macaddr,
			fdir->input.flow.mac_vlan_flow.mac_addr.addr_bytes, 6);
			en |= NTUPLE_FLTR_ALLOC_INPUT_EN_DST_MACADDR;
	}

	if (fdir->action.behavior == RTE_ETH_FDIR_REJECT) {
		filter->flags = HWRM_CFA_NTUPLE_FILTER_ALLOC_INPUT_FLAGS_DROP;
		filter1 = STAILQ_FIRST(&vnic0->filter);
		//filter1 = bnxt_get_l2_filter(bp, filter, vnic0);
	} else {
		filter->dst_id = vnic->fw_vnic_id;
		for (i = 0; i < ETHER_ADDR_LEN; i++)
			if (filter->dst_macaddr[i] == 0x00)
				filter1 = STAILQ_FIRST(&vnic0->filter);
			else
				filter1 = bnxt_get_l2_filter(bp, filter, vnic);
	}

	if (filter1 == NULL)
		return -EINVAL;

	en |= HWRM_CFA_NTUPLE_FILTER_ALLOC_INPUT_ENABLES_L2_FILTER_ID;
	filter->fw_l2_filter_id = filter1->fw_l2_filter_id;

	filter->enables = en;

	return 0;
}

static struct bnxt_filter_info *
bnxt_match_fdir(struct bnxt *bp, struct bnxt_filter_info *nf)
{
	struct bnxt_filter_info *mf = NULL;
	int i;

	for (i = bp->nr_vnics - 1; i >= 0; i--) {
		struct bnxt_vnic_info *vnic = &bp->vnic_info[i];

		STAILQ_FOREACH(mf, &vnic->filter, next) {
			if (mf->filter_type == nf->filter_type &&
			    mf->flags == nf->flags &&
			    mf->src_port == nf->src_port &&
			    mf->src_port_mask == nf->src_port_mask &&
			    mf->dst_port == nf->dst_port &&
			    mf->dst_port_mask == nf->dst_port_mask &&
			    mf->ip_protocol == nf->ip_protocol &&
			    mf->ip_addr_type == nf->ip_addr_type &&
			    mf->ethertype == nf->ethertype &&
			    mf->vni == nf->vni &&
			    mf->tunnel_type == nf->tunnel_type &&
			    mf->l2_ovlan == nf->l2_ovlan &&
			    mf->l2_ovlan_mask == nf->l2_ovlan_mask &&
			    mf->l2_ivlan == nf->l2_ivlan &&
			    mf->l2_ivlan_mask == nf->l2_ivlan_mask &&
			    !memcmp(mf->l2_addr, nf->l2_addr, ETHER_ADDR_LEN) &&
			    !memcmp(mf->l2_addr_mask, nf->l2_addr_mask,
				    ETHER_ADDR_LEN) &&
			    !memcmp(mf->src_macaddr, nf->src_macaddr,
				    ETHER_ADDR_LEN) &&
			    !memcmp(mf->dst_macaddr, nf->dst_macaddr,
				    ETHER_ADDR_LEN) &&
			    !memcmp(mf->src_ipaddr, nf->src_ipaddr,
				    sizeof(nf->src_ipaddr)) &&
			    !memcmp(mf->src_ipaddr_mask, nf->src_ipaddr_mask,
				    sizeof(nf->src_ipaddr_mask)) &&
			    !memcmp(mf->dst_ipaddr, nf->dst_ipaddr,
				    sizeof(nf->dst_ipaddr)) &&
			    !memcmp(mf->dst_ipaddr_mask, nf->dst_ipaddr_mask,
				    sizeof(nf->dst_ipaddr_mask)))
				return mf;
		}
	}
	return NULL;
}

static int
bnxt_fdir_filter(struct rte_eth_dev *dev,
		 enum rte_filter_op filter_op,
		 void *arg)
{
	struct bnxt *bp = (struct bnxt *)dev->data->dev_private;
	struct rte_eth_fdir_filter *fdir  = (struct rte_eth_fdir_filter *)arg;
	struct bnxt_filter_info *filter, *match;
	struct bnxt_vnic_info *vnic;
	int ret = 0, i;

	if (filter_op == RTE_ETH_FILTER_NOP)
		return 0;

	if (arg == NULL && filter_op != RTE_ETH_FILTER_FLUSH)
		return -EINVAL;

	switch (filter_op) {
	case RTE_ETH_FILTER_ADD:
	case RTE_ETH_FILTER_DELETE:
		/* FALLTHROUGH */
		filter = bnxt_get_unused_filter(bp);
		if (filter == NULL) {
			RTE_LOG(ERR, PMD,
				"Not enough resources for a new flow.\n");
			return -ENOMEM;
		}

		ret = bnxt_parse_fdir_filter(bp, fdir, filter);
		if (ret != 0)
			goto free_filter;
		filter->filter_type = HWRM_CFA_NTUPLE_FILTER;

		match = bnxt_match_fdir(bp, filter);
		if (match != NULL && filter_op == RTE_ETH_FILTER_ADD) {
			RTE_LOG(ERR, PMD, "Flow already exists.\n");
			ret = -EEXIST;
			goto free_filter;
		}
		if (match == NULL && filter_op == RTE_ETH_FILTER_DELETE) {
			RTE_LOG(ERR, PMD, "Flow does not exist.\n");
			ret = -ENOENT;
			goto free_filter;
		}

		if (fdir->action.behavior == RTE_ETH_FDIR_REJECT)
			vnic = STAILQ_FIRST(&bp->ff_pool[0]);
		else
			vnic =
			STAILQ_FIRST(&bp->ff_pool[fdir->action.rx_queue]);

		if (filter_op == RTE_ETH_FILTER_ADD) {
			ret = bnxt_hwrm_set_ntuple_filter(bp,
							  filter->dst_id,
							  filter);
			if (ret)
				goto free_filter;
			STAILQ_INSERT_TAIL(&vnic->filter, filter, next);
		} else {
			ret = bnxt_hwrm_clear_ntuple_filter(bp, match);
			STAILQ_REMOVE(&vnic->filter, match,
				      bnxt_filter_info, next);
			bnxt_free_filter(bp, match);
			filter->fw_l2_filter_id = -1;
			bnxt_free_filter(bp, filter);
		}
		break;
	case RTE_ETH_FILTER_FLUSH:
		for (i = bp->nr_vnics - 1; i >= 0; i--) {
			struct bnxt_vnic_info *vnic = &bp->vnic_info[i];

			STAILQ_FOREACH(filter, &vnic->filter, next) {
				if (filter->filter_type ==
				    HWRM_CFA_NTUPLE_FILTER) {
					ret =
					bnxt_hwrm_clear_ntuple_filter(bp,
								      filter);
					STAILQ_REMOVE(&vnic->filter, filter,
						      bnxt_filter_info, next);
				}
			}
		}
		return ret;
	case RTE_ETH_FILTER_UPDATE:
	case RTE_ETH_FILTER_STATS:
	case RTE_ETH_FILTER_INFO:
		/* FALLTHROUGH */
		RTE_LOG(ERR, PMD, "operation %u not implemented", filter_op);
		break;
	default:
		RTE_LOG(ERR, PMD, "unknown operation %u", filter_op);
		ret = -EINVAL;
		break;
	}
	return ret;

free_filter:
	filter->fw_l2_filter_id = -1;
	bnxt_free_filter(bp, filter);
	return ret;
}

static int
bnxt_filter_ctrl_op(struct rte_eth_dev *dev __rte_unused,
		    enum rte_filter_type filter_type,
		    enum rte_filter_op filter_op, void *arg)
{
	int ret = 0;

	switch (filter_type) {
	case RTE_ETH_FILTER_TUNNEL:
		RTE_LOG(ERR, PMD,
			"filter type: %d: To be implemented\n", filter_type);
		break;
	case RTE_ETH_FILTER_FDIR:
		ret = bnxt_fdir_filter(dev, filter_op, arg);
		break;
	case RTE_ETH_FILTER_NTUPLE:
		ret = bnxt_ntuple_filter(dev, filter_op, arg);
		break;
	case RTE_ETH_FILTER_ETHERTYPE:
		ret = bnxt_ethertype_filter(dev, filter_op, arg);
		break;
	case RTE_ETH_FILTER_GENERIC:
		if (filter_op != RTE_ETH_FILTER_GET)
			return -EINVAL;
		*(const void **)arg = &bnxt_flow_ops;
		break;
	default:
		RTE_LOG(ERR, PMD,
			"Filter type (%d) not supported", filter_type);
		ret = -EINVAL;
		break;
	}
	return ret;
}

static const uint32_t *
bnxt_dev_supported_ptypes_get_op(struct rte_eth_dev *dev)
{
	static const uint32_t ptypes[] = {
		RTE_PTYPE_L2_ETHER_VLAN,
		RTE_PTYPE_L3_IPV4_EXT_UNKNOWN,
		RTE_PTYPE_L3_IPV6_EXT_UNKNOWN,
		RTE_PTYPE_L4_ICMP,
		RTE_PTYPE_L4_TCP,
		RTE_PTYPE_L4_UDP,
		RTE_PTYPE_INNER_L3_IPV4_EXT_UNKNOWN,
		RTE_PTYPE_INNER_L3_IPV6_EXT_UNKNOWN,
		RTE_PTYPE_INNER_L4_ICMP,
		RTE_PTYPE_INNER_L4_TCP,
		RTE_PTYPE_INNER_L4_UDP,
		RTE_PTYPE_UNKNOWN
	};

	if (dev->rx_pkt_burst == bnxt_recv_pkts)
		return ptypes;
	return NULL;
}



static int
bnxt_get_eeprom_length_op(struct rte_eth_dev *dev)
{
	struct bnxt *bp = (struct bnxt *)dev->data->dev_private;
	int rc;
	uint32_t dir_entries;
	uint32_t entry_length;

	RTE_LOG(INFO, PMD, "%s(): %04x:%02x:%02x:%02x\n",
		__func__, bp->pdev->addr.domain, bp->pdev->addr.bus,
		bp->pdev->addr.devid, bp->pdev->addr.function);

	rc = bnxt_hwrm_nvm_get_dir_info(bp, &dir_entries, &entry_length);
	if (rc != 0)
		return rc;

	return dir_entries * entry_length;
}

static int
bnxt_get_eeprom_op(struct rte_eth_dev *dev,
		struct rte_dev_eeprom_info *in_eeprom)
{
	struct bnxt *bp = (struct bnxt *)dev->data->dev_private;
	uint32_t index;
	uint32_t offset;

	RTE_LOG(INFO, PMD, "%s(): %04x:%02x:%02x:%02x in_eeprom->offset = %d "
		"len = %d\n", __func__, bp->pdev->addr.domain,
		bp->pdev->addr.bus, bp->pdev->addr.devid,
		bp->pdev->addr.function, in_eeprom->offset, in_eeprom->length);

	if (in_eeprom->offset == 0) /* special offset value to get directory */
		return bnxt_get_nvram_directory(bp, in_eeprom->length,
						in_eeprom->data);

	index = in_eeprom->offset >> 24;
	offset = in_eeprom->offset & 0xffffff;

	if (index != 0)
		return bnxt_hwrm_get_nvram_item(bp, index - 1, offset,
					   in_eeprom->length, in_eeprom->data);

	return 0;
}

static bool bnxt_dir_type_is_ape_bin_format(uint16_t dir_type)
{
	switch (dir_type) {
	case BNX_DIR_TYPE_CHIMP_PATCH:
	case BNX_DIR_TYPE_BOOTCODE:
	case BNX_DIR_TYPE_BOOTCODE_2:
	case BNX_DIR_TYPE_APE_FW:
	case BNX_DIR_TYPE_APE_PATCH:
	case BNX_DIR_TYPE_KONG_FW:
	case BNX_DIR_TYPE_KONG_PATCH:
	case BNX_DIR_TYPE_BONO_FW:
	case BNX_DIR_TYPE_BONO_PATCH:
		return true;
	}

	return false;
}

static bool bnxt_dir_type_is_other_exec_format(uint16_t dir_type)
{
	switch (dir_type) {
	case BNX_DIR_TYPE_AVS:
	case BNX_DIR_TYPE_EXP_ROM_MBA:
	case BNX_DIR_TYPE_PCIE:
	case BNX_DIR_TYPE_TSCF_UCODE:
	case BNX_DIR_TYPE_EXT_PHY:
	case BNX_DIR_TYPE_CCM:
	case BNX_DIR_TYPE_ISCSI_BOOT:
	case BNX_DIR_TYPE_ISCSI_BOOT_IPV6:
	case BNX_DIR_TYPE_ISCSI_BOOT_IPV4N6:
		return true;
	}

	return false;
}

static bool bnxt_dir_type_is_executable(uint16_t dir_type)
{
	return bnxt_dir_type_is_ape_bin_format(dir_type) ||
		bnxt_dir_type_is_other_exec_format(dir_type);
}

static int
bnxt_set_eeprom_op(struct rte_eth_dev *dev,
		struct rte_dev_eeprom_info *in_eeprom)
{
	struct bnxt *bp = (struct bnxt *)dev->data->dev_private;
	uint8_t index, dir_op;
	uint16_t type, ext, ordinal, attr;

	RTE_LOG(INFO, PMD, "%s(): %04x:%02x:%02x:%02x in_eeprom->offset = %d "
		"len = %d\n", __func__, bp->pdev->addr.domain,
		bp->pdev->addr.bus, bp->pdev->addr.devid,
		bp->pdev->addr.function, in_eeprom->offset, in_eeprom->length);

	if (!BNXT_PF(bp)) {
		RTE_LOG(ERR, PMD, "NVM write not supported from a VF\n");
		return -EINVAL;
	}

	type = in_eeprom->magic >> 16;

	if (type == 0xffff) { /* special value for directory operations */
		index = in_eeprom->magic & 0xff;
		dir_op = in_eeprom->magic >> 8;
		if (index == 0)
			return -EINVAL;
		switch (dir_op) {
		case 0x0e: /* erase */
			if (in_eeprom->offset != ~in_eeprom->magic)
				return -EINVAL;
			return bnxt_hwrm_erase_nvram_directory(bp, index - 1);
		default:
			return -EINVAL;
		}
	}

	/* Create or re-write an NVM item: */
	if (bnxt_dir_type_is_executable(type) == true)
		return -EOPNOTSUPP;
	ext = in_eeprom->magic & 0xffff;
	ordinal = in_eeprom->offset >> 16;
	attr = in_eeprom->offset & 0xffff;

	return bnxt_hwrm_flash_nvram(bp, type, ordinal, ext, attr,
				     in_eeprom->data, in_eeprom->length);
	return 0;
}

/*
 * Initialization
 */

static const struct eth_dev_ops bnxt_dev_ops = {
	.dev_infos_get = bnxt_dev_info_get_op,
	.dev_close = bnxt_dev_close_op,
	.dev_configure = bnxt_dev_configure_op,
	.dev_start = bnxt_dev_start_op,
	.dev_stop = bnxt_dev_stop_op,
	.dev_set_link_up = bnxt_dev_set_link_up_op,
	.dev_set_link_down = bnxt_dev_set_link_down_op,
	.stats_get = bnxt_stats_get_op,
	.stats_reset = bnxt_stats_reset_op,
	.rx_queue_setup = bnxt_rx_queue_setup_op,
	.rx_queue_release = bnxt_rx_queue_release_op,
	.tx_queue_setup = bnxt_tx_queue_setup_op,
	.tx_queue_release = bnxt_tx_queue_release_op,
	.rx_queue_intr_enable = bnxt_rx_queue_intr_enable_op,
	.rx_queue_intr_disable = bnxt_rx_queue_intr_disable_op,
	.reta_update = bnxt_reta_update_op,
	.reta_query = bnxt_reta_query_op,
	.rss_hash_update = bnxt_rss_hash_update_op,
	.rss_hash_conf_get = bnxt_rss_hash_conf_get_op,
	.link_update = bnxt_link_update_op,
	.promiscuous_enable = bnxt_promiscuous_enable_op,
	.promiscuous_disable = bnxt_promiscuous_disable_op,
	.allmulticast_enable = bnxt_allmulticast_enable_op,
	.allmulticast_disable = bnxt_allmulticast_disable_op,
	.mac_addr_add = bnxt_mac_addr_add_op,
	.mac_addr_remove = bnxt_mac_addr_remove_op,
	.flow_ctrl_get = bnxt_flow_ctrl_get_op,
	.flow_ctrl_set = bnxt_flow_ctrl_set_op,
	.udp_tunnel_port_add  = bnxt_udp_tunnel_port_add_op,
	.udp_tunnel_port_del  = bnxt_udp_tunnel_port_del_op,
	.vlan_filter_set = bnxt_vlan_filter_set_op,
	.vlan_offload_set = bnxt_vlan_offload_set_op,
	.vlan_pvid_set = bnxt_vlan_pvid_set_op,
	.mtu_set = bnxt_mtu_set_op,
	.mac_addr_set = bnxt_set_default_mac_addr_op,
	.xstats_get = bnxt_dev_xstats_get_op,
	.xstats_get_names = bnxt_dev_xstats_get_names_op,
	.xstats_reset = bnxt_dev_xstats_reset_op,
	.fw_version_get = bnxt_fw_version_get,
	.set_mc_addr_list = bnxt_dev_set_mc_addr_list_op,
	.rxq_info_get = bnxt_rxq_info_get_op,
	.txq_info_get = bnxt_txq_info_get_op,
	.dev_led_on = bnxt_dev_led_on_op,
	.dev_led_off = bnxt_dev_led_off_op,
	.xstats_get_by_id = bnxt_dev_xstats_get_by_id_op,
	.xstats_get_names_by_id = bnxt_dev_xstats_get_names_by_id_op,
	.rx_queue_count = bnxt_rx_queue_count_op,
	.rx_descriptor_status = bnxt_rx_descriptor_status_op,
	.tx_descriptor_status = bnxt_tx_descriptor_status_op,
	.filter_ctrl = bnxt_filter_ctrl_op,
	.dev_supported_ptypes_get = bnxt_dev_supported_ptypes_get_op,
	.get_eeprom_length    = bnxt_get_eeprom_length_op,
	.get_eeprom           = bnxt_get_eeprom_op,
	.set_eeprom           = bnxt_set_eeprom_op,
};

static bool bnxt_vf_pciid(uint16_t id)
{
	if (id == BROADCOM_DEV_ID_57304_VF ||
	    id == BROADCOM_DEV_ID_57406_VF ||
	    id == BROADCOM_DEV_ID_5731X_VF ||
	    id == BROADCOM_DEV_ID_5741X_VF ||
	    id == BROADCOM_DEV_ID_57414_VF ||
	    id == BROADCOM_DEV_ID_STRATUS_NIC_VF)
		return true;
	return false;
}

static int bnxt_init_board(struct rte_eth_dev *eth_dev)
{
	struct bnxt *bp = eth_dev->data->dev_private;
	struct rte_pci_device *pci_dev = RTE_ETH_DEV_TO_PCI(eth_dev);
	int rc;

	/* enable device (incl. PCI PM wakeup), and bus-mastering */
	if (!pci_dev->mem_resource[0].addr) {
		RTE_LOG(ERR, PMD,
			"Cannot find PCI device base address, aborting\n");
		rc = -ENODEV;
		goto init_err_disable;
	}

	bp->eth_dev = eth_dev;
	bp->pdev = pci_dev;

	bp->bar0 = (void *)pci_dev->mem_resource[0].addr;
	if (!bp->bar0) {
		RTE_LOG(ERR, PMD, "Cannot map device registers, aborting\n");
		rc = -ENOMEM;
		goto init_err_release;
	}
	return 0;

init_err_release:
	if (bp->bar0)
		bp->bar0 = NULL;

init_err_disable:

	return rc;
}

static int bnxt_dev_uninit(struct rte_eth_dev *eth_dev);

#define ALLOW_FUNC(x)	\
	{ \
		typeof(x) arg = (x); \
		bp->pf.vf_req_fwd[((arg) >> 5)] &= \
		~rte_cpu_to_le_32(1 << ((arg) & 0x1f)); \
	}
static int
bnxt_dev_init(struct rte_eth_dev *eth_dev)
{
	struct rte_pci_device *pci_dev = RTE_ETH_DEV_TO_PCI(eth_dev);
	char mz_name[RTE_MEMZONE_NAMESIZE];
	const struct rte_memzone *mz = NULL;
	static int version_printed;
	uint32_t total_alloc_len;
	rte_iova_t mz_phys_addr;
	struct bnxt *bp;
	int rc;

	if (version_printed++ == 0)
		RTE_LOG(INFO, PMD, "%s\n", bnxt_version);

	eth_dev->dev_ops = &bnxt_dev_ops;
	eth_dev->rx_pkt_burst = &bnxt_recv_pkts;
	eth_dev->tx_pkt_burst = &bnxt_xmit_pkts;

	if (rte_eal_process_type() != RTE_PROC_PRIMARY)
		return 0;

	rte_eth_copy_pci_info(eth_dev, pci_dev);

	bp = eth_dev->data->dev_private;

	rte_atomic64_init(&bp->rx_mbuf_alloc_fail);
	bp->dev_stopped = 1;

	if (bnxt_vf_pciid(pci_dev->id.device_id))
		bp->flags |= BNXT_FLAG_VF;

	rc = bnxt_init_board(eth_dev);
	if (rc) {
		RTE_LOG(ERR, PMD,
			"Board initialization failed rc: %x\n", rc);
		goto error;
	}

	if (BNXT_PF(bp) && pci_dev->id.device_id != BROADCOM_DEV_ID_NS2) {
		snprintf(mz_name, RTE_MEMZONE_NAMESIZE,
			 "bnxt_%04x:%02x:%02x:%02x-%s", pci_dev->addr.domain,
			 pci_dev->addr.bus, pci_dev->addr.devid,
			 pci_dev->addr.function, "rx_port_stats");
		mz_name[RTE_MEMZONE_NAMESIZE - 1] = 0;
		mz = rte_memzone_lookup(mz_name);
		total_alloc_len = RTE_CACHE_LINE_ROUNDUP(
				sizeof(struct rx_port_stats) + 512);
		if (!mz) {
			mz = rte_memzone_reserve(mz_name, total_alloc_len,
						 SOCKET_ID_ANY,
						 RTE_MEMZONE_2MB |
						 RTE_MEMZONE_SIZE_HINT_ONLY);
			if (mz == NULL)
				return -ENOMEM;
		}
		memset(mz->addr, 0, mz->len);
		mz_phys_addr = mz->iova;
		if ((unsigned long)mz->addr == mz_phys_addr) {
			RTE_LOG(WARNING, PMD,
				"Memzone physical address same as virtual.\n");
			RTE_LOG(WARNING, PMD,
				"Using rte_mem_virt2iova()\n");
			mz_phys_addr = rte_mem_virt2iova(mz->addr);
			if (mz_phys_addr == 0) {
				RTE_LOG(ERR, PMD,
				"unable to map address to physical memory\n");
				return -ENOMEM;
			}
		}

		bp->rx_mem_zone = (const void *)mz;
		bp->hw_rx_port_stats = mz->addr;
		bp->hw_rx_port_stats_map = mz_phys_addr;

		snprintf(mz_name, RTE_MEMZONE_NAMESIZE,
			 "bnxt_%04x:%02x:%02x:%02x-%s", pci_dev->addr.domain,
			 pci_dev->addr.bus, pci_dev->addr.devid,
			 pci_dev->addr.function, "tx_port_stats");
		mz_name[RTE_MEMZONE_NAMESIZE - 1] = 0;
		mz = rte_memzone_lookup(mz_name);
		total_alloc_len = RTE_CACHE_LINE_ROUNDUP(
				sizeof(struct tx_port_stats) + 512);
		if (!mz) {
			mz = rte_memzone_reserve(mz_name, total_alloc_len,
						 SOCKET_ID_ANY,
						 RTE_MEMZONE_2MB |
						 RTE_MEMZONE_SIZE_HINT_ONLY);
			if (mz == NULL)
				return -ENOMEM;
		}
		memset(mz->addr, 0, mz->len);
		mz_phys_addr = mz->iova;
		if ((unsigned long)mz->addr == mz_phys_addr) {
			RTE_LOG(WARNING, PMD,
				"Memzone physical address same as virtual.\n");
			RTE_LOG(WARNING, PMD,
				"Using rte_mem_virt2iova()\n");
			mz_phys_addr = rte_mem_virt2iova(mz->addr);
			if (mz_phys_addr == 0) {
				RTE_LOG(ERR, PMD,
				"unable to map address to physical memory\n");
				return -ENOMEM;
			}
		}

		bp->tx_mem_zone = (const void *)mz;
		bp->hw_tx_port_stats = mz->addr;
		bp->hw_tx_port_stats_map = mz_phys_addr;

		bp->flags |= BNXT_FLAG_PORT_STATS;
	}

	rc = bnxt_alloc_hwrm_resources(bp);
	if (rc) {
		RTE_LOG(ERR, PMD,
			"hwrm resource allocation failure rc: %x\n", rc);
		goto error_free;
	}
	rc = bnxt_hwrm_ver_get(bp);
	if (rc)
		goto error_free;
	bnxt_hwrm_queue_qportcfg(bp);

	bnxt_hwrm_func_qcfg(bp);

	/* Get the MAX capabilities for this function */
	rc = bnxt_hwrm_func_qcaps(bp);
	if (rc) {
		RTE_LOG(ERR, PMD, "hwrm query capability failure rc: %x\n", rc);
		goto error_free;
	}
	if (bp->max_tx_rings == 0) {
		RTE_LOG(ERR, PMD, "No TX rings available!\n");
		rc = -EBUSY;
		goto error_free;
	}
	eth_dev->data->mac_addrs = rte_zmalloc("bnxt_mac_addr_tbl",
					ETHER_ADDR_LEN * bp->max_l2_ctx, 0);
	if (eth_dev->data->mac_addrs == NULL) {
		RTE_LOG(ERR, PMD,
			"Failed to alloc %u bytes needed to store MAC addr tbl",
			ETHER_ADDR_LEN * bp->max_l2_ctx);
		rc = -ENOMEM;
		goto error_free;
	}
	/* Copy the permanent MAC from the qcap response address now. */
	memcpy(bp->mac_addr, bp->dflt_mac_addr, sizeof(bp->mac_addr));
	memcpy(&eth_dev->data->mac_addrs[0], bp->mac_addr, ETHER_ADDR_LEN);

	if (bp->max_ring_grps < bp->rx_cp_nr_rings) {
		/* 1 ring is for default completion ring */
		RTE_LOG(ERR, PMD, "Insufficient resource: Ring Group\n");
		rc = -ENOSPC;
		goto error_free;
	}

	bp->grp_info = rte_zmalloc("bnxt_grp_info",
				sizeof(*bp->grp_info) * bp->max_ring_grps, 0);
	if (!bp->grp_info) {
		RTE_LOG(ERR, PMD,
			"Failed to alloc %zu bytes to store group info table\n",
			sizeof(*bp->grp_info) * bp->max_ring_grps);
		rc = -ENOMEM;
		goto error_free;
	}

	/* Forward all requests if firmware is new enough */
	if (((bp->fw_ver >= ((20 << 24) | (6 << 16) | (100 << 8))) &&
	    (bp->fw_ver < ((20 << 24) | (7 << 16)))) ||
	    ((bp->fw_ver >= ((20 << 24) | (8 << 16))))) {
		memset(bp->pf.vf_req_fwd, 0xff, sizeof(bp->pf.vf_req_fwd));
	} else {
		RTE_LOG(WARNING, PMD,
			"Firmware too old for VF mailbox functionality\n");
		memset(bp->pf.vf_req_fwd, 0, sizeof(bp->pf.vf_req_fwd));
	}

	/*
	 * The following are used for driver cleanup.  If we disallow these,
	 * VF drivers can't clean up cleanly.
	 */
	ALLOW_FUNC(HWRM_FUNC_DRV_UNRGTR);
	ALLOW_FUNC(HWRM_VNIC_FREE);
	ALLOW_FUNC(HWRM_RING_FREE);
	ALLOW_FUNC(HWRM_RING_GRP_FREE);
	ALLOW_FUNC(HWRM_VNIC_RSS_COS_LB_CTX_FREE);
	ALLOW_FUNC(HWRM_CFA_L2_FILTER_FREE);
	ALLOW_FUNC(HWRM_STAT_CTX_FREE);
	ALLOW_FUNC(HWRM_PORT_PHY_QCFG);
	ALLOW_FUNC(HWRM_VNIC_TPA_CFG);
	rc = bnxt_hwrm_func_driver_register(bp);
	if (rc) {
		RTE_LOG(ERR, PMD,
			"Failed to register driver");
		rc = -EBUSY;
		goto error_free;
	}

	RTE_LOG(INFO, PMD,
		DRV_MODULE_NAME " found at mem %" PRIx64 ", node addr %pM\n",
		pci_dev->mem_resource[0].phys_addr,
		pci_dev->mem_resource[0].addr);

	rc = bnxt_hwrm_func_reset(bp);
	if (rc) {
		RTE_LOG(ERR, PMD, "hwrm chip reset failure rc: %x\n", rc);
		rc = -1;
		goto error_free;
	}

	if (BNXT_PF(bp)) {
		//if (bp->pf.active_vfs) {
			// TODO: Deallocate VF resources?
		//}
		if (bp->pdev->max_vfs) {
			rc = bnxt_hwrm_allocate_vfs(bp, bp->pdev->max_vfs);
			if (rc) {
				RTE_LOG(ERR, PMD, "Failed to allocate VFs\n");
				goto error_free;
			}
		} else {
			rc = bnxt_hwrm_allocate_pf_only(bp);
			if (rc) {
				RTE_LOG(ERR, PMD,
					"Failed to allocate PF resources\n");
				goto error_free;
			}
		}
	}

	bnxt_hwrm_port_led_qcaps(bp);

	rc = bnxt_setup_int(bp);
	if (rc)
		goto error_free;

	rc = bnxt_alloc_mem(bp);
	if (rc)
		goto error_free_int;

	rc = bnxt_request_int(bp);
	if (rc)
		goto error_free_int;

	rc = bnxt_alloc_def_cp_ring(bp);
	if (rc)
		goto error_free_int;

	bnxt_enable_int(bp);
	bnxt_init_nic(bp);

	return 0;

error_free_int:
	bnxt_disable_int(bp);
	bnxt_free_def_cp_ring(bp);
	bnxt_hwrm_func_buf_unrgtr(bp);
	bnxt_free_int(bp);
	bnxt_free_mem(bp);
error_free:
	bnxt_dev_uninit(eth_dev);
error:
	return rc;
}

static int
bnxt_dev_uninit(struct rte_eth_dev *eth_dev) {
	struct bnxt *bp = eth_dev->data->dev_private;
	int rc;

	if (rte_eal_process_type() != RTE_PROC_PRIMARY)
		return -EPERM;

	bnxt_disable_int(bp);
	bnxt_free_int(bp);
	bnxt_free_mem(bp);
	if (eth_dev->data->mac_addrs != NULL) {
		rte_free(eth_dev->data->mac_addrs);
		eth_dev->data->mac_addrs = NULL;
	}
	rc = bnxt_hwrm_func_driver_unregister(bp, 0);
	bnxt_free_hwrm_resources(bp);
	rte_memzone_free((const struct rte_memzone *)bp->tx_mem_zone);
	rte_memzone_free((const struct rte_memzone *)bp->rx_mem_zone);
	if (bp->dev_stopped == 0)
		bnxt_dev_close_op(eth_dev);
	if (bp->pf.vf_info)
		rte_free(bp->pf.vf_info);
	eth_dev->dev_ops = NULL;
	eth_dev->rx_pkt_burst = NULL;
	eth_dev->tx_pkt_burst = NULL;

	return rc;
}

static int bnxt_pci_probe(struct rte_pci_driver *pci_drv __rte_unused,
	struct rte_pci_device *pci_dev)
{
	return rte_eth_dev_pci_generic_probe(pci_dev, sizeof(struct bnxt),
		bnxt_dev_init);
}

static int bnxt_pci_remove(struct rte_pci_device *pci_dev)
{
	return rte_eth_dev_pci_generic_remove(pci_dev, bnxt_dev_uninit);
}

static struct rte_pci_driver bnxt_rte_pmd = {
	.id_table = bnxt_pci_id_map,
	.drv_flags = RTE_PCI_DRV_NEED_MAPPING |
		RTE_PCI_DRV_INTR_LSC | RTE_PCI_DRV_IOVA_AS_VA,
	.probe = bnxt_pci_probe,
	.remove = bnxt_pci_remove,
};

static bool
is_device_supported(struct rte_eth_dev *dev, struct rte_pci_driver *drv)
{
	if (strcmp(dev->device->driver->name, drv->driver.name))
		return false;

	return true;
}

bool is_bnxt_supported(struct rte_eth_dev *dev)
{
	return is_device_supported(dev, &bnxt_rte_pmd);
}

RTE_PMD_REGISTER_PCI(net_bnxt, bnxt_rte_pmd);
RTE_PMD_REGISTER_PCI_TABLE(net_bnxt, bnxt_pci_id_map);
RTE_PMD_REGISTER_KMOD_DEP(net_bnxt, "* igb_uio | uio_pci_generic | vfio-pci");
