/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#ifndef COMMON__OPTIONS_H
#define COMMON__OPTIONS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct options {
	const char *config;
	const char *address;
	uint16_t port;
	unsigned int verbose;
	bool non_detachable_sockets;
	bool demo;
	bool ipv6;
};

extern struct options g_opts;

#endif
