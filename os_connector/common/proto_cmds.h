/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

enum imnand_protocol_cmd {
	/* Ping command, possibly for active keep-alives.
	 *
	 * Query platform can be used for keep-alives as well, but is mainly
	 * used for getting information on the platform that the OS connector
	 * uses.
	 */
	IMNAND_PROTOCOL_CMD_PING = 0,
	IMNAND_PROTOCOL_CMD_QUERY_PLATFORM = 1,

	/* Generic commands for detection, etc */
	IMNAND_PROTOCOL_CMD_QUERY_DEVICES = 2,
	IMNAND_PROTOCOL_CMD_QUERY_DRIVERS = 3,
	IMNAND_PROTOCOL_CMD_QUERY_DEVICE = 4,

	/* Configuration commands -
	 * Loading configuration implictly takes a lock (i.e. ownership) over
	 * a NAND device for the current TCP connection, unloading releases it.
	 * Reloading does unload and load in one command, without releasing the
	 * lock.
	 */
	IMNAND_PROTOCOL_CMD_LOAD_MTD_CONFIGURATION = 5,
	IMNAND_PROTOCOL_CMD_UNLOAD_MTD_CONFIGURATION = 6,
	IMNAND_PROTOCOL_CMD_RELOAD_MTD_CONFIGURATION = 7,

	/* I/O commands */
	IMNAND_PROTOCOL_CMD_ERASE_DEVICE_ALL_BLOCKS = 8,
	IMNAND_PROTOCOL_CMD_ERASE_DEVICE_BLOCK = 9,
	IMNAND_PROTOCOL_CMD_WRITE_DEVICE_PAGE = 10,
	IMNAND_PROTOCOL_CMD_READ_DEVICE_PAGE = 11,
};

#endif
