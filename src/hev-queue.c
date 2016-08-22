/*
 ============================================================================
 Name        : hev-queue.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2013 everyone.
 Description : Queue data structure
 ============================================================================
 */

#include <stdlib.h>

#include "hev-queue.h"
#include "hev-slist.h"
#include "hev-memory-allocator.h"

struct _HevQueue
{
	HevSList *slist;

	unsigned int ref_count;
};

HevQueue *
hev_queue_new (void)
{
	HevQueue *self = NULL;

	self = HEV_MEMORY_ALLOCATOR_ALLOC (sizeof (HevQueue));
	if (self) {
		self->slist = NULL;
		self->ref_count = 1;
	}

	return self;
}

HevQueue *
hev_queue_ref (HevQueue *self)
{
	if (self) {
		self->ref_count ++;
		return self;
	}

	return NULL;
}

void
hev_queue_unref (HevQueue *self)
{
	if (self) {
		self->ref_count --;
		if (0 == self->ref_count) {
			hev_slist_free (self->slist);
			HEV_MEMORY_ALLOCATOR_FREE (self);
		}
	}
}

void
hev_queue_push (HevQueue *self, void *data)
{
	if (self)
	  self->slist = hev_slist_append (self->slist, data);
}

void *
hev_queue_pop (HevQueue *self)
{
	if (self && self->slist) {
		void *data = NULL;
		data = hev_slist_data (self->slist);
		self->slist = hev_slist_remove (self->slist, data);
		return data;
	}

	return NULL;
}

void *
hev_queue_peek_head (HevQueue *self)
{
	if (self && self->slist)
	  return hev_slist_data (self->slist);

	return NULL;
}

void *
hev_queue_peek_tail (HevQueue *self)
{
	if (self && self->slist) {
		HevSList *slist = NULL;
		slist = hev_slist_last (self->slist);
		return hev_slist_data (slist);
	}

	return NULL;
}

