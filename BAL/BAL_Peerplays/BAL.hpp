// This file implements various BAL abstractions based on concrete EOSIO primitives
#pragma once

#include <BAL/ID.hpp>
#include <BAL/IO.hpp>
#include <BAL/Table.hpp>
#include <BAL/Dispatcher.hpp>
#include <BAL/Declarations.hpp>

#include <graphene/chain/database.hpp>
#include <graphene/chain/evaluator.hpp>

#include <sstream>

namespace BAL {

// Base class for contracts using the BAL (this is the Peerplays version).
class Contract : public graphene::chain::evaluator<Contract> {
protected:
    static std::string contractName;
    static AccountName contractOwner;
    static graphene::chain::database* blockchain;
    static uint8_t databaseSpaceId;
    static std::unique_ptr<Dispatcher> dispatcher;
    static std::optional<graphene::chain::op_evaluator::evaluator_handle> evaluatorHandle;

    const graphene::chain::custom_operation* actionOperation = nullptr;
    Dispatcher::Storage dispatcherStorage;

public:
    using operation_type = graphene::chain::custom_operation;

    template<class FinalContract>
    static void Initialize(std::string_view contractName, AccountName contractOwner,
                           graphene::chain::database& blockchain, uint8_t databaseSpaceId) {
        namespace TL = Util::TypeList;

        // Store static contract information
        Contract::contractName = contractName;
        Contract::contractOwner = contractOwner;
        Contract::blockchain = &blockchain;
        Contract::databaseSpaceId = databaseSpaceId;

        // Create the dispatcher based on the contract's declared actions
        dispatcher = std::make_unique<ContractDispatcher<FinalContract>>();

        // Register the contract's declared indexes with the blockchain
        using Tables = typename ContractDeclarations<FinalContract>::Tables;
        TL::runtime::ForEach(Tables(), [&blockchain](auto t) {
            using Table = typename decltype(t)::type;
            using GrapheneTable = typename Table::GrapheneTable;
            blockchain.add_index<graphene::chain::primary_index<GrapheneTable>>();
        });

        // Register the contract itself with the blockchain
        auto handle = blockchain.register_evaluator<FinalContract>();
        Verify(handle.valid(), "Registered evaluator with blockchain, but did not get evaluator handle back!");
        evaluatorHandle = std::move(*handle);
    }

    static void Deregister() {
        if (blockchain != nullptr && evaluatorHandle.has_value())
            blockchain->delete_evaluator(std::move(*evaluatorHandle));
    }

    // Get the table defined by the supplied object
    template<typename Table>
    Table getTable(Scope scope) {
        BAL::Verify(blockchain != nullptr, "Asked to get table before contract was initialized!");
        return Table(*blockchain, scope);
    }
    // Convenience method to get a table scoped by an ID from another table
    template<typename Table, typename Tag>
    Table getTable(BAL::ID<Tag> scope) { return getTable<Table>(scope.value); }

    // Require the authorization of the provided account in order to execute the contract
    void requireAuthorization(AccountName account);
    void requireAuthorization(AccountId account);

    // Check if the provided account exists in the underlying blockchain
    bool accountExists(AccountName account);
    bool accountExists(AccountId account);

    // Convert an AccountName to AccountId
    std::optional<AccountId> getAccountId(AccountName name);
    // Convert an AccountId to AccountName
    std::optional<AccountName> getAccountName(AccountId id);

    // Get the current transaction ID
    TransactionId currentTransactionId() { return trx_state->_trx->id(); }

    // Get the current blockchain time
    Timestamp currentTime() {
        Verify(blockchain != nullptr, "Asked to get blockchain time before contract was initialized!");
        return blockchain->head_block_time();
    }

    // Get the name of the account which owns/provides this smart contract
    AccountName ownerAccount() {
        Verify(contractOwner.length() > 0, "Asked to get contract owner account before contract was intialized!");
        return contractOwner;
    }

