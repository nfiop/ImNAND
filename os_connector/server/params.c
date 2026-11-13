/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common/logger.h"
#include "common/prog_opts.h"
#include "common/types.h"

static void print_usage(const char *prog)
{
	printf(
	    "Usage: %s [OPTIONS]\n"
	    "\n"
	    "Options:\n"
	    "  -c, --config FILE                JSON mapping configuration "
	    "file\n"
	    "  -p, --port PORT                  Server port (default: 8080)\n"
	    "  -D, --demo                       Demo mode (fake device, "
	    "disables config)\n"
	    "  -d, --non-detachable-sockets     Force non-detachable sockets\n"
	    "  -a, --address ADDR               Server listen address "
	    "(default: 0.0.0.0)\n"
	    "  -6, --ipv6                       Enable IPv6\n"
	    "  -v, --verbose                    Increase verbosity "
	    "(repeatable)\n"
	    "  -V, --version                    Print version and exit\n"
	    "  -h, --help                       Show this help\n"
	    "\n"
	    "Examples:\n"
	    "  %s -c mapping.json -p 9000\n"
	    "  %s --demo --non-detachable-sockets --verbose\n"
	    "  %s -a :: -6 -p 8080\n",
	    prog, prog, prog, prog);
}

static bool parse_port(const char *arg, uint16_t *port)
{
	char *end = NULL;
	errno = 0;

	unsigned long value = strtoul(arg, &end, 10);

	if (errno || end == arg || *end != '\0' || value < 1 || value > 65535) {
		return false;
	}

	*port = (uint16_t)value;
	return true;
}

static void print_options(const struct options *opts)
{
	printf("Configuration:\n");
	printf("  config:                    %s\n",
	    opts->config ? opts->config : "(none)");
	printf("  demo:                      %s\n", opts->demo ? "yes" : "no");
	printf("  non-detachable-sockets:    %s\n",
	    opts->non_detachable_sockets ? "yes" : "no");
	printf("  address:                   %s\n", opts->address);
	printf("  port:                      %u\n", opts->port);
	printf("  IPv6:                      %s\n",
	    opts->ipv6 ? "enabled" : "disabled");
	printf("  verbose:                   %u\n", opts->verbose);
}

int parse_args(int argc, char **argv)
{
	static const struct option long_options[] = {
	    {"config", required_argument, NULL, 'c'},
	    {"port", required_argument, NULL, 'p'},
	    {"demo", no_argument, NULL, 'D'},
	    {"non-detachable-sockets", no_argument, NULL, 'd'},
	    {"address", required_argument, NULL, 'a'},
	    {"ipv6", no_argument, NULL, '6'},
	    {"verbose", no_argument, NULL, 'v'},
	    {"version", no_argument, NULL, 'V'},
	    {"help", no_argument, NULL, 'h'}, {NULL, 0, NULL, 0}};

	int opt;

	while ((opt = getopt_long(
		    argc, argv, "c:p:Dda:64vVh", long_options, NULL)) != -1) {
		switch (opt) {
		case 'c':
			g_opts.config = optarg;
			break;

		case 'p':
			if (!parse_port(optarg, &g_opts.port)) {
				fprintf(stderr, "Invalid port: %s\n", optarg);
				return EXIT_FAILURE;
			}
			break;

		case 'D':
			g_opts.demo = true;
			g_opts.config = NULL;
			break;

		case 'd':
			g_opts.non_detachable_sockets = true;
			break;

		case 'a':
			g_opts.address = optarg;
			break;

		case '6':
			g_opts.ipv6 = true;
			break;

		case 'v':
			g_opts.verbose++;
			break;

		case 'V':
			printf("%s\n", "");
			return EXIT_SUCCESS;

		case 'h':
			print_usage(argv[0]);
			return EXIT_SUCCESS;

		default:
			print_usage(argv[0]);
			return EXIT_FAILURE;
		}
	}

	/* Reject unexpected positional arguments. */
	if (optind < argc) {
		fprintf(stderr, "Unexpected argument: %s\n", argv[optind]);
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}

	/* Demo mode and configuration file are mutually exclusive. */
	if (g_opts.demo) {
		g_opts.config = NULL;
	} else if (!g_opts.config) {
		fprintf(stderr, "Error: --config is required unless --demo is "
				"specified.\n");
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}

	LOG_IF(1, print_options(&g_opts));

	return EXIT_SUCCESS;
}
