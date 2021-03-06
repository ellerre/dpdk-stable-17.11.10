/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2016 Intel Corporation. All rights reserved.
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
 *     * Neither the name of Intel Corporation nor the names of its
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

#include <linux/vhost.h>
#include <linux/virtio_net.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#ifdef RTE_LIBRTE_VHOST_NUMA
#include <numaif.h>
#endif

#include <rte_errno.h>
#include <rte_ethdev.h>
#include <rte_log.h>
#include <rte_string_fns.h>
#include <rte_memory.h>
#include <rte_malloc.h>
#include <rte_vhost.h>
#include <rte_rwlock.h>

#include "iotlb.h"
#include "vhost.h"
#include "vhost_user.h"

struct virtio_net *vhost_devices[MAX_VHOST_DEVICE];

/* Called with iotlb_lock read-locked */
uint64_t
__vhost_iova_to_vva(struct virtio_net *dev, struct vhost_virtqueue *vq,
		    uint64_t iova, uint64_t *size, uint8_t perm)
{
	uint64_t vva, tmp_size;

	if (unlikely(!*size))
		return 0;

	tmp_size = *size;

	vva = vhost_user_iotlb_cache_find(vq, iova, &tmp_size, perm);
	if (tmp_size == *size)
		return vva;

	iova += tmp_size;

	if (!vhost_user_iotlb_pending_miss(vq, iova, perm)) {
		/*
		 * iotlb_lock is read-locked for a full burst,
		 * but it only protects the iotlb cache.
		 * In case of IOTLB miss, we might block on the socket,
		 * which could cause a deadlock with QEMU if an IOTLB update
		 * is being handled. We can safely unlock here to avoid it.
		 */
		vhost_user_iotlb_rd_unlock(vq);

		vhost_user_iotlb_pending_insert(vq, iova, perm);
		if (vhost_user_iotlb_miss(dev, iova, perm)) {
			RTE_LOG(ERR, VHOST_CONFIG,
				"IOTLB miss req failed for IOVA 0x%" PRIx64 "\n",
				iova);
			vhost_user_iotlb_pending_remove(vq, iova, 1, perm);
		}

		vhost_user_iotlb_rd_lock(vq);
	}

	return 0;
}

struct virtio_net *
get_device(int vid)
{
	struct virtio_net *dev = vhost_devices[vid];

	if (unlikely(!dev)) {
		RTE_LOG(ERR, VHOST_CONFIG,
			"(%d) device not found.\n", vid);
	}

	return dev;
}

#define VHOST_LOG_PAGE	4096

/*
 * Atomically set a bit in memory.
 */
static __rte_always_inline void
vhost_set_bit(unsigned int nr, volatile uint8_t *addr)
{
#if defined(RTE_TOOLCHAIN_GCC) && (GCC_VERSION < 70100)
	/*
	 * __sync_ built-ins are deprecated, but __atomic_ ones
	 * are sub-optimized in older GCC versions.
	 */
	__sync_fetch_and_or_1(addr, (1U << nr));
#else
	__atomic_fetch_or(addr, (1U << nr), __ATOMIC_RELAXED);
#endif
}

static __rte_always_inline void
vhost_log_page(uint8_t *log_base, uint64_t page)
{
	vhost_set_bit(page % 8, &log_base[page / 8]);
}

void
__vhost_log_write(struct virtio_net *dev, uint64_t addr, uint64_t len)
{
	uint64_t page;

	if (unlikely(!dev->log_base || !len))
		return;

	if (unlikely(dev->log_size <= ((addr + len - 1) / VHOST_LOG_PAGE / 8)))
		return;

	/* To make sure guest memory updates are committed before logging */
	rte_smp_wmb();

	page = addr / VHOST_LOG_PAGE;
	while (page * VHOST_LOG_PAGE < addr + len) {
		vhost_log_page((uint8_t *)(uintptr_t)dev->log_base, page);
		page += 1;
	}
}

void
__vhost_log_write_iova(struct virtio_net *dev, struct vhost_virtqueue *vq,
			     uint64_t iova, uint64_t len)
{
	uint64_t hva, gpa, map_len;
	map_len = len;

	hva = __vhost_iova_to_vva(dev, vq, iova, &map_len, VHOST_ACCESS_RW);
	if (map_len != len) {
		RTE_LOG(ERR, VHOST_CONFIG,
			"Failed to write log for IOVA 0x%" PRIx64 ". No IOTLB entry found\n",
			iova);
		return;
	}

	gpa = hva_to_gpa(dev, hva, len);
	if (gpa)
		__vhost_log_write(dev, gpa, len);
}