    // graphene::chain::evaluator interface
    graphene::protocol::void_result do_evaluate(const Contract::operation_type& op) {
        try {
            auto parseResult = dispatcher->parse(contractName, op, dispatcherStorage);
            using Result = Dispatcher::ParseResult;

            switch(parseResult) {
            case Result::ParseOK:
                Log("Parsed an action for contract", contractName, "successfully.");
                break;
            case Result::ParseFailed:
                Log("Parse failed. Operation data was", std::string(op.data.data(), op.data.size()));
                dispatcherStorage.clear();
                break;
            case Result::ExtraData:
                Log("Parse ostensibly succeeded, but extra data leftover. Aborting action. Operation data was",
                    std::string(op.data.data(), op.data.size()));
                dispatcherStorage.clear();
                break;
            case Result::WrongMagic:
                // Not our operation. Just ignore it.
                break;
            }
        } catch (const fc::exception& e) {
            ilog("[${C}] Parsing failed with error: ${E}.\nPayload was: ${D}",
                 ("C", contractName)("E", e.to_detail_string())("D", std::string(op.data.data(), op.data.size())));
            dispatcherStorage.clear();
        }

        return {};
    }
    graphene::protocol::void_result do_apply(const Contract::operation_type& op) {
        if (dispatcherStorage.empty())
            return {};

        try {
            Log("Applying action for contract", contractName);
            actionOperation = &op;

            // I'm not even supposed to have access to this, but I do, so... Set up undo states
            bool undoEnabled = blockchain->_undo_db.enabled();
            if (!undoEnabled)
                blockchain->_undo_db.enable();
            auto undoSession = blockchain->_undo_db.start_undo_session();

            // Run the contract
            dispatcher->dispatch(*this, dispatcherStorage);

            // Clean up undo states
            undoSession.merge();
            if (!undoEnabled)
                blockchain->_undo_db.disable();

            actionOperation = nullptr;
        } catch (const fc::exception& e) {
            ilog("[${C}] Contract rejected operation with error: ${E}.\nPayload was: ${D}",
                 ("C", contractName)("E", e.to_detail_string())("D", std::string(op.data.data(), op.data.size())));
        }

        return {};
    }

    // Contracts must override these member types with their own lists of tables and actions
    using Tables = Util::TypeList::List<void>;
    using Actions = Util::TypeList::List<void>;

    constexpr const static uint8_t& OBJECT_SPACE_ID = databaseSpaceId;
};

// Resolve a forward declaration from Table.hpp
template<typename>
struct ContractSpaceId {
    constexpr const static uint8_t& value = Contract::OBJECT_SPACE_ID;
};

namespace Detail {
template<typename... Ts>
inline std::string ToMessage(const Ts&... pieces) {
    std::ostringstream msgStream;
    auto printer = [&msgStream](auto field) -> char {
        msgStream << fc::format_string("${F} ", fc::mutable_variant_object("F", fc::variant(field, 100)));
        return 0;
    };
    char zeroes[] = {printer(pieces)...};
    (void)zeroes;

    auto message = msgStream.str();
    if (message.size() > 0 && message.back() == ' ')
        message.resize(message.size()-1);

    return message;
}
}

// A helper template to abort the contract, combining several data elements into the error message
template<typename T, typename... Ts>
inline void Abort(const T& first, const Ts&... others) {
    Abort(std::string_view(Detail::ToMessage(first, others...)));
}

// A helper template to verify a condition, and combine several data elements into the message if the condition fails
template<typename T, typename... Ts>
inline void Verify(bool condition, const T& first, const Ts&... others) {
    Verify(condition, std::string_view(Detail::ToMessage(first, others...)));
}

// A helper template to log several data elements
template<typename T, typename... Ts>
inline void Log(T first, Ts... others) {
    ilog("Message from contract: ${MSG}", ("MSG", Detail::ToMessage(first, others...)));
}

} // namespace BAL

FC_REFLECT_TEMPLATE((typename Tag), BAL::ID<Tag>, (value))
