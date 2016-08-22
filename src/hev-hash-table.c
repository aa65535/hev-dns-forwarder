/*
 ============================================================================
 Name        : hev-hash-table.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2013 everyone.
 Description : Hash table data structure
 ============================================================================
 */

#include <string.h>

#include "hev-hash-table.h"

#define HASH_TABLE_MIN_SHIFT	3 /* 1 << 3 == 8 buckets */

#define UNUSED_HASH_VALUE 0
#define TOMBSTONE_HASH_VALUE 1
#define HASH_IS_UNUSED(h_) ((h_) == UNUSED_HASH_VALUE)
#define HASH_IS_TOMBSTONE(h_) ((h_) == TOMBSTONE_HASH_VALUE)
#define HASH_IS_REAL(h_) ((h_) >= 2)

#define MAX(a, b) ((a > b) ? a : b)

struct _HevHashTable
{
	int size;
	int mod;
	unsigned int mask;
	int nnodes;
	int noccupied;
	unsigned int ref_count;

	void **keys;
	unsigned int *hashes;
	void **values;

	HevHTHashFunc hash_func;
	HevHTEqualFunc key_equal_func;

	HevDestroyNotify key_destroy_notify;
	HevDestroyNotify value_destroy_notify;
};

/* Each table size has an associated prime modulo (the first prime
 * lower than the table size) used to find the initial bucket. Probing
 * then works modulo 2^n. The prime modulo is necessary to get a
 * good distribution with poor hash functions.
 */
static const int prime_mod [] =
{
  1,          /* For 1 << 0 */
  2,
  3,
  7,
  13,
  31,
  61,
  127,
  251,
  509,
  1021,
  2039,
  4093,
  8191,
  16381,
  32749,
  65521,      /* For 1 << 16 */
  131071,
  262139,
  524287,
  1048573,
  2097143,
  4194301,
  8388593,
  16777213,
  33554393,
  67108859,
  134217689,
  268435399,
  536870909,
  1073741789,
  2147483647  /* For 1 << 31 */
};

unsigned int
hev_hash_table_direct_hash (const void *v)
{
#if (4 == __SIZEOF_POINTER__)
	return (unsigned int) v;
#elif (8 == __SIZEOF_POINTER__)
	return (unsigned int)(unsigned long) v;
#else
#error "The size of pointer isn't supported!"
#endif
}

bool
hev_hash_table_direct_equal (const void *v1, const void *v2)
{
	return v1 == v2;
}

unsigned int
hev_hash_table_int_hash (const void *v)
{
	return *(const int *) v;
}

bool
hev_hash_table_int_equal (const void *v1, const void *v2)
{
	return *((const int *) v1) == *((const int *) v2);
}

unsigned int
hev_hash_table_int64_hash (const void *v)
{
	return (unsigned int) *(const int64_t *) v;
}

bool
hev_hash_table_int64_equal (const void *v1, const void *v2)
{
	return *((const int64_t *) v1) == *((const int64_t *) v2);
}

unsigned int
hev_hash_table_double_hash (const void *v)
{
	return (unsigned int) *(const double *) v;
}

bool
hev_hash_table_double_equal (const void *v1, const void *v2)
{
	return *((const double *) v1) == *(( const double *) v2);
}

unsigned int
hev_hash_table_str_hash (const void *v)
{
	const signed char *p = NULL;
	unsigned int h = 5381;

	for (p=v; '\0'!=*p; p++)
	  h = (h << 5) + h + *p;

	return h;
}

bool
hev_hash_table_str_equal (const void *v1, const void *v2)
{
	return (0 == strcmp (v1, v2));
}

static void
hev_hash_table_set_shift (HevHashTable *self, int shift)
{
	int i = 0;
	unsigned int mask = 0;

	self->size = 1 << shift;
	self->mod = prime_mod[shift];

	for (i=0; i<shift; i++) {
		mask <<= 1;
		mask |= 1;
	}

	self->mask = mask;
}

