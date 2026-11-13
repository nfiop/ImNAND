/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#ifndef COMMON__LOGGER_H
#define COMMON__LOGGER_H

#include "common/prog_opts.h"
#include <stdio.h>

/*
 * Print to stderr if the configured verbosity is >= level.
 *
 * Usage:
 *   LOG_VERBOSE(1, "Starting application\n");
 *   LOG_VERBOSE(2, "Debug value: %d\n", value);
 */
#define LOG_VERBOSE(level, ...)                                                \
	do {                                                                   \
		if (g_opts.verbose >= (level))                                 \
			fprintf(stderr, __VA_ARGS__);                          \
	} while (0)

/*
 * Convenience macros.
 */
#define LOG_INFO(...) LOG_VERBOSE(1, __VA_ARGS__)
#define LOG_DEBUG(...) LOG_VERBOSE(2, __VA_ARGS__)
#define LOG_TRACE(...) LOG_VERBOSE(3, __VA_ARGS__)

/*
 * Execute a statement only when verbosity is sufficient.
 *
 * Usage:
 *   LOG_IF(2, expensive_debug_operation());
 */
#define LOG_IF(level, statement)                                               \
	do {                                                                   \
		if (g_opts.verbose >= (level))                                 \
			statement;                                             \
	} while (0)

#endif
