/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#ifndef COMMON__NET_ORDER_H
#define COMMON__NET_ORDER_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#define REQUEST_HEADER_SIZE 16
#define RESPONSE_HEADER_SIZE 14

typedef struct {
	uint32_t data_length;
	uint32_t command;
	uint64_t request_id;
	uint8_t *data;
} protocol_request_t;

typedef struct {
	uint16_t status;
	uint32_t data_length;
	uint64_t request_id;
	uint8_t *data;
} protocol_response_t;

static inline void write_u16(uint8_t *buf, uint16_t value)
{
	buf[0] = (uint8_t)(value >> 8);
	buf[1] = (uint8_t)value;
}

static inline void write_u32(uint8_t *buf, uint32_t value)
{
	buf[0] = (uint8_t)(value >> 24);
	buf[1] = (uint8_t)(value >> 16);
	buf[2] = (uint8_t)(value >> 8);
	buf[3] = (uint8_t)value;
}

static inline void write_u64(uint8_t *buf, uint64_t value)
{
	for (int i = 0; i < 8; i++)
		buf[i] = (uint8_t)(value >> (56 - i * 8));
}

static inline uint16_t read_u16(const uint8_t *buf)
{
	return ((uint16_t)buf[0] << 8) | (uint16_t)buf[1];
}

static inline uint32_t read_u32(const uint8_t *buf)
{
	return ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) |
	       ((uint32_t)buf[2] << 8) | (uint32_t)buf[3];
}

static inline uint64_t read_u64(const uint8_t *buf)
{
	uint64_t value = 0;

	for (int i = 0; i < 8; i++)
		value = (value << 8) | buf[i];

	return value;
}

#endif