static void
hev_hash_table_remove_all_nodes (HevHashTable *self, bool notify)
{
	int i = -1;
	void *key = NULL, *value = NULL;

	self->nnodes = 0;
	self->noccupied = 0;

	if (!notify || ((NULL == self->key_destroy_notify) &&
			(NULL == self->value_destroy_notify))) {
		memset (self->hashes, 0, self->size * sizeof (unsigned int));
		memset (self->keys, 0, self->size * sizeof (void *));
		memset (self->values, 0, self->size * sizeof (void *));

		return ;
	}

	for (i=0; i<self->size; i++) {
		if (HASH_IS_REAL (self->hashes[i])) {
			key = self->keys[i];
			value = self->values[i];

			self->hashes[i] = UNUSED_HASH_VALUE;
			self->keys[i] = NULL;
			self->values[i] = NULL;

			if (NULL != self->key_destroy_notify)
			  self->key_destroy_notify (key);

			if (NULL != self->value_destroy_notify)
			  self->value_destroy_notify (value);
		} else if (HASH_IS_TOMBSTONE (self->hashes[i])) {
			self->hashes[i] = UNUSED_HASH_VALUE;
		}
	}
}

static inline unsigned int
hev_hash_table_lookup_node (HevHashTable *self, const void *key,
			unsigned int *hash_return)
{
	unsigned int node_index = 0;
	unsigned int node_hash = 0;
	unsigned int hash_value = 0;
	unsigned int first_tombstone = 0;
	bool have_tombstone = false;
	unsigned int step = 0;

	hash_value = self->hash_func (key);
	if (!HASH_IS_REAL (hash_value))
	  hash_value = 2;

	*hash_return = hash_value;

	node_index = hash_value % self->mod;
	node_hash = self->hashes[node_index];

	while (!HASH_IS_UNUSED (node_hash)) {
		/* We first check if our full hash values
		* are equal so we can avoid calling the full-blown
		* key equality function in most cases.
		*/
		if (node_hash == hash_value) {
			void *node_key = self->keys[node_index];

			if (self->key_equal_func) {
			if (self->key_equal_func (node_key, key))
			return node_index;
			} else if (node_key == key) {
			return node_index;
			}
		} else if (HASH_IS_TOMBSTONE (node_hash) && !have_tombstone) {
			first_tombstone = node_index;
			have_tombstone = true;
		}

		step++;
		node_index += step;
		node_index &= self->mask;
		node_hash = self->hashes[node_index];
	}

	if (have_tombstone)
	  return first_tombstone;

	return node_index;
}

static void *
new0 (size_t block, size_t count)
{
	void *data = HEV_MEMORY_ALLOCATOR_ALLOC (block * count);
	if (data)
	  memset (data, 0, block * count);
	return data;
}

static void *
memdup (void *src, size_t len)
{
	void *data = HEV_MEMORY_ALLOCATOR_ALLOC (len);
	if (data)
	  memcpy (data, src, len);
	return data;
}

static int
hev_hash_table_find_closest_shift (int n)
{
	int i;

	for (i=0; n; i++)
	  n >>= 1;

	return i;
}

static void
hev_hash_table_set_shift_from_size (HevHashTable *self, int size)
{
	int shift;

	shift = hev_hash_table_find_closest_shift (size);
	shift = MAX (shift, HASH_TABLE_MIN_SHIFT);

	hev_hash_table_set_shift (self, shift);
}

