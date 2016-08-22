/*
 ============================================================================
 Name        : hev-list.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2013 everyone.
 Description : Doubly-linked list data structure
 ============================================================================
 */

#include <stdlib.h>

#include "hev-list.h"
#include "hev-memory-allocator.h"

struct _HevList
{
	void *data;
	HevList *prev;
	HevList *next;
};

HevList *
hev_list_append (HevList *self, void *data)
{
	HevList *new = HEV_MEMORY_ALLOCATOR_ALLOC (sizeof (HevList));
	if (new) {
		new->data = data;
		new->next = NULL;
		if (self) {
			HevList *last = hev_list_last (self);
			last->next = new;
			new->prev = last;
			return self;
		} else {
			new->prev = NULL;
			return new;
		}
	}

	return NULL;
}

HevList *
hev_list_prepend (HevList *self, void *data)
{
	HevList *new = HEV_MEMORY_ALLOCATOR_ALLOC (sizeof (HevList));
	if (new) {
		new->data = data;
		new->prev = NULL;
		if (self) {
			HevList *first = hev_list_first (self);
			first->prev = new;
			new->next = first;
		} else {
			new->next = NULL;
		}
		return new;
	}

	return NULL;
}

HevList *
hev_list_insert (HevList *self, void *data, unsigned int position)
{
	HevList *new = HEV_MEMORY_ALLOCATOR_ALLOC (sizeof (HevList));
	if (new) {
		new->data = data;
		if (self) {
			HevList *node = NULL, *last = NULL, *first = hev_list_first (self);
			unsigned int i = 0;
			for (node=first; node; node=node->next,i++) {
				if (i == position)
				  break;
				last = node;
			}
			if (node) {
				new->next = node;
				new->prev = node->prev;
				if (node->prev)
				  node->prev->next = new;
				else
				  first = new;
				node->prev = new;
			} else {
				new->next = NULL;
				new->prev = last;
				last->next = new;
			}
			return first;
		} else {
			new->prev = NULL;
			new->next = NULL;
			return new;
		}
	}

	return NULL;
}

HevList *
hev_list_insert_before (HevList *self, void *data, HevList *sibling)
{
	HevList *new = HEV_MEMORY_ALLOCATOR_ALLOC (sizeof (HevList));
	if (new) {
		new->data = data;
		if (self) {
			HevList *node = NULL, *last = NULL, *first = hev_list_first (self);
			for (node=first; node; node=node->next) {
				if (node == sibling)
				  break;
				last = node;
			}
			if (node) {
				new->next = node;
				new->prev = node->prev;
				if (node->prev)
				  node->prev->next = new;
				else
				  first = new;
				node->prev = new;
			} else {
				new->next = NULL;
				new->prev = last;
				last->next = new;
			}
			return first;
		} else {
			new->prev = NULL;
			new->next = NULL;
			return new;
		}
	}

	return NULL;
}

HevList *
hev_list_remove (HevList *self, const void *data)
{
	if (self) {
		HevList *node = NULL, *first = hev_list_first (self);
		for (node=first; node; node=node->next) {
			if (data == node->data) {
				if (node->prev)
				  node->prev->next = node->next;
				else
				  first = node->next;
				if (node->next)
				  node->next->prev = node->prev;
				HEV_MEMORY_ALLOCATOR_FREE (node);
				break;
			}
		}
		return first;
	}

	return NULL;
}

HevList *
hev_list_remove_all (HevList *self, const void *data)
{
	if (self) {
		HevList *node = NULL, *first = hev_list_first (self);
		for (node=first; node;) {
			HevList *curr = node;
			node = node->next;
			if (data == curr->data) {
				if (curr->prev)
				  curr->prev->next = curr->next;
				else
				  first = curr->next;
				if (curr->next)
				  curr->next->prev = curr->prev;
				HEV_MEMORY_ALLOCATOR_FREE (curr);
			}
		}
		return first;
	}

	return NULL;
}

HevList *
hev_list_first (HevList *self)
{
	if (self) {
		HevList **first = NULL;
		for (first=&self; *first && (*first)->prev;)
		  first = &(*first)->prev;
		return *first;
	}

	return NULL;
}

HevList *
hev_list_last (HevList *self)
{
	if (self) {
		while (self->next)
		  self = self->next;
	}

	return self;
}

HevList *
hev_list_previous (HevList *self)
{
	if (self)
	  return self->prev;

	return NULL;
}

HevList *
hev_list_next (HevList *self)
{
	if (self)
	  return self->next;

	return NULL;
}

unsigned int
hev_list_length (HevList *self)
{
	HevList *first = hev_list_first (self);
	if (first) {
		HevList *node = NULL;
		unsigned int count = 0;
		for (node=first; node; node=node->next)
		  count ++;
		return count;
	}

	return 0;
}

void
hev_list_free (HevList *self)
{
	if (self) {
		HevList *node = NULL;
		for (node=self->next; node;) {
			HevList *curr = node;
			node = node->next;
			HEV_MEMORY_ALLOCATOR_FREE (curr);
		}
		for (node=self; node;) {
			HevList *curr = node;
			node = node->prev;
			HEV_MEMORY_ALLOCATOR_FREE (curr);
		}
	}
}

