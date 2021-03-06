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

#ifndef RTE_PMD_MLX5_RXTX_H_
#define RTE_PMD_MLX5_RXTX_H_

#include <stddef.h>
#include <stdint.h>
#include <sys/queue.h>

/* Verbs header. */
/* ISO C doesn't support unnamed structs/unions, disabling -pedantic. */
#ifdef PEDANTIC
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#include <infiniband/verbs.h>
#include <infiniband/mlx5dv.h>
#ifdef PEDANTIC
#pragma GCC diagnostic error "-Wpedantic"
#endif

#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <rte_common.h>
#include <rte_hexdump.h>
#include <rte_atomic.h>

#include "mlx5_utils.h"
#include "mlx5.h"
#include "mlx5_autoconf.h"
#include "mlx5_defs.h"
#include "mlx5_prm.h"

struct mlx5_rxq_stats {
	unsigned int idx; /**< Mapping index. */
#ifdef MLX5_PMD_SOFT_COUNTERS
	uint64_t ipackets; /**< Total of successfully received packets. */
	uint64_t ibytes; /**< Total of successfully received bytes. */
#endif
	uint64_t idropped; /**< Total of packets dropped when RX ring full. */
	uint64_t rx_nombuf; /**< Total of RX mbuf allocation failures. */
};

struct mlx5_txq_stats {
	unsigned int idx; /**< Mapping index. */
#ifdef MLX5_PMD_SOFT_COUNTERS
	uint64_t opackets; /**< Total of successfully sent packets. */
	uint64_t obytes; /**< Total of successfully sent bytes. */
#endif
	uint64_t oerrors; /**< Total number of failed transmitted packets. */
};

struct priv;

/* Memory Region object. */
struct mlx5_mr {
	const struct rte_memseg *memseg;
	struct ibv_mr *ibv_mr; /* Verbs Memory Region. */
};

/* Cache entry for Memory Region. */
struct mlx5_mr_cache {
	uintptr_t start; /* Start address of MR. */
	uintptr_t end; /* End address of MR. */
	uint32_t lkey; /* rte_cpu_to_be_32(ibv_mr->lkey). */
} __rte_packed;

/* Per-queue MR control descriptor. */
struct mlx5_mr_ctrl {
	uint16_t bh_n; /* Size of MR cache table for bottom-half. */
	uint16_t mru; /* Index of last hit entry. */
	uint16_t head; /* Index of the oldest entry. */
	struct mlx5_mr_cache cache[MLX5_MR_CACHE_N]; /* MR cache. */
	struct mlx5_mr_cache (*cache_bh)[]; /* MR cache for bottom-half. */
} __rte_packed;

/* MR table size including padding at index 0. */
#define MR_TABLE_SZ(n) ((n) + MLX5_MR_LOOKUP_TABLE_PAD)

/* Actual table size excluding padding at index 0. */
#define MR_N(n) ((n) - MLX5_MR_LOOKUP_TABLE_PAD)

/* Whether there's only one entry in MR lookup table. */
#define IS_SINGLE_MR(n) (MR_N(n) <= 1)

/* Compressed CQE context. */
struct rxq_zip {
	uint16_t ai; /* Array index. */
	uint16_t ca; /* Current array index. */
	uint16_t na; /* Next array index. */
	uint16_t cq_ci; /* The next CQE. */
	uint32_t cqe_cnt; /* Number of CQEs. */
};

