/*-
 *   BSD LICENSE
 *
 *   Copyright 2016 Freescale Semiconductor, Inc. All rights reserved.
 *   Copyright 2017 NXP.
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
 *     * Neither the name of  Freescale Semiconductor, Inc nor the names of its
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
/* System headers */
#include <stdio.h>
#include <inttypes.h>
#include <unistd.h>
#include <limits.h>
#include <sched.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include <rte_byteorder.h>
#include <rte_common.h>
#include <rte_interrupts.h>
#include <rte_log.h>
#include <rte_debug.h>
#include <rte_pci.h>
#include <rte_atomic.h>
#include <rte_branch_prediction.h>
#include <rte_memory.h>
#include <rte_tailq.h>
#include <rte_eal.h>
#include <rte_alarm.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_malloc.h>
#include <rte_ring.h>

#include <rte_dpaa_bus.h>
#include <rte_dpaa_logs.h>
#include <dpaa_mempool.h>

#include <dpaa_ethdev.h>
#include <dpaa_rxtx.h>

#include <fsl_usd.h>
#include <fsl_qman.h>
#include <fsl_bman.h>
#include <fsl_fman.h>

/* Keep track of whether QMAN and BMAN have been globally initialized */
static int is_global_init;

struct rte_dpaa_xstats_name_off {
	char name[RTE_ETH_XSTATS_NAME_SIZE];
	uint32_t offset;
};

static const struct rte_dpaa_xstats_name_off dpaa_xstats_strings[] = {
	{"rx_align_err",
		offsetof(struct dpaa_if_stats, raln)},
	{"rx_valid_pause",
		offsetof(struct dpaa_if_stats, rxpf)},
	{"rx_fcs_err",
		offsetof(struct dpaa_if_stats, rfcs)},
	{"rx_vlan_frame",
		offsetof(struct dpaa_if_stats, rvlan)},
	{"rx_frame_err",
		offsetof(struct dpaa_if_stats, rerr)},
	{"rx_drop_err",
		offsetof(struct dpaa_if_stats, rdrp)},
	{"rx_undersized",
		offsetof(struct dpaa_if_stats, rund)},
	{"rx_oversize_err",
		offsetof(struct dpaa_if_stats, rovr)},
	{"rx_fragment_pkt",
		offsetof(struct dpaa_if_stats, rfrg)},
	{"tx_valid_pause",
		offsetof(struct dpaa_if_stats, txpf)},
	{"tx_fcs_err",
		offsetof(struct dpaa_if_stats, terr)},
	{"tx_vlan_frame",
		offsetof(struct dpaa_if_stats, tvlan)},
	{"rx_undersized",
		offsetof(struct dpaa_if_stats, tund)},
};

static int
dpaa_mtu_set(struct rte_eth_dev *dev, uint16_t mtu)
{
	struct dpaa_if *dpaa_intf = dev->data->dev_private;

	PMD_INIT_FUNC_TRACE();

	if (mtu < ETHER_MIN_MTU)
		return -EINVAL;
	if (mtu > ETHER_MAX_LEN)
		dev->data->dev_conf.rxmode.jumbo_frame = 1;
	else
		dev->data->dev_conf.rxmode.jumbo_frame = 0;

	dev->data->dev_conf.rxmode.max_rx_pkt_len = mtu;

	fman_if_set_maxfrm(dpaa_intf->fif, mtu);

	return 0;
}

static int
dpaa_eth_dev_configure(struct rte_eth_dev *dev __rte_unused)
{
	PMD_INIT_FUNC_TRACE();

	if (dev->data->dev_conf.rxmode.jumbo_frame == 1) {
		if (dev->data->dev_conf.rxmode.max_rx_pkt_len <=
		    DPAA_MAX_RX_PKT_LEN)
			return dpaa_mtu_set(dev,
				dev->data->dev_conf.rxmode.max_rx_pkt_len);
		else
			return -1;
	}
	return 0;
}

static const uint32_t *
dpaa_supported_ptypes_get(struct rte_eth_dev *dev)
{
	static const uint32_t ptypes[] = {
		/*todo -= add more types */
		RTE_PTYPE_L2_ETHER,
		RTE_PTYPE_L3_IPV4,
		RTE_PTYPE_L3_IPV4_EXT,
		RTE_PTYPE_L3_IPV6,
		RTE_PTYPE_L3_IPV6_EXT,
		RTE_PTYPE_L4_TCP,
		RTE_PTYPE_L4_UDP,
		RTE_PTYPE_L4_SCTP
	};

	PMD_INIT_FUNC_TRACE();

	if (dev->rx_pkt_burst == dpaa_eth_queue_rx)
		return ptypes;
	return NULL;
}

static int dpaa_eth_dev_start(struct rte_eth_dev *dev)
{
	struct dpaa_if *dpaa_intf = dev->data->dev_private;

	PMD_INIT_FUNC_TRACE();

	/* Change tx callback to the real one */
	dev->tx_pkt_burst = dpaa_eth_queue_tx;
	fman_if_enable_rx(dpaa_intf->fif);

	return 0;
}