static void
hev_hash_table_resize (HevHashTable *self)
{
	void **new_keys;
	void **new_values;
	unsigned int *new_hashes;
	int old_size, i;

	old_size = self->size;
	hev_hash_table_set_shift_from_size (self, self->nnodes * 2);

	new_keys = new0 (sizeof (void *), self->size);
	if (self->keys == self->values)
	  new_values = new_keys;
	else
	  new_values = new0 (sizeof (void *), self->size);
	new_hashes = new0 (sizeof (unsigned int), self->size);

	for (i=0; i<old_size; i++) {
		unsigned int node_hash = self->hashes[i];
		unsigned int hash_val;
		unsigned int step = 0;

		if (!HASH_IS_REAL (node_hash))
		  continue;

		hash_val = node_hash % self->mod;

		while (!HASH_IS_UNUSED (new_hashes[hash_val])) {
			step++;
			hash_val += step;
			hash_val &= self->mask;
		}

		new_hashes[hash_val] = self->hashes[i];
		new_keys[hash_val] = self->keys[i];
		new_values[hash_val] = self->values[i];
	}

	if (self->keys != self->values)
	  HEV_MEMORY_ALLOCATOR_FREE (self->values);

	HEV_MEMORY_ALLOCATOR_FREE (self->keys);
	HEV_MEMORY_ALLOCATOR_FREE (self->hashes);

	self->keys = new_keys;
	self->values = new_values;
	self->hashes = new_hashes;

	self->noccupied = self->nnodes;
}

static inline void
hev_hash_table_maybe_resize (HevHashTable *self)
{
  int noccupied = self->noccupied;
  int size = self->size;

  if ((size > self->nnodes * 4 && size > 1 << HASH_TABLE_MIN_SHIFT) ||
      (size <= noccupied + (noccupied / 16)))
	hev_hash_table_resize (self);
}

static bool
hev_hash_table_insert_node (HevHashTable *self, unsigned int node_index,
			unsigned int key_hash, void *new_key, void *new_value,
			bool keep_new_key, bool reusing_key)
{
	bool already_exists;
	unsigned int old_hash;
	void *key_to_free = NULL;
	void *value_to_free = NULL;

	old_hash = self->hashes[node_index];
	already_exists = HASH_IS_REAL (old_hash);

	/* Proceed in three steps.  First, deal with the key because it is the
	* most complicated.  Then consider if we need to split the table in
	* two (because writing the value will result in the set invariant
	* becoming broken).  Then deal with the value.
	*
	* There are three cases for the key:
	*
	*  - entry already exists in table, reusing key:
	*    free the just-passed-in new_key and use the existing value
	*
	*  - entry already exists in table, not reusing key:
	*    free the entry in the table, use the new key
	*
	*  - entry not already in table:
	*    use the new key, free nothing
	*
	* We update the hash at the same time...
	*/
	if (already_exists) {
		/* Note: we must record the old value before writing the new key
		* because we might change the value in the event that the two
		* arrays are shared.
		*/
		value_to_free = self->values[node_index];

		if (keep_new_key) {
			key_to_free = self->keys[node_index];
			self->keys[node_index] = new_key;
		} else {
			key_to_free = new_key;
		}
	} else {
		self->hashes[node_index] = key_hash;
		self->keys[node_index] = new_key;
	}

	/* Step two: check if the value that we are about to write to the
	* table is the same as the key in the same position.  If it's not,
	* split the table.
	*/
	if (self->keys == self->values && self->keys[node_index] != new_value)
	  self->values = memdup (self->keys, sizeof (void *) * self->size);

	/* Step 3: Actually do the write */
	self->values[node_index] = new_value;

	/* Now, the bookkeeping... */
	if (!already_exists) {
		self->nnodes++;

		if (HASH_IS_UNUSED (old_hash)) {
		/* We replaced an empty node, and not a tombstone */
			self->noccupied++;
			hev_hash_table_maybe_resize (self);
		}
	}

	if (already_exists) {
		if (self->key_destroy_notify && !reusing_key)
		  (* self->key_destroy_notify) (key_to_free);
		if (self->value_destroy_notify)
		  (* self->value_destroy_notify) (value_to_free);
	}

	return !already_exists;
}

static bool
hev_hash_table_insert_internal (HevHashTable *self,
			void *key, void *value, bool keep_new_key)
{
	unsigned int key_hash;
	unsigned int node_index;

	if (!self)
	  return false;

	node_index = hev_hash_table_lookup_node (self, key, &key_hash);
	return hev_hash_table_insert_node (self, node_index, key_hash,
				key, value, keep_new_key, false);
}

