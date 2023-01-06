/*--------------------------------------------------------------------------------------------------
 * ybcFunction.h
 *	  prototypes for ybcFunction.c
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
 * src/include/executor/ybcModifyTable.h
 * src/include/executor/ybcFunction.h
 *
 */

#pragma once

#include "postgres.h"

#include "yb/yql/pggate/ybc_pg_typedefs.h"
#include "yb/yql/pggate/ybc_pggate.h"

extern void YbSetFunctionParam(YBCPgFunction handle, const char *name,
							   int attr_typid, uint64_t datum, bool is_null);
extern void YbSetSRFTargets(YBCPgFunction handle, TupleDesc desc);
extern bool YbSRFGetNext(YBCPgFunction handle, uint64_t *values, bool *is_nulls);
extern bool YbExecuteScalar(YBCPgFunction func, uint64_t datum, bool *isnull);
