static void read_exact_cb(
    uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
	read_context_t *ctx = stream->data;

	if (nread < 0) {
		uv_read_stop(stream);

		ctx->callback(stream, (int)nread, ctx->buffer, ctx->received,
		    ctx->user_data);

		free(buf->base);
		free(ctx);
		return;
	}

	if (nread == 0) {
		free(buf->base);
		return;
	}

	size_t remaining = ctx->length - ctx->received;
	size_t copy_len = (size_t)nread < remaining ? (size_t)nread : remaining;

	memcpy(ctx->buffer + ctx->received, buf->base, copy_len);
	ctx->received += copy_len;

	free(buf->base);

	if (ctx->received == ctx->length) {
		uv_read_stop(stream);

		ctx->callback(
		    stream, 0, ctx->buffer, ctx->length, ctx->user_data);

		free(ctx);
	}
}

static void read_exact_alloc_cb(
    uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
	(void)handle;

	size_t size = suggested_size ? suggested_size : 4096;

	buf->base = malloc(size);
	buf->len = buf->base ? size : 0;
}

static int read_exact(uv_stream_t *stream, size_t length,
    read_callback_t callback, void *user_data)
{
	if (length == 0 || !callback)
		return UV_EINVAL;

	read_context_t *ctx = calloc(1, sizeof(*ctx));

	if (!ctx)
		return UV_ENOMEM;

	ctx->stream = stream;
	ctx->length = length;
	ctx->callback = callback;
	ctx->user_data = user_data;
	ctx->buffer = malloc(length);

	if (!ctx->buffer) {
		free(ctx);
		return UV_ENOMEM;
	}

	stream->data = ctx;

	int rc = uv_read_start(stream, read_exact_alloc_cb, read_exact_cb);

	if (rc < 0) {
		free(ctx->buffer);
		free(ctx);
	}

	return rc;
}

typedef void (*request_callback_t)(uv_stream_t *stream, int status,
    protocol_request_t *request, void *user_data);

typedef struct {
	request_callback_t callback;
	void *user_data;
	protocol_request_t request;
} request_context_t;

static void request_payload_cb(uv_stream_t *stream, int status, uint8_t *buffer,
    size_t length, void *user_data)
{
	request_context_t *ctx = user_data;

	if (status == 0) {
		ctx->request.data = buffer;
		ctx->request.data_length = (uint32_t)length;
	} else {
		free(buffer);
	}

	ctx->callback(stream, status, &ctx->request, ctx->user_data);

	free(ctx);
}

static void request_header_cb(uv_stream_t *stream, int status, uint8_t *buffer,
    size_t length, void *user_data)
{
	request_context_t *ctx = user_data;

	if (status < 0) {
		free(buffer);
		ctx->callback(stream, status, NULL, ctx->user_data);
		free(ctx);
		return;
	}

	ctx->request.data_length = read_u32(buffer);
	ctx->request.command = read_u32(buffer + 4);
	ctx->request.request_id = read_u64(buffer + 8);

	free(buffer);

	if (ctx->request.data_length == 0) {
		ctx->callback(stream, 0, &ctx->request, ctx->user_data);
		free(ctx);
		return;
	}

	int rc = read_exact(
	    stream, ctx->request.data_length, request_payload_cb, ctx);

	if (rc < 0) {
		ctx->callback(stream, rc, NULL, ctx->user_data);
		free(ctx);
	}
}

int protocol_read_request(
    uv_stream_t *stream, request_callback_t callback, void *user_data)
{
	request_context_t *ctx = calloc(1, sizeof(*ctx));

	if (!ctx)
		return UV_ENOMEM;

	ctx->callback = callback;
	ctx->user_data = user_data;

	int rc =
	    read_exact(stream, REQUEST_HEADER_SIZE, request_header_cb, ctx);

	if (rc < 0)
		free(ctx);

	return rc;
}

typedef struct {
	uv_write_t req;
	uint8_t *buffer;
} write_context_t;

typedef void (*write_callback_t)(
    uv_stream_t *stream, int status, void *user_data);

typedef struct {
	write_callback_t callback;
	void *user_data;
	write_context_t write;
} response_write_context_t;

static void response_write_cb(uv_write_t *req, int status)
{
	response_write_context_t *ctx = (response_write_context_t *)req->data;

	free(ctx->write.buffer);

	if (ctx->callback)
		ctx->callback(req->handle, status, ctx->user_data);

	free(ctx);
}

int protocol_write_response(uv_stream_t *stream,
    const protocol_response_t *response, write_callback_t callback,
    void *user_data)
{
	if (!stream || !response)
		return UV_EINVAL;

	if (response->data_length && !response->data)
		return UV_EINVAL;

	if (response->data_length > SIZE_MAX - RESPONSE_HEADER_SIZE)
		return UV_EINVAL;

	size_t total_size = RESPONSE_HEADER_SIZE + response->data_length;

	response_write_context_t *ctx = calloc(1, sizeof(*ctx));

	if (!ctx)
		return UV_ENOMEM;

	ctx->write.buffer = malloc(total_size);

	if (!ctx->write.buffer) {
		free(ctx);
		return UV_ENOMEM;
	}

	ctx->callback = callback;
	ctx->user_data = user_data;

	uint8_t *buf = ctx->write.buffer;

	write_u16(buf, response->status);
	write_u32(buf + 2, response->data_length);
	write_u64(buf + 6, response->request_id);

	if (response->data_length) {
		memcpy(buf + RESPONSE_HEADER_SIZE, response->data,
		    response->data_length);
	}

	uv_buf_t uvbuf = uv_buf_init((char *)buf, (unsigned int)total_size);

	ctx->write.req.data = ctx;

	int rc =
	    uv_write(&ctx->write.req, stream, &uvbuf, 1, response_write_cb);

	if (rc < 0) {
		free(ctx->write.buffer);
		free(ctx);
		return rc;
	}

	return 0;
}

static void on_response_written(
    uv_stream_t *stream, int status, void *user_data)
{
	(void)stream;
	(void)user_data;

	if (status < 0)
		fprintf(stderr, "Write error: %s\n", uv_strerror(status));
}

static void on_request(uv_stream_t *stream, int status,
    protocol_request_t *request, void *user_data)
{
	(void)user_data;

	if (status < 0) {
		fprintf(stderr, "Read error: %s\n", uv_strerror(status));
		return;
	}

	printf("Command: %u\n", request->command);
	printf("Request ID: %llu\n", (unsigned long long)request->request_id);

	protocol_response_t response = {.status = 0,
	    .request_id = request->request_id,
	    .data_length = 0,
	    .data = NULL};

	protocol_write_response(stream, &response, on_response_written, NULL);

	free(request->data);
}
