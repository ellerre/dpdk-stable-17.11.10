/*-
 *   BSD LICENSE
 *
 *   Copyright 2015 6WIND S.A.
 *   Copyright 2015 Mellanox.
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
 *     * Neither the name of 6WIND S.A. nor the names of its
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

#ifndef RTE_PMD_MLX5_H_
#define RTE_PMD_MLX5_H_

#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/queue.h>

/* Verbs header. */
/* ISO C doesn't support unnamed structs/unions, disabling -pedantic. */
#ifdef PEDANTIC
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#include <infiniband/verbs.h>
#ifdef PEDANTIC
#pragma GCC diagnostic error "-Wpedantic"
#endif

#include <rte_pci.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_spinlock.h>
#include <rte_interrupts.h>
#include <rte_errno.h>
#include <rte_flow.h>

#include "mlx5_utils.h"
#include "mlx5_rxtx.h"
#include "mlx5_autoconf.h"
#include "mlx5_defs.h"

enum {
	PCI_VENDOR_ID_MELLANOX = 0x15b3,
};

enum {
	PCI_DEVICE_ID_MELLANOX_CONNECTX4 = 0x1013,
	PCI_DEVICE_ID_MELLANOX_CONNECTX4VF = 0x1014,
	PCI_DEVICE_ID_MELLANOX_CONNECTX4LX = 0x1015,
	PCI_DEVICE_ID_MELLANOX_CONNECTX4LXVF = 0x1016,
	PCI_DEVICE_ID_MELLANOX_CONNECTX5 = 0x1017,
	PCI_DEVICE_ID_MELLANOX_CONNECTX5VF = 0x1018,
	PCI_DEVICE_ID_MELLANOX_CONNECTX5EX = 0x1019,
	PCI_DEVICE_ID_MELLANOX_CONNECTX5EXVF = 0x101a,
	PCI_DEVICE_ID_MELLANOX_CONNECTX5BF = 0xa2d2,
};

struct mlx5_xstats_ctrl {
	/* Number of device stats. */
	uint16_t stats_n;
	/* Index in the device counters table. */
	uint16_t dev_table_idx[MLX5_MAX_XSTATS];
	uint64_t base[MLX5_MAX_XSTATS];
};

/* Flow list . */
TAILQ_HEAD(mlx5_flows, rte_flow);

/**
 * Type of objet being allocated.
 */
enum mlx5_verbs_alloc_type {
	MLX5_VERBS_ALLOC_TYPE_NONE,
	MLX5_VERBS_ALLOC_TYPE_TX_QUEUE,
	MLX5_VERBS_ALLOC_TYPE_RX_QUEUE,
};

/**
 * Verbs allocator needs a context to know in the callback which kind of
 * resources it is allocating.
 */
struct mlx5_verbs_alloc_ctx {
	enum mlx5_verbs_alloc_type type; /* Kind of object being allocated. */
	const void *obj; /* Pointer to the DPDK object. */
};

