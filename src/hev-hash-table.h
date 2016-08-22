/*
 ============================================================================
 Name        : hev-hash-table.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2013 everyone.
 Description : Hash table data structure
 ============================================================================
 */

#ifndef __HEV_HASH_TABLE_H__
#define __HEV_HASH_TABLE_H__

#include <stddef.h>
#include <stdbool.h>

#include "hev-list.h"
#include "hev-memory-allocator.h"

typedef struct _HevHashTable HevHashTable;
typedef unsigned int (*HevHTHashFunc) (const void *key);
typedef bool (*HevHTEqualFunc) (const void *a, const void *b);
typedef void (*HevHTFunc) (void *key, void *value, void *user_data);
typedef bool (*HevHTRFunc) (void *key, void *value, void *user_data);

unsigned int hev_hash_table_direct_hash (const void *v);
bool hev_hash_table_direct_equal (const void *v1, const void *v2);

unsigned int hev_hash_table_int_hash (const void *v);
bool hev_hash_table_int_equal (const void *v1, const void *v2);

unsigned int hev_hash_table_int64_hash (const void *v);
bool hev_hash_table_int64_equal (const void *v1, const void *v2);

unsigned int hev_hash_table_double_hash (const void *v);
bool hev_hash_table_double_equal (const void *v1, const void *v2);

unsigned int hev_hash_table_str_hash (const void *v);
bool hev_hash_table_str_equal (const void *v1, const void *v2);

HevHashTable * hev_hash_table_new (HevHTHashFunc hash_func, HevHTEqualFunc key_equal_func);
HevHashTable * hev_hash_table_new_full (HevHTHashFunc hash_func, HevHTEqualFunc key_equal_func,
			HevDestroyNotify key_destroy_notify, HevDestroyNotify value_destroy_notify);

HevHashTable * hev_hash_table_ref (HevHashTable *self);
void hev_hash_table_unref (HevHashTable *self);

bool hev_hash_table_insert (HevHashTable *self, void *key, void *value);
bool hev_hash_table_replace (HevHashTable *self, void *key, void *value);
bool hev_hash_table_add (HevHashTable *self, void *key);
bool hev_hash_table_contains (HevHashTable *self, const void *key);
unsigned int hev_hash_table_size (HevHashTable *self);
void * hev_hash_table_lookup (HevHashTable *self, const void *key);
bool hev_hash_table_lookup_extended (HevHashTable *self, const void *lookup_key, void **orig_key, void **value);
void hev_hash_table_foreach (HevHashTable *self, HevHTFunc func, void *user_data);
void * hev_hash_table_find (HevHashTable *self, HevHTRFunc predicate, void *user_data);
bool hev_hash_table_remove (HevHashTable *self, const void *key);
bool hev_hash_table_steal (HevHashTable *self, const void *key);
unsigned int hev_hash_table_foreach_remove (HevHashTable *self, HevHTRFunc func, void *user_data);
unsigned int hev_hash_table_foreach_steal (HevHashTable *self, HevHTRFunc func, void *user_data);
void hev_hash_table_remove_all (HevHashTable *self);
void hev_hash_table_steal_all (HevHashTable *self);
HevList * hev_hash_table_get_keys (HevHashTable *self);
HevList * hev_hash_table_get_values (HevHashTable *self);

#endif /* __HEV_HASH_TABLE_H__ */

