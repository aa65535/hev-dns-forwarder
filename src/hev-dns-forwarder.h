/*
 ============================================================================
 Name        : hev-dns-forwarder.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2014 everyone.
 Description : DNS Forwarder
 ============================================================================
 */

#ifndef __HEV_DNS_FORWARDER_H__
#define __HEV_DNS_FORWARDER_H__

#include "hev-lib.h"

typedef struct _HevDNSForwarder HevDNSForwarder;

HevDNSForwarder * hev_dns_forwarder_new (HevEventLoop *loop,
			const char *addr, unsigned short port,
			const char *upstream);

HevDNSForwarder * hev_dns_forwarder_ref (HevDNSForwarder *self);
void hev_dns_forwarder_unref (HevDNSForwarder *self);

#endif /* __HEV_DNS_FORWARDER_H__ */

