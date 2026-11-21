/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common/logger.h"
#include "common/prog_opts.h"
#include "common/types.h"

#include "ufedmd-common/ints.h"

#define DEFAULT_NAND_PAGE_DATA_SIZE 2048
#define DEFAULT_NAND_PAGE_OOB_SIZE 64
#define DEFAULT_NAND_PAGES_PER_BLOCK 64
#define DEFAULT_NAND_BLOCKS_PER_CHIP 1024

struct eraseblock_metadata {
	bool erased;
	bool bad;
};

struct emulated_nand_device {
	char *name;
	char *vendor_name;

	/* With OOB included */
	size_t eraseblock_total_size;
	/* Without OOB */
	size_t eraseblock_total_data_size;

	size_t eraseblocks_count;
	struct eraseblock_metadata **blocks;

	size_t pages_count;
	size_t page_data_size;
	size_t page_oob_size;

	/* Buffer for data+OOB for all pages*/
	uint8_t *buffer;

	/* With OOB included */
	size_t buf_total_size;

	/* Without OOB */
	size_t buf_total_data_size;
};

static struct emulated_nand_device *s_dev;

static errno_t create_demo_nand_device(char *name, char *vendor_name,
    size_t page_data_size, size_t page_oob_size, size_t pages_per_block,
    size_t blocks_per_chip)
{
	errno_t rc;

	/* This is the only strict requirement. Any NAND chip I've seen or
	 * searched for had either 512 bytes per page (old NAND chips) or a
	 * multiply of it (2048, 4096, etc).
	 */
	if ((page_data_size % 512) != 0)
		return -EINVAL;

	if (addition_size_t_would_overflow(page_data_size, page_oob_size))

		if (multiplication_size_t_would_overflow(
			page_data_size, pages_per_block))

			s_dev = zalloc_obj(s_dev);
	if (!s_dev) {
		rc = -ENOMEM;
		goto exit;
	}

	s_dev->name = strdup(name);
	if (!s_dev->name) {
		rc = -ENOMEM;
		goto free_obj;
	}

	s_dev->vendor_name = strdup(vendor_name);
	if (!s_dev->vendor_name) {
		rc = -ENOMEM;
		goto free_name;
	}

	s_dev->buf_total_size = (page_data_size + page_oob_size) *
				pages_per_block * blocks_per_chip;
	s_dev->buffer = malloc(s_dev->buf_total_size);
	if (s_dev->buffer == NULL) {
		rc = -ENOMEM;
		goto free_vendor_name;
	}

	s_dev->eraseblocks_count = blocks_per_chip;
	s_dev->blocks =
	    calloc(blocks_per_chip, sizeof(struct eraseblock_metadata *));
	if (s_dev->blocks == NULL) {
		rc = -ENOMEM;
		goto free_buffer;
	}

	s_dev->buf_total_data_size =
	    page_data_size * pages_per_block * blocks_per_chip;
	s_dev->eraseblock_total_size =
	    (page_data_size + page_oob_size) * pages_per_block;
	s_dev->eraseblock_total_data_size = page_data_size * pages_per_block;
	return 0;

free_buffer:
	free(s_dev->buffer);
free_vendor_name:
	free(s_dev->vendor_name);
free_name:
	free(s_dev->name);
free_obj:
	free(s_dev);
exit:
	return rc;
}

static errno_t create_default_demo_nand_device()
{
	/* We create a default NAND flash demo device based on this parameters;
	 * which are sane enough to have - Vendor	Part
	 * ID Code	Full ID		 MB	 Page OOB Block	 ONFI Micron
	 * MT29F1G08ABBDA	0xA1	0x2CA1801502 128 2048 64  128 KB 1.0
	 *
	 * The actual buffer size is calculated -
	 * (PAGE_SIZE + OOB_SIZE_PER_PAGE) * PAGES_PER_BLOCK * BLOCKS_PER_IC
	 *
	 */
	return create_demo_nand_device("MT29F1G08ABBDA", "Micron",
	    DEFAULT_NAND_PAGE_DATA_SIZE, DEFAULT_NAND_PAGE_OOB_SIZE,
	    DEFAULT_NAND_PAGES_PER_BLOCK, DEFAULT_NAND_BLOCKS_PER_CHIP);
}

// static struct platform_io_ops demo_ops = {
// 	io_cmd_rc_t (*write_page)(struct nand_device *device,
// 	    size_t erase_block_idx, size_t page_idx, const uint8_t *buf,
// 	    size_t len);
// 	io_cmd_rc_t (*read_page)(struct nand_device *device,
// 	    size_t erase_block_idx, size_t page_idx, uint8_t *buf,
// 	    size_t maxlen);
// 	io_cmd_rc_t (*erase_block)(
// 	    struct nand_device *device, size_t erase_block_idx);
// 	io_cmd_rc_t (*erase_all_blocks)(struct nand_device *device);

// 	io_cmd_rc_t (*load_mtd_configuration)(
// 	    struct nand_device *device, json_t *root_document);
// 	io_cmd_rc_t (*unload_mtd_configuration)(struct nand_device *device);
// };

errno_t demo_platform_main(struct platform *platform, struct options *opts)
{
	errno_t rc;
	UNUSED(opts);
	UNUSED(platform);

	// TODO: Add option to specify a "non-default" NAND geometry
	// TODO: Add option to specify a NAND flash ID to get a specific
	// geometry
	// TODO: Add option to specify a file as the backing storage (via mmap)
	// TODO: Add option to specify a set of marked bad blocks
	// rc = platform_base_init(platform, "demo", )
	rc = create_default_demo_nand_device();
	return rc;
}