static void dpaa_eth_dev_stop(struct rte_eth_dev *dev)
{
	struct dpaa_if *dpaa_intf = dev->data->dev_private;

	PMD_INIT_FUNC_TRACE();

	fman_if_disable_rx(dpaa_intf->fif);
	dev->tx_pkt_burst = dpaa_eth_tx_drop_all;
}

static void dpaa_eth_dev_close(struct rte_eth_dev *dev)
{
	PMD_INIT_FUNC_TRACE();

	dpaa_eth_dev_stop(dev);
}

static int
dpaa_fw_version_get(struct rte_eth_dev *dev __rte_unused,
		     char *fw_version,
		     size_t fw_size)
{
	int ret;
	FILE *svr_file = NULL;
	unsigned int svr_ver = 0;

	PMD_INIT_FUNC_TRACE();

	svr_file = fopen(DPAA_SOC_ID_FILE, "r");
	if (!svr_file) {
		DPAA_PMD_ERR("Unable to open SoC device");
		return -ENOTSUP; /* Not supported on this infra */
	}
	if (fscanf(svr_file, "svr:%x", &svr_ver) <= 0)
		DPAA_PMD_ERR("Unable to read SoC device");

	fclose(svr_file);

	ret = snprintf(fw_version, fw_size, "SVR:%x-fman-v%x",
		       svr_ver, fman_ip_rev);
	ret += 1; /* add the size of '\0' */

	if (fw_size < (uint32_t)ret)
		return ret;
	else
		return 0;
}

static void dpaa_eth_dev_info(struct rte_eth_dev *dev,
			      struct rte_eth_dev_info *dev_info)
{
	struct dpaa_if *dpaa_intf = dev->data->dev_private;

	PMD_INIT_FUNC_TRACE();

	dev_info->max_rx_queues = dpaa_intf->nb_rx_queues;
	dev_info->max_tx_queues = dpaa_intf->nb_tx_queues;
	dev_info->min_rx_bufsize = DPAA_MIN_RX_BUF_SIZE;
	dev_info->max_rx_pktlen = DPAA_MAX_RX_PKT_LEN;
	dev_info->max_mac_addrs = DPAA_MAX_MAC_FILTER;
	dev_info->max_hash_mac_addrs = 0;
	dev_info->max_vfs = 0;
	dev_info->max_vmdq_pools = ETH_16_POOLS;
	dev_info->flow_type_rss_offloads = DPAA_RSS_OFFLOAD_ALL;
	dev_info->speed_capa = (ETH_LINK_SPEED_1G |
				ETH_LINK_SPEED_10G);
	dev_info->rx_offload_capa =
		(DEV_RX_OFFLOAD_IPV4_CKSUM |
		DEV_RX_OFFLOAD_UDP_CKSUM   |
		DEV_RX_OFFLOAD_TCP_CKSUM);
	dev_info->tx_offload_capa =
		(DEV_TX_OFFLOAD_IPV4_CKSUM  |
		DEV_TX_OFFLOAD_UDP_CKSUM   |
		DEV_TX_OFFLOAD_TCP_CKSUM);
}

static int dpaa_eth_link_update(struct rte_eth_dev *dev,
				int wait_to_complete __rte_unused)
{
	struct dpaa_if *dpaa_intf = dev->data->dev_private;
	struct rte_eth_link *link = &dev->data->dev_link;

	PMD_INIT_FUNC_TRACE();

	if (dpaa_intf->fif->mac_type == fman_mac_1g)
		link->link_speed = 1000;
	else if (dpaa_intf->fif->mac_type == fman_mac_10g)
		link->link_speed = 10000;
	else
		DPAA_PMD_ERR("invalid link_speed: %s, %d",
			     dpaa_intf->name, dpaa_intf->fif->mac_type);

	link->link_status = dpaa_intf->valid;
	link->link_duplex = ETH_LINK_FULL_DUPLEX;
	link->link_autoneg = ETH_LINK_AUTONEG;
	return 0;
}

static int dpaa_eth_stats_get(struct rte_eth_dev *dev,
			       struct rte_eth_stats *stats)
{
	struct dpaa_if *dpaa_intf = dev->data->dev_private;

	PMD_INIT_FUNC_TRACE();

	fman_if_stats_get(dpaa_intf->fif, stats);
	return 0;
}

static void dpaa_eth_stats_reset(struct rte_eth_dev *dev)
{
	struct dpaa_if *dpaa_intf = dev->data->dev_private;

	PMD_INIT_FUNC_TRACE();

	fman_if_stats_reset(dpaa_intf->fif);
}

static int
dpaa_dev_xstats_get(struct rte_eth_dev *dev, struct rte_eth_xstat *xstats,
		    unsigned int n)
{
	struct dpaa_if *dpaa_intf = dev->data->dev_private;
	unsigned int i = 0, num = RTE_DIM(dpaa_xstats_strings);
	uint64_t values[sizeof(struct dpaa_if_stats) / 8];

	if (xstats == NULL)
		return 0;

	if (n < num)
		return num;

	fman_if_stats_get_all(dpaa_intf->fif, values,
			      sizeof(struct dpaa_if_stats) / 8);

	for (i = 0; i < num; i++) {
		xstats[i].id = i;
		xstats[i].value = values[dpaa_xstats_strings[i].offset / 8];
	}
	return i;
}