/* RX queue descriptor. */
struct mlx5_rxq_data {
	unsigned int csum:1; /* Enable checksum offloading. */
	unsigned int csum_l2tun:1; /* Same for L2 tunnels. */
	unsigned int hw_timestamp:1; /* Enable HW timestamp. */
	unsigned int vlan_strip:1; /* Enable VLAN stripping. */
	unsigned int crc_present:1; /* CRC must be subtracted. */
	unsigned int sges_n:2; /* Log 2 of SGEs (max buffers per packet). */
	unsigned int cqe_n:4; /* Log 2 of CQ elements. */
	unsigned int elts_n:4; /* Log 2 of Mbufs. */
	unsigned int rss_hash:1; /* RSS hash result is enabled. */
	unsigned int mark:1; /* Marked flow available on the queue. */
	unsigned int :15; /* Remaining bits. */
	volatile uint32_t *rq_db;
	volatile uint32_t *cq_db;
	uint16_t port_id;
	uint32_t rq_ci;
	uint32_t rq_pi;
	uint32_t cq_ci;
	uint16_t rq_repl_thresh; /* Threshold for buffer replenishment. */
	struct mlx5_mr_ctrl mr_ctrl;
	volatile struct mlx5_wqe_data_seg(*wqes)[];
	volatile struct mlx5_cqe(*cqes)[];
	struct rxq_zip zip; /* Compressed context. */
	struct rte_mbuf *(*elts)[];
	struct rte_mempool *mp;
	struct mlx5_rxq_stats stats;
	uint64_t mbuf_initializer; /* Default rearm_data for vectorized Rx. */
	struct rte_mbuf fake_mbuf; /* elts padding for vectorized Rx. */
	void *cq_uar; /* CQ user access region. */
	uint32_t cqn; /* CQ number. */
	uint8_t cq_arm_sn; /* CQ arm seq number. */
} __rte_cache_aligned;

/* Verbs Rx queue elements. */
struct mlx5_rxq_ibv {
	LIST_ENTRY(mlx5_rxq_ibv) next; /* Pointer to the next element. */
	rte_atomic32_t refcnt; /* Reference counter. */
	struct mlx5_rxq_ctrl *rxq_ctrl; /* Back pointer to parent. */
	struct ibv_cq *cq; /* Completion Queue. */
	struct ibv_wq *wq; /* Work Queue. */
	struct ibv_comp_channel *channel;
};

/* RX queue control descriptor. */
struct mlx5_rxq_ctrl {
	LIST_ENTRY(mlx5_rxq_ctrl) next; /* Pointer to the next element. */
	rte_atomic32_t refcnt; /* Reference counter. */
	struct priv *priv; /* Back pointer to private data. */
	struct mlx5_rxq_ibv *ibv; /* Verbs elements. */
	struct mlx5_rxq_data rxq; /* Data path structure. */
	unsigned int socket; /* CPU socket ID for allocations. */
	unsigned int irq:1; /* Whether IRQ is enabled. */
	uint16_t idx; /* Queue index. */
};

/* Indirection table. */
struct mlx5_ind_table_ibv {
	LIST_ENTRY(mlx5_ind_table_ibv) next; /* Pointer to the next element. */
	rte_atomic32_t refcnt; /* Reference counter. */
	struct ibv_rwq_ind_table *ind_table; /**< Indirection table. */
	uint16_t queues_n; /**< Number of queues in the list. */
	uint16_t queues[]; /**< Queue list. */
};

/* Hash Rx queue. */
struct mlx5_hrxq {
	LIST_ENTRY(mlx5_hrxq) next; /* Pointer to the next element. */
	rte_atomic32_t refcnt; /* Reference counter. */
	struct mlx5_ind_table_ibv *ind_table; /* Indirection table. */
	struct ibv_qp *qp; /* Verbs queue pair. */
	uint64_t hash_fields; /* Verbs Hash fields. */
	uint8_t rss_key_len; /* Hash key length in bytes. */
	uint8_t rss_key[]; /* Hash key. */
};