static void
hev_hash_table_remove_node (HevHashTable *self, int i, bool notify)
{
	void *key;
	void *value;

	key = self->keys[i];
	value = self->values[i];

	/* Erect tombstone */
	self->hashes[i] = TOMBSTONE_HASH_VALUE;

	/* Be GC friendly */
	self->keys[i] = NULL;
	self->values[i] = NULL;

	self->nnodes--;

	if (notify && self->key_destroy_notify)
	  self->key_destroy_notify (key);

	if (notify && self->value_destroy_notify)
	  self->value_destroy_notify (value);
}

static bool
hev_hash_table_remove_internal (HevHashTable *self, const void *key, bool notify)
{
	unsigned int node_index;
	unsigned int node_hash;

	if (!self)
	  return false;

	node_index = hev_hash_table_lookup_node (self, key, &node_hash);

	if (!HASH_IS_REAL (self->hashes[node_index]))
	  return false;

	hev_hash_table_remove_node (self, node_index, notify);
	hev_hash_table_maybe_resize (self);

	return true;
}

static unsigned int
hev_hash_table_foreach_remove_or_steal (HevHashTable *self,
			HevHTRFunc func, void *user_data, bool notify)
{
	unsigned int deleted = 0;
	int i;

	for (i=0; i<self->size; i++) {
		unsigned int node_hash = self->hashes[i];
		void *node_key = self->keys[i];
		void *node_value = self->values[i];

		if (HASH_IS_REAL (node_hash) &&
				(* func) (node_key, node_value, user_data)) {
			hev_hash_table_remove_node (self, i, notify);
			deleted++;
		}
	}

	hev_hash_table_maybe_resize (self);

	return deleted;
}

HevHashTable *
hev_hash_table_new_full (HevHTHashFunc hash_func, HevHTEqualFunc key_equal_func,
			HevDestroyNotify key_destroy_notify, HevDestroyNotify value_destroy_notify)
{
	HevHashTable *self = NULL;

	self = HEV_MEMORY_ALLOCATOR_ALLOC (sizeof (HevHashTable));
	if (self) {
		self->ref_count = 1;
		hev_hash_table_set_shift (self, HASH_TABLE_MIN_SHIFT);
		self->nnodes = 0;
		self->noccupied = 0;
		self->hash_func = hash_func ? hash_func : hev_hash_table_direct_hash;
		self->key_equal_func = key_equal_func;
		self->key_destroy_notify = key_destroy_notify;
		self->value_destroy_notify = value_destroy_notify;
		self->keys = new0 (sizeof (void *), self->size);
		self->values = self->keys;
		self->hashes = new0 (sizeof (unsigned int), self->size);
	}

	return self;
}

HevHashTable *
hev_hash_table_new (HevHTHashFunc hash_func, HevHTEqualFunc key_equal_func)
{
	return hev_hash_table_new_full (hash_func, key_equal_func, NULL, NULL);
}

HevHashTable *
hev_hash_table_ref (HevHashTable *self)
{
	if (self)
	  self->ref_count ++;
	return self;
}

void
hev_hash_table_unref (HevHashTable *self)
{
	if (self) {
		self->ref_count --;
		if (0 == self->ref_count) {
			hev_hash_table_remove_all_nodes (self, true);
			if (self->keys != self->values)
			  HEV_MEMORY_ALLOCATOR_FREE (self->values);
			HEV_MEMORY_ALLOCATOR_FREE (self->keys);
			HEV_MEMORY_ALLOCATOR_FREE (self->hashes);
			HEV_MEMORY_ALLOCATOR_FREE (self);
		}
	}
}

bool
hev_hash_table_insert (HevHashTable *self, void *key, void *value)
{
	return hev_hash_table_insert_internal (self, key, value, false);
}

bool
hev_hash_table_replace (HevHashTable *self, void *key, void *value)
{
	return hev_hash_table_insert_internal (self, key, value, true);
}

bool
hev_hash_table_add (HevHashTable *self, void *key)
{
	return hev_hash_table_insert_internal (self, key, key, true);
}

bool
hev_hash_table_contains (HevHashTable *self, const void *key)
{
	unsigned int node_index;
	unsigned int node_hash;

	if (!self)
	  return false;

	node_index = hev_hash_table_lookup_node (self, key, &node_hash);

	return HASH_IS_REAL (self->hashes[node_index]);
}

