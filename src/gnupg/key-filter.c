#include <stdlib.h>

#include "gnupg/gpg-common.h"

int filter_gpg_keys_by_predicate(struct gpg_key_list *keys,
		int (*predicate)(gpgme_key_t, void *), void *optional_data)
{
	int filtered_keys = 0;
	struct gpg_key_list_node *node = keys->head;
	while (node) {
		struct gpg_key_list_node *next = node->next;
		struct gpg_key_list_node *prev = node->prev;
		if (predicate(node->key, optional_data)) {
			node = next;
			continue;
		}

		if (keys->head == node)
			keys->head = next;
		if (keys->tail == node)
			keys->tail = prev;

		if (prev)
			prev->next = next;
		if (next)
			next->prev = prev;

		gpgme_key_release(node->key);
		free(node);

		node = next;
		filtered_keys++;
	}

	return filtered_keys;
}

int filter_gpg_unusable_keys(gpgme_key_t key, void *data)
{
	// Unused
	(void) data;

	if (key->expired)
		return 0;
	if (key->disabled)
		return 0;
	if (key->invalid)
		return 0;
	if (key->revoked)
		return 0;

	if (!key->can_encrypt)
		return 0;

	return 1;
}

int filter_gpg_secret_keys(gpgme_key_t key, void *data)
{
	// Unused
	(void) data;

	return !key->secret;
}
