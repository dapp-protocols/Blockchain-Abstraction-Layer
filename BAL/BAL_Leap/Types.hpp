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

// Specialization for std::numeric_limits as suggested by
// https://www.codeproject.com/tips/737853/cplusplus-numeric-limits-and-template-specializati
namespace std {
    template<> class numeric_limits<BAL::AccountHandle> {
    public:
        static BAL::AccountHandle min() {
            return BAL::AccountHandle(std::numeric_limits<uint64_t>::min());
        };
        static BAL::AccountHandle max() {
            return BAL::AccountHandle(std::numeric_limits<uint64_t>::max());
        };
    };
}