unsigned int
hev_hash_table_size (HevHashTable *self)
{
	return self ? self->nnodes : 0;
}

void *
hev_hash_table_lookup (HevHashTable *self, const void *key)
{
	unsigned int node_index;
	unsigned int node_hash;

	if (!self)
	  return NULL;

	node_index = hev_hash_table_lookup_node (self, key, &node_hash);
	return HASH_IS_REAL (self->hashes[node_index]) ? self->values[node_index] : NULL;
}

bool
hev_hash_table_lookup_extended (HevHashTable *self, const void *lookup_key, void **orig_key, void **value)
{
	unsigned int node_index;
	unsigned int node_hash;

	if (!self)
	  return false;

	node_index = hev_hash_table_lookup_node (self, lookup_key, &node_hash);

	if (!HASH_IS_REAL (self->hashes[node_index]))
	  return false;

	if (orig_key)
	  *orig_key = self->keys[node_index];
	if (value)
	  *value = self->values[node_index];

	return true;
}

void
hev_hash_table_foreach (HevHashTable *self, HevHTFunc func, void *user_data)
{
	int i;

	if (!self || !func)
	  return;

	for (i=0; i<self->size; i++) {
		unsigned int node_hash = self->hashes[i];
		void *node_key = self->keys[i];
		void *node_value = self->values[i];

		if (HASH_IS_REAL (node_hash))
		  (* func) (node_key, node_value, user_data);
	}
}

void *
hev_hash_table_find (HevHashTable *self, HevHTRFunc predicate, void *user_data)
{
	int i;
	bool match;

	if (!self || !predicate)
	  return NULL;

	match = false;

	for (i=0; i<self->size; i++) {
		unsigned int node_hash = self->hashes[i];
		void *node_key = self->keys[i];
		void *node_value = self->values[i];

		if (HASH_IS_REAL (node_hash))
		  match = predicate (node_key, node_value, user_data);

		if (match)
		  return node_value;
	}

	return NULL;
}

bool
hev_hash_table_remove (HevHashTable *self, const void *key)
{
	return hev_hash_table_remove_internal (self, key, true);
}

bool
hev_hash_table_steal (HevHashTable *self, const void *key)
{
	return hev_hash_table_remove_internal (self, key, false);
}

unsigned int
hev_hash_table_foreach_remove (HevHashTable *self, HevHTRFunc func, void *user_data)
{
	if (!self || !func)
	  return 0;

	return hev_hash_table_foreach_remove_or_steal (self, func, user_data, true);
}

unsigned int
hev_hash_table_foreach_steal (HevHashTable *self, HevHTRFunc func, void *user_data)
{
	if (!self || !func)
	  return 0;

	return hev_hash_table_foreach_remove_or_steal (self, func, user_data, false);
}

void
hev_hash_table_remove_all (HevHashTable *self)
{
	if (!self)
	  return;

	hev_hash_table_remove_all_nodes (self, true);
	hev_hash_table_maybe_resize (self);
}

void
hev_hash_table_steal_all (HevHashTable *self)
{
	if (!self)
	  return;

	hev_hash_table_remove_all_nodes (self, false);
	hev_hash_table_maybe_resize (self);
}

HevList *
hev_hash_table_get_keys (HevHashTable *self)
{
	int i;
	HevList *retval;

	if (!self)
	  return NULL;

	retval = NULL;
	for (i=0; i<self->size; i++) {
		if (HASH_IS_REAL (self->hashes[i]))
		  retval = hev_list_prepend (retval, self->keys[i]);
	}

	return retval;
}

HevList *
hev_hash_table_get_values (HevHashTable *self)
{
	int i;
	HevList *retval;

	if (!self)
	  return NULL;

	retval = NULL;
	for (i=0; i<self->size; i++) {
		if (HASH_IS_REAL (self->hashes[i]))
		  retval = hev_list_prepend (retval, self->values[i]);
	}

	return retval;
}

