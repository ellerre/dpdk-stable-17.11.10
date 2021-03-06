/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2016-2017 Intel Corporation. All rights reserved.
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
#include <unistd.h>

#include <rte_eal.h>
#include <rte_cryptodev.h>
#ifdef RTE_LIBRTE_PMD_CRYPTO_SCHEDULER
#include <rte_cryptodev_scheduler.h>
#endif

#include "cperf.h"
#include "cperf_options.h"
#include "cperf_test_vector_parsing.h"
#include "cperf_test_throughput.h"
#include "cperf_test_latency.h"
#include "cperf_test_verify.h"
#include "cperf_test_pmd_cyclecount.h"

#define NUM_SESSIONS 2048
#define SESS_MEMPOOL_CACHE_SIZE 64

const char *cperf_test_type_strs[] = {
	[CPERF_TEST_TYPE_THROUGHPUT] = "throughput",
	[CPERF_TEST_TYPE_LATENCY] = "latency",
	[CPERF_TEST_TYPE_VERIFY] = "verify",
	[CPERF_TEST_TYPE_PMDCC] = "pmd-cyclecount"
};

const char *cperf_op_type_strs[] = {
	[CPERF_CIPHER_ONLY] = "cipher-only",
	[CPERF_AUTH_ONLY] = "auth-only",
	[CPERF_CIPHER_THEN_AUTH] = "cipher-then-auth",
	[CPERF_AUTH_THEN_CIPHER] = "auth-then-cipher",
	[CPERF_AEAD] = "aead"
};

const struct cperf_test cperf_testmap[] = {
		[CPERF_TEST_TYPE_THROUGHPUT] = {
				cperf_throughput_test_constructor,
				cperf_throughput_test_runner,
				cperf_throughput_test_destructor
		},
		[CPERF_TEST_TYPE_LATENCY] = {
				cperf_latency_test_constructor,
				cperf_latency_test_runner,
				cperf_latency_test_destructor
		},
		[CPERF_TEST_TYPE_VERIFY] = {
				cperf_verify_test_constructor,
				cperf_verify_test_runner,
				cperf_verify_test_destructor
		},
		[CPERF_TEST_TYPE_PMDCC] = {
				cperf_pmd_cyclecount_test_constructor,
				cperf_pmd_cyclecount_test_runner,
				cperf_pmd_cyclecount_test_destructor
		}
};

static int
cperf_initialize_cryptodev(struct cperf_options *opts, uint8_t *enabled_cdevs,
			struct rte_mempool *session_pool_socket[])
{
	uint8_t enabled_cdev_count = 0, nb_lcores, cdev_id;
	unsigned int i, j;
	int ret;

	enabled_cdev_count = rte_cryptodev_devices_get(opts->device_type,
			enabled_cdevs, RTE_CRYPTO_MAX_DEVS);
	if (enabled_cdev_count == 0) {
		printf("No crypto devices type %s available\n",
				opts->device_type);
		return -EINVAL;
	}

	nb_lcores = rte_lcore_count() - 1;

	if (nb_lcores < 1) {
		RTE_LOG(ERR, USER1,
			"Number of enabled cores need to be higher than 1\n");
		return -EINVAL;
	}

	/*
	 * Use less number of devices,
	 * if there are more available than cores.
	 */
	if (enabled_cdev_count > nb_lcores)
		enabled_cdev_count = nb_lcores;

	/* Create a mempool shared by all the devices */
	uint32_t max_sess_size = 0, sess_size;

	for (cdev_id = 0; cdev_id < rte_cryptodev_count(); cdev_id++) {
		sess_size = rte_cryptodev_get_private_session_size(cdev_id);
		if (sess_size > max_sess_size)
			max_sess_size = sess_size;
	}

	/*
	 * Calculate number of needed queue pairs, based on the amount
	 * of available number of logical cores and crypto devices.
	 * For instance, if there are 4 cores and 2 crypto devices,
	 * 2 queue pairs will be set up per device.
	 */
	opts->nb_qps = (nb_lcores % enabled_cdev_count) ?
				(nb_lcores / enabled_cdev_count) + 1 :
				nb_lcores / enabled_cdev_count;