void
__vhost_log_cache_sync(struct virtio_net *dev, struct vhost_virtqueue *vq)
{
	unsigned long *log_base;
	int i;

	if (unlikely(!dev->log_base))
		return;

	log_base = (unsigned long *)(uintptr_t)dev->log_base;

	/*
	 * It is expected a write memory barrier has been issued
	 * before this function is called.
	 */

	for (i = 0; i < vq->log_cache_nb_elem; i++) {
		struct log_cache_entry *elem = vq->log_cache + i;

#if defined(RTE_TOOLCHAIN_GCC) && (GCC_VERSION < 70100)
		/*
		 * '__sync' builtins are deprecated, but '__atomic' ones
		 * are sub-optimized in older GCC versions.
		 */
		__sync_fetch_and_or(log_base + elem->offset, elem->val);
#else
		__atomic_fetch_or(log_base + elem->offset, elem->val,
				__ATOMIC_RELAXED);
#endif
	}

	rte_smp_wmb();

	vq->log_cache_nb_elem = 0;
}

static __rte_always_inline void
vhost_log_cache_page(struct virtio_net *dev, struct vhost_virtqueue *vq,
			uint64_t page)
{
	uint32_t bit_nr = page % (sizeof(unsigned long) << 3);
	uint32_t offset = page / (sizeof(unsigned long) << 3);
	int i;

	for (i = 0; i < vq->log_cache_nb_elem; i++) {
		struct log_cache_entry *elem = vq->log_cache + i;

		if (elem->offset == offset) {
			elem->val |= (1UL << bit_nr);
			return;
		}
	}

	if (unlikely(i >= VHOST_LOG_CACHE_NR)) {
		/*
		 * No more room for a new log cache entry,
		 * so write the dirty log map directly.
		 */
		rte_smp_wmb();
		vhost_log_page((uint8_t *)(uintptr_t)dev->log_base, page);

		return;
	}

	vq->log_cache[i].offset = offset;
	vq->log_cache[i].val = (1UL << bit_nr);
	vq->log_cache_nb_elem++;
}

void
__vhost_log_cache_write(struct virtio_net *dev, struct vhost_virtqueue *vq,
			uint64_t addr, uint64_t len)
{
	uint64_t page;

	if (unlikely(!dev->log_base || !len))
		return;

	if (unlikely(dev->log_size <= ((addr + len - 1) / VHOST_LOG_PAGE / 8)))
		return;

	page = addr / VHOST_LOG_PAGE;
	while (page * VHOST_LOG_PAGE < addr + len) {
		vhost_log_cache_page(dev, vq, page);
		page += 1;
	}
}

void
__vhost_log_cache_write_iova(struct virtio_net *dev, struct vhost_virtqueue *vq,
			     uint64_t iova, uint64_t len)
{
	uint64_t hva, gpa, map_len;
	map_len = len;

	hva = __vhost_iova_to_vva(dev, vq, iova, &map_len, VHOST_ACCESS_RW);
	if (map_len != len) {
		RTE_LOG(ERR, VHOST_CONFIG,
			"Failed to write log for IOVA 0x%" PRIx64 ". No IOTLB entry found\n",
			iova);
		return;
	}

	gpa = hva_to_gpa(dev, hva, len);
	if (gpa)
		__vhost_log_cache_write(dev, vq, gpa, len);
}

static void
cleanup_vq(struct vhost_virtqueue *vq, int destroy)
{
	if ((vq->callfd >= 0) && (destroy != 0))
		close(vq->callfd);
	if (vq->kickfd >= 0)
		close(vq->kickfd);
}

/*
 * Unmap any memory, close any file descriptors and
 * free any memory owned by a device.
 */
void
cleanup_device(struct virtio_net *dev, int destroy)
{
	uint32_t i;

	vhost_backend_cleanup(dev);

	for (i = 0; i < dev->nr_vring; i++)
		cleanup_vq(dev->virtqueue[i], destroy);
}

/*
 * Release virtqueues and device memory.
 */
static void
free_device(struct virtio_net *dev)
{
	uint32_t i;
	struct vhost_virtqueue *vq;

	for (i = 0; i < dev->nr_vring; i++) {
		vq = dev->virtqueue[i];

		rte_free(vq->shadow_used_ring);
		rte_free(vq->batch_copy_elems);
		rte_mempool_free(vq->iotlb_pool);
		rte_free(vq);
	}

	rte_free(dev);
}

