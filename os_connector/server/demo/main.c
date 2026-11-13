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

int demo_platform_main(struct options *opts)
{
	UNUSED(opts);
	return 0;
}
