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

#include <boost/container/small_vector.hpp>

#include "yb/common/ql_value.h"
#include "yb/common/schema.h"

#include "yb/yql/pggate/util/pg_function_helpers.h"
#include "yb/yql/pggate/ybc_pggate.h"

#include "yb/util/net/net_util.h"
#include "yb/util/yb_partition.h"

namespace yb {
namespace pggate {
namespace util {

Result<QLValuePB> SetValueHelper<std::string>::Apply(
    const std::string& strval, const DataType data_type) {
  QLValuePB value_pb;
  switch (data_type) {
    case STRING:
      value_pb.set_string_value(strval);
      break;
    case BINARY:
      value_pb.set_binary_value(strval);
      break;
    default:
      return STATUS_FORMAT(InvalidArgument, "unexpected string type $1", data_type);
  }
  return value_pb;
}

Result<QLValuePB> SetValueHelper<int32_t>::Apply(const int32_t intval, const DataType data_type) {
  QLValuePB value_pb;
  switch (data_type) {
    case INT64:
      value_pb.set_int64_value(intval);
      break;
    case INT32:
      value_pb.set_int32_value(intval);
      break;
    case INT16:
      if (intval < std::numeric_limits<int16_t>::min() ||
          intval > std::numeric_limits<int16_t>::max()) {
        return STATUS(InvalidArgument, "overflow or underflow in conversion to int16_t");
      }
      value_pb.set_int16_value(intval);
      break;
    case INT8:
      if (intval < std::numeric_limits<int8_t>::min() ||
          intval > std::numeric_limits<int8_t>::max()) {
        return STATUS(InvalidArgument, "overflow or underflow in conversion to int8_t");
      }
      value_pb.set_int8_value(intval);
      break;
    case UINT64:
      if (intval < 0) {
        return STATUS(InvalidArgument, "underflow in conversion to uint64_t");
      }
      value_pb.set_uint64_value(static_cast<uint64_t>(intval));
      break;
    case UINT32:
      if (intval < 0) {
        return STATUS(InvalidArgument, "underflow in conversion to uint32_t");
      }
      value_pb.set_uint32_value(static_cast<uint32_t>(intval));
      break;
    default:
      return STATUS_FORMAT(InvalidArgument, "unexpected int type $1", data_type);
  }
  return value_pb;
}

Result<QLValuePB> SetValueHelper<uint32_t>::Apply(const uint32_t intval, const DataType data_type) {
  QLValuePB value_pb;
  switch (data_type) {
    case INT64:
      value_pb.set_int64_value(intval);
      break;
    case INT32:
      if (intval > std::numeric_limits<int32_t>::max()) {
        return STATUS(InvalidArgument, "overflow in conversion to int32_t");
      }
      value_pb.set_int32_value(intval);
      break;
    case INT16:
      if (intval > std::numeric_limits<int16_t>::max()) {
        return STATUS(InvalidArgument, "overflow in conversion to int16_t");
      }
      value_pb.set_int16_value(intval);
      break;
    case INT8:
      if (intval > std::numeric_limits<int8_t>::max()) {
        return STATUS(InvalidArgument, "overflow in conversion to int8_t");
      }
      value_pb.set_int8_value(intval);
      break;
    case UINT64:
      value_pb.set_uint64_value(intval);
      break;
    case UINT32:
      value_pb.set_uint32_value(intval);
      break;
    default:
      return STATUS_FORMAT(InvalidArgument, "unexpected int type $1", data_type);
  }
  return value_pb;
}

Result<QLValuePB> SetValueHelper<Uuid>::Apply(const Uuid& uuid_val, const DataType data_type) {
  std::string buffer;
  uuid_val.ToBytes(&buffer);

  QLValuePB result;
  result.set_binary_value(buffer);
  return result;
}

Result<QLValuePB> SetValueHelper<MicrosTime>::Apply(
    const MicrosTime& time_val, const DataType data_type) {
  QLValuePB value_pb;

  value_pb.set_int64_value(YBCGetPgCallbacks()->UnixEpochToPostgresEpoch(time_val));
  return value_pb;
}

Result<QLValuePB> SetValueHelper<bool>::Apply(const bool bool_val, const DataType data_type) {
  QLValuePB value_pb;
  value_pb.set_bool_value(bool_val);
  return value_pb;
}

template <typename Container>
Result<QLValuePB> ConvertStringArrayToQLValue(const Container& str_vals, const DataType data_type) {
  if (data_type != BINARY) {
    return STATUS_FORMAT(InvalidArgument, "unexpected string array type $1", data_type);
  }
  QLValuePB value_pb;
  size_t size;
  char* value;

  if (str_vals.size() > std::numeric_limits<int>::max()) {
    return STATUS(InvalidArgument, "overflow in conversion to int");
  }

  int count = (int)str_vals.size();
  std::vector<const char*> strings(count);

  for (int i = 0; i < count; ++i) {
    strings[i] = str_vals[i].c_str();
  }

  YBCGetPgCallbacks()->ConstructTextArrayDatum(strings.data(), count, &value, &size);

  value_pb.set_binary_value(value, size);
  return value_pb;
}

Result<QLValuePB> SetValueHelper<std::vector<std::string>>::Apply(
    const std::vector<std::string>& str_vals, const DataType data_type) {
  return ConvertStringArrayToQLValue(str_vals, data_type);
}

Result<QLValuePB> SetValueHelper<google::protobuf::RepeatedPtrField<std::string>>::Apply(
    const google::protobuf::RepeatedPtrField<std::string>& str_vals, const DataType data_type) {
  return ConvertStringArrayToQLValue(str_vals, data_type);
}

Result<uint32_t> GetValueHelper<uint32_t>::Retrieve(
    const QLValuePB& ql_val, const YBCPgTypeEntity* pg_type, bool& is_null) {
  if (pg_type->yb_type != YB_YQL_DATA_TYPE_UINT32) {
    return STATUS_FORMAT(InvalidArgument, "unexpected data type $1", YB_YQL_DATA_TYPE_UINT32);
  }

  if (IsNull(ql_val)) {
    is_null = true;
    return 0;
  }

  return ql_val.uint32_value();
}

Result<Uuid> GetValueHelper<Uuid>::Retrieve(
    const QLValuePB& ql_val, const YBCPgTypeEntity* pg_type, bool& is_null) {
  if (pg_type->yb_type != YB_YQL_DATA_TYPE_BINARY) {
    return STATUS_FORMAT(InvalidArgument, "unexpected data type $1", YB_YQL_DATA_TYPE_BINARY);
  }

  if (IsNull(ql_val)) {
    is_null = true;
    return Uuid::Nil();
  }

  // Postgres stores UUIDs in host byte order, so this should be fine
  return VERIFY_RESULT(Uuid::FullyDecode(Slice(ql_val.binary_value())));
}

Result<std::pair<int, DataType>> ColumnIndexAndType(
    const std::string& col_name, const Schema& schema) {
  auto column_index = schema.find_column(col_name);
  if (column_index == Schema::kColumnNotFound) {
    return STATUS_SUBSTITUTE(NotFound, "Couldn't find column $0 in schema", col_name);
  }
  const DataType data_type = schema.column(column_index).type_info()->type;
  return std::make_pair(column_index, data_type);
}

}  // namespace util
}  // namespace pggate
}  // namespace yb
