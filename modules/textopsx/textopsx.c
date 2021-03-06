/**
 *
 * Copyright (C) 2001-2003 FhG Fokus
 *
 * This file is part of ser, a free SIP server.
 *
 * ser is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * For a license to use the ser software under conditions
 * other than those described here, or to purchase support for this
 * software, please contact iptel.org by e-mail at the following addresses:
 *    info@iptel.org
 *
 * ser is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fnmatch.h>

#include "../../sr_module.h"
#include "../../dprint.h"
#include "../../data_lump.h"
#include "../../msg_translator.h"
#include "../../tcp_options.h"
#include "../../mod_fix.h"

#include "api.h"

MODULE_VERSION

static int msg_apply_changes_f(sip_msg_t *msg, char *str1, char *str2);

static int change_reply_status_f(sip_msg_t*, char*, char*);
static int change_reply_status_fixup(void** param, int param_no);

static int w_keep_hf_f(sip_msg_t*, char*, char*);

static int w_fnmatch2_f(sip_msg_t*, char*, char*);
static int w_fnmatch3_f(sip_msg_t*, char*, char*, char*);
static int fixup_fnmatch(void** param, int param_no);

static int w_remove_body_f(struct sip_msg*, char*, char *);
static int bind_textopsx(textopsx_api_t *tob);

static int mod_init(void);

/* cfg functions */
static cmd_export_t cmds[] = {
	{"msg_apply_changes",    (cmd_function)msg_apply_changes_f,     0,
		0, REQUEST_ROUTE },
	{"change_reply_status",	 change_reply_status_f,	                2,
		change_reply_status_fixup, ONREPLY_ROUTE },
	{"remove_body",          (cmd_function)w_remove_body_f,         0,
		0, ANY_ROUTE },
	{"keep_hf",              (cmd_function)w_keep_hf_f,             1,
		fixup_regexp_null, ANY_ROUTE },
	{"fnmatch",              (cmd_function)w_fnmatch2_f,            2,
		fixup_fnmatch, ANY_ROUTE },
	{"fnmatch",              (cmd_function)w_fnmatch3_f,            3,
		fixup_fnmatch, ANY_ROUTE },
	{"bind_textopsx",        (cmd_function)bind_textopsx,           1,
		0, ANY_ROUTE },


	{0,0,0,0,0}
};

/* module exports structure */
struct module_exports exports= {
	"textopsx",
	cmds, /* cfg functions */
	0, /* RPC methods */
	0, /* cfg parameters */
	mod_init, /* initialization function */
	0, /* response function */
	0, /* destroy function */
	0, /* on_cancel function */
	0  /* per-child init function */
};


/**
 * init module function
 */
static int mod_init(void)
{
#ifdef USE_TCP
	tcp_set_clone_rcvbuf(1);
#endif
	return 0;
}

/**
 *
 */
