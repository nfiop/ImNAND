/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#error "Windows is not supported yet"
#elif defined(__APPLE__) && defined(__MACH__)
#error "MacOS is not supported currently"
#elif defined(__linux__)
#define platform_main linux_platform_main
#else
#error "Not a supported OS"
#endif

#include "common/logger.h"
#include "common/prog_opts.h"
#include "common/types.h"
#include "server/hotplug_queue.h"

extern int platform_main(struct options *opts);
extern int demo_platform_main(struct options *opts);
extern int parse_args(int argc, char **argv);

hotplug_queue_t g_hotplug_queue;

struct options g_opts = {.config = NULL,
    .address = "0.0.0.0",
    .port = 8080,
    .verbose = 0,
    .non_detachable_sockets = false,
    .demo = false,
    .ipv6 = false};

static int initialize_platform()
{
	int rc;

	if (!g_opts.demo) {
		rc = platform_main(&g_opts);
	} else {
#if defined(__linux__)
		LOG_INFO("info: using demo mode, consider using nandsim for "
			 "MTD integration\n");
#endif
		rc = demo_platform_main(&g_opts);
	}

	return rc;
}

int main(int argc, char **argv)
{
	int rc;
	rc = parse_args(argc, argv);
	if (rc != EXIT_SUCCESS)
		return EXIT_FAILURE;

	/* Initialize a POSIX-based hotplug queue for delivering hot-plug events
	 * from the platform.
	 */
	queue_init(&g_hotplug_queue);

	/* Initialize platform I/O ops vtable and platform-specific structures
	 */
	rc = initialize_platform();
	if (rc != EXIT_SUCCESS)
		return EXIT_FAILURE;

	/* TODO: Initialize server with libuv, prepare to defer execution to its
	 * threads
	 */

	return 0;
}
