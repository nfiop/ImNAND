/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Liav A.
 *
 * Originally written for the sipbell project.
 * See https://github.com/supercomputer7/sipbell for more details.
 */

#ifndef HOTPLUG_QUEUE_H__
#define HOTPLUG_QUEUE_H__

#include <pthread.h>
#include <stdlib.h>

#define QUEUE_SIZE 256

#include "common/types.h"

typedef enum {
	HOTPLUG_EVENT_NEW_ADAPTER,
	HOTPLUG_EVENT_REMOVED_ADAPTER,
	HOTPLUG_EVENT_SOCKET_ATTACH,
	HOTPLUG_EVENT_SOCKET_DETACH,
} hotplug_event_type_t;

typedef struct {
	uint16_t vid;
	uint16_t pid;

	uint8_t bus_id;
	uint8_t dev_id;
} usb_device_id_t;

typedef struct {
	hotplug_event_type_t type;

	/* Removed adapter's events should point to an existing adapter
	 * Likewise, socket detach events should point to an existing slot.
	 * Socket attach events should point to an existing slot as well.
	 *
	 * New NAND adapters are not referenced yet and we only care about
	 * detection of new USB devices and not "new" platform-builtin
	 * interfaces (relevant to Linux), hence the new_usb_dev field.
	 */
	union {
		struct nand_device_adapter_slot *slot;
		struct nand_device_adapter *adapter;
		usb_device_id_t new_usb_dev;
	} obj;
} hotplug_event_t;

typedef struct {
	hotplug_event_t items[QUEUE_SIZE];
	int head;
	int tail;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
} hotplug_queue_t;

extern hotplug_queue_t g_hotplug_queue;

void queue_init(hotplug_queue_t *q);
int queue_push(hotplug_queue_t *q, hotplug_event_t *queued_event);
void queue_pop_blocking(hotplug_queue_t *q, hotplug_event_t *recv_event);

#define GLOBAL_QUEUE_PUSH_HOTPLUG_EVENT(__queued_event)                        \
	queue_push(&g_hotplug_queue, __queued_event)

#define GLOBAL_QUEUE_POP_HOTPLUG_EVENT(__recv_event)                           \
	queue_pop_blocking(&g_hotplug_queue, __recv_event)

#endif