static int
dpaa_xstats_get_names(__rte_unused struct rte_eth_dev *dev,
		      struct rte_eth_xstat_name *xstats_names,
		      unsigned int limit)
{
	unsigned int i, stat_cnt = RTE_DIM(dpaa_xstats_strings);

	if (limit < stat_cnt)
		return stat_cnt;

	if (xstats_names != NULL)
		for (i = 0; i < stat_cnt; i++)
			snprintf(xstats_names[i].name,
				 sizeof(xstats_names[i].name),
				 "%s",
				 dpaa_xstats_strings[i].name);

	return stat_cnt;
}

static int
dpaa_xstats_get_by_id(struct rte_eth_dev *dev, const uint64_t *ids,
		      uint64_t *values, unsigned int n)
{
	unsigned int i, stat_cnt = RTE_DIM(dpaa_xstats_strings);
	uint64_t values_copy[sizeof(struct dpaa_if_stats) / 8];

	if (!ids) {
		struct dpaa_if *dpaa_intf = dev->data->dev_private;

		if (n < stat_cnt)
			return stat_cnt;

		if (!values)
			return 0;

		fman_if_stats_get_all(dpaa_intf->fif, values_copy,
				      sizeof(struct dpaa_if_stats) / 8);

		for (i = 0; i < stat_cnt; i++)
			values[i] =
				values_copy[dpaa_xstats_strings[i].offset / 8];

		return stat_cnt;
	}

	dpaa_xstats_get_by_id(dev, NULL, values_copy, stat_cnt);

	for (i = 0; i < n; i++) {
		if (ids[i] >= stat_cnt) {
			DPAA_PMD_ERR("id value isn't valid");
			return -1;
		}
		values[i] = values_copy[ids[i]];
	}
	return n;
}

static int
dpaa_xstats_get_names_by_id(
	struct rte_eth_dev *dev,
	struct rte_eth_xstat_name *xstats_names,
	const uint64_t *ids,
	unsigned int limit)
{
	unsigned int i, stat_cnt = RTE_DIM(dpaa_xstats_strings);
	struct rte_eth_xstat_name xstats_names_copy[stat_cnt];

	if (!ids)
		return dpaa_xstats_get_names(dev, xstats_names, limit);

	dpaa_xstats_get_names(dev, xstats_names_copy, limit);

	for (i = 0; i < limit; i++) {
		if (ids[i] >= stat_cnt) {
			DPAA_PMD_ERR("id value isn't valid");
			return -1;
		}
		strcpy(xstats_names[i].name, xstats_names_copy[ids[i]].name);
	}
	return limit;
}

static void dpaa_eth_promiscuous_enable(struct rte_eth_dev *dev)
{
	struct dpaa_if *dpaa_intf = dev->data->dev_private;

	PMD_INIT_FUNC_TRACE();

	fman_if_promiscuous_enable(dpaa_intf->fif);
}

static void dpaa_eth_promiscuous_disable(struct rte_eth_dev *dev)
{
	struct dpaa_if *dpaa_intf = dev->data->dev_private;

	PMD_INIT_FUNC_TRACE();

	fman_if_promiscuous_disable(dpaa_intf->fif);
}

static void dpaa_eth_multicast_enable(struct rte_eth_dev *dev)
{
	struct dpaa_if *dpaa_intf = dev->data->dev_private;

	PMD_INIT_FUNC_TRACE();

	fman_if_set_mcast_filter_table(dpaa_intf->fif);
}

static void dpaa_eth_multicast_disable(struct rte_eth_dev *dev)
{
	struct dpaa_if *dpaa_intf = dev->data->dev_private;

	PMD_INIT_FUNC_TRACE();

	fman_if_reset_mcast_filter_table(dpaa_intf->fif);
}

static
int dpaa_eth_rx_queue_setup(struct rte_eth_dev *dev, uint16_t queue_idx,
			    uint16_t nb_desc __rte_unused,
			    unsigned int socket_id __rte_unused,
			    const struct rte_eth_rxconf *rx_conf __rte_unused,
			    struct rte_mempool *mp)
{
	struct dpaa_if *dpaa_intf = dev->data->dev_private;

	PMD_INIT_FUNC_TRACE();

	DPAA_PMD_INFO("Rx queue setup for queue index: %d", queue_idx);

	if (!dpaa_intf->bp_info || dpaa_intf->bp_info->mp != mp) {
		struct fman_if_ic_params icp;
		uint32_t fd_offset;
		uint32_t bp_size;

		if (!mp->pool_data) {
			DPAA_PMD_ERR("Not an offloaded buffer pool!");
			return -1;
		}
		dpaa_intf->bp_info = DPAA_MEMPOOL_TO_POOL_INFO(mp);

		memset(&icp, 0, sizeof(icp));
		/* set ICEOF for to the default value , which is 0*/
		icp.iciof = DEFAULT_ICIOF;
		icp.iceof = DEFAULT_RX_ICEOF;
		icp.icsz = DEFAULT_ICSZ;
		fman_if_set_ic_params(dpaa_intf->fif, &icp);

		fd_offset = RTE_PKTMBUF_HEADROOM + DPAA_HW_BUF_RESERVE;
		fman_if_set_fdoff(dpaa_intf->fif, fd_offset);

		/* Buffer pool size should be equal to Dataroom Size*/
		bp_size = rte_pktmbuf_data_room_size(mp);
		fman_if_set_bp(dpaa_intf->fif, mp->size,
			       dpaa_intf->bp_info->bpid, bp_size);
		dpaa_intf->valid = 1;
		DPAA_PMD_INFO("if =%s - fd_offset = %d offset = %d",
			    dpaa_intf->name, fd_offset,
			fman_if_get_fdoff(dpaa_intf->fif));
	}
	dev->data->rx_queues[queue_idx] = &dpaa_intf->rx_queues[queue_idx];

