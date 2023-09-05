# Smart Contract Tutorial

[[_TOC_]]

In this document, we explore a tutorial on how to create a smart contract based on the Blockchain Abstraction Layer. A smart contract is the basis of a decentralized application. A decentralized application is an application based on and delivered by verifiable, decentralized technologies. A blockchain is an example of a decentralized back-end technology, and a smart contract is a specific cluster of functionalities defined within a given blockchain. The smart contract, which executes within the blockchain, forms the back-end for a decentralized application. Users interact with this contract back-end using a front-end app running on their device.

A smart contract consists of executable code which is loaded into a blockchain node and executed by the blockchain when it is invoked by a transaction being processed by that blockchain. The Blockchain Abstraction Layer, or BAL, is a library and environment for smart contract code which abstracts the disparate interfaces and environments of multiple blockchains, providing a unified and consistent basis for smart contracts so that a single implemented contract can be deployed unmodified on multiple blockchains.

In this tutorial, we will explore the process of creating a new smart contract from scratch using the BAL. This contract will implement the back-end for a decentralized inventory and supply chain tracking system which can be used to track raw materials and products as they progress through a supply chain. The contract will track materials as they are moved between warehouses, manufactured into products, and delivered to customers, ensuring that a delivered product can be tracked to the origins of its constituent materials.


### Smart Contract Design
While a smart contract is not a user application, it is important to consider, before writing the code, the people who will be using the contract and how they will be interacting with it. This makes it easy to create quality apps for the contract: apps that enable people to easily and intuitively use the contract to solve their problems or satisfy their customers.

In the case of our supply chain contract, we want a solution which can be utilized by manufacturers, warehouses, cargo carriers, and vendors to preserve detailed records of how products are produced, including the origins and handling of the materials from which those products were made and where the materials and products were stored or processed, up to the point of sale.

##### Design Principles
Several principles will inform the design of our contract. These principles include:
 - Minimum Coordination
 - Scalability
 - Flexibility
 - Simplicity
 - Traceability

**Minimum Coordination**
In its intended usage, the contract will be used by many independent organizations with unrelated ownership, distinct processes, disparate standards, and diverse backgrounds. The contract should work within these organizations' existing paradigms, adding value to their existing product without imposing changes to their processes or complicating their negotiations with partners.

**Scalability**
The contract should be designed with resource constraints in mind, to ensure that it can function indefinitely without unmanageable growth in resource consumption. It should minimize usage of blockchain space and bandwidth, and remove records from blockchain RAM when they are no longer needed.

**Flexibility**
The contract is intended for use by many parties, possibly via different apps, so it needs to be flexible to their different needs and focuses. It should define only a baseline minimum functionality common to all participants in a supply chain, and it must be capable of following the vicissitudes and changing plans that can occur in the real world.

**Simplicity**
The contract should be simple and easy to understand, with clear documentation and minimal complexity or focus on detail. The details and frills can be managed by front-end applications, but variations between front-end nuances should not impair the core functionality of the contract.

**Traceability**
Only live data necessary for contract operations is kept in blockchain RAM; supply chain information must be extracted from historical data. This process is straightforward, but slow and resource intensive. The contract should be designed so that historical data contains linkages which can be followed and supply chain history can be compiled with no more work than is necessary.

#### Design Specifications
Bearing these principles in mind, we design the contract to track inventory as it enters the system, moves between warehouses, and is processed and combined into products.