int
vring_translate(struct virtio_net *dev, struct vhost_virtqueue *vq)
{
	uint64_t req_size, size;

	if (!(dev->features & (1ULL << VIRTIO_F_IOMMU_PLATFORM)))
		return -1;

	req_size = sizeof(struct vring_desc) * vq->size;
	size = req_size;
	vq->desc = (struct vring_desc *)(uintptr_t)vhost_iova_to_vva(dev, vq,
						vq->ring_addrs.desc_user_addr,
						&size, VHOST_ACCESS_RW);
	if (!vq->desc || size != req_size)
		return -1;

	req_size = sizeof(struct vring_avail);
	req_size += sizeof(uint16_t) * vq->size;
	if (dev->features & (1ULL << VIRTIO_RING_F_EVENT_IDX))
		req_size += sizeof(uint16_t);
	size = req_size;
	vq->avail = (struct vring_avail *)(uintptr_t)vhost_iova_to_vva(dev, vq,
						vq->ring_addrs.avail_user_addr,
						&size, VHOST_ACCESS_RW);
	if (!vq->avail || size != req_size)
		return -1;

	req_size = sizeof(struct vring_used);
	req_size += sizeof(struct vring_used_elem) * vq->size;
	if (dev->features & (1ULL << VIRTIO_RING_F_EVENT_IDX))
		req_size += sizeof(uint16_t);
	size = req_size;
	vq->used = (struct vring_used *)(uintptr_t)vhost_iova_to_vva(dev, vq,
						vq->ring_addrs.used_user_addr,
						&size, VHOST_ACCESS_RW);
	if (!vq->used || size != req_size)
		return -1;

	vq->access_ok = 1;

	return 0;
}

void
vring_invalidate(struct virtio_net *dev, struct vhost_virtqueue *vq)
{
	if (dev->features & (1ULL << VIRTIO_F_IOMMU_PLATFORM))
		vhost_user_iotlb_wr_lock(vq);

	vq->access_ok = 0;
	vq->desc = NULL;
	vq->avail = NULL;
	vq->used = NULL;
	vq->log_guest_addr = 0;

	if (dev->features & (1ULL << VIRTIO_F_IOMMU_PLATFORM))
		vhost_user_iotlb_wr_unlock(vq);
}

static void
init_vring_queue(struct virtio_net *dev, uint32_t vring_idx)
{
	struct vhost_virtqueue *vq;

	if (vring_idx >= VHOST_MAX_VRING) {
		RTE_LOG(ERR, VHOST_CONFIG,
				"Failed not init vring, out of bound (%d)\n",
				vring_idx);
		return;
	}

	vq = dev->virtqueue[vring_idx];

	memset(vq, 0, sizeof(struct vhost_virtqueue));

	vq->kickfd = VIRTIO_UNINITIALIZED_EVENTFD;
	vq->callfd = VIRTIO_UNINITIALIZED_EVENTFD;

	vhost_user_iotlb_init(dev, vring_idx);
	/* Backends are set to -1 indicating an inactive device. */
	vq->backend = -1;

	TAILQ_INIT(&vq->zmbuf_list);
}

static void
reset_vring_queue(struct virtio_net *dev, uint32_t vring_idx)
{
	struct vhost_virtqueue *vq;
	int callfd;

	if (vring_idx >= VHOST_MAX_VRING) {
		RTE_LOG(ERR, VHOST_CONFIG,
				"Failed not init vring, out of bound (%d)\n",
				vring_idx);
		return;
	}

	vq = dev->virtqueue[vring_idx];
	callfd = vq->callfd;
	init_vring_queue(dev, vring_idx);
	vq->callfd = callfd;
}

int
alloc_vring_queue(struct virtio_net *dev, uint32_t vring_idx)
{
	struct vhost_virtqueue *vq;

	vq = rte_malloc(NULL, sizeof(struct vhost_virtqueue), 0);
	if (vq == NULL) {
		RTE_LOG(ERR, VHOST_CONFIG,
			"Failed to allocate memory for vring:%u.\n", vring_idx);
		return -1;
	}

	dev->virtqueue[vring_idx] = vq;
	init_vring_queue(dev, vring_idx);
	rte_spinlock_init(&vq->access_lock);

	dev->nr_vring += 1;

	return 0;
}

/*
 * Reset some variables in device structure, while keeping few
 * others untouched, such as vid, ifname, nr_vring: they
 * should be same unless the device is removed.
 */