	for (i = 0; i < enabled_cdev_count &&
			i < RTE_CRYPTO_MAX_DEVS; i++) {
		cdev_id = enabled_cdevs[i];
#ifdef RTE_LIBRTE_PMD_CRYPTO_SCHEDULER
		/*
		 * If multi-core scheduler is used, limit the number
		 * of queue pairs to 1, as there is no way to know
		 * how many cores are being used by the PMD, and
		 * how many will be available for the application.
		 */
		if (!strcmp((const char *)opts->device_type, "crypto_scheduler") &&
				rte_cryptodev_scheduler_mode_get(cdev_id) ==
				CDEV_SCHED_MODE_MULTICORE)
			opts->nb_qps = 1;
#endif

		struct rte_cryptodev_info cdev_info;
		uint8_t socket_id = rte_cryptodev_socket_id(cdev_id);
		/* range check the socket_id - negative values become big
		 * positive ones due to use of unsigned value
		 */
		if (socket_id >= RTE_MAX_NUMA_NODES)
			socket_id = 0;

		rte_cryptodev_info_get(cdev_id, &cdev_info);
		if (opts->nb_qps > cdev_info.max_nb_queue_pairs) {
			printf("Number of needed queue pairs is higher "
				"than the maximum number of queue pairs "
				"per device.\n");
			printf("Lower the number of cores or increase "
				"the number of crypto devices\n");
			return -EINVAL;
		}
		struct rte_cryptodev_config conf = {
			.nb_queue_pairs = opts->nb_qps,
			.socket_id = socket_id
		};

		struct rte_cryptodev_qp_conf qp_conf = {
			.nb_descriptors = opts->nb_descriptors
		};

		if (session_pool_socket[socket_id] == NULL) {
			char mp_name[RTE_MEMPOOL_NAMESIZE];
			struct rte_mempool *sess_mp;

			snprintf(mp_name, RTE_MEMPOOL_NAMESIZE,
				"sess_mp_%u", socket_id);

			sess_mp = rte_mempool_create(mp_name,
						NUM_SESSIONS,
						max_sess_size,
						SESS_MEMPOOL_CACHE_SIZE,
						0, NULL, NULL, NULL,
						NULL, socket_id,
						0);

			if (sess_mp == NULL) {
				printf("Cannot create session pool on socket %d\n",
					socket_id);
				return -ENOMEM;
			}

			printf("Allocated session pool on socket %d\n", socket_id);
			session_pool_socket[socket_id] = sess_mp;
		}

		ret = rte_cryptodev_configure(cdev_id, &conf);
		if (ret < 0) {
			printf("Failed to configure cryptodev %u", cdev_id);
			return -EINVAL;
		}

		for (j = 0; j < opts->nb_qps; j++) {
			ret = rte_cryptodev_queue_pair_setup(cdev_id, j,
				&qp_conf, socket_id,
				session_pool_socket[socket_id]);
			if (ret < 0) {
				printf("Failed to setup queue pair %u on "
					"cryptodev %u",	j, cdev_id);
				return -EINVAL;
			}
		}

		ret = rte_cryptodev_start(cdev_id);
		if (ret < 0) {
			printf("Failed to start device %u: error %d\n",
					cdev_id, ret);
			return -EPERM;
		}
	}

	return enabled_cdev_count;
}