struct priv {
	struct rte_eth_dev_data *dev_data;  /* Pointer to device data. */
	struct ibv_context *ctx; /* Verbs context. */
	struct ibv_device_attr_ex device_attr; /* Device properties. */
	struct ibv_pd *pd; /* Protection Domain. */
	char ibdev_path[IBV_SYSFS_PATH_MAX]; /* IB device path for secondary */
	struct ether_addr mac[MLX5_MAX_MAC_ADDRESSES]; /* MAC addresses. */
	uint16_t vlan_filter[MLX5_MAX_VLAN_IDS]; /* VLAN filters table. */
	unsigned int vlan_filter_n; /* Number of configured VLAN filters. */
	/* Device properties. */
	uint16_t mtu; /* Configured MTU. */
	uint8_t port; /* Physical port number. */
	unsigned int hw_csum:1; /* Checksum offload is supported. */
	unsigned int hw_csum_l2tun:1; /* Same for L2 tunnels. */
	unsigned int hw_vlan_strip:1; /* VLAN stripping is supported. */
	unsigned int hw_fcs_strip:1; /* FCS stripping is supported. */
	unsigned int hw_padding:1; /* End alignment padding is supported. */
	unsigned int mps:2; /* Multi-packet send mode (0: disabled). */
	unsigned int mpw_hdr_dseg:1; /* Enable DSEGs in the title WQEBB. */
	unsigned int cqe_comp:1; /* Whether CQE compression is enabled. */
	unsigned int tso:1; /* Whether TSO is supported. */
	unsigned int tunnel_en:1;
	unsigned int isolated:1; /* Whether isolated mode is enabled. */
	unsigned int tx_vec_en:1; /* Whether Tx vector is enabled. */
	unsigned int rx_vec_en:1; /* Whether Rx vector is enabled. */
	unsigned int counter_set_supported:1; /* Counter set is supported. */
	/* Whether Tx offloads for tunneled packets are supported. */
	unsigned int max_tso_payload_sz; /* Maximum TCP payload for TSO. */
	unsigned int txq_inline; /* Maximum packet size for inlining. */
	unsigned int txqs_inline; /* Queue number threshold for inlining. */
	unsigned int txqs_vec; /* Queue number threshold for vectorized Tx. */
	unsigned int inline_max_packet_sz; /* Max packet size for inlining. */
	/* RX/TX queues. */
	unsigned int rxqs_n; /* RX queues array size. */
	unsigned int txqs_n; /* TX queues array size. */
	struct mlx5_rxq_data *(*rxqs)[]; /* RX queues. */
	struct mlx5_txq_data *(*txqs)[]; /* TX queues. */
	unsigned int ind_table_max_size; /* Maximum indirection table size. */
	struct rte_eth_rss_conf rss_conf; /* RSS configuration. */
	struct rte_intr_handle intr_handle; /* Interrupt handler. */
	unsigned int (*reta_idx)[]; /* RETA index table. */
	unsigned int reta_idx_n; /* RETA index size. */
	struct mlx5_hrxq_drop *flow_drop_queue; /* Flow drop queue. */
	struct mlx5_flows flows; /* RTE Flow rules. */
	struct mlx5_flows ctrl_flows; /* Control flow rules. */
	struct mlx5_mr (*mr)[]; /* Static MR table. */
	struct mlx5_mr_cache (*mr_cache)[]; /* Global MR cache table. */
	unsigned int mr_n; /* Size of static MR table. */
	LIST_HEAD(rxq, mlx5_rxq_ctrl) rxqsctrl; /* DPDK Rx queues. */
	LIST_HEAD(rxqibv, mlx5_rxq_ibv) rxqsibv; /* Verbs Rx queues. */
	LIST_HEAD(hrxq, mlx5_hrxq) hrxqs; /* Verbs Hash Rx queues. */
	LIST_HEAD(txq, mlx5_txq_ctrl) txqsctrl; /* DPDK Tx queues. */
	LIST_HEAD(txqibv, mlx5_txq_ibv) txqsibv; /* Verbs Tx queues. */
	/* Verbs Indirection tables. */
	LIST_HEAD(ind_tables, mlx5_ind_table_ibv) ind_tbls;
	uint32_t link_speed_capa; /* Link speed capabilities. */
	struct mlx5_xstats_ctrl xstats_ctrl; /* Extended stats control. */
	rte_spinlock_t mr_lock; /* MR Lock. */
	int primary_socket; /* Unix socket for primary process. */
	void *uar_base; /* Reserved address space for UAR mapping */
	struct rte_intr_handle intr_handle_socket; /* Interrupt handler. */
	struct mlx5_verbs_alloc_ctx verbs_alloc_ctx;
	/* Context for Verbs allocator. */
};

#define PORT_ID(priv) ((priv)->dev_data->port_id)
#define ETH_DEV(priv) (&rte_eth_devices[PORT_ID(priv)])

/* mlx5.c */

