/* sp-ul_db module
 *
 * Copyright (C) 2007 1&1 Internet AG
 *
 * This file is part of openser, a free SIP server.
 *
 * openser is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * openser is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef SP_P_USRLOC_UL_DB_FORM_QUERY_H
#define SP_P_USRLOC_UL_DB_FORM_QUERY_H

#include "../../lib/srdb1/db.h"
#include "ul_db_handle.h"

typedef enum {
	UL_DB_INS,
	UL_DB_REPL,
	UL_DB_INS_UPD,
	UL_DB_UPD,
	UL_DB_DEL,
}ul_db_op_t;

int db_submit_query(ul_db_op_t ul_op,
					ul_db_handle_t * handle, 
					str * table, 
					db_key_t* _k, 
					db_op_t* _o,
					db_val_t* _v, 
					db_key_t* _uk, 
					db_val_t* _uv, 
					int _n, 
					int _un
				   );

#endif