	return 0;
}

static
void dpaa_eth_rx_queue_release(void *rxq __rte_unused)
{
	PMD_INIT_FUNC_TRACE();
}

static
int dpaa_eth_tx_queue_setup(struct rte_eth_dev *dev, uint16_t queue_idx,
			    uint16_t nb_desc __rte_unused,
		unsigned int socket_id __rte_unused,
		const struct rte_eth_txconf *tx_conf __rte_unused)
{
	struct dpaa_if *dpaa_intf = dev->data->dev_private;

	PMD_INIT_FUNC_TRACE();

	DPAA_PMD_INFO("Tx queue setup for queue index: %d", queue_idx);
	dev->data->tx_queues[queue_idx] = &dpaa_intf->tx_queues[queue_idx];
	return 0;
}

static void dpaa_eth_tx_queue_release(void *txq __rte_unused)
{
	PMD_INIT_FUNC_TRACE();
}

static int dpaa_link_down(struct rte_eth_dev *dev)
{
	PMD_INIT_FUNC_TRACE();

	dpaa_eth_dev_stop(dev);
	return 0;
}

static int dpaa_link_up(struct rte_eth_dev *dev)
{
	PMD_INIT_FUNC_TRACE();

	dpaa_eth_dev_start(dev);
	return 0;
}

static int
dpaa_flow_ctrl_set(struct rte_eth_dev *dev,
		   struct rte_eth_fc_conf *fc_conf)
{
	struct dpaa_if *dpaa_intf = dev->data->dev_private;
	struct rte_eth_fc_conf *net_fc;

	PMD_INIT_FUNC_TRACE();

	if (!(dpaa_intf->fc_conf)) {
		dpaa_intf->fc_conf = rte_zmalloc(NULL,
			sizeof(struct rte_eth_fc_conf), MAX_CACHELINE);
		if (!dpaa_intf->fc_conf) {
			DPAA_PMD_ERR("unable to save flow control info");
			return -ENOMEM;
		}
	}
	net_fc = dpaa_intf->fc_conf;

	if (fc_conf->high_water < fc_conf->low_water) {
		DPAA_PMD_ERR("Incorrect Flow Control Configuration");
		return -EINVAL;
	}

	if (fc_conf->mode == RTE_FC_NONE) {
		return 0;
	} else if (fc_conf->mode == RTE_FC_TX_PAUSE ||
		 fc_conf->mode == RTE_FC_FULL) {
		fman_if_set_fc_threshold(dpaa_intf->fif, fc_conf->high_water,
					 fc_conf->low_water,
				dpaa_intf->bp_info->bpid);
		if (fc_conf->pause_time)
			fman_if_set_fc_quanta(dpaa_intf->fif,
					      fc_conf->pause_time);
	}

	/* Save the information in dpaa device */
	net_fc->pause_time = fc_conf->pause_time;
	net_fc->high_water = fc_conf->high_water;
	net_fc->low_water = fc_conf->low_water;
	net_fc->send_xon = fc_conf->send_xon;
	net_fc->mac_ctrl_frame_fwd = fc_conf->mac_ctrl_frame_fwd;
	net_fc->mode = fc_conf->mode;
	net_fc->autoneg = fc_conf->autoneg;

	return 0;
}

static int
dpaa_flow_ctrl_get(struct rte_eth_dev *dev,
		   struct rte_eth_fc_conf *fc_conf)
{
	struct dpaa_if *dpaa_intf = dev->data->dev_private;
	struct rte_eth_fc_conf *net_fc = dpaa_intf->fc_conf;
	int ret;

	PMD_INIT_FUNC_TRACE();

	if (net_fc) {
		fc_conf->pause_time = net_fc->pause_time;
		fc_conf->high_water = net_fc->high_water;
		fc_conf->low_water = net_fc->low_water;
		fc_conf->send_xon = net_fc->send_xon;
		fc_conf->mac_ctrl_frame_fwd = net_fc->mac_ctrl_frame_fwd;
		fc_conf->mode = net_fc->mode;
		fc_conf->autoneg = net_fc->autoneg;
		return 0;
	}
	ret = fman_if_get_fc_threshold(dpaa_intf->fif);
	if (ret) {
		fc_conf->mode = RTE_FC_TX_PAUSE;
		fc_conf->pause_time = fman_if_get_fc_quanta(dpaa_intf->fif);
	} else {
		fc_conf->mode = RTE_FC_NONE;
	}

	return 0;
}