int mlx5_getenv_int(const char *);

/* mlx5_ethdev.c */

int mlx5_get_ifname(const struct rte_eth_dev *dev, char (*ifname)[IF_NAMESIZE]);
int mlx5_ifreq(const struct rte_eth_dev *dev, int req, struct ifreq *ifr);
int mlx5_get_mtu(struct rte_eth_dev *dev, uint16_t *mtu);
int mlx5_set_flags(struct rte_eth_dev *dev, unsigned int keep,
		   unsigned int flags);
int mlx5_dev_configure(struct rte_eth_dev *dev);
void mlx5_dev_infos_get(struct rte_eth_dev *dev, struct rte_eth_dev_info *info);
const uint32_t *mlx5_dev_supported_ptypes_get(struct rte_eth_dev *dev);
int mlx5_link_update(struct rte_eth_dev *dev, int wait_to_complete);
int mlx5_force_link_status_change(struct rte_eth_dev *dev, int status);
int mlx5_dev_set_mtu(struct rte_eth_dev *dev, uint16_t mtu);
int mlx5_dev_get_flow_ctrl(struct rte_eth_dev *dev,
			   struct rte_eth_fc_conf *fc_conf);
int mlx5_dev_set_flow_ctrl(struct rte_eth_dev *dev,
			   struct rte_eth_fc_conf *fc_conf);
int mlx5_ibv_device_to_pci_addr(const struct ibv_device *device,
				struct rte_pci_addr *pci_addr);
void mlx5_dev_link_status_handler(void *arg);
void mlx5_dev_interrupt_handler(void *arg);
void mlx5_dev_interrupt_handler_uninstall(struct rte_eth_dev *dev);
void mlx5_dev_interrupt_handler_install(struct rte_eth_dev *dev);
int mlx5_set_link_down(struct rte_eth_dev *dev);
int mlx5_set_link_up(struct rte_eth_dev *dev);
eth_tx_burst_t mlx5_select_tx_function(struct rte_eth_dev *dev);
eth_rx_burst_t mlx5_select_rx_function(struct rte_eth_dev *dev);

/* mlx5_mac.c */

int mlx5_get_mac(struct rte_eth_dev *dev, uint8_t (*mac)[ETHER_ADDR_LEN]);
void mlx5_mac_addr_remove(struct rte_eth_dev *dev, uint32_t index);
int mlx5_mac_addr_add(struct rte_eth_dev *dev, struct ether_addr *mac,
		      uint32_t index, uint32_t vmdq);
void mlx5_mac_addr_set(struct rte_eth_dev *dev, struct ether_addr *mac_addr);

/* mlx5_rss.c */

int mlx5_rss_hash_update(struct rte_eth_dev *dev,
			 struct rte_eth_rss_conf *rss_conf);
int mlx5_rss_hash_conf_get(struct rte_eth_dev *dev,
			   struct rte_eth_rss_conf *rss_conf);
int mlx5_rss_reta_index_resize(struct rte_eth_dev *dev, unsigned int reta_size);
int mlx5_dev_rss_reta_query(struct rte_eth_dev *dev,
			    struct rte_eth_rss_reta_entry64 *reta_conf,
			    uint16_t reta_size);
int mlx5_dev_rss_reta_update(struct rte_eth_dev *dev,
			     struct rte_eth_rss_reta_entry64 *reta_conf,
			     uint16_t reta_size);

/* mlx5_rxmode.c */

void mlx5_promiscuous_enable(struct rte_eth_dev *dev);
void mlx5_promiscuous_disable(struct rte_eth_dev *dev);
void mlx5_allmulticast_enable(struct rte_eth_dev *dev);
void mlx5_allmulticast_disable(struct rte_eth_dev *dev);

/* mlx5_stats.c */

void mlx5_xstats_init(struct rte_eth_dev *dev);
int mlx5_stats_get(struct rte_eth_dev *dev, struct rte_eth_stats *stats);
void mlx5_stats_reset(struct rte_eth_dev *dev);
int mlx5_xstats_get(struct rte_eth_dev *dev, struct rte_eth_xstat *stats,
		    unsigned int n);
