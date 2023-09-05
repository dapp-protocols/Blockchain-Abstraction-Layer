#ifdef BAL_PLATFORM_LEAP

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
            TL::Runtime::ForEach(ContractName::Actions(), \
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
                    std::apply([receiver, code, &data](auto... args) { \
                        ContractName contract(receiver, code, data); \
                        ActionDescription::Call(contract, args...); \
                    }, args); \
                } \
            }); \
            BAL::Verify(found, "Could not find a matching action to dispatch"); \
        } \
    } \
}

#endif