static int
dpaa_dev_add_mac_addr(struct rte_eth_dev *dev,
			     struct ether_addr *addr,
			     uint32_t index,
			     __rte_unused uint32_t pool)
{
	int ret;
	struct dpaa_if *dpaa_intf = dev->data->dev_private;

	PMD_INIT_FUNC_TRACE();

	ret = fman_if_add_mac_addr(dpaa_intf->fif, addr->addr_bytes, index);

	if (ret)
		RTE_LOG(ERR, PMD, "error: Adding the MAC ADDR failed:"
			" err = %d", ret);
	return 0;
}

static void
dpaa_dev_remove_mac_addr(struct rte_eth_dev *dev,
			  uint32_t index)
{
	struct dpaa_if *dpaa_intf = dev->data->dev_private;

	PMD_INIT_FUNC_TRACE();

	fman_if_clear_mac_addr(dpaa_intf->fif, index);
}

static void
dpaa_dev_set_mac_addr(struct rte_eth_dev *dev,
		       struct ether_addr *addr)
{
	int ret;
	struct dpaa_if *dpaa_intf = dev->data->dev_private;

	PMD_INIT_FUNC_TRACE();

	ret = fman_if_add_mac_addr(dpaa_intf->fif, addr->addr_bytes, 0);
	if (ret)
		RTE_LOG(ERR, PMD, "error: Setting the MAC ADDR failed %d", ret);
}

static struct eth_dev_ops dpaa_devops = {
	.dev_configure		  = dpaa_eth_dev_configure,
	.dev_start		  = dpaa_eth_dev_start,
	.dev_stop		  = dpaa_eth_dev_stop,
	.dev_close		  = dpaa_eth_dev_close,
	.dev_infos_get		  = dpaa_eth_dev_info,
	.dev_supported_ptypes_get = dpaa_supported_ptypes_get,

	.rx_queue_setup		  = dpaa_eth_rx_queue_setup,
	.tx_queue_setup		  = dpaa_eth_tx_queue_setup,
	.rx_queue_release	  = dpaa_eth_rx_queue_release,
	.tx_queue_release	  = dpaa_eth_tx_queue_release,

	.flow_ctrl_get		  = dpaa_flow_ctrl_get,
	.flow_ctrl_set		  = dpaa_flow_ctrl_set,

	.link_update		  = dpaa_eth_link_update,
	.stats_get		  = dpaa_eth_stats_get,
	.xstats_get		  = dpaa_dev_xstats_get,
	.xstats_get_by_id	  = dpaa_xstats_get_by_id,
	.xstats_get_names_by_id	  = dpaa_xstats_get_names_by_id,
	.xstats_get_names	  = dpaa_xstats_get_names,
	.xstats_reset		  = dpaa_eth_stats_reset,
	.stats_reset		  = dpaa_eth_stats_reset,
	.promiscuous_enable	  = dpaa_eth_promiscuous_enable,
	.promiscuous_disable	  = dpaa_eth_promiscuous_disable,
	.allmulticast_enable	  = dpaa_eth_multicast_enable,
	.allmulticast_disable	  = dpaa_eth_multicast_disable,
	.mtu_set		  = dpaa_mtu_set,
	.dev_set_link_down	  = dpaa_link_down,
	.dev_set_link_up	  = dpaa_link_up,
	.mac_addr_add		  = dpaa_dev_add_mac_addr,
	.mac_addr_remove	  = dpaa_dev_remove_mac_addr,
	.mac_addr_set		  = dpaa_dev_set_mac_addr,

	.fw_version_get		  = dpaa_fw_version_get,
};

static int dpaa_fc_set_default(struct dpaa_if *dpaa_intf)
{
	struct rte_eth_fc_conf *fc_conf;
	int ret;

	PMD_INIT_FUNC_TRACE();

	if (!(dpaa_intf->fc_conf)) {
		dpaa_intf->fc_conf = rte_zmalloc(NULL,
			sizeof(struct rte_eth_fc_conf), MAX_CACHELINE);
		if (!dpaa_intf->fc_conf) {
			DPAA_PMD_ERR("unable to save flow control info");
			return -ENOMEM;
		}
	}
	fc_conf = dpaa_intf->fc_conf;
	ret = fman_if_get_fc_threshold(dpaa_intf->fif);
	if (ret) {
		fc_conf->mode = RTE_FC_TX_PAUSE;
		fc_conf->pause_time = fman_if_get_fc_quanta(dpaa_intf->fif);
	} else {
		fc_conf->mode = RTE_FC_NONE;
	}

	return 0;
}