/* TX queue descriptor. */
__extension__
struct mlx5_txq_data {
	uint16_t elts_head; /* Current counter in (*elts)[]. */
	uint16_t elts_tail; /* Counter of first element awaiting completion. */
	uint16_t elts_comp; /* Counter since last completion request. */
	uint16_t mpw_comp; /* WQ index since last completion request. */
	uint16_t cq_ci; /* Consumer index for completion queue. */
#ifndef NDEBUG
	uint16_t cq_pi; /* Producer index for completion queue. */
#endif
	uint16_t wqe_ci; /* Consumer index for work queue. */
	uint16_t wqe_pi; /* Producer index for work queue. */
	uint16_t elts_n:4; /* (*elts)[] length (in log2). */
	uint16_t cqe_n:4; /* Number of CQ elements (in log2). */
	uint16_t wqe_n:4; /* Number of of WQ elements (in log2). */
	uint16_t inline_en:1; /* When set inline is enabled. */
	uint16_t tso_en:1; /* When set hardware TSO is enabled. */
	uint16_t tunnel_en:1;
	/* When set TX offload for tunneled packets are supported. */
	uint16_t mpw_hdr_dseg:1; /* Enable DSEGs in the title WQEBB. */
	uint16_t max_inline; /* Multiple of RTE_CACHE_LINE_SIZE to inline. */
	uint16_t inline_max_packet_sz; /* Max packet size for inlining. */
	uint32_t qp_num_8s; /* QP number shifted by 8. */
	uint32_t flags; /* Flags for Tx Queue. */
	struct mlx5_mr_ctrl mr_ctrl;
	volatile struct mlx5_cqe (*cqes)[]; /* Completion queue. */
	volatile void *wqes; /* Work queue (use volatile to write into). */
	volatile uint32_t *qp_db; /* Work queue doorbell. */
	volatile uint32_t *cq_db; /* Completion queue doorbell. */
	volatile void *bf_reg; /* Blueflame register remapped. */
	struct rte_mbuf *(*elts)[]; /* TX elements. */
	struct mlx5_txq_stats stats; /* TX queue counters. */
} __rte_cache_aligned;

/* Verbs Rx queue elements. */
struct mlx5_txq_ibv {
	LIST_ENTRY(mlx5_txq_ibv) next; /* Pointer to the next element. */
	rte_atomic32_t refcnt; /* Reference counter. */
	struct mlx5_txq_ctrl *txq_ctrl; /* Pointer to the control queue. */
	struct ibv_cq *cq; /* Completion Queue. */
	struct ibv_qp *qp; /* Queue Pair. */
};

/* TX queue control descriptor. */
struct mlx5_txq_ctrl {
	LIST_ENTRY(mlx5_txq_ctrl) next; /* Pointer to the next element. */
	rte_atomic32_t refcnt; /* Reference counter. */
	struct priv *priv; /* Back pointer to private data. */
	unsigned int socket; /* CPU socket ID for allocations. */
	unsigned int max_inline_data; /* Max inline data. */
	unsigned int max_tso_header; /* Max TSO header size. */
	struct mlx5_txq_ibv *ibv; /* Verbs queue object. */
	struct mlx5_txq_data txq; /* Data path structure. */
	off_t uar_mmap_offset; /* UAR mmap offset for non-primary process. */
	volatile void *bf_reg_orig; /* Blueflame register from verbs. */
	uint16_t idx; /* Queue index. */
};

/* mlx5_rxq.c */

extern uint8_t rss_hash_default_key[];
extern const size_t rss_hash_default_key_len;

void mlx5_rxq_cleanup(struct mlx5_rxq_ctrl *rxq_ctrl);
int mlx5_rx_queue_setup(struct rte_eth_dev *dev, uint16_t idx, uint16_t desc,
			unsigned int socket, const struct rte_eth_rxconf *conf,
			struct rte_mempool *mp);