static int msg_apply_changes_f(sip_msg_t *msg, char *str1, char *str2)
{
	struct dest_info dst;
	str obuf;
	sip_msg_t tmp;

	if(get_route_type()!=REQUEST_ROUTE)
	{
		LM_ERR("invalid usage - not in request route\n");
		return -1;
	}

	init_dest_info(&dst);
	dst.proto = PROTO_UDP;
	obuf.s = build_req_buf_from_sip_req(msg,
			(unsigned int*)&obuf.len, &dst,
			BUILD_NO_LOCAL_VIA|BUILD_NO_VIA1_UPDATE);
	if(obuf.s == NULL)
	{
		LM_ERR("couldn't update msg buffer content\n");
		return -1;
	}
	if(obuf.len>=BUF_SIZE)
	{
		LM_ERR("new buffer overflow (%d)\n", obuf.len);
		pkg_free(obuf.s);
		return -1;
	}
	/* temporary copy */
	memcpy(&tmp, msg, sizeof(sip_msg_t));

	/* reset dst uri and path vector to avoid freeing - restored later */
	if(msg->dst_uri.s!=NULL)
	{
		msg->dst_uri.s = NULL;
		msg->dst_uri.len = 0;
	}
	if(msg->path_vec.s!=NULL)
	{
		msg->path_vec.s = NULL;
		msg->path_vec.len = 0;
	}

	/* free old msg structure */
	free_sip_msg(msg);
	memset(msg, 0, sizeof(sip_msg_t));

	/* restore msg fields */
	msg->buf                = tmp.buf;
	msg->id                 = tmp.id;
	msg->rcv                = tmp.rcv;
	msg->set_global_address = tmp.set_global_address;
	msg->set_global_port    = tmp.set_global_port;
	msg->flags              = tmp.flags;
	msg->msg_flags          = tmp.msg_flags;
	msg->hash_index         = tmp.hash_index;
	msg->force_send_socket  = tmp.force_send_socket;
	msg->fwd_send_flags     = tmp.fwd_send_flags;
	msg->rpl_send_flags     = tmp.rpl_send_flags;
	msg->dst_uri            = tmp.dst_uri;
	msg->path_vec           = tmp.path_vec;

	memcpy(msg->buf, obuf.s, obuf.len);
	msg->len = obuf.len;
	msg->buf[msg->len] = '\0';

	/* free new buffer - copied in the static buffer from old sip_msg_t */
	pkg_free(obuf.s);

	/* reparse the message */
	LM_DBG("SIP Request content updated - reparsing\n");
	if (parse_msg(msg->buf, msg->len, msg)!=0){
		LM_ERR("parse_msg failed\n");
		return -1;
	}

	return 1;
}


/**
 *
 */
static int change_reply_status_fixup(void** param, int param_no)
{
	if (param_no == 1) {
		return fixup_var_int_12(param, param_no);
	} else if (param_no == 2)
		return fixup_var_pve_str_12(param, param_no);
	else
		return 0;
}

/**
 *
 */
static int change_reply_status_f(struct sip_msg* msg, char* _code, char* _reason)
{
	int	code;
	str	reason;
	struct lump	*l;
	char	*ch;

	if (get_int_fparam(&code, msg, (fparam_t*)_code)
		|| get_str_fparam(&reason, msg, (fparam_t*)_reason)
		|| (reason.len == 0)
	) {
		LOG(L_ERR, "ERROR: textops: cannot get parameter\n");
		return -1;
	}

	if ((code < 100) || (code > 699)) {
		LOG(L_ERR, "ERROR: textops: wrong status code: %d\n",
				code);
		return -1;
	}

	if (((code < 300) || (msg->REPLY_STATUS < 300))
		&& (code/100 != msg->REPLY_STATUS/100)
	) {
		LOG(L_ERR, "ERROR: textops: the class of provisional or "
			"positive final replies cannot be changed\n");
		return -1;
	}

	/* rewrite the status code directly in the message buffer */
	msg->first_line.u.reply.statuscode = code;
	msg->first_line.u.reply.status.s[2] = code % 10 + '0'; code /= 10;
	msg->first_line.u.reply.status.s[1] = code % 10 + '0'; code /= 10;
	msg->first_line.u.reply.status.s[0] = code + '0';

	l = del_lump(msg,
		msg->first_line.u.reply.reason.s - msg->buf,
		msg->first_line.u.reply.reason.len,
		0);
	if (!l) {
		LOG(L_ERR, "ERROR: textops(): Failed to add del lump\n");
		return -1;
	}
	/* clone the reason phrase, the lumps need to be pkg allocated */
	ch = (char *)pkg_malloc(reason.len);
	if (!ch) {
		LOG(L_ERR, "ERROR: textops: Not enough memory\n");
		return -1;
	}
	memcpy(ch, reason.s, reason.len);
	if (insert_new_lump_after(l, ch, reason.len, 0)==0){
		LOG(L_ERR, "ERROR: textops: failed to add new lump: %.*s\n",
			reason.len, ch);
		pkg_free(ch);
		return -1;
	}

	return 1;
}


/**
 *
 */