/* Initialise an Rx FQ */
static int dpaa_rx_queue_init(struct qman_fq *fq,
			      uint32_t fqid)
{
	struct qm_mcc_initfq opts = {0};
	int ret;

	PMD_INIT_FUNC_TRACE();

	ret = qman_reserve_fqid(fqid);
	if (ret) {
		DPAA_PMD_ERR("reserve rx fqid %d failed with ret: %d",
			     fqid, ret);
		return -EINVAL;
	}

	DPAA_PMD_DEBUG("creating rx fq %p, fqid %d", fq, fqid);
	ret = qman_create_fq(fqid, QMAN_FQ_FLAG_NO_ENQUEUE, fq);
	if (ret) {
		DPAA_PMD_ERR("create rx fqid %d failed with ret: %d",
			fqid, ret);
		return ret;
	}

	opts.we_mask = QM_INITFQ_WE_DESTWQ | QM_INITFQ_WE_FQCTRL |
		       QM_INITFQ_WE_CONTEXTA;

	opts.fqd.dest.wq = DPAA_IF_RX_PRIORITY;
	opts.fqd.fq_ctrl = QM_FQCTRL_AVOIDBLOCK | QM_FQCTRL_CTXASTASHING |
			   QM_FQCTRL_PREFERINCACHE;
	opts.fqd.context_a.stashing.exclusive = 0;
	opts.fqd.context_a.stashing.annotation_cl = DPAA_IF_RX_ANNOTATION_STASH;
	opts.fqd.context_a.stashing.data_cl = DPAA_IF_RX_DATA_STASH;
	opts.fqd.context_a.stashing.context_cl = DPAA_IF_RX_CONTEXT_STASH;

	/*Enable tail drop */
	opts.we_mask = opts.we_mask | QM_INITFQ_WE_TDTHRESH;
	opts.fqd.fq_ctrl = opts.fqd.fq_ctrl | QM_FQCTRL_TDE;
	qm_fqd_taildrop_set(&opts.fqd.td, CONG_THRESHOLD_RX_Q, 1);

	ret = qman_init_fq(fq, 0, &opts);
	if (ret)
		DPAA_PMD_ERR("init rx fqid %d failed with ret: %d", fqid, ret);
	return ret;
}

/* Initialise a Tx FQ */
static int dpaa_tx_queue_init(struct qman_fq *fq,
			      struct fman_if *fman_intf)
{
	struct qm_mcc_initfq opts = {0};
	int ret;

	PMD_INIT_FUNC_TRACE();

	ret = qman_create_fq(0, QMAN_FQ_FLAG_DYNAMIC_FQID |
			     QMAN_FQ_FLAG_TO_DCPORTAL, fq);
	if (ret) {
		DPAA_PMD_ERR("create tx fq failed with ret: %d", ret);
		return ret;
	}
	opts.we_mask = QM_INITFQ_WE_DESTWQ | QM_INITFQ_WE_FQCTRL |
		       QM_INITFQ_WE_CONTEXTB | QM_INITFQ_WE_CONTEXTA;
	opts.fqd.dest.channel = fman_intf->tx_channel_id;
	opts.fqd.dest.wq = DPAA_IF_TX_PRIORITY;
	opts.fqd.fq_ctrl = QM_FQCTRL_PREFERINCACHE;
	opts.fqd.context_b = 0;
	/* no tx-confirmation */
	opts.fqd.context_a.hi = 0x80000000 | fman_dealloc_bufs_mask_hi;
	opts.fqd.context_a.lo = 0 | fman_dealloc_bufs_mask_lo;
	DPAA_PMD_DEBUG("init tx fq %p, fqid %d", fq, fq->fqid);
	ret = qman_init_fq(fq, QMAN_INITFQ_FLAG_SCHED, &opts);
	if (ret)
		DPAA_PMD_ERR("init tx fqid %d failed %d", fq->fqid, ret);
	return ret;
}

#ifdef RTE_LIBRTE_DPAA_DEBUG_DRIVER
/* Initialise a DEBUG FQ ([rt]x_error, rx_default). */
static int dpaa_debug_queue_init(struct qman_fq *fq, uint32_t fqid)
{
	struct qm_mcc_initfq opts = {0};
	int ret;

	PMD_INIT_FUNC_TRACE();

	ret = qman_reserve_fqid(fqid);
	if (ret) {
		DPAA_PMD_ERR("Reserve debug fqid %d failed with ret: %d",
			fqid, ret);
		return -EINVAL;
	}
	/* "map" this Rx FQ to one of the interfaces Tx FQID */
	DPAA_PMD_DEBUG("Creating debug fq %p, fqid %d", fq, fqid);
	ret = qman_create_fq(fqid, QMAN_FQ_FLAG_NO_ENQUEUE, fq);
	if (ret) {
		DPAA_PMD_ERR("create debug fqid %d failed with ret: %d",
			fqid, ret);
		return ret;
	}
	opts.we_mask = QM_INITFQ_WE_DESTWQ | QM_INITFQ_WE_FQCTRL;
	opts.fqd.dest.wq = DPAA_IF_DEBUG_PRIORITY;
	ret = qman_init_fq(fq, 0, &opts);
	if (ret)
		DPAA_PMD_ERR("init debug fqid %d failed with ret: %d",
			    fqid, ret);
	return ret;
}
#endif