void mlx5_rx_queue_release(void *dpdk_rxq);
int mlx5_rx_intr_vec_enable(struct rte_eth_dev *dev);
void mlx5_rx_intr_vec_disable(struct rte_eth_dev *dev);
int mlx5_rx_intr_enable(struct rte_eth_dev *dev, uint16_t rx_queue_id);
int mlx5_rx_intr_disable(struct rte_eth_dev *dev, uint16_t rx_queue_id);
struct mlx5_rxq_ibv *mlx5_rxq_ibv_new(struct rte_eth_dev *dev, uint16_t idx);
struct mlx5_rxq_ibv *mlx5_rxq_ibv_get(struct rte_eth_dev *dev, uint16_t idx);
int mlx5_rxq_ibv_release(struct mlx5_rxq_ibv *rxq_ibv);
int mlx5_rxq_ibv_releasable(struct mlx5_rxq_ibv *rxq_ibv);
int mlx5_rxq_ibv_verify(struct rte_eth_dev *dev);
struct mlx5_rxq_ctrl *mlx5_rxq_new(struct rte_eth_dev *dev, uint16_t idx,
				   uint16_t desc, unsigned int socket,
				   struct rte_mempool *mp);
struct mlx5_rxq_ctrl *mlx5_rxq_get(struct rte_eth_dev *dev, uint16_t idx);
int mlx5_rxq_release(struct rte_eth_dev *dev, uint16_t idx);
int mlx5_rxq_releasable(struct rte_eth_dev *dev, uint16_t idx);
int mlx5_rxq_verify(struct rte_eth_dev *dev);
int rxq_alloc_elts(struct mlx5_rxq_ctrl *rxq_ctrl);
struct mlx5_ind_table_ibv *mlx5_ind_table_ibv_new(struct rte_eth_dev *dev,
						  uint16_t queues[],
						  uint16_t queues_n);
struct mlx5_ind_table_ibv *mlx5_ind_table_ibv_get(struct rte_eth_dev *dev,
						  uint16_t queues[],
						  uint16_t queues_n);
int mlx5_ind_table_ibv_release(struct rte_eth_dev *dev,
			       struct mlx5_ind_table_ibv *ind_tbl);
int mlx5_ind_table_ibv_verify(struct rte_eth_dev *dev);
struct mlx5_hrxq *mlx5_hrxq_new(struct rte_eth_dev *dev, uint8_t *rss_key,
				uint8_t rss_key_len, uint64_t hash_fields,
				uint16_t queues[], uint16_t queues_n);
struct mlx5_hrxq *mlx5_hrxq_get(struct rte_eth_dev *dev, uint8_t *rss_key,
				uint8_t rss_key_len, uint64_t hash_fields,
				uint16_t queues[], uint16_t queues_n);
int mlx5_hrxq_release(struct rte_eth_dev *dev, struct mlx5_hrxq *hxrq);
int mlx5_hrxq_ibv_verify(struct rte_eth_dev *dev);

/* mlx5_txq.c */

int mlx5_tx_queue_setup(struct rte_eth_dev *dev, uint16_t idx, uint16_t desc,
			unsigned int socket, const struct rte_eth_txconf *conf);
void mlx5_tx_queue_release(void *dpdk_txq);
int mlx5_tx_uar_remap(struct rte_eth_dev *dev, int fd);
struct mlx5_txq_ibv *mlx5_txq_ibv_new(struct rte_eth_dev *dev, uint16_t idx);
struct mlx5_txq_ibv *mlx5_txq_ibv_get(struct rte_eth_dev *dev, uint16_t idx);
int mlx5_txq_ibv_release(struct mlx5_txq_ibv *txq_ibv);
int mlx5_txq_ibv_releasable(struct mlx5_txq_ibv *txq_ibv);
int mlx5_txq_ibv_verify(struct rte_eth_dev *dev);
struct mlx5_txq_ctrl *mlx5_txq_new(struct rte_eth_dev *dev, uint16_t idx,
				   uint16_t desc, unsigned int socket,
				   const struct rte_eth_txconf *conf);
struct mlx5_txq_ctrl *mlx5_txq_get(struct rte_eth_dev *dev, uint16_t idx);
int mlx5_txq_release(struct rte_eth_dev *dev, uint16_t idx);
int mlx5_txq_releasable(struct rte_eth_dev *dev, uint16_t idx);
int mlx5_txq_verify(struct rte_eth_dev *dev);
void txq_alloc_elts(struct mlx5_txq_ctrl *txq_ctrl);

/* mlx5_rxtx.c */

