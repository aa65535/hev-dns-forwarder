/*
 ============================================================================
 Name        : hev-queue.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2013 everyone.
 Description : Queue data structure
 ============================================================================
 */

#ifndef __HEV_QUEUE_H__
#define __HEV_QUEUE_H__

typedef struct _HevQueue HevQueue;

HevQueue * hev_queue_new (void);

HevQueue * hev_queue_ref (HevQueue *self);
void hev_queue_unref (HevQueue *self);

void hev_queue_push (HevQueue *self, void *data);
void * hev_queue_pop (HevQueue *self);

void * hev_queue_peek_head (HevQueue *self);
void * hev_queue_peek_tail (HevQueue *self);

#endif /* __HEV_QUEUE_H__ */