void mlx5_xstats_reset(struct rte_eth_dev *dev);
int mlx5_xstats_get_names(struct rte_eth_dev *dev __rte_unused,
			  struct rte_eth_xstat_name *xstats_names,
			  unsigned int n);

/* mlx5_vlan.c */

int mlx5_vlan_filter_set(struct rte_eth_dev *dev, uint16_t vlan_id, int on);
void mlx5_vlan_strip_queue_set(struct rte_eth_dev *dev, uint16_t queue, int on);
int mlx5_vlan_offload_set(struct rte_eth_dev *dev, int mask);

/* mlx5_trigger.c */

int mlx5_dev_start(struct rte_eth_dev *dev);
void mlx5_dev_stop(struct rte_eth_dev *dev);
int mlx5_traffic_enable(struct rte_eth_dev *dev);
void mlx5_traffic_disable(struct rte_eth_dev *dev);
int mlx5_traffic_restart(struct rte_eth_dev *dev);

/* mlx5_flow.c */

int mlx5_flow_validate(struct rte_eth_dev *dev,
		       const struct rte_flow_attr *attr,
		       const struct rte_flow_item items[],
		       const struct rte_flow_action actions[],
		       struct rte_flow_error *error);
struct rte_flow *mlx5_flow_create(struct rte_eth_dev *dev,
				  const struct rte_flow_attr *attr,
				  const struct rte_flow_item items[],
				  const struct rte_flow_action actions[],
				  struct rte_flow_error *error);
int mlx5_flow_destroy(struct rte_eth_dev *dev, struct rte_flow *flow,
		      struct rte_flow_error *error);
void mlx5_flow_list_flush(struct rte_eth_dev *dev, struct mlx5_flows *list);
int mlx5_flow_flush(struct rte_eth_dev *dev, struct rte_flow_error *error);
int mlx5_flow_query(struct rte_eth_dev *dev, struct rte_flow *flow,
		    enum rte_flow_action_type action, void *data,
		    struct rte_flow_error *error);
int mlx5_flow_isolate(struct rte_eth_dev *dev, int enable,
		      struct rte_flow_error *error);
int mlx5_dev_filter_ctrl(struct rte_eth_dev *dev,
			 enum rte_filter_type filter_type,
			 enum rte_filter_op filter_op,
			 void *arg);
int mlx5_flow_start(struct rte_eth_dev *dev, struct mlx5_flows *list);
void mlx5_flow_stop(struct rte_eth_dev *dev, struct mlx5_flows *list);
int mlx5_flow_verify(struct rte_eth_dev *dev);
int mlx5_ctrl_flow_vlan(struct rte_eth_dev *dev,
			struct rte_flow_item_eth *eth_spec,
			struct rte_flow_item_eth *eth_mask,
			struct rte_flow_item_vlan *vlan_spec,
			struct rte_flow_item_vlan *vlan_mask);
int mlx5_ctrl_flow(struct rte_eth_dev *dev,
		   struct rte_flow_item_eth *eth_spec,
		   struct rte_flow_item_eth *eth_mask);
int mlx5_flow_create_drop_queue(struct rte_eth_dev *dev);
void mlx5_flow_delete_drop_queue(struct rte_eth_dev *dev);

/* mlx5_socket.c */

int mlx5_socket_init(struct rte_eth_dev *dev);
void mlx5_socket_uninit(struct rte_eth_dev *dev);
void mlx5_socket_handle(struct rte_eth_dev *dev);
int mlx5_socket_connect(struct rte_eth_dev *dev);

/* mlx5_mr.c */

int mlx5_mr_register_memseg(struct rte_eth_dev *dev);
void mlx5_mr_deregister_memseg(struct rte_eth_dev *dev);

#endif /* RTE_PMD_MLX5_H_ */
