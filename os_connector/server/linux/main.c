/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

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

errno_t is_module_loaded(const char *module_name)
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

static errno_t check_ufedm_inserted_in_dynamic_mode()
{
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

static errno_t check_ufedm_inserted_in_static_mode()
{
	/* The ufedm_proxy0 device node is guaranteed to exist in static mode,
	 * because there must be simply at least one device being attached.
	 */
	if (is_path_existing_and_accessible("/dev/ufedm_proxy0")) {
		LOG_INFO("/dev/ufedm_proxy0 is not accessible.\n");
		return -ENODEV;
	}

	return 0;
}

errno_t linux_platform_main(struct platform *platform, struct options *opts)
{
	bool static_mode;
	errno_t rc;

	UNUSED(opts);
	UNUSED(platform);
	UNUSED(static_mode);

	/* Check that ufedm is inserted already */
	rc = is_module_loaded("ufedm");
	if (rc < 0) {
		if (rc == -ENXIO)
			fprintf(stderr, "Module ufedm is NOT inserted.\n");
		else
			fprintf(stderr, "Error checking module status.\n");
		return rc;
	}

	static_mode = false;

	rc = check_ufedm_inserted_in_dynamic_mode();
	if (rc < 0) {
		rc = check_ufedm_inserted_in_static_mode();
		if (rc < 0)
			return rc;

		static_mode = true;
	}

	return 0;
}
