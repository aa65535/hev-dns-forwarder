/*
 ============================================================================
 Name        : hev-list.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2013 everyone.
 Description : Doubly-linked list data structure
 ============================================================================
 */

#ifndef __HEV_LIST_H__
#define __HEV_LIST_H__

typedef struct _HevList HevList;

HevList * hev_list_append (HevList *self, void *data);
HevList * hev_list_prepend (HevList *self, void *data);
HevList * hev_list_insert (HevList *self, void *data, unsigned int position);
HevList * hev_list_insert_before (HevList *self, void *data, HevList *sibling);
HevList * hev_list_remove (HevList *self, const void *data);
HevList * hev_list_remove_all (HevList *self, const void *data);

HevList * hev_list_first (HevList *self);
HevList * hev_list_last (HevList *self);
HevList * hev_list_previous (HevList *self);
HevList * hev_list_next (HevList *self);

static inline void *
hev_list_data (HevList *self)
{
	return self ? *((void **) self) : NULL;
}

static inline void
hev_list_set_data (HevList *self, void *data)
{
	if (self)
	  *((void **) self) = data;
}

unsigned int hev_list_length (HevList *self);

void hev_list_free (HevList *self);

#endif /* __HEV_LIST_H__ */

