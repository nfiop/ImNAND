/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

typedef enum { TRANSPORT_TCP, TRANSPORT_UNIX } transport_t;

typedef struct {
	uv_stream_t *stream;
} client_t;

typedef struct {
	transport_t transport;
} server_t;

static void alloc_buffer(
    uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
	(void)handle;

	buf->base = malloc(suggested_size);
	buf->len = suggested_size;
}

static void on_write(uv_write_t *req, int status)
{
	free(req->data);
	free(req);

	if (status < 0)
		fprintf(stderr, "Write error: %s\n", uv_strerror(status));
}

static void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
	if (nread > 0) {
		uv_write_t *req = malloc(sizeof(*req));
		char *data = malloc(nread);

		if (!req || !data) {
			free(req);
			free(data);
			free(buf->base);
			return;
		}

		memcpy(data, buf->base, nread);

		req->data = data;

		uv_buf_t write_buf = uv_buf_init(data, (unsigned int)nread);

		int rc = uv_write(req, stream, &write_buf, 1, on_write);

		if (rc < 0) {
			free(data);
			free(req);
			fprintf(stderr, "uv_write: %s\n", uv_strerror(rc));
		}
	}

	if (nread < 0) {
		uv_close((uv_handle_t *)stream, NULL);
	}

	free(buf->base);
}

static void on_connection(uv_stream_t *server, int status)
{
	int rc;
	uv_pipe_t *pipe;
	uv_tcp_t *tcp_sock;
	uv_stream_t *client;
	server_t *ctx;

	if (status < 0) {
		fprintf(stderr, "Connection error: %s\n", uv_strerror(status));
		return;
	}

	ctx = server->data;

	if (ctx->transport == TRANSPORT_TCP) {
		tcp_sock = malloc(sizeof(*tcp_sock));

		if (!tcp_sock)
			return;

		uv_tcp_init(server->loop, tcp_sock);
		client = (uv_stream_t *)tcp_sock;
	} else {
		pipe = malloc(sizeof(*pipe));

		if (!pipe)
			return;

		uv_pipe_init(server->loop, pipe, 0);
		client = (uv_stream_t *)pipe;
	}

	rc = uv_accept(server, client);

	if (rc < 0) {
		free(client);
		return;
	}

	uv_read_start(client, alloc_buffer, on_read);
}

static int prepare_socket_path(const char *path, bool force_remove)
{
	int rc;
	struct stat st;

	rc = lstat(path, &st);

	if (rc < 0) {
		if (errno == ENOENT)
			return 0;
		return -errno;
	}

	if (!force_remove)
		return -ESTALE;

	if (!S_ISSOCK(st.st_mode))
		return -EEXIST;

	rc = unlink(path);
	if (rc < 0)
		return -errno;

	return 0;
}

int run_server()
{
	uv_loop_t *loop = uv_default_loop();

	server_t *ctx = calloc(1, sizeof(*ctx));
	if (!ctx)
		return 1;

	if (strcmp(argv[1], "tcp") == 0) {
		ctx->transport = TRANSPORT_TCP;

		uv_tcp_t server;
		struct sockaddr_in addr;

		int port = atoi(argv[2]);

		uv_tcp_init(loop, &server);

		uv_ip4_addr("0.0.0.0", port, &addr);

		server.data = ctx;

		uv_tcp_bind(&server, (const struct sockaddr *)&addr, 0);

		uv_listen((uv_stream_t *)&server, 128, on_connection);

		printf("TCP server listening on port %d\n", port);
	} else if (strcmp(argv[1], "unix") == 0) {
		uv_pipe_t server;

		ctx->transport = TRANSPORT_UNIX;

		prepare_socket_path(argv[2], true);

		uv_pipe_init(loop, &server, 0);

		server.data = ctx;

		uv_pipe_bind(&server, argv[2]);

		uv_listen((uv_stream_t *)&server, 128, on_connection);

		printf("UNIX server listening on %s\n", argv[2]);
	} else {
		fprintf(stderr, "Unknown transport\n");
		return 1;
	}

	return uv_run(loop, UV_RUN_DEFAULT);
}
