#pragma once

#include <BAL/Name.hpp>

#include <eosio/name.hpp>
#include <eosio/system.hpp>

namespace BAL {

using AccountName = Name;
using AccountId = Name::raw;
using AccountHandle = AccountName;
using Timestamp = eosio::time_point_sec;
using UInt128 = unsigned __int128;
using UInt256 = eosio::fixed_bytes<32>;
using Scope = uint64_t;

using TransactionId = eosio::checksum256;

}