extern uint32_t mlx5_ptype_table[];

void mlx5_set_ptype_table(void);
uint16_t mlx5_tx_burst(void *dpdk_txq, struct rte_mbuf **pkts,
		       uint16_t pkts_n);
uint16_t mlx5_tx_burst_mpw(void *dpdk_txq, struct rte_mbuf **pkts,
			   uint16_t pkts_n);
uint16_t mlx5_tx_burst_mpw_inline(void *dpdk_txq, struct rte_mbuf **pkts,
				  uint16_t pkts_n);
uint16_t mlx5_tx_burst_empw(void *dpdk_txq, struct rte_mbuf **pkts,
			    uint16_t pkts_n);
uint16_t mlx5_rx_burst(void *dpdk_rxq, struct rte_mbuf **pkts, uint16_t pkts_n);
uint16_t removed_tx_burst(void *dpdk_txq, struct rte_mbuf **pkts,
			  uint16_t pkts_n);
uint16_t removed_rx_burst(void *dpdk_rxq, struct rte_mbuf **pkts,
			  uint16_t pkts_n);
int mlx5_rx_descriptor_status(void *rx_queue, uint16_t offset);
int mlx5_tx_descriptor_status(void *tx_queue, uint16_t offset);

/* Vectorized version of mlx5_rxtx.c */
int mlx5_check_raw_vec_tx_support(struct rte_eth_dev *dev);
int mlx5_check_vec_tx_support(struct rte_eth_dev *dev);
int mlx5_rxq_check_vec_support(struct mlx5_rxq_data *rxq_data);
int mlx5_check_vec_rx_support(struct rte_eth_dev *dev);
uint16_t mlx5_tx_burst_raw_vec(void *dpdk_txq, struct rte_mbuf **pkts,
			       uint16_t pkts_n);
uint16_t mlx5_tx_burst_vec(void *dpdk_txq, struct rte_mbuf **pkts,
			   uint16_t pkts_n);
uint16_t mlx5_rx_burst_vec(void *dpdk_txq, struct rte_mbuf **pkts,
			   uint16_t pkts_n);

/* mlx5_mr.c */

int mlx5_mr_update_mp(struct rte_eth_dev *dev, struct mlx5_mr_cache *lkp_tbl,
		      uint16_t n, struct rte_mempool *mp);
uint32_t mlx5_rx_mb2mr_bh(struct mlx5_rxq_data *rxq, uintptr_t addr);
uint32_t mlx5_tx_mb2mr_bh(struct mlx5_txq_data *txq, uintptr_t addr);

#ifndef NDEBUG
/**
 * Verify or set magic value in CQE.
 *
 * @param cqe
 *   Pointer to CQE.
 *
 * @return
 *   0 the first time.
 */
static inline int
check_cqe_seen(volatile struct mlx5_cqe *cqe)
{
	static const uint8_t magic[] = "seen";
	volatile uint8_t (*buf)[sizeof(cqe->rsvd0)] = &cqe->rsvd0;
	int ret = 1;
	unsigned int i;

	for (i = 0; i < sizeof(magic) && i < sizeof(*buf); ++i)
		if (!ret || (*buf)[i] != magic[i]) {
			ret = 0;
			(*buf)[i] = magic[i];
		}
	return ret;
}
#endif /* NDEBUG */

/**
 * Check whether CQE is valid.
 *
 * @param cqe
 *   Pointer to CQE.
 * @param cqes_n
 *   Size of completion queue.
 * @param ci
 *   Consumer index.
 *
 * @return
 *   0 on success, 1 on failure.
 */
