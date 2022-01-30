#ifdef BAL_PLATFORM_EOSIO

#include <BAL/BAL.hpp>

#include <optional>

#define BAL_MAIN(ContractName, ContractAdmin) \
namespace BAL { \
void Abort(std::string_view message) { eosio::check(false, message.data(), message.size()); } \
void Verify(bool condition, std::string_view message) { \
    if (!condition) { \
        eosio::print(message.data(), message.size()); \
        eosio::eosio_exit(1); \
    } \
    eosio::check(condition, message.data(), message.size()); } \
void Log(std::string_view message) { eosio::print(std::string(message), "\n"); } \
} /* namespace BAL */ \
extern "C" { \
    void apply(uint64_t receiver, uint64_t code, uint64_t action) { \
        namespace TL = Util::TypeList; \
        if (receiver == code) { \
            BAL::Log("Received action", BAL::Name(action)); \
            bool found = false; \
            TL::runtime::ForEach(ContractName::Actions(), \
                                 [&found, receiver=BAL::Name(receiver), code=BAL::Name(code), action=BAL::Name(action)] (auto a) { \
                using ActionDescription = typename decltype(a)::type; \
                if (action == ActionDescription::ActionName) { \
                    BAL::Log("Action recognized. Parsing arguments"); \
                    found = true; \
                    char buffer[eosio::action_data_size()]; \
                    eosio::read_action_data(buffer, sizeof(buffer)); \
                    typename ActionDescription::Arguments args; \
                    eosio::datastream<const char*> data(buffer, sizeof(buffer)); \
                    data >> args; \
                    BAL::Log("Dispatching action", action); \
                    boost::mp11::tuple_apply([receiver, code, &data](auto... args) { \
                        ContractName contract(receiver, code, data); \
                        ActionDescription::Call(contract, args...); \
                    }, args); \
                } \
            }); \
            BAL::Verify(found, "Could not find a matching action to dispatch"); \
        } \
    } \
} \

#elif BAL_PLATFORM_PEERPLAYS

#include <BAL/BAL.hpp>
#include <BAL/Name.hpp>

#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/database.hpp>

#include <boost/config.hpp>

#include <optional>

// Declare contract interface for dynamic Peerplays node
extern "C" BOOST_SYMBOL_EXPORT const char* contractName;
extern "C" BOOST_SYMBOL_EXPORT bool registerContract(graphene::chain::database&, uint8_t);
extern "C" BOOST_SYMBOL_EXPORT void deregisterContract();

#define BAL_MAIN(ContractName, ContractAdmin) \
namespace BAL { \
std::string Contract::contractName = ::contractName; \
BAL::AccountName Contract::contractOwner; \
graphene::chain::database* Contract::blockchain = nullptr; \
uint8_t Contract::databaseSpaceId = ~0; \
std::unique_ptr<Dispatcher> Contract::dispatcher; \
std::optional<graphene::chain::op_evaluator::evaluator_handle> Contract::evaluatorHandle; \
const graphene::chain::account_object* GetAccount(graphene::chain::database* blockchain, BAL::AccountName account) { \
    Verify(blockchain != nullptr, "Asked to check if account exists before the contract was intialized!"); \
    auto& accountIndex = blockchain->get_index_type<graphene::chain::account_index>().indices() \
            .get<graphene::chain::by_name>(); \
    auto pos = accountIndex.find(account); \
    if (pos != accountIndex.end()) \
        return &*pos; \
    return nullptr; \
} \
void Contract::requireAuthorization(BAL::AccountName account) { \
    auto* accountObject = GetAccount(blockchain, account); \
    Verify(accountObject != nullptr, "Asked to require authorizing account, but provided account does not exist!"); \
    requireAuthorization(accountObject->id.instance()); \
} \
void Contract::requireAuthorization(AccountId account) { \
    Verify(actionOperation != nullptr, "Asked to require authorization for action, but action operation is not set!"); \
    graphene::chain::account_id_type accountId(account); \
    Verify(actionOperation->required_auths.contains(accountId), \
           "Required authorization of account", accountId, "but no such authorization given"); \
} \
bool Contract::accountExists(BAL::AccountName account) { \
    return GetAccount(blockchain, account) != nullptr; \
} \
bool BAL::Contract::accountExists(AccountId account) { \
    Verify(blockchain != nullptr, "Asked to check if account exists before the contract was intialized!"); \
    return blockchain->find(graphene::chain::account_id_type(account)) != nullptr; \
} \
std::optional<AccountId> Contract::getAccountId(BAL::AccountName name) { \
    auto account = GetAccount(blockchain, name); \
    if (account != nullptr) \
        return account->id.instance(); \
    return {}; \
} \
std::optional<BAL::AccountName> Contract::getAccountName(BAL::AccountId id) { \
    Verify(blockchain != nullptr, "Asked to get account's name before the contract was intialized!"); \
    auto account = blockchain->find(graphene::chain::account_id_type(id)); \
    if (account != nullptr) \
        return account->name; \
    return {}; \
} \
void Abort(std::string_view message) { \
    FC_THROW_EXCEPTION(fc::assert_exception, "Contract aborted with message: ${MSG}", ("MSG", std::string(message))); \
} \
void Verify(bool condition, std::string_view message) { \
    FC_ASSERT(condition, "Contract failed due to unsatisfied verification. Message: ${MSG}", \
              ("MSG", std::string(message))); \
} \
void Log(std::string_view message) { \
    ilog("Message from contract: ${MSG}", ("MSG", std::string(message))); \
} \
} /* namespace BAL */ \
const char* contractName = #ContractName; \
bool registerContract(graphene::chain::database& db, uint8_t spaceId) { \
    try { \
        BAL::Contract::Initialize<ContractName>(contractName, #ContractAdmin, db, spaceId); \
        return true; \
    } catch (fc::exception_ptr e) { \
        BAL::Log("Failed to register contract:", e->to_detail_string()); \
        return false; \
    } \
} \
void deregisterContract() { \
    BAL::Contract::Deregister(); \
} \

#endif