static int w_remove_body_f(struct sip_msg *msg, char *p1, char *p2)
{
	str body = {0,0};

	body.len = 0;
	body.s = get_body(msg);
	if (body.s==0)
	{
		LM_DBG("no body in the message\n");
		return 1;
	}
	body.len = msg->buf + msg->len - body.s;
	if (body.len<=0)
	{
		LM_DBG("empty body in the message\n");
		return 1;
	}
	if(del_lump(msg, body.s - msg->buf, body.len, 0) == 0)
	{
		LM_ERR("cannot remove body\n");
		return -1;
	}
	return 1;
}


/**
 *
 */
static int w_keep_hf_f(struct sip_msg* msg, char* key, char* foo)
{
	struct hdr_field *hf;
	regex_t *re;
	regmatch_t pmatch;
	char c;
	struct lump* l;

	re = (regex_t*)key;

	/* we need to be sure we have seen all HFs */
	parse_headers(msg, HDR_EOH_F, 0);
	for (hf=msg->headers; hf; hf=hf->next)
	{
		switch(hf->type) {
			case HDR_FROM_T:
			case HDR_TO_T:
			case HDR_CALLID_T:
			case HDR_CSEQ_T:
			case HDR_VIA_T:
			case HDR_VIA2_T:
			case HDR_CONTACT_T:
			case HDR_CONTENTLENGTH_T:
			case HDR_CONTENTTYPE_T:
			case HDR_ROUTE_T:
			case HDR_RECORDROUTE_T:
			case HDR_MAXFORWARDS_T:
				continue;
			default:
				;
		}

		c = hf->name.s[hf->name.len];
		hf->name.s[hf->name.len] = '\0';
		if (regexec(re, hf->name.s, 1, &pmatch, 0)!=0)
		{
			/* no match => remove */
			hf->name.s[hf->name.len] = c;
			l=del_lump(msg, hf->name.s-msg->buf, hf->len, 0);
			if (l==0)
			{
				LM_ERR("cannot remove header\n");
				return -1;
			}
		} else {
			hf->name.s[hf->name.len] = c;
		}
	}

	return -1;
}

/**
 *
 */
static int w_fnmatch(str *val, str *match, str *flags)
{
	int i;
	i = 0;
#ifdef FNM_CASEFOLD
	if(flags && (flags->s[0]=='i' || flags->s[0]=='I'))
		i = FNM_CASEFOLD;
#endif
	if(fnmatch(match->s, val->s, i)==0)
		return 0;
	return -1;
}

/**
 *
 */
static int w_fnmatch2_f(sip_msg_t *msg, char *val, char *match)
{
	str sval;
	str smatch;
	if(get_str_fparam(&sval, msg, (fparam_t*)val)<0
			|| get_str_fparam(&smatch, msg, (fparam_t*)match)<0)
	{
		LM_ERR("invalid parameters");
		return -1;
	}
	if(w_fnmatch(&sval, &smatch, NULL)<0)
		return -1;
	return 1;
}

/**
 *
 */
static int w_fnmatch3_f(sip_msg_t *msg, char *val, char *match, char *flags)
{
	str sval;
	str smatch;
	str sflags;
	if(get_str_fparam(&sval, msg, (fparam_t*)val)<0
			|| get_str_fparam(&smatch, msg, (fparam_t*)match)<0
			|| get_str_fparam(&sflags, msg, (fparam_t*)flags)<0)
	{
		LM_ERR("invalid parameters");
		return -1;
	}
	if(w_fnmatch(&sval, &smatch, &sflags)<0)
		return -1;
	return 1;
}

/**
 *
 */
static int fixup_fnmatch(void** param, int param_no)
{
	if (param_no == 1) {
		return fixup_var_pve_12(param, param_no);
	} else if (param_no == 2) {
		return fixup_var_pve_12(param, param_no);
	} else if (param_no == 3) {
		return fixup_var_pve_12(param, param_no);
	} else {
		return 0;
	}

}

/*
 * Function to load the textops api.
 */
static int bind_textopsx(textopsx_api_t *tob){
	if(tob==NULL){
		LM_WARN("textopsx_binds: Cannot load textopsx API into a NULL pointer\n");
		return -1;
	}
	tob->msg_apply_changes = msg_apply_changes_f;
	return 0;
}
