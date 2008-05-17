/*
 * This file is part of Firestorm NIDS.
 * Copyright (c) 2008 Gianni Tedesco <gianni@scaramanga.co.uk>
 * Released under the terms of the GNU GPL version 3
*/

#include <stdio.h>

#include <firestorm.h>
#include <f_capture.h>
#include <f_decode.h>

struct _pipeline {
	struct list_head p_sources;
	unsigned int p_type;
};

pipeline_t pipeline_new(void)
{
	struct _pipeline *p;

	p = calloc(1, sizeof(*p));
	if ( p ) {
		INIT_LIST_HEAD(&p->p_sources);
	}
	return p;
}

void pipeline_free(pipeline_t p)
{
	struct _source *s, *tmp;

	list_for_each_entry_safe(s, tmp, &p->p_sources, s_list) {
		list_del(&s->s_list);
		_source_free(s);
	}

	free(p);
}

int pipeline_add_source(pipeline_t p, source_t s)
{
	unsigned int type;

	assert(p != NULL);
	assert(s != NULL);
	assert(s->s_capdev != NULL);

	type = (s->s_capdev->c_flags & CAPDEV_ASYNC) == 0;
	if ( list_empty(&p->p_sources) ) {
		p->p_type = type;
	}else if ( type != p->p_type ) {
		mesg(M_ERR, "%s: adding %s[%s]: cannot mix sync and async "
			"captures.", __FUNCTION__,
			s->s_capdev->c_name, s->s_name);
		return 0;
	}

	list_add_tail(&s->s_list, &p->p_sources);
	return 1;
}

int pipeline_go(pipeline_t p)
{
	struct _source *s, *tmp;
	pkt_t pkt;

	list_for_each_entry_safe(s, tmp, &p->p_sources, s_list) {
		mesg(M_INFO, "pipeline: starting: %s[%s]",
			s->s_capdev->c_name, s->s_name);
		for(;;){
			pkt = s->s_capdev->c_dequeue(s);
			if ( pkt == NULL )
				break;
			decode(s, pkt);
		}
		mesg(M_INFO, "pipeline: finishing: %s[%s]",
			s->s_capdev->c_name, s->s_name);
		list_del(&s->s_list);
		_source_free(s);
	}

	return 1;
}
