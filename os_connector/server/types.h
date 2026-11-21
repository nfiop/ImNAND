/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#ifndef SERVER__PLATFORM_BASE_H
#define SERVER__PLATFORM_BASE_H

#include "common/types.h"

void platform_base_destroy(struct platform *platform);

errno_t platform_base_init(
    struct platform *platform, char *name, struct platform_io_ops *ops);

errno_t nand_device_adapter_base_init(struct nand_device_adapter *adapter,
    char *name, host_intf_type_t intf_type);

#endif