**Creation of Warehouses and Inventory**
Any blockchain account can add new warehouses, and add inventory to the warehouses it has created. The contract tracks only minimal data about these warehouses and inventory -- namely, a short description text of 250 characters or less. This description provides adequate space to store some metadata and an [IPFS Content ID](https://ipfs.io/), an [IPNS name](https://docs.ipfs.io/concepts/ipns/), or some other reference to further data. This keeps the bulk of data off-chain, thus supporting scalability.

Inventory can be added to a warehouse by the warehouse manager, which is the account that created the warehouse. The warehouse manager can also update or delete the warehouse, or transfer it to a new manager.

**Movement of Inventory**
To move inventory out of the warehouse, it can either be transferred to another warehouse directly or shipped via a cargo carrier (which is just a blockchain account). In both cases, a manifest of inventory is specified to be moved. In the case of direct transfer, the inventories of both warehouses are updated. In the case of shipping, the manifest of inventory is added to a list of manifests currently held by that carrier, and the inventory of the sending warehouse is updated to reflect the removal of materials. Later on, the carrier can deliver a manifest or sub-manifest to any warehouse (including the one from which the manifest originated), or to another carrier, and the inventories are updated accordingly.

In all cases of inventory transfer, whether between warehouses and/or carriers, both the origin and destination must authorize the transaction to acknowledge the transfer of inventory. Carriers and warehouses can also remove their inventory, to indicate losses or delivery to a customer and removal from the system. In all cases of inventory transfer or removal, a description of up to 250 characters can be included to attach documentation to the event.

In this manner, inventory can be moved in ad-hoc ways, and is not constrained to a planned schedule of movement. Planned schedules can be included by apps in the description data, but these are not factored into the contract, which supports the principle of flexibility. Furthermore, no party is required to authorize any action until it involves a direct transfer of inventory to/from that party, to support the principle of minimum coordination.

**Manufacturing of Inventory**
Inventory in a warehouse can also be manufactured into other inventory. When inventory is manufactured, several items are consumed and several others are produced. Consumed items are removed from the warehouse's inventory and produced items are added to it. Inventory can be produced during manufacturing without any inventory being consumed, or inventory can be consumed without any being produced; however, manufacturing events which neither consume nor produce inventory are not allowed. Manufacturing events can specify a description of up to 250 characters.

While manufacturing processes in the real world can be incredibly complex and intricate, this model attempts to capture the fundamentals of what happens from an inventory perspective in a single, easily understood event: manufacturing can consume some items and produce other items. This allows a great deal of flexibility in recording events while also supporting the principle of simplicity.

**History of Inventory**
At all times during storage and shipping of inventory, the contract tracks two historical fields for each unit of inventory: the origin of the inventory, and its movement history. The inventory's origin is the event that created it, which is either the addition of new inventory to a warehouse, or a manufacturing event which produced new inventory. The movement history is the list of all events where the inventory was moved between warehouses and/or cargo carriers. The entire movement history of a unit of inventory is kept until that inventory is removed from the system, whether it is removed directly, or it is manufactured into something else. The origin and movement history are stored as references to the blockchain events that created or moved inventory, so information on inventory's history can be collected by looking up these events in the blockchain history. This supports the principle of traceability.

In typical usage, an item of inventory will be removed from the contract tracking at the moment that it is delivered to a customer. At this instant, the inventory's record, including its historical information, is removed from the blockchain RAM. It may become customary to provide this record to the customer in hard copy for their reference. This record would contain the origin of the inventory (presumably a manufacturing event) and the movement history (a list of transfer events). The customer could locate these events in blockchain history, and inspect them for documentation to learn more about the product's history. If desired, the customer could dig deeper and recover the inventory records of the materials that went into the manufacturing of his product, and inspect their origins and movements as well. At worst, this could require them to replay blockchain history in order to reproduce blockchain RAM at the time of one of these historical events, in order to locate desired information. Manufacturers and vendors wishing to make this information readily available to customers could proactively archive it for easy retrieval, so that customers could access it easily without needing to mine blockchain history for the information.

#### Contract Interface
Before designing the interface of our supply chain tracking smart contract, a brief introduction to how smart contracts based on the BAL are invoked by users and what they do when they are.

##### How Smart Contracts Are Used
Both smart contracts and the blockchains that run them have limited channels for user interaction and ingress of data. The only mechanism by which people can interact or data can be introduced for processing or storage is _transactions._ A transaction is an atomic group of actions taken within the blockchain that either all succeed or all fail. These actions can provide new data and trigger processing within the chain. To be processed, an action must be part of a transaction, the transaction must bear proper signatures to authorize the contained actions, and then the transaction must be included in a block in accordance with the blockchain's consensus mechanism. Only valid and authenticated transactions can be included in blocks, and only once a transaction is included in a block are its actions processed.

A smart contract defines its API as a set of actions, and these actions can be added to transactions to invoke the contract API. When the transaction is included in a block, the blockchain evaluates the actions to determine what contract code to run. The arguments are decoded and used to invoke the contract code. The contract also defines data structures to store in the blockchain database, and can read and write these records while processing an action. Generally speaking, this is the only way a smart contract can execute code: to process an action in a transaction.

The only means a smart contract has of persisting data is the blockchain database, also known as blockchain RAM. This is intentional, as any other storage would circumvent the blockchain consensus protocol and break the guarantee that nodes in the blockchain always remain in sync. The blockchain database automatically handles backing up contract state before changes, and ensures that the state remains fully consistent even during blockchain reorganizations, when state must be rewound and fast forwarded to correct synchronization errors in the blockchain network.

Within the BAL and its supported blockchains, the blockchain database is an in-memory, relational database. A smart contract utilizes the blockchain database by declaring tables to be stored and then reading and writing rows in those tables. These tables are arbitrary structures containing several fields of arbitrary types. All tables must feature a 64-bit primary key which is usually used as an ID field. Tables may also define several secondary keys, which are calculated from the fields of the table, to enable the table to be sorted or searched based on other indexes than the primary key.

In some blockchains, such as EOS, storage space in blockchain tables is billed to accounts and is often expensive, so it is prudent to minimize the amount of data stored here and delete records as soon as they are no longer needed. The blockchain database is visible to clients via blockchain API servers, and clients can read this data to inspect blockchain state without making transactions; however, clients submitting transactions based upon this state cannot be certain that the state will not have been changed by the time the transaction is processed.

##### Supply Chain Contract Actions
The actions of a contract are the sole interface by which applications, on behalf of users, interact with the contract. Now that we have considered how our smart contract is intended to work and benefit users, we design our contract's interface to the wider world. Because the actions define how users invoke the contract, it is ideal to lay them out in the design phase rather than during implementation, so that they form a natural interface rather than merely the interface that was easy to make. While the final implementation may deviate slightly from the actions laid out below, adhering to the design will help to ensure that our end result is relevant and cognizable to users.

**Principles of Smart Contract Interfaces** When designing the actions for a smart contract, it is helpful to consider how smart contract interfaces work in real life. To execute an action in a smart contract, the action is added to a transaction, signed, and broadcast to the blockchain network. There is a variable time delay between the time of broadcast and the time the transaction is included in a block. In some cases, a lot can change during this delay. This creates an uncertainty principle of blockchain interaction: the state of the database can be seen by clients as they craft their transaction, but the client cannot be certain that the state will not have been changed by the time the action runs. Consider also during the interface design that smart contract interfaces are often difficult to update: there are technical complications that must be considered, but often there are politics involved in updating smart contracts as well.

In light of these considerations, a good smart contract interface allows interaction which is either assertive or unilateral. Assertive interaction will take action only if the database remains in the anticipated state when the action is processed, and fail otherwise. Unilateral interaction puts the database into a known state regardless of its prior state. The actions should be designed to allow both modes of operation. A good contract interface will also be flexible while still being unambiguous. The [TMTOWTDI principle](https://en.wikipedia.org/wiki/There%27s_more_than_one_way_to_do_it) factors in here: while there should be one obvious way to achieve any desired result, there should ideally be other ways to achieve it as well. This is especially beneficial in smart contract interface design, as smart contracts are used by people from all walks of life, and their needs are as diverse as their backgrounds. Building flexibility into the interface of the contract helps to ensure that all needs can be met.

**Warehouse Management Actions**

These actions manage the warehouse as a whole, and can only be taken by the warehouse manager.

    Action: AddWarehouse
    Description:
       Create a new warehouse within the system, to store and manufacture inventory. The warehouse
       manager account will have full control of the warehouse inventory and operations. This
       account will also be billed for the storage of the warehouse and inventory records. The
       warehouse ID will be defined when this action is processed by the blockchain.
    Arguments:
     - manager [AccountId] The warehouse manager. This account must authorize this action.
     - description [string] A description of the warehouse -- format is application-defined (max.
         250 chars).

    Action: UpdateWarehouse
    Description:
       Update the manager and/or description of an existing warehouse. Either the newManager or the
       newDescription field must be defined.
    Arguments:
     - warehouse [WarehouseId] The ID of the warehouse to update.
     - currentManager [AccountId] The current warehouse manager account. This account must
         authorize this action.
     - newManager [optional<AccountId>] The new warehouse manager. This account will be billed for
         storage of the warehouse and inventory records. The new manager must also authorize the
         action.
     - newDescription [optional<string>] Updated description of the warehouse. Format is
         application-defined (max. 250 chars, cannot match old description).
     - documentation [string] Documentation on the update (max. 250 chars).

    Action: DeleteWarehouse
    Description:
       Remove a warehouse from the tracking system, releasing its ID and ending billing for storage
       of records of the warehouse and stock.
    Arguments:
     - warehouse [WarehouseId] The ID of the warehouse to remove.
     - manager [AccountId] The warehouse manager. This account must authorize this action.
     - removeInventory [bool] Whether to delete warehouse's inventory when deleting the warehouse.
         This is a safety check. If the warehouse has inventory and this is false, the action will
         fail.
     - documentation [string] Documentation on the event (max. 250 chars).

**Inventory Management Actions**

These actions manage the inventory of a warehouse.

    Action: AddInventory
    Description:
       Add inventory to the warehouse's stock. The new inventory will have no movement history and
       its origin will point to this action. The inventory ID will be defined when this action is
       processed by the blockchain. The warehouse manager will be billed for the storage of the new
       inventory record.

       Inventory records with a quantity of zero are permitted, and still retain an inventory ID
       and consume billable storage space. To remove inventory from storage, releasing the ID and
       storage space, use RemoveInventory.
    Arguments:
     - warehouse [WarehouseId] The ID of the warehouse to add inventory to.
     - manager [AccountId] The warehouse manager, who must authorize this action.
     - description [string] A string describing the inventory. Format is application-defined (max.
         250 chars).
     - quantity [uint32] Number of units of inventory to add to stock
    
    Action: AdjustInventory
    Description:
       Adjust the quantity in stock or description of a record of inventory without updating the
       tracking history. Either the description or quantity must be updated. 
    Arguments:
     - warehouse [WarehouseId] The ID of the warehouse to adjust inventory in.
     - manager [AccountId] The warehouse manager, who must authorize this action.
     - inventory [InventoryId] The ID of the inventory record to adjust.
     - newDescription [optional<string>] Updated description string (max. 250 chars).
     - newQuantity [optional<variant<uint32, int32>>] Adjustment to quantity of the inventory. May
         be either absolute (unsigned variant) or a delta (signed variant). Delta cannot be zero,
         and cannot adjust quantity in stock to be a negative value.
     - documentation [string] Documentation on the adjustment (max. 250 chars).
    
    Action: RemoveInventory
    Description:
       Remove inventory from the warehouse's stock. Can be used either to decrease quantity in
       stock, or to remove the record entirely, releasing the inventory ID and billable storage
       space.
    Arguments:
     - warehouse [WarehouseId] The ID of the warehouse to remove inventory from.
     - manager [AccountId] The warehouse manager, who must authorize this action.
     - inventory [InventoryId] The ID of the inventory record to remove inventory from.
     - quantity [uint32] Quantity adjustment. Positive values indicate number of units to remove,
         and cannot exceed the number of units in stock. Zero means remove all units and set
         quantity in stock to zero.
     - deleteRecord [bool] Whether to delete record or not. Quantity in stock must be zero to
         delete, so if quantity argument is positive and does not match units in stock, the action
         will fail. A quantity argument of zero removes all stock, so deletion will not fail.
     - documentation [string] Documentation on the removal (max. 250 chars).
    
    Action: ManufactureInventory
    Description:
       Manufacture some inventory into other inventory. This takes a list of inventory IDs and
       quantities to consume, and a list of inventory descriptions and quantities produced. The
       consumed inventory is removed from the warehouse's stock, and the produced inventory is
       added to it. If the warehouse stock is not adequate to supply the inventory consumed, the
       action will fail. If any quantity to produce or consume is zero, the action will fail. The
       produced inventory will have no movement history, and its origin will be set to this action.
       The warehouse manager is billed for inventory storage.
    Arguments:
     - warehouse [WarehouseId] The ID of the warehouse to manufacture inventory in.
     - manager [AccountId] The warehouse manager, who must authorize this action.
     - consume [map<InventoryId, uint32>] Inventory IDs and quantities to consume in manufacturing.
     - produce [map<string, uint32>] Inventory descriptions (max. 250 chars) and quantities to
         produce as a result of manufacturing.
     - deleteConsumed [bool] If true, inventory records which are completely consumed will be
         deleted from billable storage.
     - documentation [string] Documentation on the manufacturing event (max. 250 chars).
    
    Action: TransferInventory
    Description:
       Transfer a manifest of inventory directly to another warehouse. Removes the inventory in the
       manifest from the source warehouse's stock and adds it to the destination's. Records in the
       destination warehouse's inventory will be updated to include this action in their movement
       history. If the source warehouse's stock does not contain sufficient inventory to satisfy
       the manifest, the action will fail. If any quantity on the manifest is zero, the action will
       fail. The destination warehouse's manager is billed for the storage of new inventory
       records. The destination warehouse must be different from the source warehouse.
    Arguments:
     - sourceWarehouse [WarehouseId] The ID of the warehouse sending inventory.
     - sourceManager [AccountId] The warehouse manager authorizing this action on the source side.
     - destinationWarehouse [WarehouseId] The ID of the warehouse receiving inventory.
     - destinationManager [AccountId] The warehouse manager authorizing this action on the
         destination side.
     - manifest [map<InventoryId, uint32>] The manifest of inventory to transfer. Includes the
         inventory IDs and quantities to transfer.
     - deleteConsumed [bool] If true, inventory records which are completely consumed will be
         deleted from billable storage.
     - documentation [string] Documentation on the transfer (max. 250 chars).
    
    Action: ShipInventory
    Description:
       Transfer a manifest of inventory to a cargo carrier. Removes the inventory in the manifest
       from the warehouse's stock and updates the carrier's manifests and cargo to account for the
       merchandise. The inventory records in the carrier's records will have this action added to
       their movement history. If the warehouse's stock does not contain sufficent inventory to
       satisfy the manifest, the action will fail. If any quantity on the manifest is zero, the
       action will fail. The manifest ID will be defined when this action is processed by the
       blockchain. The carrier will be billed for storage of the cargo and manifest records.
    Arguments;
     - warehouse [WarehouseId] The ID of the warehouse shipping inventory.
     - manager [AccountId] The warehouse manager authorizing this shipment from the warehouse.
     - carrier [AccountId] The cargo carrier accepting and authorizing this shipment.
     - manifest [map<InventoryId, uint32>] The shipping manifest. Includes the inventory IDs and
         quantities to ship.
     - deleteConsumed [bool] If true, inventory records which are completely consumed will be
         deleted from billable storage.
     - documentation [string] Documentation on the shipment (max. 250 chars).

**Cargo Carrier Actions**

These actions can be used by a cargo carrier to manage inventory in their care.

    Action: RemoveCargo
    Description:
       Remove cargo from a specified manifest from tracking. Decreases the quantity of inventory
       by the specified amount, and if the remaining quantity is zero, deletes the record from
       billable storage and, if the manifest is empty after this deletion, deletes the manifest
       record as well. If the specified quantity to remove is zero, or exceeds the quantity held
       by the carrier, the action will fail.
    Arguments:
     - carrier [AccountId] The cargo carrier removing cargo.
     - manifest [ManifestId] The manifest of inventory from which cargo will be removed.
     - cargo [CargoId] The ID of the cargo in the manifest to remove.
     - quantity [uint32] The quantity of units of cargo to remove.
     - documentation [string] Documentation on the cargo removal (max. 250 chars).
    
    Action: TransferCargo
    Description:
       Transfer a manifest or sub-manifest of cargo to a different shipping carrier. Removes the
       cargo from the source carrier's manifest and adds it to the destination carrier's manifests
       and cargo. Records in the destination carrier's cargo will have this action added to their
       movement history. Cargo and manifest records which are emptied in the source carrier's
       records will be removed from billable storage. Destination carrier will be billed for new
       cargo and manifest records. If transferred quantities exceed source carrier's quantity held,
       or if any transferred quantities are zero, the action will fail. If submanifest specifies
       any cargo IDs that are not on the specified manifest, the action will fail. The destination
       carrier must be different from the source carrier.
    Arguments:
     - sourceCarrier [AccountId] The cargo carrier transferring cargo away. This account must
         authorize the transaction.
     - destinationCarrier [AccountId] The cargo carrier receiving cargo. This account must
         authorize the transaction.
     - manifest [ManifestId] The manifest of inventory from which to transfer cargo.
     - submanifest [map<CargoId, uint32>] If empty, the entire manifest is transferred. Otherwise,
         only the specified cargo IDs and quantities are transferred.
     - documentation [string] Documentation on the cargo transfer (max. 250 chars).
    
    Action: DeliverCargo
    Description:
       Deliver a manifest or sub-manifest of cargo to a warehouse. Removes the cargo from the
       carrier's manifest and adds it to the warehouse's inventory. Records in the warehouse's
       inventory will have this action added to their movement history. Cargo records which are
       emptied in the carrier's records will be removed from billable storage. Warehouse manager
       will be billed for new inventory records. If delivered quantities exceed carrier's quantity
       held, or if any delivered quantities are zero, the action will fail. If the submanifest
       specifies any cargo IDs that are not on the specified manifest, the action will fail.
    Arguments:
     - carrier [AccountId] The cargo carrier delivering cargo. This account must authorize the
         transaction.
     - warehouse [WarehouseId] The warehouse receiving a shipment.
     - manager [AccountId] The warehouse manager authorizing the receipt of shipment. This account
         must authorize the transaction.
     - manifest [ManifestId] The manifest of inventory from which to deliver cargo.
     - submanifest [map<CargoId, uint32>] If empty, the entire manifest is delivered. Otherwise,
         only the specified cargo IDs and quantities are delivered.
     - documentation [string] Documentation on the delivery (max. 250 chars).

#### Persistent Storage
A smart contract persists data using the blockchain database, which is an in-memory, relational database. As such, it features tables with several columns, or fields, and primary and secondary keys, which can be used to sort the rows on different metrics to allow for efficient lookup and access. In general, only keys which the smart contract itself needs should be defined -- if a client needs to access the data with some other sorting order than the back-end needs, then this additional indexing should be done by the client itself or some middleware API server.

The full definition of a properly normalized database schema for the contract is an implementation concern rather than a design concern; however, it is beneficial to consider the database during the design phase and to form a general plan for what tables will exist and what information they will store.

**Table Scopes** The BAL uses a kind of table partitioning scheme called table "scopes." Based upon the EOS blockchain's scopes, scopes are used to partition rows of a table off from each other into sections that a single execution of a contract action will not cross over. This paves the way for a future parallelization of smart contract execution, where multiple actions that affect different scopes of a table could execute simultaneously without risk of them possibly interfering with each other or creating race conditions. While this parallelization is not yet implemented today, it is useful to design scopes into contracts anyways, to enable them to benefit from future performance gains.

Our supply chain tracking smart contract will use the following tables. It is possible that other tables or data may be added during implementation, but this provides a general outline of the database for the contract.
 - Warehouse list
   - Scope: global
   - Stores warehouse ID and description
 - Inventory list
   - Scope: Warehouse ID
   - Stores inventory IDs, descriptions, quantities, origins, and movement histories
 - Manifest list
   - Scope: Carrier Account ID
   - Stores manifest IDs
 - Cargo list
   - Scope: Carrier Account ID
   - Stores cargo IDs, descriptions, quantities, origins, movement histories, and related manifest ID


### Smart Contract Implementation
With our contract design in hand, it is time to begin coding the implementation. Let's start by making an empty contract and getting it to build. Before we begin, we need to install the blockchain SDK for at least one supported blockchain. In the case of EOS, we would install the [Leap CDT](https://github.com/AntelopeIO/cdt/) as Leap is the foundation of the EOS blockchain. We will also need git, cmake, and the various other components of a development environment. Installing these is left as an exercise for the reader.

#### Initialize the Repo
Once our development environment is ready, we can begin. First, create a new git repository and make an initial commit (yes, it's a bit silly, but it's necessary for the next step!)
```
[~] $ mkdir SupplyChain
[~] $ cd SupplyChain
[~/SupplyChain] $ git init
Initialized empty Git repository in ~/SupplyChain/.git/
[~/SupplyChain] $ echo "This is the README for my Supply Chain contract" > README.md
[~/SupplyChain] $ git add README.md
[~/SupplyChain] $ git commit -m "Initial commit"
[master (root-commit) 0d1b19d] Initial commit
 1 file changed, 1 insertion(+)
 create mode 100644 README.md
[~/SupplyChain] $ 
```

Alright. Now we add the BAL to our repo. The best way to do this is as a git subtree, which is why we needed the first commit in: git can't add a subtree without at least one commit.
```
[~/SupplyChain] $ # First, we add a remote for the BAL upstream repo
[~/SupplyChain] $ git remote add -f bal-upstream https://github.com/dapp-protocols/Blockchain-Abstraction-Layer
remote: Enumerating objects: 77, done.
remote: Counting objects: 100% (77/77), done.
remote: Compressing objects: 100% (73/73), done.
remote: Total 77 (delta 31), reused 0 (delta 0), pack-reused 0
Unpacking objects: 100% (77/77), 48.42 KiB | 550.00 KiB/s, done.
From https://github.com/dapp-protocols/Blockchain-Abstraction-Layer
 * [new branch]      master     -> bal-upstream/master
[~/SupplyChain] $ 
[~/SupplyChain] $ # Next, add the subtree as a new directory in the SupplyChain repo
[~/SupplyChain] $ git subtree add --prefix BAL bal-upstream master --squash
git fetch bal-upstream master
From https://github.com/dapp-protocols/Blockchain-Abstraction-Layer
 * branch            master   -> FETCH_HEAD
Added dir 'BAL'
[~/SupplyChain] $ 
[~/SupplyChain] $ # Look here: we have the BAL in our repo!
[~/SupplyChain] $ ls
BAL Readme.md
[~/SupplyChain] $ 
```

For those unfamiliar with subtrees, Atlassian has a great introduction [here](https://www.atlassian.com/git/tutorials/git-subtree).

#### Create an Empty Contract
Now that we have the BAL, we can add an empty contract. First, create a header called `SupplyChain.hpp` and declare an empty contract like so:
```c++
#pragma once

#include <BAL/BAL.hpp>

class SupplyChain : public BAL::Contract {
public:
    using BAL::Contract::Contract;

    using Actions = Util::TypeList::List<>;
    using Tables = Util::TypeList::List<>;
};
```
OK, so what do we have here? Well, we make our contract as a class, called `SupplyChain`, and inherit from the BAL contract base class. Next, we pass through the BAL contract's constructor as our own, so the BAL knows how to instantiate our class. Finally, we declare our lists of Actions and Tables, which right now are just empty. That's it!

Now, add a `main.cpp`. This file declares our contract to the BAL by telling the BAL what the contract class's name is, and what blockchain account the contract will be registered under.
```c++
#include "SupplyChain.hpp"

#include <BAL/Main.hpp>

// Declare our contract class, SupplyChain, to the BAL, and associate it with the supplychain blockchain account
BAL_MAIN(SupplyChain, supplychain)
```
Technically, this macro does a lot of stuff, like registering the contract code and tables with the blockchain node and generating a dispatcher to detect when a transaction is calling our contract, decoding the arguments, and calling the action code, etc. etc. You can read `BAL/Main.hpp` if you want the details, but it's also easy to think of it like a macro that generates `int main(int argc, const char** argv)`, parses the arguments, instantiates our contract, and calls the right method.

Finally, make a CMakeLists.txt to build our contract:
```cmake
project(SupplyChain)
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)

include(BAL/CMakeLists.txt)

BAL_CONTRACT(NAME SupplyChain BAL_PATH BAL SOURCES main.cpp)
```

Now run CMake and build the contract!
```
[~/SupplyChain] $ cmake . -DCDT_PATH=/opt/leap-cdt
-- Defining BAL contract targets for contract SupplyChain
-- Setting up CDT Wasm Toolchain 3.1.0-dev at /opt/leap-cdt
Leap CDT Found; enabling Leap contract build
-- Configuring done
-- Generating done
-- Build files have been written to: ~/SupplyChain
[~/SupplyChain] $ 
[~/SupplyChain] $ make
  <... output of make ...>
[~/SupplyChain] $ 
```

#### Add Our First Action
Now our contract is built for Leap at `BAL/Leap/SupplyChain/SupplyChain.wasm`. Let's go ahead and add a simple action. Edit `SupplyChain.hpp` and add a method, then describe it into the actions list:
```c++
#pragma once

#include <BAL/BAL.hpp>

class SupplyChain : public BAL::Contract {
public:
    using BAL::Contract::Contract;

    void SayHello(std::string name) {
        // Verify is used to assert that a condition is true, and abort the contract with an error if it is not
        BAL::Verify(!name.empty(), "Aww, how can I say hello if I don't know what to call you?");
        // Log does what you'd expect: it prints a message to the contract log
        BAL::Log("Hello,", name);
        // BAL::Abort can also be used to cancel the contract with an error message
        // Verify, Log, and Abort can all accept arbitrarily many arguments of many types and print them.
        // These are declared in BAL/IO.hpp
    }

    using Actions = Util::TypeList::List<DESCRIBE_ACTION("say.hello"_N, SupplyChain::SayHello)>;
    using Tables = Util::TypeList::List<>;
};
```
See that `DESCRIBE_ACTION` macro there? That's a shorthand to create a record for the BAL, telling it what our action is called and where the function is. The arguments are the action's name and the full name of the method, including the class name. The action's name isn't just a string, it's actually a `BAL::Name` which is the same as `eosio::name`. As a result, there are some [funky rules](https://developers.eos.io/manuals/eosio.cdt/latest/best-practices/naming-conventions/#table-struct-class-function-action-names) on how those names are formatted.

To deploy the contract on EOS and other Antelope blockchains, we'll also need an ABI. We can make the Leap compiler attempt to generate those automatically by putting `[[eosio::action("action.name")]]` at the beginning of our action methods, i.e. `[[eosio::action("say.hello")]] void SayHello(std::string name)`, but this process is inexact and may require manual intervention. To avoid confusion, here's an ABI for our contract:

```json
{
    "____comment": "This file was generated with eosio-abigen. DO NOT EDIT ",
    "version": "eosio::abi/1.1",
    "types": [],
    "structs": [
        {
            "name": "SayHello",
            "base": "",
            "fields": [
                {
                    "name": "name",
                    "type": "string"
                }
            ]
        }
    ],
    "actions": [
        {
            "name": "say.hello",
            "type": "SayHello",
            "ricardian_contract": ""
        }
    ],
    "tables": [],
    "ricardian_clauses": [],
    "variants": []
}
```
Now that we have our first action, we can load our contract into a blockchain and run it! This is left as an exercise for the reader, but on EOS, you can find guidance in the official documentation:
1. [Create contract wallet](https://developers.eos.io/welcome/v2.0/getting-started/development-environment/create-development-wallet)
1. [Launch a testnet](https://developers.eos.io/welcome/v2.0/getting-started/development-environment/start-your-node-setup)
1. [Create some user accounts](https://developers.eos.io/welcome/v2.0/getting-started/development-environment/create-test-accounts)
1. [Create your contract account](https://developers.eos.io/welcome/v2.0/tutorials/tic-tac-toe-game-contract/#contract-account)
1. [Deploy and run your contract](https://developers.eos.io/welcome/v2.0/tutorials/tic-tac-toe-game-contract/#deploy)

**EOS Note:** If contract logs aren't showing in `nodeos` output, [try this](https://eosio.stackexchange.com/questions/5264/eosio-v2-0-2-contract-prints-are-not-shown-even-using-nodeos-with-contracts)

#### Writing Supply Chain Tracking Contract
Now that we've covered the basics of creating a contract and writing our first action, let's change our `SayHello` action to our first real action: `AddWarehouse`. To implement this action, we'll need our first table as well, so let's define that too:

```c++
#pragma once

#include <BAL/BAL.hpp>
#include <BAL/Table.hpp>
#include <BAL/Reflect.hpp>

// Import some common names
using BAL::AccountHandle;
using BAL::ID;
using BAL::Name;
using BAL::Table;
using std::string;

// Give names to all of the tables in our contract
constexpr static auto WarehouseTableName = "warehouses"_N;

// Declare unique ID types for all table types, so different ID types can't be cross-assigned
using WarehouseId = BAL::ID<BAL::NameTag<WarehouseTableName>>;

class SupplyChain : public BAL::Contract {
public:
    using BAL::Contract::Contract;

    void addWarehouse(AccountHandle manager, string description);

    struct Warehouse {
        // Make some declarations to tell BAL about our table
        using Contract = SupplyChain;
        constexpr static Name TableName = WarehouseTableName;

        WarehouseId id;
        AccountHandle manager;
        string description;

        // Getter for the primary key (this is required)
        WarehouseId primary_key() const { return id; }
    };
    using Warehouses = Table<Warehouse>;

    using Actions = Util::TypeList::List<DESCRIBE_ACTION("add.wrhs"_N, SupplyChain::addWarehouse)>;
    using Tables = Util::TypeList::List<Warehouses>;
};

// Reflect the table fields
BAL_REFLECT(SupplyChain::Warehouse, (id)(manager)(description)
```
Note the use of the `AccountHandle` type to represent the manager account. This is a type defined by the BAL to reference the type conventionally used on the current blockchain back-end to uniquely identify an account. For example, on Leap, this would be an `eosio::name`. This is the type contracts should typically use to reference accounts in their actions and tables.

To define a table, we define a row of the table. This is just a struct; however, there are several accoutrements that need to go along with the struct to turn it into a table. We need a unique `BAL::Name` for it, a unique ID type for it, and it needs to declare public members specifying its name, what contract it belongs to, and how to get its primary key. Finally, the row needs to be reflected at the bottom so the BAL knows what its fields are. Once we've made all of these declarations, we can turn a row into a table by instantiating the `Table` template with it. We name our row `Warehouse` and name the table `Warehouses`. Then we add our table to our contract's list of Tables, and our table is ready to use.

##### Warehouse Management Actions
Next, we implement our action in a new `SupplyChain.cpp` file:
```c++
#include "SupplyChain.hpp"

// Declare a constant for our global scope ID. This number is arbitrary; we just want to be consistent.
constexpr static auto GLOBAL_SCOPE = "global"_N.value;
// Declare a constant for the maximum description string length
constexpr static auto MAX_DESCRIPTION_SIZE = 250;

void SupplyChain::addWarehouse(AccountHandle manager, string description) {
    // Require the manager's authorization to create a warehouse under his authority
    requireAuthorization(manager);

    // Apply max length check on description
    BAL::Verify(description.length() <= MAX_DESCRIPTION_SIZE,
                "Description may not exceed", MAX_DESCRIPTION_SIZE, "characters.");

    // Open our table in global scope
    auto warehouses = getTable<Warehouses>(GLOBAL_SCOPE);
    // Let the table pick our new row's ID
    WarehouseId newId = warehouses.nextId();
    // Add a new row to the table, billed to manager, passing in a lambda to initialize it
    warehouses.create(manager, [newId, manager, &description](Warehouse& warehouse) {
        warehouse.id = newId;
        warehouse.manager = manager;
        warehouse.description = std::move(description);
    });

    BAL::Log("Successfully created new warehouse with ID", newId, "for manager", manager);
}
```
Our warehouses table isn't really partitioned -- all rows are in the same partition -- so we can pick a scope arbitrarily for it, as long as we always use the same one. A scope is just a 64-bit number, so we can just make a number out of a name and use that. We make it a constant so there's no risk of mistyping it somewhere.

The implementation of our action is straightforward: require the manager's authorization, and if that succeeds, create the warehouse. To create the warehouse, we open the warehouses table in our global scope, get the next available row ID, and create a new row, populating it with the ID and user-supplied values, billing its storage to the manager.

We add our new `SupplyChain.cpp` file to our `CMakeLists.txt` sources list, and our contract builds! We have now defined our first table, and written an action to populate it.

Now we implement our next action, UpdateWarehouse:
```c++

void SupplyChain::updateWarehouse(AccountHandle manager, WarehouseId warehouseId, optional<AccountHandle> newManager,
                                  optional<string> newDescription, string documentation) {
    // Verify that we're actually changing something
    BAL::Verify(newManager.has_value() || newDescription.has_value(),
                "Cannot update warehouse: no changes specified");

    // Require the manager's authorization
    requireAuthorization(manager);
    if (newManager.has_value()) {
        BAL::Verify(manager != *newManager, "New manager must be different from current manager");
        requireAuthorization(*newManager);
    }

    // Apply max length checks
    BAL::Verify(documentation.size() <= MAX_DESCRIPTION_SIZE,
                "Documentation length must not exceed", MAX_DESCRIPTION_SIZE, "characters.");
    if (newDescription.has_value())
        BAL::Verify(newDescription->size() <= MAX_DESCRIPTION_SIZE,
                    "New description length must not exceed", MAX_DESCRIPTION_SIZE, "characters.");

    // Get warehouse record
    auto warehouses = getTable<Warehouses>(GLOBAL_SCOPE);
    const auto& warehouse = warehouses.getId(warehouseId, "Couldn't find warehouse with requested ID");

    // Check manager validity and that new description is different from old one
    BAL::Verify(warehouse.manager == manager,
                "Cannot update warehouse: authorizing account", manager, "is not the warehouse manager",
                warehouse.manager);
    if (newDescription.has_value())
        BAL::Verify(warehouse.description != *newDescription,
                    "New description must be different from current description");

    // Commit the update
    auto payer = (newManager.has_value()? *newManager : manager);
    warehouses.modify(warehouse, payer, [&newManager, &newDescription](Warehouse& warehouse) {
        if (newManager.has_value())
            warehouse.manager = *newManager;
        if (newDescription.has_value())
            warehouse.description = std::move(*newDescription);
    });

    BAL::Log("Successfully updated warehouse", warehouseId);
}
```
When adding the action to our contract's declared actions list, we name it `update.wrhs` to comply with the naming rules.

One might expect the DeleteWarehouse action to come next, but it deletes inventory, so we need to write that table first. We call the row struct for the table `Inventory` and the table itself `Stock`. This table will be scoped on the warehouse ID and will store the item's ID, description, origin, movements, and quantity in stock. The origin and movements will be `TransactionId`s for the transactions that created and moved the inventory.
```c++
using BAL::TransactionId;
using std::vector;
using std::optional;
/* ... */
constexpr static auto InventoryTableName = "inventory"_N;
/* ... */
using InventoryId = BAL::ID<BAL::NameTag<InventoryTableName>>;
/* ... */
    void updateWarehouse(AccountHandle manager, WarehouseId warehouseId, optional<AccountHandle> newManager, optional<string> newDescription, string documentation);
/* ... */
    struct Inventory {
        using Contract = SupplyChain;
        constexpr static Name TableName = InventoryTableName;

        InventoryId id;
        string description;
        TransactionId origin;
        vector<TransactionId> movement;
        uint32_t quantity = 0;

        InventoryId primary_key() const { return id; }
    };
    using Stock = Table<Inventory>;
/* ... */
    using Tables = Util::TypeList::List<Warehouses, Stock>;
/* ... */
BAL_REFLECT(SupplyChain::Inventory, (id)(description)(origin)(movement)(quantity))
```
Now we can write DeleteWarehouse:
```c++
void SupplyChain::deleteWarehouse(WarehouseId warehouseId, AccountHandle manager,
                                  bool removeInventory, string documentation) {
    // Validity checks
    requireAuthorization(manager);
    BAL::Verify(documentation.size() <= MAX_DESCRIPTION_SIZE,
                "Documentation size must not exceed", MAX_DESCRIPTION_SIZE, "characters.");

    // Manager check
    auto warehouses = getTable<Warehouses>(GLOBAL_SCOPE);
    const auto& warehouse = warehouses.getId(warehouseId,
                                             "Cannot delete warehouse: specified warehouse does not exist");
    BAL::Verify(warehouse.manager == manager, "Cannot delete warehouse", warehouseId, "because authorizing account",
                manager, "is not the warehouse manager", warehouse.manager);

    // If warehouse still has stock, check that we can remove it
    auto stock = getTable<Stock>(warehouseId);
    if (stock.begin() != stock.end()) {
        BAL::Verify(removeInventory, "Cannot delete warehouse", warehouseId,
                    "because warehouse still has inventory in stock, but removal of inventory was not authorized.");
    // All checks passed; commit the change
        // Removal authorized: delete the inventory.
        while (stock.begin() != stock.end())
            stock.erase(stock.begin());
    }

    // Delete the warehouse
    warehouses.erase(warehouse);

    BAL::Log("Successfully deleted warehouse", warehouseId);
}
```
Make sure the new actions are included in the `Actions` list in the header. This concludes the implementation of the warehouse management actions!

##### Inventory Management Actions
Next, we implement our actions for inventory management. We begin with AddInventory, also known as `add.invntry`:
```c++
void SupplyChain::addInventory(WarehouseId warehouseId, AccountHandle manager, string description, uint32_t quantity) {
    // Validity checks
    requireAuthorization(manager);
    BAL::Verify(description.size() <= MAX_DESCRIPTION_SIZE,
                "Description size cannot exceed", MAX_DESCRIPTION_SIZE, "characters.");
    auto warehouses = getTable<Warehouses>(GLOBAL_SCOPE);
    const auto& warehouse = warehouses.getId(warehouseId, "Cannot add inventory: specified warehouse does not exist");
    BAL::Verify(warehouse.manager == manager, "Cannot add inventory: authorizing account", manager,
                "is not the manager for warehouse", warehouseId);

    // Create the record
    auto stock = getTable<Stock>(warehouseId);
    InventoryId newId = stock.nextId();
    stock.create(manager, [newId, &description, quantity, origin=currentTransactionId()](Inventory& item) {
        item.id = newId;
        item.description = std::move(description);
        item.origin = std::move(origin);
        item.quantity = quantity;
    });

    BAL::Log("Successfully added item to inventory of warehouse", warehouseId);
}
```
Easy enough. Now we move on to `adj.invntry`.

We use some new types here, so in our header we need to `#include <variant>` and `using std::variant;`. To keep the code clean, we also define a type name for the quantity adjustment like so:
```c++
using Adjustment = variant<uint32_t, int32_t>;
BAL_REFLECT_TYPENAME(Adjustment)
```

And now for the implementation:
```c++
void SupplyChain::adjustInventory(WarehouseId warehouseId, AccountHandle manager, InventoryId inventoryId,
                                  optional<string> newDescription, optional<Adjustment> quantityAdjustment,
                                  string documentation) {
    // Common validity checks
    requireAuthorization(manager);
    BAL::Verify(documentation.size() <= MAX_DESCRIPTION_SIZE,
                "Documentation size cannot exceed", MAX_DESCRIPTION_SIZE, "characters.");
    if (newDescription.has_value())
        BAL::Verify(newDescription->size() <= MAX_DESCRIPTION_SIZE,
                    "New description size cannot exceed", MAX_DESCRIPTION_SIZE, "characters.");
    auto warehouses = getTable<Warehouses>(GLOBAL_SCOPE);
    const auto& warehouse = warehouses.getId(warehouseId,
                                             "Cannot adjust inventory: specified warehouse does not exist");
    BAL::Verify(warehouse.manager == manager, "Cannot adjust inventory: authorizing account", manager,
                "is not the manager for warehouse", warehouseId);
    BAL::Verify(newDescription.has_value() || quantityAdjustment.has_value(),
                "Cannot adjust inventory: no adjustment requested");
    
    // Look up inventory record
    auto stock = getTable<Stock>(warehouseId);
    const Inventory& inventory = stock.getId(inventoryId,
                                             "Cannot adjust inventory: specified inventory record does not exist");
    
    // Validate quantity adjustment
    if (quantityAdjustment.has_value() && std::holds_alternative<int32_t>(*quantityAdjustment)) {
        auto& adjustment = std::get<int32_t>(*quantityAdjustment);
        BAL::Verify(adjustment != 0, "Cannot adjust inventory: quantity delta cannot be zero");
        
        // Check negative adjustment and integer overflow
        if (adjustment < 0) {
            BAL::Verify(uint32_t(-adjustment) <= inventory.quantity, "Cannot adjust inventory quantity down by",
                        -adjustment, "because current quantity is only", inventory.quantity);
        } else {
            auto maxAdjustment = std::numeric_limits<decltype(inventory.quantity)>::max() - inventory.quantity;
            BAL::Verify(uint32_t(adjustment) < maxAdjustment, "Cannot adjust inventory quantity up by", adjustment,
                        "because it would overflow the integer");
        }
    }
    
    // Commit the adjustment
    stock.modify(inventory, manager, [&newDescription, &quantityAdjustment](Inventory& inventory) {
        if (newDescription.has_value())
            inventory.description = std::move(*newDescription);
        if (quantityAdjustment.has_value()) {
            if (std::holds_alternative<uint32_t>(*quantityAdjustment))
                inventory.quantity = std::get<uint32_t>(*quantityAdjustment);
            else
                inventory.quantity += std::get<int32_t>(*quantityAdjustment);
        }
    });

    BAL::Log("Successfully adjusted inventory record", inventoryId, "in warehouse", warehouseId);
}
```

Next, we implement RemoveInventory, also known as `rm.invntry`:
```c++
void SupplyChain::removeInventory(WarehouseId warehouseId, AccountHandle manager, InventoryId inventoryId,
                                  uint32_t quantity, bool deleteRecord, string documentation) {
    // Common validity checks
    requireAuthorization(manager);
    BAL::Verify(documentation.size() <= MAX_DESCRIPTION_SIZE,
                "Documentation size cannot exceed", MAX_DESCRIPTION_SIZE, "characters.");
    auto warehouses = getTable<Warehouses>(GLOBAL_SCOPE);
    const auto& warehouse = warehouses.getId(warehouseId,
                                             "Cannot remove inventory: specified warehouse does not exist");
    BAL::Verify(warehouse.manager == manager, "Cannot remove inventory: authorizing account", manager,
                "is not the manager for warehouse", warehouseId);

    // Look up inventory record
    auto stock = getTable<Stock>(warehouseId);
    const Inventory& inventory = stock.getId(inventoryId,
                                             "Cannot remove inventory: specified inventory record does not exist");

    // Check quantity
    if (quantity > 0)
        BAL::Verify(quantity <= inventory.quantity, "Cannot remove inventory: quantity to remove", quantity,
                    "exceeds quantity in stock", inventory.quantity);
    if (deleteRecord)
        BAL::Verify(quantity == 0 || quantity == inventory.quantity,
                    "Cannot remove inventory: record set to be deleted, but quantity in stock is not zero");

    // Commit the change
    if (deleteRecord) {
        stock.erase(inventory);

        BAL::Log("Successfully deleted inventory record", inventoryId, "from warehouse", warehouseId);
    } else {
        stock.modify(inventory, manager, [quantity](Inventory& inventory) {
            if (quantity == 0)
                inventory.quantity = 0;
            else
                inventory.quantity -= quantity;
        });

        BAL::Log("Successfully removed", quantity, "units of stock from inventory record", inventoryId,
                 "in warehouse", warehouseId);
    }
}
```

The remaining actions in the inventory management category all handle map types which are rather cumbersome in a method signature, so we give them nicer names in our header, similarly to how we did above with the `Adjustment` type:
```c++
using PickList = map<InventoryId, uint32_t>;
BAL_REFLECT_TYPENAME(PickList)
using ProductionList = map<string, uint32_t>;
BAL_REFLECT_TYPENAME(ProductionList)
```

Now we can implement our ManufactureInventory action (calling its action `manufacture`). Since we'll be adding new inventory records, which is redundant with the AddInventory action we implemented earlier, we extract the relevant code to a protected method, to avoid duplicate code.

```c++
void SupplyChain::createInventory(WarehouseId warehouseId, AccountHandle payer, string description, uint32_t quantity) {
    auto stock = getTable<Stock>(warehouseId);
    InventoryId newId = stock.nextId();
    stock.create(payer, [newId, &description, quantity, origin=currentTransactionId()](Inventory& item) {
        item.id = newId;
        item.description = std::move(description);
        item.origin = std::move(origin);
        item.quantity = quantity;
    });
}
```
And now for our action:
```c++
void SupplyChain::manufactureInventory(WarehouseId warehouseId, AccountHandle manager, PickList consume,
                                       ProductionList produce, bool deleteConsumed, string documentation) {
    // Common validity checks
    requireAuthorization(manager);
    BAL::Verify(documentation.size() <= MAX_DESCRIPTION_SIZE,
                "Documentation size cannot exceed", MAX_DESCRIPTION_SIZE, "characters.");
    auto warehouses = getTable<Warehouses>(GLOBAL_SCOPE);
    const auto& warehouse = warehouses.getId(warehouseId,
                                             "Cannot manufacture inventory: specified warehouse does not exist");
    BAL::Verify(warehouse.manager == manager, "Cannot manufacture inventory: authorizing account", manager,
                "is not the manager for warehouse", warehouseId);

    // Check that we're actually doing something
    BAL::Verify(consume.size() > 0 || produce.size() > 0,
                "Cannot manufacture inventory if inventory is neither consumed nor produced");
    // Check produced items
    for (const auto& [description, quantity] : produce) {
        BAL::Verify(description.size() <= MAX_DESCRIPTION_SIZE, "Cannot manufacture inventory: produced item "
                    "description longer than max", MAX_DESCRIPTION_SIZE, "characters");
        BAL::Verify(quantity > 0, "Cannot manufacture inventory: cannot produce zero quantity of any item");
    }

    // Look up all inventory records now and store references to them for later
    auto stock = getTable<Stock>(warehouseId);
    auto consumeRecords = map<InventoryId, std::reference_wrapper<const Inventory>>();
    std::transform(consume.begin(), consume.end(), std::inserter(consumeRecords, consumeRecords.begin()),
                   [&stock](const auto& idQuantityPair) {
        auto id = idQuantityPair.first;
        return std::make_pair(id, stock.getId(id, "Cannot manufacture inventory: Inventory ID not found"));
    });

    // Check all consumed inventories have sufficient quantity
    for (auto [id, consumed] : consume) {
        auto available = consumeRecords.at(id).get().quantity;
        BAL::Verify(consumed > 0, "Cannot manufacture inventory: cannot consume zero units of any item");
        BAL::Verify(available >= consumed, "Cannot manufacture inventory: need to consume", consumed, "units of", id,
                    "but only", available, "units in stock");
    }

    // Commit the changes
    // Adjust consumed stock
    for (auto [id, consumed] : consume) {
        // ID should be the first element, so use constant time lookup if possible
        auto itr = consumeRecords.begin()->first == id? consumeRecords.begin() : consumeRecords.find(id);
        const Inventory& inventory = itr->second;

        if (deleteConsumed && consumed == inventory.quantity)
            stock.erase(inventory);
        else
            stock.modify(inventory, manager, [c=consumed](Inventory& inventory) { inventory.quantity -= c; });

        // Remove record from store: we won't need it again
        consumeRecords.erase(itr);
    }
    // Add produced stock
    for (auto [description, produced] : produce)
        createInventory(warehouseId, manager, description, produced);

    BAL::Log("Successfully manufactured", consume.size(), "items into", produce.size(), "other items in warehouse",
             warehouseId);
}
```

Next comes TransferInventory, which we name `xfer.invntry`:
```c++
void SupplyChain::transferInventory(WarehouseId sourceWarehouseId, AccountHandle sourceManager,
                                    WarehouseId destinationWarehouseId, AccountHandle destinationManager,
                                    PickList manifest, bool deleteConsumed, string documentation) {
    // Common validity checks
    requireAuthorization(sourceManager);
    requireAuthorization(destinationManager);
    BAL::Verify(documentation.size() <= MAX_DESCRIPTION_SIZE,
                "Documentation size cannot exceed", MAX_DESCRIPTION_SIZE, "characters.");
    BAL::Verify(sourceWarehouseId != destinationWarehouseId,
                "Cannot transfer inventory: source and destination are the same");
    auto warehouses = getTable<Warehouses>(GLOBAL_SCOPE);
    const auto& sourceWarehouse = warehouses.getId(sourceWarehouseId,
                                                   "Cannot transfer inventory: source warehouse does not exist");
    const auto& destinationWarehouse = warehouses.getId(destinationWarehouseId,
                                                   "Cannot transfer inventory: destination warehouse does not exist");
    BAL::Verify(sourceWarehouse.manager == sourceManager, "Cannot transfer inventory: authorizing account",
                sourceManager, "is not the manager for source warehouse", sourceWarehouseId);
    BAL::Verify(destinationWarehouse.manager == destinationManager, "Cannot transfer inventory: authorizing account",
                destinationManager, "is not the manager for destination warehouse", destinationWarehouseId);

    // Look up the manifest items
    auto sourceStock = getTable<Stock>(sourceWarehouseId);
    map<InventoryId, std::reference_wrapper<const Inventory>> manifestInventory;
    std::transform(manifest.begin(), manifest.end(), std::inserter(manifestInventory, manifestInventory.begin()),
                   [&sourceStock](const auto& idQuantityPair) {
        auto id = idQuantityPair.first;
        return std::make_pair(id, sourceStock.getId(id, "Cannot transfer inventory: inventory ID not found"));
    });

    // Check manifest quantities
    for (auto& [id, transferred] : manifest) {
        auto available = manifestInventory.at(id).get().quantity;
        BAL::Verify(transferred > 0, "Cannot transfer inventory: cannot transfer zero units of", id);
        BAL::Verify(available >= transferred, "Cannot transfer inventory: need to take", transferred, "units of", id,
                    "from warehouse", sourceWarehouseId, "but only", available, "in stock");
    }

    // Update the warehouse stocks
    auto destinationStock = getTable<Stock>(destinationWarehouseId);
    for (auto& [id, transferred] : manifest) {
        // ID should be the first element, so use constant time lookup if possible
        auto itr = manifestInventory.begin()->first == id? manifestInventory.begin() : manifestInventory.find(id);
        const Inventory& inventory = itr->second;

        // Add to destination stock
        InventoryId newId = destinationStock.nextId();
        destinationStock.create(destinationManager, [this, newId, &inventory](Inventory& newRecord) {
            newRecord.id = newId;
            newRecord.description = inventory.description;
            newRecord.origin = inventory.origin;
            newRecord.movement = inventory.movement;
            // Update movement history
            newRecord.movement.push_back(currentTransactionId());
        });

        // Remove from source stock
        if (deleteConsumed && transferred == inventory.quantity)
            sourceStock.erase(inventory);
        else
            sourceStock.modify(inventory, sourceManager, [t=transferred](Inventory& inventory) {
                inventory.quantity -= t;
            });

        // Remove the record; we won't need it again
        manifestInventory.erase(itr);
    }

    BAL::Log("Successfully transferred", manifest.size(), "items from warehouse", sourceWarehouseId, "to warehouse",
             destinationWarehouseId);
}
```

The next action to implement is ShipInventory, but before we can implement it, we must implement the tables that will store the cargo carriers' cargo and manifests. This will involve two tables, one to store the manifests held by a given carrier, and another to store the cargo. The cargo table is unique within this contract, as it is the only one to need a secondary index. Secondary indexes allow us to sort, search, and group records by some other value than their primary key alone. In the case of the cargo table, we want a secondary index to allow us to iterate cargo records by manifest ID.

To track a secondary index on a table, we accessorize the table with a few more declarations so the BAL knows how to track the index. First, we declare a name for our index so we can fetch it later. Then we add a method which returns the field (or a calculation from several fields) that we're indexing on. Lastly, we add a list of secondary indexes to our table, and in each list entry, we specify the index's **name**, the **table**, the **type** of the field it indexes, and the **method** that returns the value to sort on.

We create the tables like so:
```c++
constexpr static auto ManifestTableName = "manifests"_N;
constexpr static auto CargoTableName = "cargo"_N;
/* ... */
using ManifestId = BAL::ID<BAL::NameTag<ManifestTableName>>;
using CargoId = BAL::ID<BAL::NameTag<CargoTableName>>;
/* ... */
    struct Manifest {
        using Contract = SupplyChain;
        constexpr static Name TableName = ManifestTableName;

        ManifestId id;
        string description;
        WarehouseId sender;

        ManifestId primary_key() const { return id; }
    };
    using Manifests = Table<Manifest>;

    struct Cargo {
        using Contract = SupplyChain;
        constexpr static Name TableName = CargoTableName;

        CargoId id;
        ManifestId manifest;
        string description;
        TransactionId origin;
        vector<TransactionId> movement;
        uint32_t quantity = 0;

        CargoId primary_key() const { return id; }
        uint64_t manifest_key() const { return manifest; }

        constexpr static auto ByManifest = "by.manifest"_N;
        using SecondaryIndexes =
                Util::TypeList::List<BAL::SecondaryIndex<ByManifest, Cargo, uint64_t, &Cargo::manifest_key>>;
    };
    using CargoStock = Table<Cargo>;
/* ... */
    using Tables = Util::TypeList::List<Warehouses, Stock, Manifests, CargoStock>;
/* ... */
BAL_REFLECT(SupplyChain::Manifest, (id)(description)(sender))
BAL_REFLECT(SupplyChain::Cargo, (id)(manifest)(description)(origin)(movement)(quantity))
```

Now we are ready to implement the `ship.invntry` action. Along the way, however, we decide that the process of fetching items in a `PickList` and checking the available quantities is overly redundant between our actions, so we extract it into a method and use that method to simplify the implementations of our Manufacture, Transfer, and Ship Inventory actions. To keep the method's interface clean, we define a type alias, in the class by our declaration so we can use the Inventory type: `using PickedItems = map<InventoryId, std::reference_wrapper<const Inventory>>;`. After this, we define our new method and action like so:
```c++
SupplyChain::PickedItems SupplyChain::processPickList(const Stock& stock, PickList list) {
    PickedItems picked;
    for (const auto& [id, required] : list) {
        BAL::Verify(required > 0, "Unable to collect stock: cannot collect zero units of", id);
        PickedItems::value_type pick{id, stock.getId(id, "No such Inventory ID")};
        auto itr = picked.emplace_hint(--picked.end(), std::move(pick));
        auto available = itr->second.get().quantity;
        BAL::Verify(available >= required, "Unable to collect stock: required", required, "units of", id, "but only",
                    available, "units in stock");
    }
    return picked;
}

void SupplyChain::shipInventory(WarehouseId warehouseId, AccountHandle manager, AccountHandle carrier, PickList manifest,
                                bool deleteConsumed, string documentation) {
    // Common checks
    requireAuthorization(manager);
    requireAuthorization(carrier);
    BAL::Verify(documentation.size() <= MAX_DESCRIPTION_SIZE,
                "Documentation length may not exceed", MAX_DESCRIPTION_SIZE, "characters.");
    Warehouses warehouses = getTable<Warehouses>(GLOBAL_SCOPE);
    const Warehouse& warehouse = warehouses.getId(warehouseId,
                                                  "Cannot ship inventory: Specified warehouse does not exist");
    BAL::Verify(warehouse.manager == manager, "Cannot ship inventory: authorizing account", manager,
                "is not the manager of warehouse", warehouseId);

    // Pick the manifest
    auto stock = getTable<Stock>(warehouseId);
    auto pickedManifest = processPickList(stock, manifest);
    
    // Commit the changes
    // Create manifest record for carrier
    auto manifests = getTable<Manifests>(carrier);
    ManifestId newManifestId = manifests.nextId();
    manifests.create(carrier, [newManifestId, &documentation, warehouseId](Manifest& manifest) {
        manifest.id = newManifestId;
        manifest.description = std::move(documentation);
        manifest.sender = warehouseId;
    });
    // Convert warehouse inventory into carrier cargo (load the truck!)
    auto cargoStock = getTable<CargoStock>(carrier);
    for (const auto& [id, shipped] : manifest) {
        // ID should be the first element of pickedManifest, so use constant time lookup if possible
        auto itr = pickedManifest.begin()->first == id? pickedManifest.begin() : pickedManifest.find(id);
        const Inventory& inventory = itr->second;
        pickedManifest.erase(itr);
        
        // Create the new cargo record before possibly deleting the inventory record
        CargoId newId = cargoStock.nextId();
        cargoStock.create(carrier, [newId, newManifestId, &inventory, quantity=shipped, this](Cargo& cargo) {
            cargo.id = newId;
            cargo.manifest = newManifestId;
            cargo.description = inventory.description;
            cargo.quantity = quantity;
            cargo.origin = inventory.origin;
            cargo.movement = inventory.movement;
            // Add shipment to cargo's movement history
            cargo.movement.push_back(currentTransactionId());
        });
        
        // Now remove the inventory
        if (deleteConsumed && shipped == inventory.quantity)
            stock.erase(inventory);
        else
            stock.modify(inventory, manager, [quantity=shipped](Inventory& inventory) {
                inventory.quantity -= quantity;
            });
    }

    BAL::Log("Successfully shipped", manifest.size(), "items of inventory from warehouse", warehouseId,
             "with carrier", carrier);
}
```

The implementation of the inventory management actions are now complete.

##### Cargo Carrier Actions
Finally, we implement the cargo carrier actions. We begin with RemoveCargo, or `rm.cargo`:
```c++
void SupplyChain::removeCargo(AccountHandle carrier, ManifestId manifestId, CargoId cargoId, uint32_t quantity,
                              string documentation) {
    // Validity checks
    requireAuthorization(carrier);
    BAL::Verify(documentation.size() < MAX_DESCRIPTION_SIZE, "Documentation size cannot exceed", MAX_DESCRIPTION_SIZE,
                "characters.");
    BAL::Verify(quantity > 0, "Cannot remove zero units of cargo.");
    Manifests manifests = getTable<Manifests>(carrier);
    const auto& manifest = manifests.getId(manifestId, "Could not remove cargo: specified manifest does not exist");
    CargoStock stock = getTable<CargoStock>(carrier);
    const Cargo& cargo = stock.getId(cargoId, "Could not remove cargo: specified cargo ID does not exist");
    BAL::Verify(cargo.quantity >= quantity, "Could not remove cargo: need to remove", quantity, "units, but only",
                cargo.quantity, "units are held by carrier", carrier);

    // Commit the change
    if (quantity == cargo.quantity) {
        BAL::Log("Deleting cargo entry", cargoId, "as its quantity is now zero");
        stock.erase(cargo);

        // Are there any more cargo entries in this manifest?
        auto byManifest = stock.getSecondaryIndex<Cargo::ByManifest>();
        if (byManifest.find(manifestId) == byManifest.end()) {
            // No more cargo on this manifest. Delete the manifest record too.
            BAL::Log("Deleting manifest", manifestId, "as it no longer contains any cargo");
            manifests.erase(manifest);
        }
    } else {
        stock.modify(cargo, carrier, [quantity](Cargo& cargo) { cargo.quantity -= quantity; });
    }

    BAL::Log("Successfully removed", quantity, "units of cargo", cargoId, "from carrier", carrier,
             "manifest", manifestId);
}
```

We proceed to TransferCargo, or `xfer.cargo`. This action takes a map as one of its arguments, so to keep the interface tidy, we create an alias for it:
```c++
using CargoManifest = map<CargoId, uint32_t>;
BAL_REFLECT_TYPENAME(CargoManifest)
```
Next, in the process of implementing this action, we decide that, once again, the logic of picking the cargo manifest is so similar to the logic of picking an inventory manifest, which we extracted into the `processPickList` method, that we might as well just modify the method so it works here to! Now, we're dealing with different types, yes, but the syntax is exactly identical, so we can easily make a template, and as a bonus, the template arguments can be inferred, so the existing call sites don't even need to be updated!
```c++
    // PickedItems<Item> is a map of an ItemId to a const Item reference (uses reference_wrapper)
    template<typename Item, typename ID = decltype(Item::id)>
    using PickedItems = map<ID, std::reference_wrapper<const Item>>;
    // Pick a map of ID to quantity from the given stock table, giving errors for zero- or over-quantity picks
    // No need to explicitly specify template arguments, they can be inferred!
    template<typename Item, typename ID = decltype(Item::id)>
    PickedItems<Item, ID> processPickList(const Table<Item>& stock, map<ID, uint32_t> list) {
        using Return = PickedItems<Item, ID>;
        Return picked;
        for (const auto& [id, required] : list) {
            BAL::Verify(required > 0, "Unable to collect stock: cannot collect zero units of", id);
            typename Return::value_type pick{id, stock.getId(id, "No such Inventory ID")};
            auto itr = picked.emplace_hint(--picked.end(), std::move(pick));
            auto available = itr->second.get().quantity;
            BAL::Verify(available >= required, "Unable to collect stock: required", required, "units of", id,
                        "but only", available, "units in stock");
        }
        return picked;
    }
```
Excellent, now we can implement our action. Some interesting bug-catching logic comes into play on this one, so look closely:
```c++
void SupplyChain::transferCargo(AccountHandle sourceCarrier, AccountHandle destinationCarrier, ManifestId manifestId,
                                CargoManifest submanifest, string documentation) {
    // Validity checks
    requireAuthorization(sourceCarrier);
    requireAuthorization(destinationCarrier);
    // Defer check that sourceCarrier != destinationCarrier to later, just in case...
    BAL::Verify(documentation.size() <= MAX_DESCRIPTION_SIZE, "Documentation length cannot exceed",
                MAX_DESCRIPTION_SIZE, "characters.");
    Manifests sourceManifests = getTable<Manifests>(sourceCarrier);
    const Manifest& manifest = sourceManifests.getId(manifestId, "Cannot transfer cargo: manifest does not exist");

    // Pick cargo to transfer
    CargoStock sourceStock = getTable<CargoStock>(sourceCarrier);
    PickedItems<Cargo> cargoToGo;
    if (submanifest.empty()) {
        // Transferring entire manifest! Read it all from the secondary index
        auto byManifest = sourceStock.getSecondaryIndex<Cargo::ByManifest>();
        auto range = byManifest.equalRange(manifestId);

        // The manifest should never be empty -- we should always have removed it, but make sure...
        if (range.first == range.second) {
            BAL::Log("BUG DETECTED: Manifest", manifestId, "registered with carrier", sourceCarrier,
                     "but no cargo exists in the manifest. Please report this bug!");
            // Well, there's nothing to transfer... so we're done! But clean up this mess...
            sourceManifests.erase(manifest);
            return;
        }

        // Pick the entire manifest, and update submanifest with the quantities (simplifies logic further in)
        std::transform(range.first, range.second, std::inserter(cargoToGo, cargoToGo.begin()),
                       [&submanifest](const Cargo& cargo) {
            submanifest.emplace(CargoManifest::value_type{cargo.id, cargo.quantity});
            return PickedItems<Cargo>::value_type{cargo.id, cargo};
        });
    } else {
        // Transferring a submanifest! We have a method for this
        cargoToGo = processPickList(sourceStock, submanifest);
    }

    // Now check that source != destination. Why? Well, we're past the empty manifest check now. If ever a carrier
    // had an empty manifest, it would be a bug, and it could also be rather annoying to get rid of. The check above
    // gets rid of it, but only if we get that far. By moving this check to here, we allow a carrier to get rid of
    // his empty manifest (which should never happen, but if it did...) simply by transferring it to himself.
    BAL::Verify(sourceCarrier != destinationCarrier, "Cannot transfer cargo from", sourceCarrier, "to itself.");

    // OK, we've picked our cargo to transfer. All checks passed; let's transfer it!
    // Create destination manifest
    Manifests destinationManifests = getTable<Manifests>(destinationCarrier);
    ManifestId newManifestId = destinationManifests.nextId();
    destinationManifests.create(destinationCarrier,
                                [id=newManifestId, &manifest, &documentation](Manifest& newManifest) {
        newManifest.id = id;
        newManifest.description = std::move(documentation);
        newManifest.sender = manifest.sender;
    });
    // Do the transfer
    CargoStock destinationCargo = getTable<CargoStock>(destinationCarrier);
    for (const auto& [id, quantity] : submanifest) {
        // ID should be the first element of cargoToGo, so use constant time lookup if possible
        auto itr = cargoToGo.begin()->first == id? cargoToGo.begin() : cargoToGo.find(id);
        const Cargo& cargo = itr->second;
        cargoToGo.erase(itr);

        // Technically, the submanifest could've specified cargo not on this manifest. Make sure that's not the case.
        BAL::Verify(cargo.manifest == manifestId, "Cannot transfer cargo: submanifest specifies ID", cargo.id,
                    "but that cargo belongs to manifest", cargo.manifest, "rather than the transfer manifest",
                    manifestId);

        // Populate destination with transferred cargo
        auto newId = destinationCargo.nextId();
        destinationCargo.create(destinationCarrier,
                                [newId, newManifestId, &cargo, transferred=quantity, this](Cargo& newCargo) {
            newCargo.id = newId;
            newCargo.manifest = newManifestId;
            newCargo.description = cargo.description;
            newCargo.quantity = transferred;
            newCargo.origin = cargo.origin;
            newCargo.movement = cargo.movement;
            // Add this transaction to movement history
            newCargo.movement.push_back(currentTransactionId());
        });

        // Remove cargo from source
        if (quantity == cargo.quantity)
            sourceStock.erase(cargo);
        else
            sourceStock.modify(cargo, sourceCarrier, [transferred=quantity](Cargo& cargo) {
                cargo.quantity -= transferred;
            });
    }
    // If source's manifest is now empty, delete it
    auto byManifest = sourceStock.getSecondaryIndex<Cargo::ByManifest>();
    if (!byManifest.contains(manifestId))
        sourceManifests.erase(manifest);

    BAL::Log("Successfully transferred", submanifest.size(), "units of cargo from", sourceCarrier,
             "to", destinationCarrier);
}
```

And now for our final action, DeliverCargo or `dlvr.cargo`:
```c++
void SupplyChain::deliverCargo(AccountHandle carrier, WarehouseId warehouseId, AccountHandle manager, ManifestId manifestId,
                               CargoManifest submanifest, string documentation) {
    // Validity checks
    requireAuthorization(carrier);
    requireAuthorization(manager);
    BAL::Verify(documentation.size() <= MAX_DESCRIPTION_SIZE, "Documentation size cannot exceed",
                MAX_DESCRIPTION_SIZE, "characters.");
    Warehouses warehouses = getTable<Warehouses>(GLOBAL_SCOPE);
    const Warehouse& warehouse = warehouses.getId(warehouseId,
                                                  "Cannot deliver cargo: destination warehouse does not exist");
    BAL::Verify(warehouse.manager == manager, "Cannot deliver cargo from", carrier, "because authorizing account",
                manager, "is not the manager of destination warehouse", warehouseId);
    Manifests manifests = getTable<Manifests>(carrier);
    const Manifest& manifest = manifests.getId(manifestId, "Cannot deliver cargo: specified manifest does not exist");

    // Pick cargo to deliver
    CargoStock sourceStock = getTable<CargoStock>(carrier);
    PickedItems<Cargo> cargoToGo;
    if (submanifest.empty()) {
        // Transferring entire manifest! Read it all from the secondary index
        auto byManifest = sourceStock.getSecondaryIndex<Cargo::ByManifest>();
        auto range = byManifest.equalRange(manifestId);
        // Pick the entire manifest, and update submanifest with the quantities (simplifies logic further in)
        std::transform(range.first, range.second, std::inserter(cargoToGo, cargoToGo.begin()),
                       [&submanifest](const Cargo& cargo) {
            submanifest.emplace(CargoManifest::value_type{cargo.id, cargo.quantity});
            return PickedItems<Cargo>::value_type{cargo.id, cargo};
        });
    } else {
        // Transferring a submanifest! We have a method for this
        cargoToGo = processPickList(sourceStock, submanifest);
    }

    // Commit the delivery
    Stock destinationStock = getTable<Stock>(warehouseId);
    for (const auto& [id, quantity] : submanifest) {
        // ID should be the first element of cargoToGo, so use constant time lookup if possible
        auto itr = cargoToGo.begin()->first == id? cargoToGo.begin() : cargoToGo.find(id);
        const Cargo& cargo = itr->second;
        cargoToGo.erase(itr);

        // Technically, the submanifest could've specified cargo not on this manifest. Make sure that's not the case.
        BAL::Verify(cargo.manifest == manifestId, "Cannot deliver cargo: submanifest specifies ID", cargo.id,
                    "but that cargo belongs to manifest", cargo.manifest, "rather than the delivery manifest",
                    manifestId);

        // Populate warehouse with delivered cargo
        auto newId = destinationStock.nextId();
        destinationStock.create(manager, [newId, &cargo, delivered=quantity, this](Inventory& newInventory) {
            newInventory.id = newId;
            newInventory.description = cargo.description;
            newInventory.quantity = delivered;
            newInventory.origin = cargo.origin;
            newInventory.movement = cargo.movement;
            // Add this transaction to movement history
            newInventory.movement.push_back(currentTransactionId());
        });

        // Remove cargo from source
        if (quantity == cargo.quantity)
            sourceStock.erase(cargo);
        else
            sourceStock.modify(cargo, carrier, [delivered=quantity](Cargo& cargo) {
                cargo.quantity -= delivered;
            });
    }
    // If source's manifest is now empty, delete it
    auto byManifest = sourceStock.getSecondaryIndex<Cargo::ByManifest>();
    if (!byManifest.contains(manifestId))
        manifests.erase(manifest);

    BAL::Log("Successfully delivered", submanifest.size(), "items of cargo from carrier", carrier, "to warehouse",
             warehouseId);
}
```

Now our supply chain contract is fully implemented!


### What Next?
We now have our back-end solution for a decentralized, verifiable, open source inventory and supply chain tracking system (or whatever other contract you envision to build on the BAL!). The next steps are to spin up a blockchain testnet, load in our contract, and write some unit tests to exercise it and verify that the database state is updated correctly.

#### Testing our New Contract
A complete BAL contract testing framework is planned. This framework will be capable of launching a new testnet, setting up accounts, loading the contract, invoking contract actions, and verifying assertions about the database state. At present, however, this is still a work-in-progress. This page will be updated with the solution when it is ready.

#### The Long Road Ahead
Once our contract is tested and deployed on a private or public blockchain, we will have a complete back-end solution around which we can build one or many decentralized applications. Unfortunately, a blockchain and smart contract back-end does not an application make -- nor does it make an application platform. Many problems yet exist before a consistent app can be made which is secured and decentralized to the standards set forth by the back-end. Applications based on this back-end will likely reference records via IPFS, which opens the question of what will those records contain, exactly, and how will they be hosted? Moreover, technology is yet to be developed to securely transport the application code to the user device and provide that app with a secure environment and frameworks to support its runtime operations without compromising the user's privacy and security.

In their quest to provide end-to-end verifiable, zero-compromise solutions to the user, Follow My Vote is working to design and implement solutions to these issues and many others. If you, dear reader, would also like to see the promise of blockchain and decentralized, verifiable technology wholly fulfilled with end-to-end solutions that ensure the integrity of the entire system, back-end infrastructure to front-end app... if you would like to see this potential reach fruition, then perhaps you would like to work with us toward this goal.
If so, we invite you to throw your hat in the ring and try your hand at our [dev challenge](https://followmyvote.com/dev-challenge/). We look forward to seeing your submission!
