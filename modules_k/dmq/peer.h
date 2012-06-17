#ifndef PEER_H
#define PEER_H

#include <string.h>
#include <stdlib.h>
#include "../../lock_ops.h"
#include "../../str.h"
#include "../../mem/mem.h"
#include "../../mem/shm_mem.h"
#include "../../parser/msg_parser.h"

typedef struct peer_response {
	int resp_code;
	str content_type;
	str reason;
	str body;
} peer_reponse_t;

typedef int(*peer_callback_t)(struct sip_msg*, peer_reponse_t* resp);

typedef struct dmq_peer {
	str peer_id;
	str description;
	peer_callback_t callback;
	struct dmq_peer* next;
} dmq_peer_t;

typedef struct dmq_peer_list {
	gen_lock_t lock;
	dmq_peer_t* peers;
	int count;
} dmq_peer_list_t;

extern dmq_peer_list_t* peer_list;

dmq_peer_list_t* init_peer_list();
dmq_peer_t* search_peer_list(dmq_peer_list_t* peer_list, dmq_peer_t* peer);
typedef dmq_peer_t* (*register_dmq_peer_t)(dmq_peer_t*);

dmq_peer_t* add_peer(dmq_peer_list_t* peer_list, dmq_peer_t* peer);
dmq_peer_t* find_peer(str peer_id);
int empty_peer_callback(struct sip_msg* msg, peer_reponse_t* resp);

#endif

