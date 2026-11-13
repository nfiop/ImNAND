/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#include <asm-generic/errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common/logger.h"
#include "common/prog_opts.h"
#include "common/types.h"

#ifndef VERSION
#define VERSION "unknown"
#endif

int is_module_loaded(const char *module_name)
{
	FILE *fp;
	char line[256];
	char name[256];

	fp = fopen("/proc/modules", "r");
	if (fp == NULL) {
		perror("Failed to open /proc/modules");
		return -ENOENT;
	}

	while (fgets(line, sizeof(line), fp)) {
		if (sscanf(line, "%255s", name) == 1) {
			if (strcmp(name, module_name) == 0) {
				fclose(fp);
				return 1; // Module is loaded
			}
		}
	}

	fclose(fp);

	// Module is not loaded
	return -ENXIO;
}

static bool is_path_existing_and_accessible(const char *path)
{
	return access(path, F_OK) == 0;
}

static int check_ufedm_inserted_in_dynamic_mode()
{
	int rc;

	/* Check that ufedm is inserted already */
	rc = is_module_loaded("ufedm");
	if (rc < 0) {
		if (rc == -ENXIO)
			fprintf(stderr, "Module ufedm is NOT inserted.\n");
		else
			fprintf(stderr, "Error checking module status.\n");
		return rc;
	}

	/* /proc/nand is used for mapping all NAND flash memories in runtime */
	if (is_path_existing_and_accessible("/proc/nand")) {
		LOG_INFO("/proc/nand is not accessible.\n");
		return -ENOENT;
	}

	/* the ufedm_proxy_ctl device node is used for detaching of stale MTDs
	 * and spawning of new devices, when needed.
	 */
	if (is_path_existing_and_accessible("/dev/ufedm_proxy_ctl")) {
		LOG_INFO("/dev/ufedm_proxy_ctl is not accessible.\n");
		return -ENODEV;
	}

	return 0;
}

int linux_platform_main(struct options *opts)
{
	int rc;
	UNUSED(opts);

	rc = check_ufedm_inserted_in_dynamic_mode();
	if (rc < 0)
		return rc;

	return 0;
}
