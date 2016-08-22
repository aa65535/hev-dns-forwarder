/*
 ============================================================================
 Name        : hev-async-queue.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2013 everyone.
 Description : Asynchronous communication between threads
 ============================================================================
 */

#ifndef __HEV_ASYNC_QUEUE_H__
#define __HEV_ASYNC_QUEUE_H__

typedef struct _HevAsyncQueue HevAsyncQueue;

HevAsyncQueue * hev_async_queue_new (void);

HevAsyncQueue * hev_async_queue_ref (HevAsyncQueue *self);
void hev_async_queue_unref (HevAsyncQueue *self);

void hev_async_queue_push (HevAsyncQueue *self, void *data);
void * hev_async_queue_pop (HevAsyncQueue *self);

#endif /* __HEV_ASYNC_QUEUE_H__ */

