The Blockchain Abstraction Layer (BAL)
======================================

The BAL is an adaptive layer that allows a single smart contract code implementation to compile and run on multiple blockchain back-end systems. Given that a smart contract is the back-end to a decentralized application, this can provide such applications with back-end portability, making it as easy as is technically feasible to launch clones of the application on multiple chains, or to move the back-end of an application from one chain to another.

This is valuable because blockchains are consensus forming protocols, which allow social groups to agree on a set of rules for a system and agree on the state of that system at any given instant; however, as time goes on, circumstances and needs of different people change in different ways, and as the rules evolve to continue to meet the needs of the group, some members find that they need the rules to change differently than the others. The only universally applicable solution in this event is to split the group, so some members use one new rule set while others use another. As groups divide and merge, they need to be able to move their operations from one chain to another.

The purpose of the BAL is to allow contracts to be moved from chain to chain with minimal friction. It does this by creating an abstract interface through which the contract accesses and integrates with the blockchain  upon which it is deployed. This abstract interface can then be implemented in terms of many concrete blockchain platforms, allowing contracts to be trivially (re)deployed on any supported blockchain.

## Installation and Dependencies
Currently supported blockchains are Peerplays and EOS.IO, although other platforms may and likely will be added in the future.

The BAL is, by necessity, structured as a header-only library (as EOS.IO's compiler does not support static libraries), and so the main installation step is to add the BAL repository as an include path in your contract's project.

The BAL depends on the SDKs of the target blockchains (the EOSIO CDT and/or the Peerplays libraries and dynamic node). If the SDK of a target platform is not installed, builds for that platform will be disabled, but builds for available platforms will proceed.

## General Overview and Usage
A blockchain smart contract is code which runs within a blockchain to process user actions and manage data within a certain application. The smart contract operates within the blockchain's consensus model, ensuring that the actions processed and data stored always converge for all nodes participating in the blockchain consensus and tracking the contract's state. Generally, different blockchain protocols use different semantics for their operations, meaning that although they provide largely the same functionalities, one smart contract code implementation can only target one blockchain protocol, and must be rewritten to target a different protocol.

The BAL creates an abstracted interface for essential smart contract operations which are similar across many blockchain protocols. This allows a single smart contract implementation to target the BAL interface, enabling the same contract code to compile and execute on all blockchain protocols supported by the BAL.

The essential operations that the BAL abstracts for contracts include:
- Defining smart contract Actions which users invoke to interact with the contract
- Storing persistent data processed by the contract as Tables within the blockchain database
- Logging contract status and debugging information, and cleanly handling error conditions

#### Important Caveat
It is critical for the smart contract developer to understand the relationship their smart contract has with any particular blockchain. On Turing-complete third party smart contracting blockchains[^1], such as EOSIO, smart contract logic and operations are part of blockchain consensus, and are therefore recognized as part of the transaction. On curated smart contracting blockchains[^1], such as Peerplays, third party smart contracts maintain their own data consensus, relying on the chain solely for timestamping consensus, and as a result, the smart contract's logic and operations are an informal part of the transaction. **The practical effect** of this is that, while blockchain transactions generally imply that all operations of the transation succeed or fail together, this guarantee does not hold for third party contract operations on chains where third party contracts maintain their own data consensus. On Peerplays, if our BAL contract's operation fails, this does not invalidate the transaction per Peerplays consensus: all other operations in the transaction will process regardless of the failure of our contract's operation. This is different from EOSIO, where the BAL contract is part of the blockchain consensus, and thus, if the contract's operation fails, so too does the entire transaction.

In short, on some chains, third party contract operations are informal, so their failure cannot invalidate the transaction containing them. The transaction and any other operations within it may succeed even though the third party operation failed.

## Types and Interfaces
The base class of contracts is `BAL::Contract`. Contracts may access the underlying blockchain by calling the methods of `BAL::Contract`. Note also the `BAL/IO.hpp` interface for logging and checking fatal error conditions.

To compile the contract, a final component is needed: the entrypoint (or main) of the contract. This entrypoint contains code to, as necessary, register the contract with the blockchain, detect when a blockchain transaction is invoking the contract, and execute the appropriate contract code at the appropriate times. The entrypoint must be defined in exactly one source file of the contract by invoking the `BAL_MAIN(ContractName, ContractAdmin)` macro (defined in `BAL/Main.hpp`) from the global namespace, where ContractName is the contract class's name (must be in scope) and ContractAdmin is the name of the contract owner/administrator's blockchain account.

The blockchain back-end to compile for is defined at compile time by the `BAL_PLATFORM_EOSIO` or `BAL_PLATFORM_PEERPLAYS` macro. Only one `BAL_PLATFORM_*` macro will be defined at a time.

## Getting Started
To learn how to use the BAL to create your own smart contracts, I invite you to check out the [development environment tutorial](DevEnv.md) which walks through the process of setting up a BAL-based smart contract development environment on a developer's operating system.


[^1]: While all blockchains are smart contracting platforms, there are two major classes of blockchain: those containing Turing-complete virtual machines capable of taking any third party contract code and executing it as part of the blockchain's consensus; and those with a curated selection of smart contracts built in by the blockchain's developers. In some circles, the former class are referred to as "smart contract platforms" to distinguish them from the latter class, but this is errant as all blockchains are smart contract platforms. The key difference is whether the platform contains support for arbitrary third party contracts, or limits its support to contracts vetted and formally supported by the blockchain developers themselves. Examples of Turing-complete contracting chains include Ethereum and EOSIO, while examples of curated contracting chains include Bitcoin and Peerplays.
