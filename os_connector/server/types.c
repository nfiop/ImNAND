/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#include "server/types.h"

/* It's easier to treat adapter indices as a globally-recognized
 * number which is incremented on each new instance, as there's
 * basically one platform in the program runtime.
 */
static idx_t s_global_adapter_idx = 0;
static pthread_mutex_t adapter_idx_mutex = PTHREAD_MUTEX_INITIALIZER;

static idx_t get_new_adapter_index()
{
	idx_t tmp;

	pthread_mutex_lock(&mutex);
	tmp = s_global_adapter_idx;
	s_global_adapter_idx++;
	pthread_mutex_unlock(&mutex);

	return tmp;
}

errno_t nand_device_adapter_base_init(
    struct nand_device_adapter *adapter, char *name, host_intf_type_t intf_type)
{
	errno_t rc;

	adapter->name = strdup(name);
	if (!adapter->name) {
		rc = -ENOMEM;
		goto exit;
	}

	adapter->intf_type = intf_type;
	adapter->idx = get_new_adapter_index();

	return 0;

exit:
	return rc;
}

void platform_base_destroy(struct platform *platform)
{
	pthread_rwlock_destroy(&platform->adapters_lock);
	free(platform->name);
	hashmap_free(platform->adapters);
}

errno_t platform_base_init(
    struct platform *platform, char *name, struct platform_io_ops *ops)
{
	errno_t rc;

	rc = pthread_rwlock_init(&platform->adapters_lock, NULL);
	if (rc != 0)
		goto exit;

	platform->name = strdup(name);
	if (!platform->name) {
		rc = -ENOMEM;
		goto destroy_lock;
	}

	platform->adapters = hashmap_new(sizeof(struct nand_device_adapter), 1,
	    0, 0, key_value_pair_hash, key_value_pair_compare,
	    key_value_pair_destroy, NULL);
	if (!special_params->values) {
		rc = -ENOMEM;
		goto free_name;
	}

	return 0;

destroy_lock:
	pthread_rwlock_destroy(&platform->adapters_lock);
free_name:
	free(platform->name);
exit:
	return rc;
}