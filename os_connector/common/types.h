/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#ifndef COMMON__TYPES_H
#define COMMON__TYPES_H

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* For loading of MTD configuration, we simply use Jansson directly */
#include <jansson.h>

#define MAX_NAME_LEN 256
#define UNUSED(__x) (void)__x

/* This macro is inspired from the Linux kernel - it should allocate
 * a memory area and zero it afterwards.
 */
#define zalloc_obj(__x) calloc(1, sizeof(*__x))

struct range {
	uint64_t start;
	uint64_t length;
};

static inline void copy_range_object(struct range *from, struct range *to)
{
	to->start = from->start;
	to->length = from->length;
}

struct byte_array {
	uint8_t *buf;
	size_t length;
};

typedef enum host_intf_type {
	HOST_INTF_TYPE_PLATFORM,
	HOST_INTF_TYPE_USB,
} host_intf_type_t;

typedef enum nand_device_intf_bus_type {
	NAND_DEVICE_INTF_BUS_TYPE_SPI,
	NAND_DEVICE_INTF_BUS_TYPE_PARALLEL,
} nand_device_intf_bus_type_t;

struct nand_device_adapter;
struct nand_device_adapter_slot;

struct own_token {
	pthread_mutex_t mutex;
	/* This should be either a NULL or points to a valid object, like
	 * a TCP stream, that owns the token.
	 * Whoever owns the token is responsbile for releasing it.
	 * Taking and releasing of the token requires a mutex acquire.
	 */
	void *cookie;
};

struct nand_device {
	/* A unique name, used for locating a specific chip device.
	 *
	 * On Windows, a combination of WinUSB is put together like this:
	 *  WinUSB:usb_bus:device_num:chip_num
	 *
	 * On Linux, for example, for USB, the name is "USB on Bus 001 Device
	 * 009": usb:0001:0009
	 *
	 * For other drivers (platform interfaces), such as sunxi_nand, device
	 * on chip select 1: sunxi_nand:0001
	 */
	char *name;

	/* Geometry details */
	size_t erase_block_size;
	size_t page_size; // including OOB size
	size_t page_oob_size;

	nand_device_intf_bus_type_t bus_type;

	struct nand_device_adapter *adapter;
	struct nand_device_adapter_slot *slot;

	bool onfi_capable : 1;

	struct own_token token;
};

struct nand_device_adapter_slot {
	size_t slot_idx;

	struct nand_device_adapter *adapter;

	/* Populated device. Can be NULL, if no device is populated */
	struct nand_device *device;

	/* A slot can be configured to use a detachable socket, hence making
	 * the device to be swapped in a graceful manner.
	 * If supported, we could detect such event, and refresh the device
	 * properties with no user interaction being needed on the other end.
	 * For USB adapters,
	 */
	bool uses_detachable_sockets;
};

struct nand_device_adapter {
	/* A unique name, used for the hashmap for locating a specific adapter.
	 *
	 * On Windows, a combination of WinUSB is put together like this:
	 *  WinUSB:usb_bus:device_num
	 *
	 * On Linux, for example, for USB, the name is "USB on Bus 001 Device
	 * 009": usb:0001:0009 For other drivers (platform interfaces), for
	 * example sunxi_nand platform driver, the associated kernel module is
	 * picked: sunxi_nand
	 *
	 *
	 */
	char *name;

	/* A platform interface is naturally not hot-pluggable (i.e. you can't
	 * remove it, as it's part of the SoC) but USB is removable, thus
	 * requiring inspection in such case.
	 */
	host_intf_type_t intf_type;

	/* Slots for populated and non-populated NAND devices */
	struct nand_device_adapter_slot **slots;
	size_t slots_num;
};

typedef enum {
	IO_CMD_RC_SUCCESS,

	/* Incorrect I/O settings, device not reachable, device status needs
	 * refresh after hot swap, etc.
	 */
	IO_CMD_RC_DEVICE_UNAVAILABLE,
	IO_CMD_RC_DEVICE_ADDRESS_NOT_REACHABLE,
	IO_CMD_RC_DEVICE_SELECTED_ADDRESS_OUT_OF_BOUNDS,
	IO_CMD_RC_DEVICE_STATUS_NOT_COHERENT,

	/* ECC status and errors */
	IO_CMD_RC_DEVICE_INCORRECT_ECC_STATUS,
	IO_CMD_RC_DEVICE_ECC_STRENGTH_NOT_RECOGNIZED,
	IO_CMD_RC_DEVICE_ECC_FAILED,

	/* Some safety guards */
	IO_CMD_RC_DEVICE_SCRUB_DISALLOWED,
	IO_CMD_RC_DEVICE_VALIDATION_NOT_PASSED,
} io_cmd_rc_t;

/* Strictly used as a vtable for server handling of commands */
struct platform_io_ops {
	io_cmd_rc_t (*write_page)(struct nand_device *device,
	    size_t erase_block_idx, size_t page_idx, const uint8_t *buf,
	    size_t len);
	io_cmd_rc_t (*read_page)(struct nand_device *device,
	    size_t erase_block_idx, size_t page_idx, uint8_t *buf,
	    size_t maxlen);
	io_cmd_rc_t (*erase_block)(
	    struct nand_device *device, size_t erase_block_idx);
	io_cmd_rc_t (*erase_all_blocks)(struct nand_device *device);

	io_cmd_rc_t (*load_mtd_configuration)(
	    struct nand_device *device, json_t *root_document);
	io_cmd_rc_t (*unload_mtd_configuration)(struct nand_device *device);
};

struct platform {
	/* On Windows - WinUSB, on Linux - just Linux */
	char *name;

	/* Hashmap of NAND device adapters, specifically a hashmap to handle
	 * hotplug cases of USB adapters, for example.
	 */
	struct hashmap *adapters;

	struct platform_io_ops *io_ops;
};

#endif
