/*
 * $Id: xcap_callback.c,v 1.2 2007/02/20 13:40:09 anca_vamanu Exp $
 *
 * xcap_client module - openser xcap client module
 *
 * Copyright (C) 2007 Voice Sistem S.R.L.
 *
 * This file is part of Kamailio, a free SIP server.
 *
 * Kamailio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * Kamailio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *	
 *History:
 *--------
 *  2007-08-30  initial version (anca)
 */

#include <stdio.h>
#include <stdlib.h>
#include "../../dprint.h"
#include "../../mem/shm_mem.h"
#include "../presence/hash.h"
#include "xcap_callbacks.h"
#include "xcap_client.h"

void run_xcap_update_cb(int type, str xid, char* stream)
{
	xcap_callback_t* cb;

	for (cb= xcapcb_list; cb; cb=cb->next)
	{
		if(cb->types & type) 
		{	
			LM_DBG("found callback\n");
			cb->callback(type, xid, stream);
		}
	}
}

int register_xcapcb( int types, xcap_cb f)
{
	xcap_callback_t* xcb;

	xcb= (xcap_callback_t*)shm_malloc(sizeof(xcap_callback_t));
	if(xcb== NULL)
	{
		ERR_MEM(SHARE_MEM);
	}
	memset(xcb, 0, sizeof(xcap_callback_t));

	xcb->callback= f;
	xcb->types= types;
	xcb->next= xcapcb_list;
	xcapcb_list= xcb;
	return 0;

error:
	return -1;
}

void destroy_xcapcb_list(void)
{
	xcap_callback_t* xcb, *prev_xcb;
	
	xcb= xcapcb_list;
	while(xcb)
	{
		prev_xcb= xcb;
		xcb= xcb->next;
		shm_free(xcb);
	}
}
