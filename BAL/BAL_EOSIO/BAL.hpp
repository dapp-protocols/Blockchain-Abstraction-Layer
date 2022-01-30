// This file implements various BAL abstractions based on concrete EOSIO primitives
#pragma once

#include <BAL/ID.hpp>
#include <BAL/IO.hpp>
#include <BAL/Table.hpp>
#include <BAL/Declarations.hpp>

#include <eosio/transaction.hpp>
#include <eosio/contract.hpp>
#include <eosio/action.hpp>
#include <eosio/crypto.hpp>
#include <eosio/print.hpp>

namespace BAL {

// Base class for contracts using the BAL (this is the EOSIO version).
class Contract : public eosio::contract {
public:
    using eosio::contract::contract;

    // Get the table defined by the supplied object
    template<typename Table>
    Table getTable(Scope scope) { return Table(ownerAccount(), scope); }
    // Convenience method to get a table scoped by an ID from another table
    template<typename Table, typename Tag>
    Table getTable(BAL::ID<Tag> scope) { return getTable<Table>(scope.value); }
    // Convenience method to get a table scoped by an account
    template<typename Table>
    Table getTable(AccountHandle handle) { return getTable<Table>(static_cast<uint64_t>(handle.value)); }

    // Require the authorization of the provided account in order to execute the contract
    void requireAuthorization(AccountName account) { eosio::require_auth(account); }
    void requireAuthorization(AccountId account) { eosio::require_auth(Name(account)); }

    // Check if the provided account exists in the underlying blockchain
    bool accountExists(AccountName account) { return eosio::is_account(account); }
    bool accountExists(AccountId account) { return eosio::is_account(Name(account)); }

    // Convert an AccountName to an AccountId
    std::optional<AccountId> getAccountId(AccountName name) { return name; }
    // Convert an AccountId to an AccountName
    std::optional<AccountName> getAccountName(AccountId id) { return Name(id); }
    std::optional<AccountName> getAccountName(AccountHandle handle) { return handle; }

    // Get the current transaction ID
    TransactionId currentTransactionId() {
        // Getting the transaction ID is expensive! Cache it. VM RAM is not kept between transactions.
        static std::optional<TransactionId> cache;
        if (cache)
            return *cache;

        size_t transactionSize = eosio::transaction_size();
        char transactionBuffer[transactionSize];

        size_t bytesRead = eosio::read_transaction(transactionBuffer, transactionSize);
        Verify(bytesRead == transactionSize, "Failed to read transaction");

        cache = eosio::sha256(transactionBuffer, bytesRead);
        return *cache;
    }

    // Get the current blockchain time
    Timestamp currentTime() { return eosio::current_time_point(); }

    // Get the name of the account which owns/provides this smart contract
    AccountName ownerAccount() { return get_self(); }

    // Contracts must override these member types with their own lists of tables and actions
    using Tables = Util::TypeList::List<void>;
    using Actions = Util::TypeList::List<void>;
};

// A helper template to abort the contract, combining several data elements into the error message
template<typename T, typename... Ts>
inline void Abort(const T& first, const Ts&... others) {
    Log("Contract aborted:", first, others...);
    eosio::check(false, "Contract aborted");
}

// A helper template to verify a condition, and combine several data elements into the message if the condition fails
template<typename T, typename... Ts>
inline void Verify(bool condition, const T& first, const Ts&... others) {
    if (!condition)
        Log("Condition verification failed:", first, others...);
    eosio::check(condition, "Condition verification failed. See last message for details.");
}

// A helper template to log several data elements
template<typename T, typename... Ts>
inline void Log(T first, Ts... others) {
    eosio::print(first);
    if constexpr (sizeof...(others) > 0) {
        eosio::print(" ");
        Log(others...);
    } else {
        eosio::print("\n");
    }
}

}