void
reset_device(struct virtio_net *dev)
{
	uint32_t i;

	dev->features = 0;
	dev->protocol_features = 0;
	dev->flags = 0;

	for (i = 0; i < dev->nr_vring; i++)
		reset_vring_queue(dev, i);
}

/*
 * Invoked when there is a new vhost-user connection established (when
 * there is a new virtio device being attached).
 */
int
vhost_new_device(void)
{
	struct virtio_net *dev;
	int i;

	dev = rte_zmalloc(NULL, sizeof(struct virtio_net), 0);
	if (dev == NULL) {
		RTE_LOG(ERR, VHOST_CONFIG,
			"Failed to allocate memory for new dev.\n");
		return -1;
	}

	for (i = 0; i < MAX_VHOST_DEVICE; i++) {
		if (vhost_devices[i] == NULL)
			break;
	}
	if (i == MAX_VHOST_DEVICE) {
		RTE_LOG(ERR, VHOST_CONFIG,
			"Failed to find a free slot for new device.\n");
		rte_free(dev);
		return -1;
	}

	vhost_devices[i] = dev;
	dev->vid = i;
	dev->slave_req_fd = -1;

	return i;
}

/*
 * Invoked when there is the vhost-user connection is broken (when
 * the virtio device is being detached).
 */
void
vhost_destroy_device(int vid)
{
	struct virtio_net *dev = get_device(vid);

	if (dev == NULL)
		return;

	if (dev->flags & VIRTIO_DEV_RUNNING) {
		dev->flags &= ~VIRTIO_DEV_RUNNING;
		dev->notify_ops->destroy_device(vid);
	}

	cleanup_device(dev, 1);
	free_device(dev);

	vhost_devices[vid] = NULL;
}

void
vhost_set_ifname(int vid, const char *if_name, unsigned int if_len)
{
	struct virtio_net *dev;
	unsigned int len;

	dev = get_device(vid);
	if (dev == NULL)
		return;

	len = if_len > sizeof(dev->ifname) ?
		sizeof(dev->ifname) : if_len;

	strncpy(dev->ifname, if_name, len);
	dev->ifname[sizeof(dev->ifname) - 1] = '\0';
}

void
vhost_enable_dequeue_zero_copy(int vid)
{
	struct virtio_net *dev = get_device(vid);

	if (dev == NULL)
		return;

	dev->dequeue_zero_copy = 1;
}

int
rte_vhost_get_mtu(int vid, uint16_t *mtu)
{
	struct virtio_net *dev = get_device(vid);

	if (!dev)
		return -ENODEV;

	if (!(dev->flags & VIRTIO_DEV_READY))
		return -EAGAIN;

	if (!(dev->features & (1ULL << VIRTIO_NET_F_MTU)))
		return -ENOTSUP;

	*mtu = dev->mtu;

	return 0;
}

int
rte_vhost_get_numa_node(int vid)
{
#ifdef RTE_LIBRTE_VHOST_NUMA
	struct virtio_net *dev = get_device(vid);
	int numa_node;
	int ret;

	if (dev == NULL)
		return -1;

	ret = get_mempolicy(&numa_node, NULL, 0, dev,
			    MPOL_F_NODE | MPOL_F_ADDR);
	if (ret < 0) {
		RTE_LOG(ERR, VHOST_CONFIG,
			"(%d) failed to query numa node: %s\n",
			vid, rte_strerror(errno));
		return -1;
	}

	return numa_node;
#else
	RTE_SET_USED(vid);
	return -1;
#endif
}

uint32_t
rte_vhost_get_queue_num(int vid)
{
	struct virtio_net *dev = get_device(vid);

	if (dev == NULL)
		return 0;

	return dev->nr_vring / 2;
}

uint16_t
rte_vhost_get_vring_num(int vid)
{
	struct virtio_net *dev = get_device(vid);

	if (dev == NULL)
		return 0;

	return dev->nr_vring;
}

int
rte_vhost_get_ifname(int vid, char *buf, size_t len)
{
	struct virtio_net *dev = get_device(vid);

	if (dev == NULL)
		return -1;

	len = RTE_MIN(len, sizeof(dev->ifname));

	strncpy(buf, dev->ifname, len);
	buf[len - 1] = '\0';

	return 0;
}

int
rte_vhost_get_negotiated_features(int vid, uint64_t *features)
{
	struct virtio_net *dev;

	dev = get_device(vid);
	if (!dev)
		return -1;

	*features = dev->features;
	return 0;
}

