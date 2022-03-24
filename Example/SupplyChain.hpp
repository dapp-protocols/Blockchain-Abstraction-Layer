#pragma once

#include <BAL/BAL.hpp>
#include <BAL/Types.hpp>
#include <BAL/Table.hpp>
#include <BAL/Reflect.hpp>

#include <variant>

// Import some common names
using BAL::AccountHandle;
using BAL::TransactionId;
using BAL::ID;
using BAL::Name;
using BAL::Table;
using std::map;
using std::string;
using std::vector;
using std::variant;
using std::optional;

// Give names to all of the tables in our contract
constexpr static auto WarehouseTableName = "warehouses"_N;
constexpr static auto InventoryTableName = "inventory"_N;
constexpr static auto ManifestTableName = "manifests"_N;
constexpr static auto CargoTableName = "cargo"_N;

// Declare unique ID types for all table types, so different ID types can't be cross-assigned
using WarehouseId = BAL::ID<BAL::NameTag<WarehouseTableName>>;
using InventoryId = BAL::ID<BAL::NameTag<InventoryTableName>>;
using ManifestId = BAL::ID<BAL::NameTag<ManifestTableName>>;
using CargoId = BAL::ID<BAL::NameTag<CargoTableName>>;

// Give names to our complex types
using Adjustment = variant<uint32_t, int32_t>;
BAL_REFLECT_TYPENAME(Adjustment)
using PickList = map<InventoryId, uint32_t>;
BAL_REFLECT_TYPENAME(PickList)
using ProductionList = map<string, uint32_t>;
BAL_REFLECT_TYPENAME(ProductionList)
using CargoManifest = map<CargoId, uint32_t>;
BAL_REFLECT_TYPENAME(CargoManifest)

class [[eosio::contract("supplychain")]] SupplyChain : public BAL::Contract {
public:
    using BAL::Contract::Contract;

    // Warehouse management actions
    [[eosio::action("add.wrhs")]]
    void addWarehouse(AccountHandle manager, string description);
    [[eosio::action("update.wrhs")]]
    void updateWarehouse(AccountHandle manager, WarehouseId warehouseId, optional<AccountHandle> newManager,
                         optional<string> newDescription, string documentation);
    [[eosio::action("delete.wrhs")]]
    void deleteWarehouse(WarehouseId warehouseId, AccountHandle manager, bool removeInventory, string documentation);

    // Inventory management actions
    [[eosio::action("add.invntry")]]
    void addInventory(WarehouseId warehouseId, AccountHandle manager, string description, uint32_t quantity);
    [[eosio::action("adj.invntry")]]
    void adjustInventory(WarehouseId warehouseId, AccountHandle manager, InventoryId inventoryId,
                         optional<string> newDescription, optional<Adjustment> quantityAdjustment,
                         string documentation);
    [[eosio::action("rm.invntry")]]
    void removeInventory(WarehouseId warehouseId, AccountHandle manager, InventoryId inventoryId,
                         uint32_t quantity, bool deleteRecord, string documentation);
    [[eosio::action("manufacture")]]
    void manufactureInventory(WarehouseId warehouseId, AccountHandle manager, PickList consume,
                              ProductionList produce, bool deleteConsumed, string documentation);
    [[eosio::action("xfer.invntry")]]
    void transferInventory(WarehouseId sourceWarehouseId, AccountHandle sourceManager,
                           WarehouseId destinationWarehouseId, AccountHandle destinationManager,
                           PickList manifest, bool deleteConsumed, string documentation);
    [[eosio::action("ship.invntry")]]
    void shipInventory(WarehouseId warehouseId, AccountHandle manager, AccountHandle carrier,
                       PickList manifest, bool deleteConsumed, string documentation);

    // Cargo carrier actions
    [[eosio::action("rm.cargo")]]
    void removeCargo(AccountHandle carrier, ManifestId manifestId, CargoId cargoId, uint32_t quantity,
                     string documentation);
    [[eosio::action("xfer.cargo")]]
    void transferCargo(AccountHandle sourceCarrier, AccountHandle destinationCarrier, ManifestId manifestId,
                       CargoManifest submanifest, string documentation);
    [[eosio::action("dlvr.cargo")]]
    void deliverCargo(AccountHandle carrier, WarehouseId warehouseId, AccountHandle manager,
                      ManifestId manifestId, CargoManifest submanifest, string documentation);