static __rte_always_inline int
check_cqe(volatile struct mlx5_cqe *cqe,
	  unsigned int cqes_n, const uint16_t ci)
{
	uint16_t idx = ci & cqes_n;
	uint8_t op_own = cqe->op_own;
	uint8_t op_owner = MLX5_CQE_OWNER(op_own);
	uint8_t op_code = MLX5_CQE_OPCODE(op_own);

	if (unlikely((op_owner != (!!(idx))) || (op_code == MLX5_CQE_INVALID)))
		return 1; /* No CQE. */
#ifndef NDEBUG
	if ((op_code == MLX5_CQE_RESP_ERR) ||
	    (op_code == MLX5_CQE_REQ_ERR)) {
		volatile struct mlx5_err_cqe *err_cqe = (volatile void *)cqe;
		uint8_t syndrome = err_cqe->syndrome;

		if ((syndrome == MLX5_CQE_SYNDROME_LOCAL_LENGTH_ERR) ||
		    (syndrome == MLX5_CQE_SYNDROME_REMOTE_ABORTED_ERR))
			return 0;
		if (!check_cqe_seen(cqe)) {
			DRV_LOG(ERR,
				"unexpected CQE error %u (0x%02x) syndrome"
				" 0x%02x",
				op_code, op_code, syndrome);
			rte_hexdump(stderr, "MLX5 Error CQE:",
				    (const void *)((uintptr_t)err_cqe),
				    sizeof(*cqe));
		}
		return 1;
	} else if ((op_code != MLX5_CQE_RESP_SEND) &&
		   (op_code != MLX5_CQE_REQ)) {
		if (!check_cqe_seen(cqe)) {
			DRV_LOG(ERR, "unexpected CQE opcode %u (0x%02x)",
				op_code, op_code);
			rte_hexdump(stderr, "MLX5 CQE:",
				    (const void *)((uintptr_t)cqe),
				    sizeof(*cqe));
		}
		return 1;
	}
#endif /* NDEBUG */
	return 0;
}

/**
 * Return the address of the WQE.
 *
 * @param txq
 *   Pointer to TX queue structure.
 * @param  wqe_ci
 *   WQE consumer index.
 *
 * @return
 *   WQE address.
 */
static inline uintptr_t *
tx_mlx5_wqe(struct mlx5_txq_data *txq, uint16_t ci)
{
	ci &= ((1 << txq->wqe_n) - 1);
	return (uintptr_t *)((uintptr_t)txq->wqes + ci * MLX5_WQE_SIZE);
}

/**
 * Manage TX completions.
 *
 * When sending a burst, mlx5_tx_burst() posts several WRs.
 *
 * @param txq
 *   Pointer to TX queue structure.
 */
static __rte_always_inline void
mlx5_tx_complete(struct mlx5_txq_data *txq)
{
	const uint16_t elts_n = 1 << txq->elts_n;
	const uint16_t elts_m = elts_n - 1;
	const unsigned int cqe_n = 1 << txq->cqe_n;
	const unsigned int cqe_cnt = cqe_n - 1;
	uint16_t elts_free = txq->elts_tail;
	uint16_t elts_tail;
	uint16_t cq_ci = txq->cq_ci;
	volatile struct mlx5_cqe *cqe = NULL;
	volatile struct mlx5_wqe_ctrl *ctrl;
	struct rte_mbuf *m, *free[elts_n];
	struct rte_mempool *pool = NULL;
	unsigned int blk_n = 0;

	cqe = &(*txq->cqes)[cq_ci & cqe_cnt];
	if (unlikely(check_cqe(cqe, cqe_n, cq_ci)))
		return;
#ifndef NDEBUG
	if ((MLX5_CQE_OPCODE(cqe->op_own) == MLX5_CQE_RESP_ERR) ||
	    (MLX5_CQE_OPCODE(cqe->op_own) == MLX5_CQE_REQ_ERR)) {
		if (!check_cqe_seen(cqe)) {
			DRV_LOG(ERR, "unexpected error CQE, Tx stopped");
			rte_hexdump(stderr, "MLX5 TXQ:",
				    (const void *)((uintptr_t)txq->wqes),
				    ((1 << txq->wqe_n) *
				     MLX5_WQE_SIZE));
		}
		return;
	}
#endif /* NDEBUG */
	++cq_ci;
	rte_io_rmb();
	txq->wqe_pi = rte_be_to_cpu_16(cqe->wqe_counter);
	ctrl = (volatile struct mlx5_wqe_ctrl *)
		tx_mlx5_wqe(txq, txq->wqe_pi);
	elts_tail = ctrl->ctrl3;
	assert((elts_tail & elts_m) < (1 << txq->wqe_n));
	/* Free buffers. */
	while (elts_free != elts_tail) {
		m = rte_pktmbuf_prefree_seg((*txq->elts)[elts_free++ & elts_m]);
		if (likely(m != NULL)) {
			if (likely(m->pool == pool)) {
				free[blk_n++] = m;
			} else {
				if (likely(pool != NULL))
					rte_mempool_put_bulk(pool,
							     (void *)free,
							     blk_n);
				free[0] = m;
				pool = m->pool;
				blk_n = 1;
			}
		}
	}
	if (blk_n)
		rte_mempool_put_bulk(pool, (void *)free, blk_n);
#ifndef NDEBUG
	elts_free = txq->elts_tail;
	/* Poisoning. */
	while (elts_free != elts_tail) {
		memset(&(*txq->elts)[elts_free & elts_m],
		       0x66,
		       sizeof((*txq->elts)[elts_free & elts_m]));
		++elts_free;
	}
#endif
	txq->cq_ci = cq_ci;
	txq->elts_tail = elts_tail;
	/* Update the consumer index. */
	rte_compiler_barrier();
	*txq->cq_db = rte_cpu_to_be_32(cq_ci);
}

