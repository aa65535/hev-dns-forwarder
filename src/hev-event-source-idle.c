/*
 ============================================================================
 Name        : hev-event-source-idle.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2013 everyone.
 Description : A idle event source
 ============================================================================
 */

#include <unistd.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#include "hev-event-source-idle.h"

static bool hev_event_source_idle_prepare (HevEventSource *source);
static bool hev_event_source_idle_check (HevEventSource *source, HevEventSourceFD *fd);
static void hev_event_source_idle_finalize (HevEventSource *source);

struct _HevEventSourceIdle
{
	HevEventSource parent;

	int event_fd;
};

static HevEventSourceFuncs hev_event_source_idle_funcs =
{
	.prepare = hev_event_source_idle_prepare,
	.check = hev_event_source_idle_check,
	.dispatch = NULL,
	.finalize = hev_event_source_idle_finalize,
};

HevEventSource *
hev_event_source_idle_new (void)
{
	int fd = -1;
	HevEventSource *source = NULL;
	HevEventSourceIdle *self = NULL;

	fd = eventfd (0, EFD_NONBLOCK);
	if (-1 == fd)
	  return NULL;

	source = hev_event_source_new (&hev_event_source_idle_funcs,
				sizeof (HevEventSourceIdle));
	if (NULL == source) {
		close (fd);
		return NULL;
	}

	self = (HevEventSourceIdle *) source;
	self->event_fd = fd;
	hev_event_source_set_priority (source, INT32_MIN);
	hev_event_source_add_fd (source, self->event_fd, EPOLLIN | EPOLLET);

	return source;
}

static bool
hev_event_source_idle_prepare (HevEventSource *source)
{
	HevEventSourceIdle *self = (HevEventSourceIdle *) source;
	eventfd_write (self->event_fd, 1);

	return true;
}

static bool
hev_event_source_idle_check (HevEventSource *source, HevEventSourceFD *fd)
{
	HevEventSourceIdle *self = (HevEventSourceIdle *) source;
	if (EPOLLIN & fd->revents) {
		eventfd_t val = 0;
		eventfd_read (self->event_fd, &val);
		fd->revents &= ~EPOLLIN;
		return true;
	}

	return false;
}

static void
hev_event_source_idle_finalize (HevEventSource *source)
{
	HevEventSourceIdle *self = (HevEventSourceIdle *) source;
	close (self->event_fd);
}

