/*--------------------------------------------------------------------------------------------------
*
* yb_lockfuncs.c
*	  Functions for SQL access to YugabyteDB locking primitives
*
* Copyright (c) YugaByte, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
* in compliance with the License.  You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software distributed under the License
* is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
* or implied.  See the License for the specific language governing permissions and limitations
* under the License.
*
* IDENTIFICATION
*		src/backend/utils/adt/yb_lockfuncs.c
*
*--------------------------------------------------------------------------------------------------
*/

#include "postgres.h"

#include "access/htup_details.h"
#include "access/xact.h"
#include "catalog/pg_type.h"
#include "funcapi.h"
#include "miscadmin.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/date.h"
#include "utils/uuid.h"

/* Working status for yb_lock_status */
typedef struct
{
	YBCLockData   *lockData;
	int			  currIdx;
} YB_Lock_Status;

/* Number of columns in yb_lock_status output */
#define YB_NUM_LOCK_STATUS_COLUMNS		18

/*
 * yb_lock_status - produce a view with one row per held or awaited lock
 */
Datum
yb_lock_status(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	YB_Lock_Status	*mystatus;
	YBCLockData		*lockData;
	YBCPgOid         relation = InvalidOid;
	YBCPgUuid 	    *transaction_id = NULL;

	/*
	 *  If this is not a superuser, do not return actual user data.
	 *  TODO: Remove this as soon as we mask out user data.
	 */
	if (!superuser_arg(GetUserId()) || !IsYbDbAdminUser(GetUserId()))
	{
		ereport(ERROR, (errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
						errmsg("permission denied: user must must be a "
							   "superuser or a member of the yb_db_admin role "
							   "to view lock status")));
	}

	if (!PG_ARGISNULL(0))
	{
		relation = PG_GETARG_OID(0);
	}

	if (!PG_ARGISNULL(1))
	{
		transaction_id = (YBCPgUuid *) PG_GETARG_UUID_P(1);
	}

	if (SRF_IS_FIRSTCALL())
	{
		TupleDesc	tupdesc;
		MemoryContext oldcontext;

		/* create a function context for cross-call persistence */
		funcctx = SRF_FIRSTCALL_INIT();

		/*
		 * switch to memory context appropriate for multiple function calls
		 */
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/* build tupdesc for result tuples */
		/* this had better match function's declaration in pg_proc.h */
		tupdesc = CreateTemplateTupleDesc(YB_NUM_LOCK_STATUS_COLUMNS, false);
		TupleDescInitEntry(tupdesc, (AttrNumber) 1, "locktype",
						   TEXTOID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 2, "database",
						   OIDOID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 3, "relation",
						   OIDOID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 4, "pid",
						   INT4OID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 5, "mode",
						   TEXTARRAYOID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 6, "granted",
						   BOOLOID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 7, "fastpath",
						   BOOLOID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 8, "waitstart",
						   TIMESTAMPTZOID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 9, "waitend",
						   TIMESTAMPTZOID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 10, "node",
						   TEXTOID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 11, "tablet_id",
						   TEXTOID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 12, "transaction_id",
						   TEXTOID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 13, "subtransaction_id",
						   INT4OID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 14, "is_explicit",
						   BOOLOID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 15, "hash_cols",
						   TEXTARRAYOID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 16, "range_cols",
						   TEXTARRAYOID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 17, "column_id",
						   INT4OID, -1, 0);
		TupleDescInitEntry(tupdesc, (AttrNumber) 18, "is_full_pk",
						   BOOLOID, -1, 0);

		funcctx->tuple_desc = BlessTupleDesc(tupdesc);

		mystatus = (YB_Lock_Status *) palloc(sizeof(YB_Lock_Status));
		funcctx->user_fctx = (void *) mystatus;

		HandleYBStatus(YBCGetLockStatusData(MyDatabaseId, relation, transaction_id, &lockData));

		mystatus->lockData = lockData;
		mystatus->currIdx = 0;

		MemoryContextSwitchTo(oldcontext);
	}

	funcctx = SRF_PERCALL_SETUP();
	mystatus = (YB_Lock_Status *) funcctx->user_fctx;
	lockData = mystatus->lockData;

	while (mystatus->currIdx < lockData->nelements)
	{
		Datum		values[YB_NUM_LOCK_STATUS_COLUMNS];
		bool		nulls[YB_NUM_LOCK_STATUS_COLUMNS];
		HeapTuple	tuple;
		Datum		result;
		YBCLockInstanceData *instance;

		instance = &(lockData->locks[mystatus->currIdx]);

		MemSet(values, 0, sizeof(values));
		MemSet(nulls, false, sizeof(nulls));

		if(instance->num_hash_cols == 0 && instance->num_range_cols == 0 && instance->column_id == 0)
			values[0] = CStringGetTextDatum("relation");
		else if(instance->column_id != 0)
			values[0] = CStringGetTextDatum("column");
		else if(instance->is_full_pk)
			values[0] = CStringGetTextDatum("key");
		else
			values[0] = CStringGetTextDatum("keyrange");


		if(instance->database != InvalidOid)
			values[1] = ObjectIdGetDatum(instance->database);
		else
			nulls[1] = true;

		if(instance->relation != InvalidOid)
			values[2] = ObjectIdGetDatum(instance->relation);
		else
			nulls[2] = true;

		nulls[3] = true;

		if (instance->num_mode_cols > 0)
		{
			Datum *mode_cols =
				palloc(instance->num_mode_cols * sizeof(Datum));
			for (int j = 0; j < instance->num_mode_cols; j++)
			{
				mode_cols[j] = CStringGetTextDatum(instance->mode_cols[j]);
			};

			values[4] = PointerGetDatum(construct_array(
				mode_cols, instance->num_mode_cols, TEXTOID, -1, false, 'i'));
		}
		else
			nulls[4] = true;

		values[5] = BoolGetDatum(instance->granted);
		values[6] = BoolGetDatum(instance->single_shard);

		if(instance->wait_start)
		{
			*instance->wait_start = (TimestampTz) *instance->wait_start -
					   ((POSTGRES_EPOCH_JDATE - UNIX_EPOCH_JDATE) * USECS_PER_DAY);

			values[7] = TimestampTzGetDatum(*instance->wait_start);
		}
		else
			nulls[7] = true;

		if(instance->wait_end)
		{
			*instance->wait_end = (TimestampTz) *instance->wait_end -
					   ((POSTGRES_EPOCH_JDATE - UNIX_EPOCH_JDATE) * USECS_PER_DAY);

			values[8] = TimestampTzGetDatum(*instance->wait_end);
		}
		else
			nulls[8] = true;

		if(instance->node)
			values[9] = CStringGetTextDatum(instance->node);
		else
			nulls[9] = true;

		if(instance->tablet_id)
			values[10] = CStringGetTextDatum(instance->tablet_id);
		else
			nulls[10] = true;

		if(instance->transaction_id)
			values[11] = CStringGetTextDatum(instance->transaction_id);
		else
			nulls[11] = true;

		values[12] = UInt32GetDatum(instance->subtransaction_id);
		values[13] = BoolGetDatum(instance->is_explicit);

       // TODO: if this is not a superuser, do not return actual user data
		if(instance->num_hash_cols > 0)
		{
			Datum *hash_cols = palloc(instance->num_hash_cols * sizeof(Datum));
			for (int j = 0; j < instance->num_hash_cols; j++)
			{
				hash_cols[j] = CStringGetTextDatum(instance->hash_cols[j]);
			};

			values[14] = PointerGetDatum(construct_array(
				hash_cols, instance->num_hash_cols, TEXTOID, -1, false, 'i'));
		}
		else
			nulls[14] = true;

       // TODO: if this is not a superuser, do not return actual user data
		if (instance->num_range_cols > 0)
		{
			Datum *range_cols =
				palloc(instance->num_range_cols * sizeof(Datum));
			for (int j = 0; j < instance->num_range_cols; j++)
			{
				range_cols[j] = CStringGetTextDatum(instance->range_cols[j]);
			};

			values[15] = PointerGetDatum(construct_array(
				range_cols, instance->num_range_cols, TEXTOID, -1, false, 'i'));
		}
		else
			nulls[15] = true;

		if (instance->column_id != 0)
		{
			values[16] = UInt32GetDatum(instance->column_id);
		}
		else
		{
			nulls[16] = true;
		}

		values[17] = instance->is_full_pk;

		tuple = heap_form_tuple(funcctx->tuple_desc, values, nulls);
		result = HeapTupleGetDatum(tuple);

		mystatus->currIdx++;

		SRF_RETURN_NEXT(funcctx, result);
	}

	SRF_RETURN_DONE(funcctx);
}