    [[eosio::action("tests.run")]]
    void runTests();

private:
    // Declare a constant for our global scope ID. This number is arbitrary; we just want to be consistent.
    constexpr static auto GLOBAL_SCOPE = "global"_N.value;

    void clean(const AccountHandle& carrier);

    void testWarehouseLifecycle1();
    void testInventoryLifecycle1();
    void testInventoryLifecycle2();
    void testShipAndDeliver1();
    void testShipAndDeliver2();
    void testShipAndDeliver3();

public:
    struct [[eosio::table("warehouses")]] Warehouse {
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

    struct [[eosio::table("inventory")]] Inventory {
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

    struct [[eosio::table("manifests")]] Manifest {
        using Contract = SupplyChain;
        constexpr static Name TableName = ManifestTableName;

        ManifestId id;
        string description;
        WarehouseId sender;

        ManifestId primary_key() const { return id; }
    };
    using Manifests = Table<Manifest>;

    struct [[eosio::table("cargo")]] Cargo {
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

    using Actions = Util::TypeList::List<DESCRIBE_ACTION("add.wrhs"_N, SupplyChain::addWarehouse),
                                         DESCRIBE_ACTION("update.wrhs"_N, SupplyChain::updateWarehouse),
                                         DESCRIBE_ACTION("delete.wrhs"_N, SupplyChain::deleteWarehouse),
                                         DESCRIBE_ACTION("add.invntry"_N, SupplyChain::addInventory),
                                         DESCRIBE_ACTION("adj.invntry"_N, SupplyChain::adjustInventory),
                                         DESCRIBE_ACTION("rm.invntry"_N, SupplyChain::removeInventory),
                                         DESCRIBE_ACTION("manufacture"_N, SupplyChain::manufactureInventory),
                                         DESCRIBE_ACTION("xfer.invntry"_N, SupplyChain::transferInventory),
                                         DESCRIBE_ACTION("ship.invntry"_N, SupplyChain::shipInventory),
                                         DESCRIBE_ACTION("rm.cargo"_N, SupplyChain::removeCargo),
                                         DESCRIBE_ACTION("xfer.cargo"_N, SupplyChain::transferCargo),
                                         DESCRIBE_ACTION("dlvr.cargo"_N, SupplyChain::deliverCargo),
                                         DESCRIBE_ACTION("tests.run"_N, SupplyChain::runTests)>;
    using Tables = Util::TypeList::List<Warehouses, Stock, Manifests, CargoStock>;

    protected:
    void createInventory(WarehouseId warehouseId, AccountHandle payer, string description, uint32_t quantity);

    template<typename Item, typename ID = decltype(Item::id)>
    using PickedItems = map<ID, std::reference_wrapper<const Item>>;
    template<typename Item, typename ID = decltype(Item::id)>
    PickedItems<Item, ID> processPickList(const Table<Item>& stock, map<ID, uint32_t> list) {
        using Return = PickedItems<Item, ID>;
        Return picked;
        for (const auto& [id, required] : list) {
            BAL::Verify(required > 0, "Unable to collect stock: cannot collect zero units of", id);
            typename Return::value_type pick{id, stock.getId(id, "No such Inventory ID")};
            auto itr = picked.emplace_hint(picked.end(), std::move(pick));
            auto available = itr->second.get().quantity;
            BAL::Verify(available >= required, "Unable to collect stock: required", required, "units of", id,
                        "but only", available, "units in stock");
        }

        return picked;
    }
};

// Reflect the table fields
BAL_REFLECT(SupplyChain::Warehouse, (id)(manager)(description))
BAL_REFLECT(SupplyChain::Inventory, (id)(description)(origin)(movement)(quantity))
BAL_REFLECT(SupplyChain::Manifest, (id)(description)(sender))
BAL_REFLECT(SupplyChain::Cargo, (id)(manifest)(description)(origin)(movement)(quantity))
