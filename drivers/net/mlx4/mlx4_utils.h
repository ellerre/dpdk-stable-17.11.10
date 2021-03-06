/*-
 *   BSD LICENSE
 *
 *   Copyright 2017 6WIND S.A.
 *   Copyright 2017 Mellanox
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

#ifndef MLX4_UTILS_H_
#define MLX4_UTILS_H_

#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include <rte_common.h>
#include <rte_log.h>

#include "mlx4.h"

/*
 * Compilation workaround for PPC64 when AltiVec is fully enabled, e.g. std=c11.
 * Otherwise there would be a type conflict between stdbool and altivec.
 */
#if defined(__PPC64__) && !defined(__APPLE_ALTIVEC__)
#undef bool
/* redefine as in stdbool.h */
#define bool _Bool
#endif

#ifndef NDEBUG

/*
 * When debugging is enabled (NDEBUG not defined), file, line and function
 * information replace the driver name (MLX4_DRIVER_NAME) in log messages.
 */

/** Return the file name part of a path. */
static inline const char *
pmd_drv_log_basename(const char *s)
{
	const char *n = s;

	while (*n)
		if (*(n++) == '/')
			s = n;
	return s;
}

#define PMD_DRV_LOG(level, ...) \
	RTE_LOG(level, PMD, \
		RTE_FMT("%s:%u: %s(): " RTE_FMT_HEAD(__VA_ARGS__,) "\n", \
			pmd_drv_log_basename(__FILE__), \
			__LINE__, \
			__func__, \
			RTE_FMT_TAIL(__VA_ARGS__,)))
#define DEBUG(...) PMD_DRV_LOG(DEBUG, __VA_ARGS__)
#define claim_zero(...) assert((__VA_ARGS__) == 0)

#else /* NDEBUG */

/*
 * Like assert(), DEBUG() becomes a no-op and claim_zero() does not perform
 * any check when debugging is disabled.
 */

#define PMD_DRV_LOG(level, ...) \
	RTE_LOG(level, PMD, \
		RTE_FMT(MLX4_DRIVER_NAME ": " \
			RTE_FMT_HEAD(__VA_ARGS__,) "\n", \
		RTE_FMT_TAIL(__VA_ARGS__,)))
#define DEBUG(...) (void)0
#define claim_zero(...) (__VA_ARGS__)

#endif /* NDEBUG */

#define INFO(...) PMD_DRV_LOG(INFO, __VA_ARGS__)
#define WARN(...) PMD_DRV_LOG(WARNING, __VA_ARGS__)
#define ERROR(...) PMD_DRV_LOG(ERR, __VA_ARGS__)

/** Allocate a buffer on the stack and fill it with a printf format string. */
#define MKSTR(name, ...) \
	char name[snprintf(NULL, 0, __VA_ARGS__) + 1]; \
	\
	snprintf(name, sizeof(name), __VA_ARGS__)

/** Generate a string out of the provided arguments. */
#define MLX4_STR(...) # __VA_ARGS__

/** Similar to MLX4_STR() with enclosed macros expanded first. */
#define MLX4_STR_EXPAND(...) MLX4_STR(__VA_ARGS__)

/** Object description used with mlx4_mallocv() and similar functions. */
struct mlx4_malloc_vec {
	size_t align; /**< Alignment constraint (power of 2), 0 if unknown. */
	size_t size; /**< Object size. */
	void **addr; /**< Storage for allocation address. */
};

/* mlx4_utils.c */

int mlx4_fd_set_non_blocking(int fd);
size_t mlx4_mallocv(const char *type, const struct mlx4_malloc_vec *vec,
		    unsigned int cnt);
size_t mlx4_zmallocv(const char *type, const struct mlx4_malloc_vec *vec,
		     unsigned int cnt);
size_t mlx4_mallocv_socket(const char *type, const struct mlx4_malloc_vec *vec,
			   unsigned int cnt, int socket);
size_t mlx4_zmallocv_socket(const char *type, const struct mlx4_malloc_vec *vec,
			    unsigned int cnt, int socket);

#endif /* MLX4_UTILS_H_ */