/* Initialise a network interface */
static int
dpaa_dev_init(struct rte_eth_dev *eth_dev)
{
	int num_cores, num_rx_fqs, fqid;
	int loop, ret = 0;
	int dev_id;
	struct rte_dpaa_device *dpaa_device;
	struct dpaa_if *dpaa_intf;
	struct fm_eth_port_cfg *cfg;
	struct fman_if *fman_intf;
	struct fman_if_bpool *bp, *tmp_bp;

	PMD_INIT_FUNC_TRACE();

	/* For secondary processes, the primary has done all the work */
	if (rte_eal_process_type() != RTE_PROC_PRIMARY) {
		eth_dev->dev_ops = &dpaa_devops;
		/* Plugging of UCODE burst API not supported in Secondary */
		eth_dev->rx_pkt_burst = dpaa_eth_queue_rx;
		return 0;
	}

	dpaa_device = DEV_TO_DPAA_DEVICE(eth_dev->device);
	dev_id = dpaa_device->id.dev_id;
	dpaa_intf = eth_dev->data->dev_private;
	cfg = &dpaa_netcfg->port_cfg[dev_id];
	fman_intf = cfg->fman_if;

	dpaa_intf->name = dpaa_device->name;

	/* save fman_if & cfg in the interface struture */
	dpaa_intf->fif = fman_intf;
	dpaa_intf->ifid = dev_id;
	dpaa_intf->cfg = cfg;

	/* Initialize Rx FQ's */
	if (getenv("DPAA_NUM_RX_QUEUES"))
		num_rx_fqs = atoi(getenv("DPAA_NUM_RX_QUEUES"));
	else
		num_rx_fqs = DPAA_DEFAULT_NUM_PCD_QUEUES;

	/* Each device can not have more than DPAA_PCD_FQID_MULTIPLIER RX
	 * queues.
	 */
	if (num_rx_fqs <= 0 || num_rx_fqs > DPAA_PCD_FQID_MULTIPLIER) {
		DPAA_PMD_ERR("Invalid number of RX queues\n");
		return -EINVAL;
	}

	dpaa_intf->rx_queues = rte_zmalloc(NULL,
		sizeof(struct qman_fq) * num_rx_fqs, MAX_CACHELINE);
	if (!dpaa_intf->rx_queues) {
		DPAA_PMD_ERR("Failed to alloc mem for RX queues\n");
		return -ENOMEM;
	}

	for (loop = 0; loop < num_rx_fqs; loop++) {
		fqid = DPAA_PCD_FQID_START + dpaa_intf->ifid *
			DPAA_PCD_FQID_MULTIPLIER + loop;
		ret = dpaa_rx_queue_init(&dpaa_intf->rx_queues[loop], fqid);
		if (ret)
			goto free_rx;
		dpaa_intf->rx_queues[loop].dpaa_intf = dpaa_intf;
	}
	dpaa_intf->nb_rx_queues = num_rx_fqs;

	/* Initialise Tx FQs. Have as many Tx FQ's as number of cores */
	num_cores = rte_lcore_count();
	dpaa_intf->tx_queues = rte_zmalloc(NULL, sizeof(struct qman_fq) *
		num_cores, MAX_CACHELINE);
	if (!dpaa_intf->tx_queues) {
		DPAA_PMD_ERR("Failed to alloc mem for TX queues\n");
		ret = -ENOMEM;
		goto free_rx;
	}

	for (loop = 0; loop < num_cores; loop++) {
		ret = dpaa_tx_queue_init(&dpaa_intf->tx_queues[loop],
					 fman_intf);
		if (ret)
			goto free_tx;
		dpaa_intf->tx_queues[loop].dpaa_intf = dpaa_intf;
	}
	dpaa_intf->nb_tx_queues = num_cores;

#ifdef RTE_LIBRTE_DPAA_DEBUG_DRIVER
	dpaa_debug_queue_init(&dpaa_intf->debug_queues[
		DPAA_DEBUG_FQ_RX_ERROR], fman_intf->fqid_rx_err);
	dpaa_intf->debug_queues[DPAA_DEBUG_FQ_RX_ERROR].dpaa_intf = dpaa_intf;
	dpaa_debug_queue_init(&dpaa_intf->debug_queues[
		DPAA_DEBUG_FQ_TX_ERROR], fman_intf->fqid_tx_err);
	dpaa_intf->debug_queues[DPAA_DEBUG_FQ_TX_ERROR].dpaa_intf = dpaa_intf;
#endif

	DPAA_PMD_DEBUG("All frame queues created");

	/* Get the initial configuration for flow control */
	dpaa_fc_set_default(dpaa_intf);

	/* reset bpool list, initialize bpool dynamically */
	list_for_each_entry_safe(bp, tmp_bp, &cfg->fman_if->bpool_list, node) {
		list_del(&bp->node);
		free(bp);
	}

	/* Populate ethdev structure */
	eth_dev->dev_ops = &dpaa_devops;
	eth_dev->rx_pkt_burst = dpaa_eth_queue_rx;
	eth_dev->tx_pkt_burst = dpaa_eth_tx_drop_all;

	/* Allocate memory for storing MAC addresses */
	eth_dev->data->mac_addrs = rte_zmalloc("mac_addr",
		ETHER_ADDR_LEN * DPAA_MAX_MAC_FILTER, 0);
	if (eth_dev->data->mac_addrs == NULL) {
		DPAA_PMD_ERR("Failed to allocate %d bytes needed to "
						"store MAC addresses",
				ETHER_ADDR_LEN * DPAA_MAX_MAC_FILTER);
		ret = -ENOMEM;
		goto free_tx;
	}

	/* copy the primary mac address */
	ether_addr_copy(&fman_intf->mac_addr, &eth_dev->data->mac_addrs[0]);

	RTE_LOG(INFO, PMD, "net: dpaa: %s: %02x:%02x:%02x:%02x:%02x:%02x\n",
		dpaa_device->name,
		fman_intf->mac_addr.addr_bytes[0],
		fman_intf->mac_addr.addr_bytes[1],
		fman_intf->mac_addr.addr_bytes[2],
		fman_intf->mac_addr.addr_bytes[3],
		fman_intf->mac_addr.addr_bytes[4],
		fman_intf->mac_addr.addr_bytes[5]);

	/* Disable RX mode */
	fman_if_discard_rx_errors(fman_intf);
	fman_if_disable_rx(fman_intf);
	/* Disable promiscuous mode */
	fman_if_promiscuous_disable(fman_intf);
	/* Disable multicast */
	fman_if_reset_mcast_filter_table(fman_intf);
	/* Reset interface statistics */
	fman_if_stats_reset(fman_intf);

	return 0;

free_tx:
	rte_free(dpaa_intf->tx_queues);
	dpaa_intf->tx_queues = NULL;
	dpaa_intf->nb_tx_queues = 0;

free_rx:
	rte_free(dpaa_intf->rx_queues);
	dpaa_intf->rx_queues = NULL;
	dpaa_intf->nb_rx_queues = 0;
	return ret;
}

