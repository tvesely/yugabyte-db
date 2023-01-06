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
//--------------------------------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "yb/common/schema.h"

#include "yb/qlexpr/ql_expr.h"

#include "yb/util/result.h"
#include "yb/util/status_fwd.h"

#include "yb/yql/pggate/pg_memctx.h"
#include "yb/yql/pggate/pg_session.h"
#include "yb/yql/pggate/pg_value.h"
#include "yb/yql/pggate/ybc_pg_typedefs.h"

namespace yb {
namespace pggate {

using qlexpr::QLTableRow;

class PgFunctionParams {
 public:
  Status AddParam(
      const std::string& name, const YBCPgTypeEntity* type_entity, uint64_t datum, bool is_null);

  size_t Size() const { return params_by_attr_.size(); }

  template <class T>
  Result<T> GetColumn(const std::string& col_name, bool& is_null) const;

  template <class T>
  Result<T> GetColumn(const uint16_t attr_num, bool& is_null) const;

  template <class T>
  Result<T> GetNotNull(const std::string& col_name) const;

  template <class T>
  Status GetNotNull(const uint16_t attr_num) const;

 private:
  Result<std::pair<std::shared_ptr<const QLValuePB>, const YBCPgTypeEntity*>> GetValueAndType(
      const uint16_t attr_num) const;

  Result<std::pair<std::shared_ptr<const QLValuePB>, const YBCPgTypeEntity*>> GetValueAndType(
      const std::string& name) const;

  std::vector<std::pair<std::shared_ptr<const QLValuePB>, const YBCPgTypeEntity*>> params_by_attr_;
  std::unordered_map<
      std::string, std::pair<std::shared_ptr<const QLValuePB>, const YBCPgTypeEntity*>>
      params_by_name_;
};

using PgFunctionDataProcessor = std::function<Result<std::list<QLTableRow>>(
    const PgFunctionParams&, const Schema&, const scoped_refptr<PgSession>&)>;

class PgFunction : public PgMemctx::Registrable {
 public:
  explicit PgFunction(PgFunctionDataProcessor processor, scoped_refptr<PgSession> pg_session)
      : schema_builder_(),
        pg_session_(pg_session),
        processor_(std::move(processor)) {}

  virtual ~PgFunction() = default;

  Status AddParam(
      const std::string name, const YBCPgTypeEntity* type_entity, uint64_t datum, bool is_null);

  Status AddTarget(
      const std::string name, const YBCPgTypeEntity* type_entity, const YBCPgTypeAttrs type_attrs);

  Status FinalizeTargets();

  Status GetNext(uint64_t* values, bool* is_nulls, bool* has_data);

 private:
  Status WritePgTuple(const QLTableRow& table_row, uint64_t* values, bool* is_nulls);

  PgFunctionParams params_;
  SchemaBuilder schema_builder_;
  Schema schema_;

  scoped_refptr<PgSession> pg_session_;
  PgFunctionDataProcessor processor_;

  bool executed_ = false;
  std::list<QLTableRow> data_;
  std::list<QLTableRow>::const_iterator current_;
};

Result<std::list<QLTableRow>> PgLockStatusRequestor(
    const PgFunctionParams& params, const Schema& schema,
    const scoped_refptr<PgSession> pg_session);

}  // namespace pggate
}  // namespace yb