static int
cperf_verify_devices_capabilities(struct cperf_options *opts,
		uint8_t *enabled_cdevs, uint8_t nb_cryptodevs)
{
	struct rte_cryptodev_sym_capability_idx cap_idx;
	const struct rte_cryptodev_symmetric_capability *capability;

	uint8_t i, cdev_id;
	int ret;

	for (i = 0; i < nb_cryptodevs; i++) {

		cdev_id = enabled_cdevs[i];

		if (opts->op_type == CPERF_AUTH_ONLY ||
				opts->op_type == CPERF_CIPHER_THEN_AUTH ||
				opts->op_type == CPERF_AUTH_THEN_CIPHER) {

			cap_idx.type = RTE_CRYPTO_SYM_XFORM_AUTH;
			cap_idx.algo.auth = opts->auth_algo;

			capability = rte_cryptodev_sym_capability_get(cdev_id,
					&cap_idx);
			if (capability == NULL)
				return -1;

			ret = rte_cryptodev_sym_capability_check_auth(
					capability,
					opts->auth_key_sz,
					opts->digest_sz,
					opts->auth_iv_sz);
			if (ret != 0)
				return ret;
		}

		if (opts->op_type == CPERF_CIPHER_ONLY ||
				opts->op_type == CPERF_CIPHER_THEN_AUTH ||
				opts->op_type == CPERF_AUTH_THEN_CIPHER) {

			cap_idx.type = RTE_CRYPTO_SYM_XFORM_CIPHER;
			cap_idx.algo.cipher = opts->cipher_algo;

			capability = rte_cryptodev_sym_capability_get(cdev_id,
					&cap_idx);
			if (capability == NULL)
				return -1;

			ret = rte_cryptodev_sym_capability_check_cipher(
					capability,
					opts->cipher_key_sz,
					opts->cipher_iv_sz);
			if (ret != 0)
				return ret;
		}

		if (opts->op_type == CPERF_AEAD) {

			cap_idx.type = RTE_CRYPTO_SYM_XFORM_AEAD;
			cap_idx.algo.aead = opts->aead_algo;

			capability = rte_cryptodev_sym_capability_get(cdev_id,
					&cap_idx);
			if (capability == NULL)
				return -1;

			ret = rte_cryptodev_sym_capability_check_aead(
					capability,
					opts->aead_key_sz,
					opts->digest_sz,
					opts->aead_aad_sz,
					opts->aead_iv_sz);
			if (ret != 0)
				return ret;
		}
	}

	return 0;
}

static int
cperf_check_test_vector(struct cperf_options *opts,
		struct cperf_test_vector *test_vec)
{
	if (opts->op_type == CPERF_CIPHER_ONLY) {
		if (opts->cipher_algo == RTE_CRYPTO_CIPHER_NULL) {
			if (test_vec->plaintext.data == NULL)
				return -1;
		} else if (opts->cipher_algo != RTE_CRYPTO_CIPHER_NULL) {
			if (test_vec->plaintext.data == NULL)
				return -1;
			if (test_vec->plaintext.length < opts->max_buffer_size)
				return -1;
			if (test_vec->ciphertext.data == NULL)
				return -1;
			if (test_vec->ciphertext.length < opts->max_buffer_size)
				return -1;
			/* Cipher IV is only required for some algorithms */
			if (opts->cipher_iv_sz &&
					test_vec->cipher_iv.data == NULL)
				return -1;
			if (test_vec->cipher_iv.length != opts->cipher_iv_sz)
				return -1;
			if (test_vec->cipher_key.data == NULL)
				return -1;
			if (test_vec->cipher_key.length != opts->cipher_key_sz)
				return -1;
		}
	} else if (opts->op_type == CPERF_AUTH_ONLY) {
		if (opts->auth_algo != RTE_CRYPTO_AUTH_NULL) {
			if (test_vec->plaintext.data == NULL)
				return -1;
			if (test_vec->plaintext.length < opts->max_buffer_size)
				return -1;
			/* Auth key is only required for some algorithms */
			if (opts->auth_key_sz &&
					test_vec->auth_key.data == NULL)
				return -1;
			if (test_vec->auth_key.length != opts->auth_key_sz)
				return -1;
			if (test_vec->auth_iv.length != opts->auth_iv_sz)
				return -1;
			/* Auth IV is only required for some algorithms */
			if (opts->auth_iv_sz && test_vec->auth_iv.data == NULL)
				return -1;
			if (test_vec->digest.data == NULL)
				return -1;
			if (test_vec->digest.length < opts->digest_sz)
				return -1;
		}

	} else if (opts->op_type == CPERF_CIPHER_THEN_AUTH ||
			opts->op_type == CPERF_AUTH_THEN_CIPHER) {
		if (opts->cipher_algo == RTE_CRYPTO_CIPHER_NULL) {
			if (test_vec->plaintext.data == NULL)
				return -1;
			if (test_vec->plaintext.length < opts->max_buffer_size)
				return -1;
		} else if (opts->cipher_algo != RTE_CRYPTO_CIPHER_NULL) {
			if (test_vec->plaintext.data == NULL)
				return -1;
			if (test_vec->plaintext.length < opts->max_buffer_size)
				return -1;
			if (test_vec->ciphertext.data == NULL)
				return -1;
			if (test_vec->ciphertext.length < opts->max_buffer_size)
				return -1;
			if (test_vec->cipher_iv.data == NULL)
				return -1;
			if (test_vec->cipher_iv.length != opts->cipher_iv_sz)
				return -1;
			if (test_vec->cipher_key.data == NULL)
				return -1;
			if (test_vec->cipher_key.length != opts->cipher_key_sz)
				return -1;
		}
		if (opts->auth_algo != RTE_CRYPTO_AUTH_NULL) {
			if (test_vec->auth_key.data == NULL)
				return -1;
			if (test_vec->auth_key.length != opts->auth_key_sz)
				return -1;
			if (test_vec->auth_iv.length != opts->auth_iv_sz)
				return -1;
			/* Auth IV is only required for some algorithms */
			if (opts->auth_iv_sz && test_vec->auth_iv.data == NULL)
				return -1;
			if (test_vec->digest.data == NULL)
				return -1;
			if (test_vec->digest.length < opts->digest_sz)
				return -1;
		}
	} else if (opts->op_type == CPERF_AEAD) {
		if (test_vec->plaintext.data == NULL)
			return -1;
		if (test_vec->plaintext.length < opts->max_buffer_size)
			return -1;
		if (test_vec->ciphertext.data == NULL)
			return -1;
		if (test_vec->ciphertext.length < opts->max_buffer_size)
			return -1;
		if (test_vec->aead_iv.data == NULL)
			return -1;
		if (test_vec->aead_iv.length != opts->aead_iv_sz)
			return -1;
		if (test_vec->aad.data == NULL)
			return -1;
		if (test_vec->aad.length != opts->aead_aad_sz)
			return -1;
		if (test_vec->digest.data == NULL)
			return -1;
		if (test_vec->digest.length < opts->digest_sz)
			return -1;
	}
	return 0;
}