static int
dpaa_dev_uninit(struct rte_eth_dev *dev)
{
	struct dpaa_if *dpaa_intf = dev->data->dev_private;

	PMD_INIT_FUNC_TRACE();

	if (rte_eal_process_type() != RTE_PROC_PRIMARY)
		return -EPERM;

	if (!dpaa_intf) {
		DPAA_PMD_WARN("Already closed or not started");
		return -1;
	}

	dpaa_eth_dev_close(dev);

	/* release configuration memory */
	if (dpaa_intf->fc_conf)
		rte_free(dpaa_intf->fc_conf);

	rte_free(dpaa_intf->rx_queues);
	dpaa_intf->rx_queues = NULL;

	rte_free(dpaa_intf->tx_queues);
	dpaa_intf->tx_queues = NULL;

	/* free memory for storing MAC addresses */
	rte_free(dev->data->mac_addrs);
	dev->data->mac_addrs = NULL;

	dev->dev_ops = NULL;
	dev->rx_pkt_burst = NULL;
	dev->tx_pkt_burst = NULL;

	return 0;
}

static int
rte_dpaa_probe(struct rte_dpaa_driver *dpaa_drv,
	       struct rte_dpaa_device *dpaa_dev)
{
	int diag;
	int ret;
	struct rte_eth_dev *eth_dev;

	PMD_INIT_FUNC_TRACE();

	/* In case of secondary process, the device is already configured
	 * and no further action is required, except portal initialization
	 * and verifying secondary attachment to port name.
	 */
	if (rte_eal_process_type() != RTE_PROC_PRIMARY) {
		eth_dev = rte_eth_dev_attach_secondary(dpaa_dev->name);
		if (!eth_dev)
			return -ENOMEM;
		return 0;
	}

	if (!is_global_init) {
		/* One time load of Qman/Bman drivers */
		ret = qman_global_init();
		if (ret) {
			DPAA_PMD_ERR("QMAN initialization failed: %d",
				     ret);
			return ret;
		}
		ret = bman_global_init();
		if (ret) {
			DPAA_PMD_ERR("BMAN initialization failed: %d",
				     ret);
			return ret;
		}

		is_global_init = 1;
	}

	ret = rte_dpaa_portal_init((void *)1);
	if (ret) {
		DPAA_PMD_ERR("Unable to initialize portal");
		return ret;
	}

	eth_dev = rte_eth_dev_allocate(dpaa_dev->name);
	if (eth_dev == NULL)
		return -ENOMEM;

	eth_dev->data->dev_private = rte_zmalloc(
					"ethdev private structure",
					sizeof(struct dpaa_if),
					RTE_CACHE_LINE_SIZE);
	if (!eth_dev->data->dev_private) {
		DPAA_PMD_ERR("Cannot allocate memzone for port data");
		rte_eth_dev_release_port(eth_dev);
		return -ENOMEM;
	}

	eth_dev->device = &dpaa_dev->device;
	eth_dev->device->driver = &dpaa_drv->driver;
	dpaa_dev->eth_dev = eth_dev;

	/* Invoke PMD device initialization function */
	diag = dpaa_dev_init(eth_dev);
	if (diag == 0)
		return 0;

	if (rte_eal_process_type() == RTE_PROC_PRIMARY)
		rte_free(eth_dev->data->dev_private);

	rte_eth_dev_release_port(eth_dev);
	return diag;
}

static int
rte_dpaa_remove(struct rte_dpaa_device *dpaa_dev)
{
	struct rte_eth_dev *eth_dev;

	PMD_INIT_FUNC_TRACE();

	eth_dev = dpaa_dev->eth_dev;
	dpaa_dev_uninit(eth_dev);

	if (rte_eal_process_type() == RTE_PROC_PRIMARY)
		rte_free(eth_dev->data->dev_private);

	rte_eth_dev_release_port(eth_dev);

	return 0;
}

static struct rte_dpaa_driver rte_dpaa_pmd = {
	.drv_type = FSL_DPAA_ETH,
	.probe = rte_dpaa_probe,
	.remove = rte_dpaa_remove,
};

RTE_PMD_REGISTER_DPAA(net_dpaa, rte_dpaa_pmd);
