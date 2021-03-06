/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2014 Intel Corporation. All rights reserved.
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

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/queue.h>

#include <rte_common.h>
#include <rte_memory.h>
#include <rte_per_lcore.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_rwlock.h>
#include <rte_eal.h>
#include <rte_lcore.h>
#include <rte_cycles.h>

#include "test.h"

/*
 * rwlock test
 * ===========
 *
 * - There is a global rwlock and a table of rwlocks (one per lcore).
 *
 * - The test function takes all of these locks and launches the
 *   ``test_rwlock_per_core()`` function on each core (except the master).
 *
 *   - The function takes the global write lock, display something,
 *     then releases the global lock.
 *   - Then, it takes the per-lcore write lock, display something, and
 *     releases the per-core lock.
 *   - Finally, a read lock is taken during 100 ms, then released.
 *
 * - The main function unlocks the per-lcore locks sequentially and
 *   waits between each lock. This triggers the display of a message
 *   for each core, in the correct order.
 *
 *   Then, it tries to take the global write lock and display the last
 *   message. The autotest script checks that the message order is correct.
 */

static rte_rwlock_t sl;
static rte_rwlock_t sl_tab[RTE_MAX_LCORE];
static rte_atomic32_t synchro;

static int
test_rwlock_per_core(__attribute__((unused)) void *arg)
{
	rte_rwlock_write_lock(&sl);
	printf("Global write lock taken on core %u\n", rte_lcore_id());
	rte_rwlock_write_unlock(&sl);

	rte_rwlock_write_lock(&sl_tab[rte_lcore_id()]);
	printf("Hello from core %u !\n", rte_lcore_id());
	rte_rwlock_write_unlock(&sl_tab[rte_lcore_id()]);

	rte_rwlock_read_lock(&sl);
	printf("Global read lock taken on core %u\n", rte_lcore_id());
	rte_delay_ms(100);
	printf("Release global read lock on core %u\n", rte_lcore_id());
	rte_rwlock_read_unlock(&sl);

	return 0;
}

static rte_rwlock_t lk = RTE_RWLOCK_INITIALIZER;
static volatile uint64_t rwlock_data;
static uint64_t lock_count[RTE_MAX_LCORE] = {0};

#define TIME_MS 100
#define TEST_RWLOCK_DEBUG 0

static int
load_loop_fn(__attribute__((unused)) void *arg)
{
	uint64_t time_diff = 0, begin;
	uint64_t hz = rte_get_timer_hz();
	uint64_t lcount = 0;
	const unsigned int lcore = rte_lcore_id();

	/* wait synchro for slaves */
	if (lcore != rte_get_master_lcore())
		while (rte_atomic32_read(&synchro) == 0)
			;

	begin = rte_rdtsc_precise();
	while (time_diff < hz * TIME_MS / 1000) {
		rte_rwlock_write_lock(&lk);
		++rwlock_data;
		rte_rwlock_write_unlock(&lk);

		rte_rwlock_read_lock(&lk);
		if (TEST_RWLOCK_DEBUG && !(lcount % 100))
			printf("Core [%u] rwlock_data = %"PRIu64"\n",
				lcore, rwlock_data);
		rte_rwlock_read_unlock(&lk);

		lcount++;
		/* delay to make lock duty cycle slightly realistic */
		rte_pause();
		time_diff = rte_rdtsc_precise() - begin;
	}

	lock_count[lcore] = lcount;
	return 0;
}

static int
test_rwlock_perf(void)
{
	unsigned int i;
	uint64_t total = 0;

	printf("\nRwlock Perf Test on %u cores...\n", rte_lcore_count());

	/* clear synchro and start slaves */
	rte_atomic32_set(&synchro, 0);
	if (rte_eal_mp_remote_launch(load_loop_fn, NULL, SKIP_MASTER) < 0)
		return -1;

	/* start synchro and launch test on master */
	rte_atomic32_set(&synchro, 1);
	load_loop_fn(NULL);

	rte_eal_mp_wait_lcore();

	RTE_LCORE_FOREACH(i) {
		printf("Core [%u] count = %"PRIu64"\n", i, lock_count[i]);
		total += lock_count[i];
	}

	printf("Total count = %"PRIu64"\n", total);

	return 0;
}

static int
test_rwlock(void)
{
	int i;

	rte_rwlock_init(&sl);
	for (i=0; i<RTE_MAX_LCORE; i++)
		rte_rwlock_init(&sl_tab[i]);

	rte_rwlock_write_lock(&sl);

	RTE_LCORE_FOREACH_SLAVE(i) {
		rte_rwlock_write_lock(&sl_tab[i]);
		rte_eal_remote_launch(test_rwlock_per_core, NULL, i);
	}

	rte_rwlock_write_unlock(&sl);

	RTE_LCORE_FOREACH_SLAVE(i) {
		rte_rwlock_write_unlock(&sl_tab[i]);
		rte_delay_ms(100);
	}

	rte_rwlock_write_lock(&sl);
	/* this message should be the last message of test */
	printf("Global write lock taken on master core %u\n", rte_lcore_id());
	rte_rwlock_write_unlock(&sl);

	rte_eal_mp_wait_lcore();

	if (test_rwlock_perf() < 0)
		return -1;

	return 0;
}

REGISTER_TEST_COMMAND(rwlock_autotest, test_rwlock);
