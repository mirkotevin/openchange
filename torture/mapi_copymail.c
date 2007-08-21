/* 
   OpenChange MAPI implementation testsuite

   Copy message from a folder to another

   Copyright (C) Julien Kerihuel 2007
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <libmapi/libmapi.h>
#include <gen_ndr/ndr_exchange.h>
#include <param.h>
#include <credentials.h>
#include <torture/mapi_torture.h>
#include <torture/torture.h>
#include <torture/torture_proto.h>
#include <samba/popt.h>

BOOL torture_rpc_mapi_copymail(struct torture_context *torture)
{
	NTSTATUS		status;
	enum MAPISTATUS		retval;
	struct dcerpc_pipe	*p;
	TALLOC_CTX		*mem_ctx;
	struct mapi_session	*session;
	mapi_object_t		obj_store;
	mapi_object_t		obj_dir_src;
	mapi_object_t		obj_dir_dst;
	mapi_object_t		obj_table;
	mapi_id_t		id_src;
	mapi_id_t		id_dst;
	struct SPropTagArray	*SPropTagArray = NULL;
	struct SRowSet		rowset;
	int			i;

	/* init torture */
	mem_ctx = talloc_init("torture_rpc_mapi_copymail");
	status = torture_rpc_connection(mem_ctx, &p, &ndr_table_exchange_emsmdb);
	if (!NT_STATUS_IS_OK(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	/* init mapi */
	if ((session = torture_init_mapi(mem_ctx)) == NULL) return False;

	/* init objects */
	mapi_object_init(&obj_store);
	mapi_object_init(&obj_dir_src);
	mapi_object_init(&obj_dir_dst);
	mapi_object_init(&obj_table);

	/* OpenMsgStore */
	retval = OpenMsgStore(&obj_store);
	mapi_errstr("OpenMsgStore", GetLastError());
	if (retval != MAPI_E_SUCCESS) return False;

	/* Get Inbox folder */
	retval = GetDefaultFolder(&obj_store, &id_src, olFolderInbox);
	mapi_errstr("GetDefaultFolder", GetLastError());
	if (retval != MAPI_E_SUCCESS) return False;

	retval = OpenFolder(&obj_store, id_src, &obj_dir_src);
	mapi_errstr("OpenFolder", GetLastError());
	if (retval != MAPI_E_SUCCESS) return False;

	/* Get Deleted Items folder */
	retval = GetDefaultFolder(&obj_store, &id_dst, olFolderDrafts);
	mapi_errstr("GetDefaultFolder", GetLastError());
	if (retval != MAPI_E_SUCCESS) return False;
	
	retval = OpenFolder(&obj_store, id_dst, &obj_dir_dst);
	mapi_errstr("OpenFolder", GetLastError());
	if (retval != MAPI_E_SUCCESS) return False;


	retval = GetContentsTable(&obj_dir_src, &obj_table);
	mapi_errstr("GetContentsTable", GetLastError());
	if (retval != MAPI_E_SUCCESS) return False;

		SPropTagArray = set_SPropTagArray(mem_ctx, 0x5,
					  PR_FID,
					  PR_MID,
					  PR_INST_ID,
					  PR_INSTANCE_NUM,
					  PR_SUBJECT);
	retval = SetColumns(&obj_table, SPropTagArray);
	MAPIFreeBuffer(SPropTagArray);
	mapi_errstr("SetColumns", GetLastError());
	if (retval != MAPI_E_SUCCESS) return False;

	while ((retval = QueryRows(&obj_table, 0xa, TBL_ADVANCE, &rowset)) != MAPI_E_NOT_FOUND && rowset.cRows) {
		for (i = 0; i < rowset.cRows; i++) {
			retval = CopyMessages(&obj_dir_src, &obj_dir_dst, rowset.aRow[i].lpProps[1].value.d);
			mapi_errstr("CopyMessages", GetLastError());
			if (retval != MAPI_E_SUCCESS) return False;
		}
	}

	/* release mapi objects
	 */
	mapi_object_release(&obj_table);
	mapi_object_release(&obj_dir_src);
	mapi_object_release(&obj_dir_dst);
	mapi_object_release(&obj_store);

	/* uninitialize mapi
	 */
	MAPIUninitialize();
	talloc_free(mem_ctx);

	return True;
}