int
rte_vhost_get_mem_table(int vid, struct rte_vhost_memory **mem)
{
	struct virtio_net *dev;
	struct rte_vhost_memory *m;
	size_t size;

	dev = get_device(vid);
	if (!dev)
		return -1;

	size = dev->mem->nregions * sizeof(struct rte_vhost_mem_region);
	m = malloc(sizeof(struct rte_vhost_memory) + size);
	if (!m)
		return -1;

	m->nregions = dev->mem->nregions;
	memcpy(m->regions, dev->mem->regions, size);
	*mem = m;

	return 0;
}

int
rte_vhost_get_vhost_vring(int vid, uint16_t vring_idx,
			  struct rte_vhost_vring *vring)
{
	struct virtio_net *dev;
	struct vhost_virtqueue *vq;

	dev = get_device(vid);
	if (!dev)
		return -1;

	if (vring_idx >= VHOST_MAX_VRING)
		return -1;

	vq = dev->virtqueue[vring_idx];
	if (!vq)
		return -1;

	vring->desc  = vq->desc;
	vring->avail = vq->avail;
	vring->used  = vq->used;
	vring->log_guest_addr  = vq->log_guest_addr;

	vring->callfd  = vq->callfd;
	vring->kickfd  = vq->kickfd;
	vring->size    = vq->size;

	return 0;
}

uint16_t
rte_vhost_avail_entries(int vid, uint16_t queue_id)
{
	struct virtio_net *dev;
	struct vhost_virtqueue *vq;
	uint16_t ret = 0;

	dev = get_device(vid);
	if (!dev)
		return 0;

	vq = dev->virtqueue[queue_id];

	rte_spinlock_lock(&vq->access_lock);

	if (unlikely(!vq->enabled || vq->avail == NULL))
		goto out;

	ret = *(volatile uint16_t *)&vq->avail->idx - vq->last_used_idx;

out:
	rte_spinlock_unlock(&vq->access_lock);
	return ret;
}

int
rte_vhost_enable_guest_notification(int vid, uint16_t queue_id, int enable)
{
	struct virtio_net *dev = get_device(vid);
	struct vhost_virtqueue *vq;
	int ret = 0;

	if (dev == NULL)
		return -1;

	if (enable) {
		RTE_LOG(ERR, VHOST_CONFIG,
			"guest notification isn't supported.\n");
		return -1;
	}

	vq = dev->virtqueue[queue_id];

	rte_spinlock_lock(&vq->access_lock);

	if (vq->used == NULL) {
		ret = -1;
		goto out;
	}

	dev->virtqueue[queue_id]->used->flags = VRING_USED_F_NO_NOTIFY;

out:
	rte_spinlock_unlock(&vq->access_lock);

	return ret;
}

void
rte_vhost_log_write(int vid, uint64_t addr, uint64_t len)
{
	struct virtio_net *dev = get_device(vid);

	if (dev == NULL)
		return;

	vhost_log_write(dev, addr, len);
}

void
rte_vhost_log_used_vring(int vid, uint16_t vring_idx,
			 uint64_t offset, uint64_t len)
{
	struct virtio_net *dev;
	struct vhost_virtqueue *vq;

	dev = get_device(vid);
	if (dev == NULL)
		return;

	if (vring_idx >= VHOST_MAX_VRING)
		return;
	vq = dev->virtqueue[vring_idx];
	if (!vq)
		return;

	vhost_log_used_vring(dev, vq, offset, len);
}

uint32_t
rte_vhost_rx_queue_count(int vid, uint16_t qid)
{
	struct virtio_net *dev;
	struct vhost_virtqueue *vq;
	uint32_t ret = 0;

	dev = get_device(vid);
	if (dev == NULL)
		return 0;

	if (unlikely(qid >= dev->nr_vring || (qid & 1) == 0)) {
		RTE_LOG(ERR, VHOST_DATA, "(%d) %s: invalid virtqueue idx %d.\n",
			dev->vid, __func__, qid);
		return 0;
	}

	vq = dev->virtqueue[qid];
	if (vq == NULL)
		return 0;

	rte_spinlock_lock(&vq->access_lock);

	if (unlikely(vq->enabled == 0 || vq->avail == NULL))
		goto out;

	ret = *((volatile uint16_t *)&vq->avail->idx) - vq->last_avail_idx;

out:
	rte_spinlock_unlock(&vq->access_lock);
	return ret;
}
