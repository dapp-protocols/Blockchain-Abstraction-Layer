#pragma once

#include <BAL/Name.hpp>

#include <graphene/chain/types.hpp>

#include <boost/multiprecision/cpp_int.hpp>

#include <variant>

namespace BAL {

using AccountName = std::string;
using AccountId = uint64_t;
using AccountHandle = AccountId;
using Timestamp = fc::time_point_sec;
using UInt128 = boost::multiprecision::checked_uint128_t;
using UInt256 = boost::multiprecision::checked_uint256_t;
using Scope = uint64_t;

using TransactionId = graphene::protocol::transaction_id_type;

}