/**
 * Look up LKEY from given lookup table by linear search. Firstly look up the
 * last-hit entry. If miss, the entire array is searched. If found, update the
 * last-hit index and return LKEY.
 *
 * @param lkp_tbl
 *   Pointer to lookup table.
 * @param[in,out] cached_idx
 *   Pointer to last-hit index.
 * @param n
 *   Size of lookup table.
 * @param addr
 *   Search key.
 *
 * @return
 *   Searched LKEY on success, UINT32_MAX on no match.
 */
static __rte_always_inline uint32_t
mlx5_mr_lookup_cache(struct mlx5_mr_cache *lkp_tbl, uint16_t *cached_idx,
		     uint16_t n, uintptr_t addr)
{
	uint16_t idx;

	if (likely(addr >= lkp_tbl[*cached_idx].start &&
		   addr < lkp_tbl[*cached_idx].end))
		return lkp_tbl[*cached_idx].lkey;
	for (idx = 0; idx < n && lkp_tbl[idx].start != 0; ++idx) {
		if (addr >= lkp_tbl[idx].start &&
		    addr < lkp_tbl[idx].end) {
			/* Found. */
			*cached_idx = idx;
			return lkp_tbl[idx].lkey;
		}
	}
	return UINT32_MAX;
}

/**
 * Query LKEY from address for Rx.
 *
 * @param rxq
 *   Pointer to Rx queue structure.
 * @param addr
 *   Address to search.
 *
 * @return
 *   LKEY on success.
 */
static __rte_always_inline uint32_t
mlx5_rx_addr2mr(struct mlx5_rxq_data *rxq, uintptr_t addr)
{
	uint32_t lkey;

	/* Linear search on MR cache array. */
	lkey = mlx5_mr_lookup_cache(rxq->mr_ctrl.cache,
				    &rxq->mr_ctrl.mru,
				    MLX5_MR_CACHE_N, addr);
	if (likely(lkey != UINT32_MAX))
		return lkey;
	DEBUG("No found in rxq->mr_cache[], last-hit = %u, head = %u)",
	      rxq->mr_ctrl.mru, rxq->mr_ctrl.head);
	/* Take slower bottom-half (binary search) on miss. */
	return mlx5_rx_mb2mr_bh(rxq, addr);
}

#define mlx5_rx_mb2mr(rxq, mb) mlx5_rx_addr2mr(rxq, (uintptr_t)((mb)->buf_addr))

