#pragma once

#include <Utils/TypeList.hpp>

#include <BAL/Name.hpp>

namespace BAL {

// A type describing a contract action, used to declare actions to the BAL
template<Name::raw N, typename A, A> struct ActionDescription;
template<Name::raw N, typename C, typename... Args, void (C::*action)(Args...)>
struct ActionDescription<N, void (C::*)(Args...), action> {
    using Contract = C;
    using Parameters = Util::TypeList::List<Args...>;
    using Arguments = std::tuple<Args...>;

    constexpr static Name ActionName = Name(N);

    static constexpr void (Contract::*Action)(Args...) = action;
    static void Call(Contract& c, const Args... args) { (c.*Action)(args...); }
};
struct toArguments {
    template<typename ActionDescription>
    struct transform { using type = typename ActionDescription::Arguments; };
};


// A convenience macro to create an ActionDescription for a given action. Action must be fully qualified name.
#define DESCRIBE_ACTION(N, A) BAL::ActionDescription<N, decltype(&A), &A>

// Contracts using the BAL must specialize this template to declare their types and actions to the BAL.
// These are used to integrate the contract with the underlying blockchain (dispatch actions, register indexes, etc)
template<typename Contract, typename=void>
struct ContractDeclarations {
    // A list of the objects around which tables will be formed. Note that the SecondaryIndexes template should also
    // be specialized for each of these object types.
    using Tables = Util::TypeList::List<>;
    // A list of ActionDescription types describing the actions defined by the contract
    using Actions = Util::TypeList::List<>;

    // Specializations should omit this type
    using Specialized = std::false_type;
};
// Automatically specialize ContractDeclarations for any contract which defines public member types Tables and Actions
template<typename Contract>
struct ContractDeclarations<Contract,
        std::enable_if_t<!std::is_same_v<typename Contract::Tables, Util::TypeList::List<void>> &&
                         !std::is_same_v<typename Contract::Actions, Util::TypeList::List<void>>, void>> {
    using Tables = typename Contract::Tables;
    using Actions = typename Contract::Actions;
};

namespace Impl {

// Typelist transformer, taking an ActionDescription and returning its Arguments tuple
struct actionToArguments {
    template<typename ActionDescription>
    struct transform { using type = typename ActionDescription::Arguments; };
};

// This template can be used to check whether ContractDeclarations is specialized for a given contract or not.
template<typename Contract, typename=void>
struct ContractIsDeclared : public std::true_type {};
template<typename Contract>
struct ContractIsDeclared<Contract, std::enable_if_t<!ContractDeclarations<Contract>::Specialized::value, void>>
        : public std::false_type {};

} // namespace Impl

} // namespace BAL

