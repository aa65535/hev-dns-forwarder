/*
 ============================================================================
 Name        : hev-async-queue.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2013 everyone.
 Description : Asynchronous communication between threads
 ============================================================================
 */

#include <stdlib.h>
#include <pthread.h>

#include "hev-queue.h"
#include "hev-async-queue.h"
#include "hev-memory-allocator.h"

struct _HevAsyncQueue
{
	HevQueue *queue;

	pthread_mutex_t mutex;
	pthread_cond_t cond;
	unsigned int waiting_threads;
	unsigned int ref_count;
};

HevAsyncQueue *
hev_async_queue_new (void)
{
	HevAsyncQueue *self = NULL;

	self = HEV_MEMORY_ALLOCATOR_ALLOC (sizeof (HevAsyncQueue));
	if (self) {
		int ret_mutex = 0, ret_cond = 0;

		self->queue = hev_queue_new ();
		self->waiting_threads = 0;
		self->ref_count = 1;
		ret_mutex = pthread_mutex_init (&self->mutex, NULL);
		ret_cond = pthread_cond_init (&self->cond, NULL);

		if (ret_mutex || ret_cond) {
			if (!ret_mutex)
			  pthread_mutex_destroy (&self->mutex);
			if (!ret_cond)
			  pthread_cond_destroy (&self->cond);
			HEV_MEMORY_ALLOCATOR_FREE (self);
			self = NULL;
		}
	}

	return self;
}

HevAsyncQueue *
hev_async_queue_ref (HevAsyncQueue *self)
{
	if (self && !pthread_mutex_lock (&self->mutex)) {
		self->ref_count ++;
		pthread_mutex_unlock (&self->mutex);

		return self;
	}

	return NULL;
}

void
hev_async_queue_unref (HevAsyncQueue *self)
{
	if (self && !pthread_mutex_lock (&self->mutex)) {
		self->ref_count --;
		if (0 == self->ref_count) {
			hev_queue_unref (self->queue);
			pthread_cond_destroy (&self->cond);
			pthread_mutex_unlock (&self->mutex);
			pthread_mutex_destroy (&self->mutex);
			HEV_MEMORY_ALLOCATOR_FREE (self);
		} else {
			pthread_mutex_unlock (&self->mutex);
		}
	}
}

void
hev_async_queue_push (HevAsyncQueue *self, void *data)
{
	if (self && !pthread_mutex_lock (&self->mutex)) {
		hev_queue_push (self->queue, data);
		if (0 < self->waiting_threads)
		  pthread_cond_signal (&self->cond);
		pthread_mutex_unlock (&self->mutex);
	}
}

void *
hev_async_queue_pop (HevAsyncQueue *self)
{
	if (self && !pthread_mutex_lock (&self->mutex)) {
		void *data = NULL;
		self->waiting_threads ++;
		while (!(data = hev_queue_pop (self->queue)))
		  pthread_cond_wait (&self->cond, &self->mutex);
		self->waiting_threads --;
		pthread_mutex_unlock (&self->mutex);

		return data;
	}

	return NULL;
}