/**
 * Query LKEY from address for Tx.
 *
 * @param txq
 *   Pointer to Tx queue structure.
 * @param addr
 *   Address to search.
 *
 * @return
 *   LKEY on success.
 */
static __rte_always_inline uint32_t
mlx5_tx_addr2mr(struct mlx5_txq_data *txq, uintptr_t addr)
{
	uint32_t lkey;

	/* Linear search on MR cache array. */
	lkey = mlx5_mr_lookup_cache(txq->mr_ctrl.cache,
				    &txq->mr_ctrl.mru,
				    MLX5_MR_CACHE_N, addr);
	if (likely(lkey != UINT32_MAX))
		return lkey;
	DEBUG("No found in txq->mr_cache[], last-hit = %u, head = %u)",
	      txq->mr_ctrl.mru, txq->mr_ctrl.head);
	/* Take slower bottom-half (binary search) on miss. */
	return mlx5_tx_mb2mr_bh(txq, addr);
}

#define mlx5_tx_mb2mr(rxq, mb) mlx5_tx_addr2mr(rxq, (uintptr_t)((mb)->buf_addr))

/**
 * Ring TX queue doorbell and flush the update if requested.
 *
 * @param txq
 *   Pointer to TX queue structure.
 * @param wqe
 *   Pointer to the last WQE posted in the NIC.
 * @param cond
 *   Request for write memory barrier after BlueFlame update.
 */
static __rte_always_inline void
mlx5_tx_dbrec_cond_wmb(struct mlx5_txq_data *txq, volatile struct mlx5_wqe *wqe,
		       int cond)
{
	uint64_t *dst = (uint64_t *)((uintptr_t)txq->bf_reg);
	volatile uint64_t *src = ((volatile uint64_t *)wqe);

	rte_io_wmb();
	*txq->qp_db = rte_cpu_to_be_32(txq->wqe_ci);
	/* Ensure ordering between DB record and BF copy. */
	rte_wmb();
	*dst = *src;
	if (cond)
		rte_wmb();
}

/**
 * Ring TX queue doorbell and flush the update by write memory barrier.
 *
 * @param txq
 *   Pointer to TX queue structure.
 * @param wqe
 *   Pointer to the last WQE posted in the NIC.
 */
static __rte_always_inline void
mlx5_tx_dbrec(struct mlx5_txq_data *txq, volatile struct mlx5_wqe *wqe)
{
	mlx5_tx_dbrec_cond_wmb(txq, wqe, 1);
}

/**
 * Convert the Checksum offloads to Verbs.
 *
 * @param txq_data
 *   Pointer to the Tx queue.
 * @param buf
 *   Pointer to the mbuf.
 *
 * @return
 *   the converted cs_flags.
 */
static __rte_always_inline uint8_t
txq_ol_cksum_to_cs(struct mlx5_txq_data *txq_data, struct rte_mbuf *buf)
{
	uint8_t cs_flags = 0;

	/* Should we enable HW CKSUM offload */
	if (buf->ol_flags &
	    (PKT_TX_IP_CKSUM | PKT_TX_TCP_CKSUM | PKT_TX_UDP_CKSUM |
	     PKT_TX_OUTER_IP_CKSUM)) {
		if (txq_data->tunnel_en &&
		    (buf->ol_flags &
		     (PKT_TX_TUNNEL_GRE | PKT_TX_TUNNEL_VXLAN))) {
			cs_flags = MLX5_ETH_WQE_L3_INNER_CSUM |
				   MLX5_ETH_WQE_L4_INNER_CSUM;
			if (buf->ol_flags & PKT_TX_OUTER_IP_CKSUM)
				cs_flags |= MLX5_ETH_WQE_L3_CSUM;
		} else {
			cs_flags = MLX5_ETH_WQE_L3_CSUM |
				   MLX5_ETH_WQE_L4_CSUM;
		}
	}
	return cs_flags;
}

#endif /* RTE_PMD_MLX5_RXTX_H_ */
