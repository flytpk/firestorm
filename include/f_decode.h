/*
 * This file is part of Firestorm NIDS.
 * Copyright (c) 2008 Gianni Tedesco <gianni@scaramanga.co.uk>
 * Released under the terms of the GNU GPL version 3
*/
#ifndef _FIRESTORM_DECODE_HEADER_INCLUDED_
#define _FIRESTORM_DECODE_HEADER_INCLUDED_

#define DECODE_DEFAULT_MIN_LAYERS	8

enum {
	NS_DLT, /* pcap DLT_* namespace */
	NS_UNIXPF, /* UNIX PF_* namespace */
	NS_ETHER, /* ethernet namespace: 0x0800 = ip, etc.. */
	NS_INET, /* IPv4 protocol id's, 7 = udp etc.. */
	NS_INET6, /* IPv6 protocol id's */
	NS_IPX, /* Novell Netware's IPX */
	NS_CISCO, /* Cisco SNAP id's */
	NS_APPLE, /* Apple SNAP id's */
	NS_USTREAM, /* User stream protocol (any stream protocol) */
	NS_UDGRAM, /* User datagram protocol (not udp per se, but anything) */
	NS_USEQPKT, /* User sequenced datagram protocol (eg. sctp) */
	NS_MAX,
};

struct _decoder {
	const char *d_label;
	void (*d_decode)(struct _pkt *p);
	struct _proto *d_protos;
	struct _decoder *d_next;
};

struct _ns_entry {
	proto_id_t nse_id;
	struct _decoder *nse_decoder;
};

struct _namespace {
	struct _ns_entry *ns_reg;
	unsigned int ns_num_reg;
	const char *ns_label;
};

struct _proto {
	struct _proto *p_next;
	struct _decoder *p_owner;
	size_t p_dcb_sz;
	const char *p_label;
};

extern struct _namespace _ns_arr[NS_MAX];

/* Decoders */
void decoder_add(struct _decoder *d);
void decoder_register(struct _decoder *d, proto_ns_t ns, proto_id_t id);

/* Protocols */
void proto_add(struct _decoder *d, struct _proto *p);

/* Decoding */
void decode(struct _source *s, struct _pkt *p);
int decode_pkt_realloc(struct _pkt *p, unsigned int min_layers);

static inline struct _decoder * _constfn
_ns_entry_search(const struct _ns_entry *p, unsigned int n, proto_id_t id)
{
	while( n ) {
		unsigned int i;

		i = (n / 2);
		if ( id < p[i].nse_id ) {
			n = i;
		}else if ( id > p[i].nse_id ) {
			p = p + (i + 1);
			n = n - (i + 1);
		}else{
			return p[i].nse_decoder;
		}
	}

	return NULL;
}

static inline void _decode_next(pkt_t pkt, proto_ns_t ns, proto_id_t id)
{
	struct _decoder *d;
	d = _ns_entry_search(_ns_arr[ns].ns_reg, _ns_arr[ns].ns_num_reg, id);
	if ( d != NULL )
		d->d_decode(pkt);
}

static inline struct _dcb *_decode_dcb_alloc(pkt_t p, size_t sz)
{
	struct _dcb *ret = p->pkt_dcb_top;
	uint8_t *ptr = (uint8_t *)ret;

	p->pkt_dcb_top = (struct _dcb *)(ptr + sz);

	if ( p->pkt_dcb_top > p->pkt_dcb_end )
		return NULL;

	ret->dcb_next = p->pkt_dcb_top;

	return (struct _dcb *)ret;
}

static inline struct _dcb *_decode_layer(pkt_t pkt, struct _proto *p)
{
	struct _dcb *ret;
	ret = _decode_dcb_alloc(pkt, p->p_dcb_sz);
	if ( ret )
		ret->dcb_proto = p;
	return ret;
}

static inline struct _dcb *_decode_layer2(pkt_t pkt,
						struct _proto *p, size_t sz)
{
	struct _dcb *ret;
	ret = _decode_dcb_alloc(pkt, sz);
	if ( ret )
		ret->dcb_proto = p;
	return ret;
}

#endif /* _FIRESTORM_DECODE_HEADER_INCLUDED_ */
