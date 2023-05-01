//--------------------------------------------------------------------------------------------------
// Copyright (c) YugaByte, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
// in compliance with the License.  You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
// or implied.  See the License for the specific language governing permissions and limitations
// under the License.
//
//--------------------------------------------------------------------------------------------------

#include "yb/client/yb_op.h"

#include "yb/yql/pggate/pg_function.h"
#include "yb/yql/pggate/util/pg_function_helpers.h"

namespace yb {
namespace pggate {

using util::GetValue;
using util::SetColumnValue;
//--------------------------------------------------------------------------------------------------
// PgFunctionParams
//--------------------------------------------------------------------------------------------------

Status PgFunctionParams::AddParam(
    const std::string& name, const YBCPgTypeEntity* type_entity, uint64_t datum, bool is_null) {
  auto value = std::make_shared<QLValuePB>();
  RETURN_NOT_OK(PgValueToPB(type_entity, datum, is_null, value.get()));
  params_by_name_.emplace(name, std::make_tuple(value, type_entity));
  params_by_attr_.push_back(std::make_tuple(value, type_entity));
  return Status::OK();
}

template <class T>
Result<T> PgFunctionParams::GetColumn(const std::string& col_name, bool& is_null) const {
  auto v = VERIFY_RESULT(GetValueAndType(col_name));
  return GetValue<T>(*v.first, v.second, is_null);
}

template <class T>
Result<T> PgFunctionParams::GetColumn(const uint16_t attr_num, bool& is_null) const {
  auto v = VERIFY_RESULT(GetValueAndType(attr_num));
  return GetValue<T>(*v.first, v.second, is_null);
}

template <class T>
Result<T> PgFunctionParams::GetNotNull(const std::string& col_name) const {
  bool is_null = false;
  auto v = VERIFY_RESULT(GetColumn<T>(col_name, is_null));
  if (is_null) return STATUS_FORMAT(InvalidArgument, "parameter is null", col_name);

  return v;
}

template <class T>
Status PgFunctionParams::GetNotNull(const uint16_t attr_num) const {
  bool is_null = false;
  auto v = VERIFY_RESULT(GetColumn<T>(attr_num, is_null));
  if (is_null) return STATUS_FORMAT(InvalidArgument, "parameter $1 is null", attr_num);

  return v;
}

Result<std::pair<std::shared_ptr<const QLValuePB>, const YBCPgTypeEntity*>>
PgFunctionParams::GetValueAndType(const uint16_t attr_num) const {
  if (attr_num >= params_by_attr_.size()) {
    return STATUS_FORMAT(InvalidArgument, "Attribute number out of range: $0", attr_num);
  }
  return params_by_attr_[attr_num];
}

Result<std::pair<std::shared_ptr<const QLValuePB>, const YBCPgTypeEntity*>>
PgFunctionParams::GetValueAndType(const std::string& name) const {
  auto it = params_by_name_.find(name);
  if (it == params_by_name_.end()) {
    return STATUS_FORMAT(InvalidArgument, "Attribute name not found: $0", name);
  }
  return it->second;
}

//--------------------------------------------------------------------------------------------------
// PgFunction
//--------------------------------------------------------------------------------------------------

Status PgFunction::AddParam(
    const std::string name, const YBCPgTypeEntity* type_entity, uint64_t datum, bool is_null) {
  return params_.AddParam(name, type_entity, datum, is_null);
}

Status PgFunction::AddTarget(
    const std::string name, const YBCPgTypeEntity* type_entity, const YBCPgTypeAttrs type_attrs) {
  RETURN_NOT_OK(schema_builder_.AddColumn(name, DataType(type_entity->yb_type)));
  return schema_builder_.SetColumnPGType(name, type_entity->type_oid);
  return schema_builder_.SetColumnPGTypmod(name, type_attrs.typmod);
}

Status PgFunction::FinalizeTargets() {
  schema_ = schema_builder_.Build();

  return Status::OK();
}

Status PgFunction::WritePgTuple(const QLTableRow& table_row, uint64_t* values, bool* is_nulls) {
  // Set the column values (used to resolve scan variables in the expression).
  for (uint32_t attno = 0; attno < schema_.num_columns(); attno++) {
    const QLValuePB* val = table_row.GetColumn(attno);
    if (!val) continue;

    ColumnSchema column = schema_.column(attno);

    uint32_t oid = column.pg_type_oid();
    const PgTypeEntity* type_entity = YBCPgFindTypeEntity(oid);
    const YBCPgTypeAttrs type_attrs = {.typmod = column.pg_typmod()};

    RETURN_NOT_OK(PgValueFromPB(type_entity, type_attrs, *val, &values[attno], &is_nulls[attno]));
  }

  return Status::OK();
}

Status PgFunction::GetNext(uint64_t* values, bool* is_nulls, bool* has_data) {
  if (!executed_) {
    executed_ = true;
    data_ = VERIFY_RESULT(processor_(params_, schema_, pg_session_));
    current_ = data_.begin();
  }

  if (is_nulls) {
    int64_t natts = schema_.num_columns();
    memset(is_nulls, true, natts * sizeof(bool));
  }

  if (current_ == data_.end()) {
    *has_data = false;
  } else {
    const QLTableRow& row = *current_;
    ++current_;

    RETURN_NOT_OK(WritePgTuple(row, values, is_nulls));
    *has_data = true;
  }

  return Status::OK();
}

//--------------------------------------------------------------------------------------------------
// PgLockStatusRequestor
//--------------------------------------------------------------------------------------------------

Result<std::list<QLTableRow>> PgLockStatusRequestor(
    const PgFunctionParams& params, const Schema& schema,
    const scoped_refptr<PgSession> pg_session) {
  std::string table_id;
  bool is_null;
  PgOid relation = VERIFY_RESULT(params.GetColumn<PgOid>("relation", is_null));
  if (!is_null) {
    PgOid database = VERIFY_RESULT(params.GetNotNull<PgOid>("database"));

    table_id = relation != kInvalidOid ? GetPgsqlTableId(database, relation) : "";
  }

  Uuid transaction = VERIFY_RESULT(params.GetColumn<Uuid>("transaction_id", is_null));

  const auto lock_status =
      VERIFY_RESULT(pg_session->GetLockStatusData(table_id, transaction.AsSlice().ToBuffer()));

  std::list<QLTableRow> data;

  for (auto& node : lock_status.node_locks()) {
    for (auto tab : node.tablet_lock_infos()) {
      for (auto& l : tab.locks()) {
        auto& row = data.emplace_back();
        std::string locktype;
        bool multiple_rows_locked = false;
        if (l.hash_cols_size() == 0 && l.range_cols_size() == 0 && !l.has_column_id()) {
          locktype = "relation";
          multiple_rows_locked = true;
        } else if (l.column_id() != 0)
          locktype = "column";
        else if (l.is_full_pk())
          locktype = "row";
        else {
          locktype = "keyrange";
          multiple_rows_locked = true;
        }

        RETURN_NOT_OK(SetColumnValue("locktype", locktype, schema, &row));

        PgOid database_oid = VERIFY_RESULT(GetPgsqlDatabaseOidByTableId(tab.table_id()));
        RETURN_NOT_OK(SetColumnValue("database", database_oid, schema, &row));

        PgOid relation_oid = VERIFY_RESULT(GetPgsqlTableOid(tab.table_id()));
        RETURN_NOT_OK(SetColumnValue("relation", relation_oid, schema, &row));

        // TODO: how to associate the pid?
        // RETURN_NOT_OK(SetColumnValue("pid", YBCGetPid(l.transaction_id()), schema, &row));

        std::vector<std::string> modes(l.modes().size());

        std::transform(l.modes().begin(), l.modes().end(), modes.begin(), [](const auto& mode) {
          return LockMode_Name(static_cast<LockMode>(mode));
        });
        if (modes.size() > 0) RETURN_NOT_OK(SetColumnValue("mode", modes, schema, &row));

        RETURN_NOT_OK(SetColumnValue("granted", l.has_wait_end_ht() ? true : false, schema, &row));
        RETURN_NOT_OK(
            SetColumnValue("fastpath", l.has_transaction_id() ? false : true, schema, &row));

        if (l.has_wait_start_ht())
          RETURN_NOT_OK(SetColumnValue(
              "waitstart", HybridTime(l.wait_start_ht()).GetPhysicalValueMicros(), schema, &row));

        if (l.has_wait_end_ht())
          RETURN_NOT_OK(SetColumnValue(
              "waitend", HybridTime(l.wait_end_ht()).GetPhysicalValueMicros(), schema, &row));

        // TODO: this should be the node of the backend holding the lock, not the node where the
        //       lock is held.
        RETURN_NOT_OK(SetColumnValue("node", node.permanent_uuid(), schema, &row));
        RETURN_NOT_OK(SetColumnValue("tablet_id", tab.tablet_id(), schema, &row));
        RETURN_NOT_OK(SetColumnValue(
            "transaction_id", VERIFY_RESULT(Uuid::FromString(l.transaction_id())), schema, &row));
        RETURN_NOT_OK(SetColumnValue("subtransaction_id", l.subtransaction_id(), schema, &row));

        // TODO: Add this when the RPC returns the status_tablet_id
        //RETURN_NOT_OK(SetColumnValue("status_tablet_id", l.status_tablet_id(), schema, &row));

        RETURN_NOT_OK(SetColumnValue("is_explicit", l.is_explicit(), schema, &row));

        if (l.hash_cols().size() > 0)
          RETURN_NOT_OK(SetColumnValue("hash_cols", l.hash_cols(), schema, &row));
        if (l.range_cols().size() > 0)
          RETURN_NOT_OK(SetColumnValue("range_cols", l.range_cols(), schema, &row));
        if (l.has_column_id())
          RETURN_NOT_OK(SetColumnValue("column_id", l.column_id(), schema, &row));
        RETURN_NOT_OK(SetColumnValue("is_full_pk", l.is_full_pk(), schema, &row));
        RETURN_NOT_OK(SetColumnValue("multiple_rows_locked", multiple_rows_locked, schema, &row));

        // TODO: Add this when the field has been added to the protobuf
        // RETURN_NOT_OK(SetColumnValue("blocked_by", l.blocked_by(), schema, &row));
      }
    }
  }

  return data;
}

}  // namespace pggate
}  // namespace yb