int
main(int argc, char **argv)
{
	struct cperf_options opts = {0};
	struct cperf_test_vector *t_vec = NULL;
	struct cperf_op_fns op_fns;

	void *ctx[RTE_MAX_LCORE] = { };
	struct rte_mempool *session_pool_socket[RTE_MAX_NUMA_NODES] = { 0 };

	int nb_cryptodevs = 0;
	uint16_t total_nb_qps = 0;
	uint8_t cdev_id, i;
	uint8_t enabled_cdevs[RTE_CRYPTO_MAX_DEVS] = { 0 };

	uint8_t buffer_size_idx = 0;

	int ret;
	uint32_t lcore_id;

	/* Initialise DPDK EAL */
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid EAL arguments!\n");
	argc -= ret;
	argv += ret;

	cperf_options_default(&opts);

	ret = cperf_options_parse(&opts, argc, argv);
	if (ret) {
		RTE_LOG(ERR, USER1, "Parsing on or more user options failed\n");
		goto err;
	}

	ret = cperf_options_check(&opts);
	if (ret) {
		RTE_LOG(ERR, USER1,
				"Checking on or more user options failed\n");
		goto err;
	}

	nb_cryptodevs = cperf_initialize_cryptodev(&opts, enabled_cdevs,
			session_pool_socket);

	if (!opts.silent)
		cperf_options_dump(&opts);

	if (nb_cryptodevs < 1) {
		RTE_LOG(ERR, USER1, "Failed to initialise requested crypto "
				"device type\n");
		nb_cryptodevs = 0;
		goto err;
	}

	ret = cperf_verify_devices_capabilities(&opts, enabled_cdevs,
			nb_cryptodevs);
	if (ret) {
		RTE_LOG(ERR, USER1, "Crypto device type does not support "
				"capabilities requested\n");
		goto err;
	}

	if (opts.test_file != NULL) {
		t_vec = cperf_test_vector_get_from_file(&opts);
		if (t_vec == NULL) {
			RTE_LOG(ERR, USER1,
					"Failed to create test vector for"
					" specified file\n");
			goto err;
		}

		if (cperf_check_test_vector(&opts, t_vec)) {
			RTE_LOG(ERR, USER1, "Incomplete necessary test vectors"
					"\n");
			goto err;
		}
	} else {
		t_vec = cperf_test_vector_get_dummy(&opts);
		if (t_vec == NULL) {
			RTE_LOG(ERR, USER1,
					"Failed to create test vector for"
					" specified algorithms\n");
			goto err;
		}
	}

	ret = cperf_get_op_functions(&opts, &op_fns);
	if (ret) {
		RTE_LOG(ERR, USER1, "Failed to find function ops set for "
				"specified algorithms combination\n");
		goto err;
	}

	if (!opts.silent)
		show_test_vector(t_vec);

	total_nb_qps = nb_cryptodevs * opts.nb_qps;

	i = 0;
	uint8_t qp_id = 0, cdev_index = 0;
	RTE_LCORE_FOREACH_SLAVE(lcore_id) {

		if (i == total_nb_qps)
			break;

		cdev_id = enabled_cdevs[cdev_index];

		uint8_t socket_id = rte_cryptodev_socket_id(cdev_id);

		ctx[i] = cperf_testmap[opts.test].constructor(
				session_pool_socket[socket_id], cdev_id, qp_id,
				&opts, t_vec, &op_fns);
		if (ctx[i] == NULL) {
			RTE_LOG(ERR, USER1, "Test run constructor failed\n");
			goto err;
		}
		qp_id = (qp_id + 1) % opts.nb_qps;
		if (qp_id == 0)
			cdev_index++;
		i++;
	}

	/* Get first size from range or list */
	if (opts.inc_buffer_size != 0)
		opts.test_buffer_size = opts.min_buffer_size;
	else
		opts.test_buffer_size = opts.buffer_size_list[0];

	while (opts.test_buffer_size <= opts.max_buffer_size) {
		i = 0;
		RTE_LCORE_FOREACH_SLAVE(lcore_id) {

			if (i == total_nb_qps)
				break;

			rte_eal_remote_launch(cperf_testmap[opts.test].runner,
				ctx[i], lcore_id);
			i++;
		}
		i = 0;
		RTE_LCORE_FOREACH_SLAVE(lcore_id) {

			if (i == total_nb_qps)
				break;
			rte_eal_wait_lcore(lcore_id);
			i++;
		}

		/* Get next size from range or list */
		if (opts.inc_buffer_size != 0)
			opts.test_buffer_size += opts.inc_buffer_size;
		else {
			if (++buffer_size_idx == opts.buffer_size_count)
				break;
			opts.test_buffer_size = opts.buffer_size_list[buffer_size_idx];
		}
	}

	i = 0;
	RTE_LCORE_FOREACH_SLAVE(lcore_id) {

		if (i == total_nb_qps)
			break;

		cperf_testmap[opts.test].destructor(ctx[i]);
		i++;
	}

	for (i = 0; i < nb_cryptodevs &&
			i < RTE_CRYPTO_MAX_DEVS; i++)
		rte_cryptodev_stop(enabled_cdevs[i]);

	free_test_vector(t_vec, &opts);

	printf("\n");
	return EXIT_SUCCESS;

err:
	i = 0;
	RTE_LCORE_FOREACH_SLAVE(lcore_id) {
		if (i == total_nb_qps)
			break;

		cdev_id = enabled_cdevs[i];

		if (ctx[i] && cperf_testmap[opts.test].destructor)
			cperf_testmap[opts.test].destructor(ctx[i]);
		i++;
	}

	for (i = 0; i < nb_cryptodevs &&
			i < RTE_CRYPTO_MAX_DEVS; i++)
		rte_cryptodev_stop(enabled_cdevs[i]);

	free_test_vector(t_vec, &opts);

	printf("\n");
	return EXIT_FAILURE;
}
