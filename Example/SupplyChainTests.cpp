#include "SupplyChain.hpp"

/*
 * Tests
 */
void SupplyChain::runTests() {
    // Require the contract's authority to run tests
    requireAuthorization(ownerAccount());
    BAL::Log("\n\nRunning tests");

    testWarehouseLifecycle1();
    testInventoryLifecycle1();
    testInventoryLifecycle2();
    testShipAndDeliver1();
    testShipAndDeliver2();
    testShipAndDeliver3();

    BAL::Log("\n\nFinished running tests");
}

// Seek a warehouse solely by its description
// An empty search result will be indicated by a nullptr
const SupplyChain::Warehouse* seekWarehouse(SupplyChain::Warehouses& warehouses, const std::string& warehouseDesc) {
    for (auto itr = warehouses.begin(); itr != warehouses.end(); itr++)
        if (itr->description == warehouseDesc)
            return &*itr;

    return nullptr;
}

// Seek inventory at a warehouse by its description and quantity
// An empty search result will be indicated by a nullptr
const InventoryId* seekInv(SupplyChain::Stock& stock, const std::string& invDesc, const uint32_t& invQty) {
    BAL::Verify(stock.begin() != stock.end(), "Warehouse inventory could not be found");

    for (auto itr = stock.begin(); itr != stock.end(); itr++)
        if ( (itr->quantity == invQty) && (itr->description == invDesc) )
            return &itr->id;

    return nullptr;
}

// Seek through cargo stock for any items belonging to a manifest
// An empty search result will be indicated by an empty vector
const vector<CargoId> seekManifestCargo(SupplyChain::CargoStock& stock, const ManifestId manifestId) {
    auto byManifest = stock.getSecondaryIndex<SupplyChain::Cargo::ByManifest>();
    auto range = byManifest.equalRange(manifestId);

    std::vector<CargoId> foundCargo;
    for (auto itr = range.first; itr != range.second; itr++)
        foundCargo.push_back(itr->id);

    return foundCargo;
}

// Seek through cargo stock for by its description and quantity
// An empty search result will be indicated by a nullptr
const CargoId* seekManifestCargo(SupplyChain::CargoStock& stock, const ManifestId manifestId,
                                   const std::string& invDesc, const uint32_t& invQty) {
    auto byManifest = stock.getSecondaryIndex<SupplyChain::Cargo::ByManifest>();
    auto range = byManifest.equalRange(manifestId);

    for (auto itr = range.first; itr != range.second; itr++)
        if ( (itr->quantity == invQty) && (itr->description == invDesc) )
            return &itr->id;

    return nullptr;
}

// Seek a manifest by its source warehouse and manifest description
// An empty search result will be indicated by a nullptr
const ManifestId* seekManifest(SupplyChain::Manifests& manifests,
                                const WarehouseId& warehouseId,
                                const std::string& manifestDesc) {
    for (auto itr = manifests.begin(); itr != manifests.end(); itr++)
        if ( (itr->sender == warehouseId) && (itr->description == manifestDesc) )
            return &(itr->id);

    return nullptr;
}

// Check whether a cargo item matches a description and quantity
bool isCargoMatch(const string& invDesc, const uint32_t invQty, SupplyChain::CargoStock& carrierStock, const CargoId& cargoId) {
    const SupplyChain::Cargo& cargo = *carrierStock.findId(cargoId);
    bool isMatch = (cargo.description == invDesc) &&
                    (cargo.quantity == invQty);

    return isMatch;
}

// Check whether a cargo description and quantity can be found in a manifest
bool isCargoInManifest(const string& invDesc, const uint32_t invQty,
                          SupplyChain::CargoStock& carrierStock, const ManifestId manifestId) {

    auto byManifest = carrierStock.getSecondaryIndex<SupplyChain::Cargo::ByManifest>();
    auto range = byManifest.equalRange(manifestId);

    for (auto itr = range.first; itr != range.second; itr++) {
        bool isMatch = (itr->description == invDesc) && (itr->quantity == invQty);
        if (isMatch)
            return true;
    }

    return false;
}


/// Delete all manifests and cargo belonging to the carrier
void SupplyChain::clean(const AccountHandle& carrier) {
    {
        Manifests manifests = getTable<Manifests>(carrier);
        auto itr = manifests.begin();
        while (itr != manifests.end()) {
            itr = manifests.erase(itr);
        }
    }

    {
        CargoStock stock = getTable<CargoStock>(carrier);
        auto itr = stock.begin();
        while (itr != stock.end()) {
            itr = stock.erase(itr);
        }
    }
}

// Test the addition, modification, and deletion of a warehouse.
// The test presumes the existence of an account named "test.alice" and "test.bob".
void SupplyChain::testWarehouseLifecycle1() {
    ///
    /// Initialize Test
    ///
    // Require the contract's authority to run tests
    requireAuthorization(ownerAccount());
    BAL::Log("\n\nTesting Warehouse Lifecycle");

    // Initialize values for the test
    BAL::AccountName manager1 = "test.alice"_N;
    const std::string warehouse1Name = "Alice's Test Warehouse 1";
    BAL::AccountName manager2 = "test.bob"_N;

    // Verify the existence of blockchain accounts needed for the test
    BAL::Verify(accountExists(manager1), "Test manager account 1 does not exist");
    BAL::Verify(accountExists(manager2), "Test manager account 2 does not exist");
    // Ensure that the warehouse does not exist
    {
        Warehouses warehouses = getTable<Warehouses>(GLOBAL_SCOPE);

        const Warehouse* w = seekWarehouse(warehouses, warehouse1Name);
        BAL::Verify(w == nullptr, "The warehouse should not exist at the start of the test!");
    }


    ///
    /// Add a warehouse
    ///
    WarehouseId warehouse1Id;
    {
        Warehouses warehouses = getTable<Warehouses>(GLOBAL_SCOPE);
        addWarehouse(manager1, warehouse1Name);

        // Verify the presence of the new warehouse
        BAL::Log("=> Checking existence of new warehouse");
        const Warehouse* w = seekWarehouse(warehouses, warehouse1Name);
        BAL::Verify(w != nullptr && w->manager == manager1, "The new warehouse was not found!");

        warehouse1Id = w->id;
    }


    ///
    // Update the warehouse's description
    ///
    const std::string warehouse1UpdatedName = "Alice's Improved Test Warehouse 1";

    // Update the warehouse description
    {
        optional<AccountHandle> nullManager;
        updateWarehouse(manager1, warehouse1Id, nullManager, warehouse1UpdatedName, "Testing");

        Warehouses warehouses = getTable<Warehouses>(GLOBAL_SCOPE);
        const Warehouse& warehouse1 = warehouses.getId(warehouse1Id, "Could not find the new warehouse");
        BAL::Verify(warehouse1.manager == manager1, "The warehouse description was not retained as expected");
        BAL::Verify(warehouse1.description == warehouse1UpdatedName, "The warehouse description was not updated as expected");
    }

    // Update the warehouse's manager to manager2
    {
        optional<string> nullDescription;
        updateWarehouse(manager1, warehouse1Id, manager2, nullDescription, "Testing");

        Warehouses warehouses = getTable<Warehouses>(GLOBAL_SCOPE);
        const Warehouse& warehouse1 = warehouses.getId(warehouse1Id, "Could not find the new warehouse");
        BAL::Verify(warehouse1.manager == manager2, "The warehouse description was not updated as expected");
        BAL::Verify(warehouse1.description == warehouse1UpdatedName, "The warehouse description was not retained as expected");
    }

    // Revert the warehouse back to its original description and manager
    {
        optional<string> newDescription = warehouse1Name;
        updateWarehouse(manager2, warehouse1Id, manager1, warehouse1Name, "Testing");

        Warehouses warehouses = getTable<Warehouses>(GLOBAL_SCOPE);
        const Warehouse& warehouse1 = warehouses.getId(warehouse1Id, "Could not find the new warehouse");
        BAL::Verify(warehouse1.manager == manager1, "The warehouse description was not updated as expected");
        BAL::Verify(warehouse1.description == warehouse1Name, "The warehouse description was not updated as expected");
    }


    ///
    /// Clean the test artifacts
    ///
    const bool removeInventory = true;
    deleteWarehouse(warehouse1Id, manager1, removeInventory, "Unit test");

    BAL::Log("Test: PASSED");
}


// Test the addition, manufacture, and transferring of inventory between warehouses
// The test presumes the existence of an account named "test.alice" and "test.bob".
void SupplyChain::testInventoryLifecycle1() {
    ///
    /// Initialize Test
    ///
    // Require the contract's authority to run tests
    requireAuthorization(ownerAccount());
    BAL::Log("\n\nTesting Warehouse Lifecycle");

    // Initialize values for the test
    BAL::AccountName manager1 = "test.alice"_N;
    const std::string warehouse1Name = "Alice's Test Warehouse 1";
    BAL::AccountName manager2 = "test.bob"_N;
    const std::string warehouse2Name = "Bob's Test Warehouse 1";

    // Verify the existence of blockchain accounts needed for the test
    BAL::Verify(accountExists(manager1), "Test manager account 1 does not exist");
    BAL::Verify(accountExists(manager2), "Test manager account 2 does not exist");
    // Ensure that the warehouses do not exist
    {
        Warehouses warehouses = getTable<Warehouses>(GLOBAL_SCOPE);

        const Warehouse* w1 = seekWarehouse(warehouses, warehouse1Name);
        BAL::Verify(w1 == nullptr, "The warehouse should not exist at the start of the test!");

        const Warehouse* w2 = seekWarehouse(warehouses, warehouse2Name);
        BAL::Verify(w2 == nullptr, "The warehouse should not exist at the start of the test!");
    }


    ///
    /// Add warehouses
    ///
    BAL::Log("=> Adding Warehouse 1");
    WarehouseId warehouse1Id;
    {
        Warehouses warehouses = getTable<Warehouses>(GLOBAL_SCOPE);
        addWarehouse(manager1, warehouse1Name);

        // Verify the presence of the new warehouse
        const Warehouse* w1 = seekWarehouse(warehouses, warehouse1Name);
        BAL::Verify(w1 != nullptr && w1->manager == manager1, "The new warehouse was not found!");

        warehouse1Id = w1->id;
    }

    BAL::Log("=> Adding Warehouse 2");
    WarehouseId warehouse2Id;
    {
        Warehouses warehouses = getTable<Warehouses>(GLOBAL_SCOPE);
        addWarehouse(manager2, warehouse2Name);

        // Verify the presence of the new warehouse
        const Warehouse* w2 = seekWarehouse(warehouses, warehouse2Name);
        BAL::Verify(w2 != nullptr && w2->manager == manager2, "The new warehouse was not found!");

        warehouse2Id = w2->id;
    }


    ///
    // Add inventory to Warehouse 1
    ///
    BAL::Log("=> Adding inventory to Warehouse 1");
    const std::string lumberDesc = "Lumber";
    const uint32_t initialQtyLumber = 10;
    const std::string graphiteDesc = "Graphite";
    const uint32_t initialQtyGraphite = 7;
    {
        addInventory(warehouse1Id, manager1, lumberDesc, initialQtyLumber);
        addInventory(warehouse1Id, manager1, graphiteDesc, initialQtyGraphite);
    }

    // Check the inventory
    InventoryId idLumberBatch1, idGraphiteBatch1;
    {
        auto stock1 = getTable<Stock>(warehouse1Id);

        const InventoryId* ptrIdLumberBatch1 = seekInv(stock1, lumberDesc, initialQtyLumber);
        BAL::Verify(ptrIdLumberBatch1 != nullptr, "Lumber inventory is missing from the warehouse");
        idLumberBatch1 = *ptrIdLumberBatch1;

        const InventoryId* ptrIdGraphiteBatch1 = seekInv(stock1, graphiteDesc, initialQtyGraphite);
        BAL::Verify(ptrIdGraphiteBatch1 != nullptr, "Graphite inventory is missing from the warehouse");
        idGraphiteBatch1 = *ptrIdGraphiteBatch1;
    }


    ///
    // Manufacture inventory in Warehouse 1
    ///
    BAL::Log("=> Manufacture inventory to Warehouse 1");
    const uint32_t qtyManux1Lumber = 1;
    const uint32_t qtyManux1Graphite = 2;
    const std::string pencilDesc = "Pencil";
    const uint32_t pencilInitialQty = 2000;
    {
        PickList consumptionList;
        consumptionList.insert(std::make_pair(idLumberBatch1, qtyManux1Lumber));
        consumptionList.insert(std::make_pair(idGraphiteBatch1, qtyManux1Graphite));

        ProductionList productionList;
        productionList.insert(std::make_pair(pencilDesc, pencilInitialQty));

        const bool deleteConsumed = true;
        manufactureInventory(warehouse1Id, manager1, consumptionList,
                                       productionList, deleteConsumed, "Testing");
    }

    // Check the inventory
    InventoryId idPencilBatch1;
    {
        auto stock1 = getTable<Stock>(warehouse1Id);

        const InventoryId* ptrIdPencilBatch1 = seekInv(stock1, pencilDesc, pencilInitialQty);
        BAL::Verify(ptrIdPencilBatch1 != nullptr, "Pencil inventory is missing from the warehouse");
        idPencilBatch1 = *ptrIdPencilBatch1;
    }


    ///
    // Transfer some inventory from Warehouse 1 to Warehouse 2
    ///
    BAL::Log("=> Transfer inventory from Warehouse 1 to Warehouse 2");
    {
        PickList manifestList;
        manifestList.insert(std::make_pair(idPencilBatch1, pencilInitialQty));

        const bool deleteConsumed = true;
        transferInventory(warehouse1Id, manager1,
                          warehouse2Id, manager2,
                          manifestList, deleteConsumed, "Testing");
    }

    // Check inventory at Warehouse 1
    {
        auto stock1 = getTable<Stock>(warehouse1Id);

        const InventoryId* ptrIdRemainingLumber = seekInv(stock1, lumberDesc, initialQtyLumber - qtyManux1Lumber);
        BAL::Verify(ptrIdRemainingLumber != nullptr, "The quantity of lumber remaining at Warehouse 1 is not as expected!");

        const InventoryId* ptrIdRemainingGraphite = seekInv(stock1, graphiteDesc, initialQtyGraphite - qtyManux1Graphite);
        BAL::Verify(ptrIdRemainingGraphite != nullptr, "The quantity of graphite remaining at Warehouse 1 is not as expected!");

        // Pencils at warehouse should be completely absent because the transfer
        // should have deleted the entry after its complete consumption
        const InventoryId* ptrIdPencilsAtW1 = seekInv(stock1, pencilDesc, pencilInitialQty);
        BAL::Verify(ptrIdPencilsAtW1 == nullptr, "The pencils transferred from Warehouse 1 should not be present at Warehouse 1!");
    }

    // Check inventory at Warehouse 2
    {
        auto stock2 = getTable<Stock>(warehouse2Id);
        const InventoryId* ptrIdPencilsAtW2 = seekInv(stock2, pencilDesc, pencilInitialQty);
        BAL::Verify(ptrIdPencilsAtW2 != nullptr, "The pencils transferred to Warehouse 2 are missing!");
    }


    ///
    /// Clean the test artifacts
    ///
    const bool removeInventory = true;
    deleteWarehouse(warehouse1Id, manager1, removeInventory, "Unit test");
    deleteWarehouse(warehouse2Id, manager2, removeInventory, "Unit test");

    BAL::Log("Test: PASSED");
}


// Test the addition, adjustment, and removal of inventory from a warehouse
//
// 1. Adjust the description of an inventory item
// 2. Adjust the quantity of an inventory by a relative amount
// 3. Adjust the quantity of an inventory by an absolute amount
// 4. Remove some of an inventory from the warehouse
// 5. Remove all of an inventory from the warehouse but retain the entry
// 6. Remove all of an inventory from the warehouse and remove the entry
//
// The test presumes the existence of an account named "test.alice"
void SupplyChain::testInventoryLifecycle2() {
    ///
    /// Initialize Test
    ///
    // Require the contract's authority to run tests
    requireAuthorization(ownerAccount());
    BAL::Log("\n\nTesting Warehouse Lifecycle 2");

    // Initialize values for the test
    BAL::AccountName manager1 = "test.alice"_N;
    const std::string warehouse1Name = "Alice's Test Warehouse 1";

    // Verify the existence of blockchain accounts needed for the test
    BAL::Verify(accountExists(manager1), "Test manager account 1 does not exist");
    // Ensure that the warehouses do not exist
    {
        Warehouses warehouses = getTable<Warehouses>(GLOBAL_SCOPE);

        const Warehouse* w1 = seekWarehouse(warehouses, warehouse1Name);
        BAL::Verify(w1 == nullptr, "The warehouse should not exist at the start of the test!");
    }


    ///
    /// Add warehouses
    ///
    BAL::Log("=> Adding Warehouse 1");
    WarehouseId warehouse1Id;
    {
        Warehouses warehouses = getTable<Warehouses>(GLOBAL_SCOPE);
        addWarehouse(manager1, warehouse1Name);

        // Verify the presence of the new warehouse
        const Warehouse* w1 = seekWarehouse(warehouses, warehouse1Name);
        BAL::Verify(w1 != nullptr && w1->manager == manager1, "The new warehouse was not found!");

        warehouse1Id = w1->id;
    }


    ///
    // Add inventory to Warehouse 1
    ///
    BAL::Log("=> Adding inventory to Warehouse 1");
    const std::string lumberDesc = "Lumber";
    const uint32_t initialQtyLumber = 10;
    const std::string graphiteDesc = "Graphite";
    const uint32_t initialQtyGraphite = 7;
    {
        addInventory(warehouse1Id, manager1, lumberDesc, initialQtyLumber);
        addInventory(warehouse1Id, manager1, graphiteDesc, initialQtyGraphite);
    }

    // Check the inventory
    InventoryId idLumberBatch1, idGraphiteBatch1;
    {
        auto stock1 = getTable<Stock>(warehouse1Id);

        const InventoryId* ptrIdLumberBatch1 =
            seekInv(stock1, lumberDesc, initialQtyLumber);
        BAL::Verify(ptrIdLumberBatch1 != nullptr,
                    "Lumber inventory is missing from the warehouse");
        idLumberBatch1 = *ptrIdLumberBatch1;

        const InventoryId* ptrIdGraphiteBatch1 =
            seekInv(stock1, graphiteDesc, initialQtyGraphite);
        BAL::Verify(ptrIdGraphiteBatch1 != nullptr,
                    "Graphite inventory is missing from the warehouse");
        idGraphiteBatch1 = *ptrIdGraphiteBatch1;
    }


    ///
    /// 1. Adjust the description of an inventory item
    ///
    const std::string graphiteDesc2 = "New and Improved Graphite!";
    {
        optional<Adjustment> noQtyAdjustment;
        adjustInventory(warehouse1Id, manager1, idGraphiteBatch1,
                         graphiteDesc2, noQtyAdjustment,
                         "Testing change of inventory description");
    }

    // Check the inventory
    {
        auto stock1 = getTable<Stock>(warehouse1Id);

        // Check the inventory of the adjusted item by its new description
        const InventoryId* ptrIdGraphiteDesc2 =
            seekInv(stock1, graphiteDesc2, initialQtyGraphite);
        BAL::Verify(ptrIdGraphiteDesc2 != nullptr,
                    "Graphite inventory is missing from the warehouse");
        BAL::Verify(*ptrIdGraphiteDesc2 == idGraphiteBatch1,
                    "The inventory ID of the adjusted item should not have changed!");

        // Check the inventory of the adjusted item by its old description
        const InventoryId* ptrIdGraphiteDesc1 =
            seekInv(stock1, graphiteDesc, initialQtyGraphite);
        BAL::Verify(ptrIdGraphiteDesc1 == nullptr,
                    "Graphite inventory by old description should not have been found in the warehouse");

        // Check the unmodified inventory
        const InventoryId* ptrIdLumberBatch1 =
            seekInv(stock1, lumberDesc, initialQtyLumber);
        BAL::Verify(ptrIdLumberBatch1 != nullptr,
                    "Lumber inventory is missing from the warehouse");
    }


    ///
    /// 2. Adjust the quantity of an inventory by a relative amount
    ///
    // Reduce the amount by 5 units
    const int32_t qtyRelAmount = -5; // Relative adjustments require int32_t
    {
        optional<string> noDescAdjustment;
        optional<Adjustment> qtyAdj = qtyRelAmount;
        adjustInventory(warehouse1Id, manager1, idGraphiteBatch1,
                         noDescAdjustment, qtyAdj,
                         "Testing change of inventory description");
    }

    // Check the inventory
    {
        auto stock1 = getTable<Stock>(warehouse1Id);

        // Check the inventory of the adjusted item by its new quantity
        const InventoryId* ptrIdGraphiteDesc2 =
            seekInv(stock1, graphiteDesc2, initialQtyGraphite + qtyRelAmount);
        BAL::Verify(ptrIdGraphiteDesc2 != nullptr,
                    "Graphite inventory is missing from the warehouse");
        BAL::Verify(*ptrIdGraphiteDesc2 == idGraphiteBatch1,
                    "The inventory ID of the adjusted item should not have changed!");

        // Check the inventory of the adjusted item by its old quantity
        const InventoryId* ptrIdGraphiteDesc1 =
            seekInv(stock1, graphiteDesc2, initialQtyGraphite);
        BAL::Verify(ptrIdGraphiteDesc1 == nullptr,
                    "Graphite inventory by old quantity should not have been found in the warehouse");

        // Check the unmodified inventory
        const InventoryId* ptrIdLumberBatch1 =
            seekInv(stock1, lumberDesc, initialQtyLumber);
        BAL::Verify(ptrIdLumberBatch1 != nullptr,
                    "Lumber inventory is missing from the warehouse");
    }


    ///
    /// 3. Adjust the quantity of an inventory by an absolute amount
    ///
    // Set the amount to 5 units
    const uint32_t qtyAbsoluteAmount = +50; // Absolute adjustments require uint32_t
    {
        optional<string> noDescAdjustment;
        optional<Adjustment> qtyAdj = qtyAbsoluteAmount;
        adjustInventory(warehouse1Id, manager1, idGraphiteBatch1,
                         noDescAdjustment, qtyAdj,
                         "Testing change of inventory description");
    }

    // Check the inventory
    {
        auto stock1 = getTable<Stock>(warehouse1Id);

        // Check the inventory of the adjusted item by its new quantity
        const InventoryId* ptrIdGraphiteDesc2 =
            seekInv(stock1, graphiteDesc2, qtyAbsoluteAmount);
        BAL::Verify(ptrIdGraphiteDesc2 != nullptr,
                    "Graphite inventory is missing from the warehouse");
        BAL::Verify(*ptrIdGraphiteDesc2 == idGraphiteBatch1,
                    "The inventory ID of the adjusted item should not have changed!");

        // Check the inventory of the adjusted item by its old quantity
        const InventoryId* ptrIdGraphiteDesc1 =
            seekInv(stock1, graphiteDesc2, initialQtyGraphite + qtyRelAmount);
        BAL::Verify(ptrIdGraphiteDesc1 == nullptr,
                    "Graphite inventory by old quantity should not have been found in the warehouse");

        // Check the unmodified inventory
        const InventoryId* ptrIdLumberBatch1 =
            seekInv(stock1, lumberDesc, initialQtyLumber);
        BAL::Verify(ptrIdLumberBatch1 != nullptr,
                    "Lumber inventory is missing from the warehouse");
    }


    ///
    /// 4. Remove some of an inventory from the warehouse
    ///
    const uint32_t qtyToRemove = 13;
    {
        const bool deleteRecord = false;
        removeInventory(warehouse1Id, manager1, idGraphiteBatch1,
                        qtyToRemove, deleteRecord, "Removing 5 units of graphite");
    }

    // Check the inventory
    {
        auto stock1 = getTable<Stock>(warehouse1Id);

        // Check the inventory of the adjusted item by its new quantity
        const InventoryId* ptrIdGraphiteDesc2 =
            seekInv(stock1, graphiteDesc2, qtyAbsoluteAmount - qtyToRemove);
        BAL::Verify(ptrIdGraphiteDesc2 != nullptr,
                    "Graphite inventory is missing from the warehouse");
        BAL::Verify(*ptrIdGraphiteDesc2 == idGraphiteBatch1,
                    "The inventory ID of the adjusted item should not have changed!");

        // Check the inventory of the adjusted item by its old quantity
        const InventoryId* ptrIdGraphiteDesc1 =
            seekInv(stock1, graphiteDesc2, qtyAbsoluteAmount);
        BAL::Verify(ptrIdGraphiteDesc1 == nullptr,
                    "Graphite inventory by old quantity should not have been found in the warehouse");

        // Check the unmodified inventory
        const InventoryId* ptrIdLumberBatch1 =
            seekInv(stock1, lumberDesc, initialQtyLumber);
        BAL::Verify(ptrIdLumberBatch1 != nullptr,
                    "Lumber inventory is missing from the warehouse");
    }


    ///
    /// 5. Remove all of an inventory from the warehouse but retain the entry
    ///
    const uint32_t qtyRemoveEverything = qtyAbsoluteAmount - qtyToRemove;
    {
        const bool deleteRecord = false; // Retain the inventory entry despite depleting it
        removeInventory(warehouse1Id, manager1, idGraphiteBatch1,
                        qtyRemoveEverything, deleteRecord,
                        "Removing remaining units of graphite");
    }

    // Check the inventory
    {
        auto stock1 = getTable<Stock>(warehouse1Id);

        // Check the inventory of the adjusted item by its new quantity
        const InventoryId* ptrIdGraphiteDesc2 =
            seekInv(stock1, graphiteDesc2, 0);
        BAL::Verify(ptrIdGraphiteDesc2 != nullptr,
                    "Graphite inventory is missing from the warehouse");
        BAL::Verify(*ptrIdGraphiteDesc2 == idGraphiteBatch1,
                    "The inventory ID of the adjusted item should not have changed!");

        // Check the inventory of the adjusted item by its old quantity
        const InventoryId* ptrIdGraphiteDesc1 =
            seekInv(stock1, graphiteDesc2, qtyAbsoluteAmount - qtyToRemove);
        BAL::Verify(ptrIdGraphiteDesc1 == nullptr,
                    "Graphite inventory by old quantity should not have been found in the warehouse");

        // Check the unmodified inventory
        const InventoryId* ptrIdLumberBatch1 =
            seekInv(stock1, lumberDesc, initialQtyLumber);
        BAL::Verify(ptrIdLumberBatch1 != nullptr,
                    "Lumber inventory is missing from the warehouse");
    }


    ///
    // 6. Remove all of an inventory from the warehouse and remove the entry
    ///
    {
        const bool deleteRecord = true; // Remove the inventory entry after depleting it
        removeInventory(warehouse1Id, manager1, idLumberBatch1,
                        initialQtyLumber, deleteRecord,
                        "Removing remaining units of lumber");
    }

    // Check the inventory
    {
        auto stock1 = getTable<Stock>(warehouse1Id);

        // Check the inventory of the removed item
        const InventoryId* ptrIdNewLumber =
            seekInv(stock1, lumberDesc, 0);
        BAL::Verify(ptrIdNewLumber == nullptr,
                    "Lumber inventory should have been removed from the warehouse");

        // Check the inventory of the adjusted item by its old quantity
        const InventoryId* ptrIdOldLumber =
            seekInv(stock1, lumberDesc, initialQtyLumber);
        BAL::Verify(ptrIdOldLumber == nullptr,
                    "Lumber inventory should have been removed from the warehouse");

        // Check the unmodified depleted inventory which should still be present
        const InventoryId* ptrIdGraphiteDesc2 =
            seekInv(stock1, graphiteDesc2, 0);
        BAL::Verify(ptrIdGraphiteDesc2 != nullptr,
                    "Graphite inventory is missing from the warehouse");
    }


    ///
    /// Clean the test artifacts
    ///
    const bool removeInventory = true;
    deleteWarehouse(warehouse1Id, manager1, removeInventory, "Unit test");

    BAL::Log("Test: PASSED");
}


/**
 * Test delivery of inventory from Warehouse 1 to Warehouse 2.
 * The test presumes the existence of an account named "test.alice", "test.bob", and "test.trains".
 */
void SupplyChain::testShipAndDeliver1() {
    ///
    /// Initialize Test
    ///
    // Require the contract's authority to run tests
    requireAuthorization(ownerAccount());
    BAL::Log("\n\nTesting Ship and Deliver 1");

    // Initialize values for the test
    BAL::AccountName manager1 = "test.alice"_N;
    const std::string warehouse1Name = "Alice's Test Warehouse 1";
    BAL::AccountName manager2 = "test.bob"_N;
    const std::string warehouse2Name = "Bob's Test Warehouse 1";
    BAL::AccountName trains = "test.trains"_N;

    // Verify the existence of blockchain accounts needed for the test
    BAL::Verify(accountExists(manager1), "Test manager account 1 does not exist");
    BAL::Verify(accountExists(manager2), "Test manager account 2 does not exist");
    BAL::Verify(accountExists(trains), "Test carrier does not exist");

    // Ensure that the warehouses do not exist
    {
        Warehouses warehouses = getTable<Warehouses>(GLOBAL_SCOPE);

        const Warehouse* w1 = seekWarehouse(warehouses, warehouse1Name);
        BAL::Verify(w1 == nullptr, "The warehouse should not exist at the start of the test!");

        const Warehouse* w2 = seekWarehouse(warehouses, warehouse2Name);
        BAL::Verify(w2 == nullptr, "The warehouse should not exist at the start of the test!");
    }

    // Ensure that the carrier has no manifests
    {
        Manifests manifests = getTable<Manifests>(trains);
        BAL::Verify(manifests.begin() == manifests.end(), "No carrier manifests should exist at the start of the test!");
    }

    // Ensure that the carrier has no cargo stock
    {
        CargoStock cargoStock = getTable<CargoStock>(trains);
        BAL::Verify(cargoStock.begin() == cargoStock.end(), "No carrier cargo stock should exist at the start of the test!");
    }



    ///
    /// Add warehouses
    ///

    BAL::Log("=> Adding Warehouse 1");
    WarehouseId warehouse1Id;
    {
        Warehouses warehouses = getTable<Warehouses>(GLOBAL_SCOPE);
        addWarehouse(manager1, warehouse1Name);

        // Verify the presence of the new warehouse
        const Warehouse* w1 = seekWarehouse(warehouses, warehouse1Name);
        BAL::Verify(w1 != nullptr && w1->manager == manager1, "The new warehouse was not found!");

        warehouse1Id = w1->id;
    }

    BAL::Log("=> Adding Warehouse 2");
    WarehouseId warehouse2Id;
    {
        Warehouses warehouses = getTable<Warehouses>(GLOBAL_SCOPE);
        addWarehouse(manager2, warehouse2Name);

        // Verify the presence of the new warehouse
        const Warehouse* w2 = seekWarehouse(warehouses, warehouse2Name);
        BAL::Verify(w2 != nullptr && w2->manager == manager2, "The new warehouse was not found!");

        warehouse2Id = w2->id;
    }


    ///
    /// Add inventory to Warehouse 1
    ///
    BAL::Log("=> Adding inventory to Warehouse 1");
    const std::string lumberDesc = "Lumber";
    const uint32_t initialQtyLumber = 10;
    const std::string graphiteDesc = "Graphite";
    const uint32_t initialQtyGraphite = 7;
    {
        addInventory(warehouse1Id, manager1, lumberDesc, initialQtyLumber);
        addInventory(warehouse1Id, manager1, graphiteDesc, initialQtyGraphite);
    }

    // Check the inventory
    InventoryId idLumberBatch1, idGraphiteBatch1;
    {
        auto stock1 = getTable<Stock>(warehouse1Id);

        const InventoryId* ptrIdLumberBatch1 = seekInv(stock1, lumberDesc, initialQtyLumber);
        BAL::Verify(ptrIdLumberBatch1 != nullptr, "Lumber inventory is missing from the warehouse");
        idLumberBatch1 = *ptrIdLumberBatch1;

        const InventoryId* ptrIdGraphiteBatch1 = seekInv(stock1, graphiteDesc, initialQtyGraphite);
        BAL::Verify(ptrIdGraphiteBatch1 != nullptr, "Graphite inventory is missing from the warehouse");
        idGraphiteBatch1 = *ptrIdGraphiteBatch1;
    }


    ///
    /// Ship all of a single inventory item from Warehouse 1 to a Carrier A
    ///
    BAL::Log("=> Shipping inventory out of Warehouse 1");
    const std::string manifestDesc = "wrhs1-lumber-wrhs2";
    {
        PickList manifest;
        manifest.insert(std::make_pair(idLumberBatch1, initialQtyLumber));

        const bool deleteConsumed = true;
        shipInventory(warehouse1Id, manager1, trains, manifest, deleteConsumed, manifestDesc);
    }

    // Find the manifest ID
    ManifestId manifestId;
    {
        Manifests manifests = getTable<Manifests>(trains);
        const ManifestId* ptrIdManifest = seekManifest(manifests, warehouse1Id, manifestDesc);
        BAL::Verify(ptrIdManifest != nullptr, "The newly created manifest was not found!");
        manifestId = *ptrIdManifest;
    }

    // Check the removal from Warehouse 1
    {
        Stock stock1 = getTable<Stock>(warehouse1Id);

        const InventoryId* ptrIdLumberBatch1 = seekInv(stock1, lumberDesc, 0);
        BAL::Verify(ptrIdLumberBatch1 == nullptr, "Lumber inventory should have been removed from Warehouse 1 but it is still present!");

        // The other inventory should still be present
        const InventoryId* ptrIdGraphiteBatch1 = seekInv(stock1, graphiteDesc, initialQtyGraphite);
        BAL::Verify(ptrIdGraphiteBatch1 != nullptr, "Graphite inventory is missing from the warehouse");
    }

    // Check the addition to the carrier's cargo
    {
        CargoStock carrierStock = getTable<CargoStock>(trains);

        BAL::Verify(isCargoInManifest(lumberDesc, initialQtyLumber, carrierStock, manifestId),
            "Did not find the lumber in the carrier's cargo manifest");

        const vector<CargoId> manifestCargo = seekManifestCargo(carrierStock, manifestId);
        BAL::Verify(manifestCargo.size() == 1, "Only a single item should be found in the cargo manifest");
    }


    ///
    /// Deliver cargo to Warehouse 2
    ///
    BAL::Log("=> Deliver Manifest 1 to Warehouse 2");
    {
        CargoManifest subManifest; // Empty sub-manifest; deliver everything

        deliverCargo(trains, warehouse2Id, manager2, manifestId, subManifest, "Test delivery");
    }

    // Check the reduction to the carrier's cargo
    {
        CargoStock carrierStock = getTable<CargoStock>(trains);

        const vector<CargoId> manifestCargo = seekManifestCargo(carrierStock, manifestId);
        BAL::Verify(manifestCargo.size() == 0, "No items should have been found in the cargo manifest");
    }

    // Check the addition to the warehouse inventory
   {
        Stock stock2 = getTable<Stock>(warehouse2Id);

        const InventoryId* ptrIdLumberBatch1 = seekInv(stock2, lumberDesc, initialQtyLumber);
        BAL::Verify(ptrIdLumberBatch1 != nullptr, "Lumber inventory is missing from Warehouse 2");
    }


    ///
    /// Clean the test artifacts
    ///
    const bool removeInventory = true;
    deleteWarehouse(warehouse1Id, manager1, removeInventory, "Unit test");
    deleteWarehouse(warehouse2Id, manager2, removeInventory, "Unit test");
    clean(trains);

    BAL::Log("Test: PASSED");
}


/**
 * Test delivery of inventory from Warehouse 1 to Warehouse 2 and 3
 * by transferring cargo between carriers.
 *
 * 1. Ship 9 units of Lumber from Warehouse 1 to "trains" carrier
 * 2. Transfer 3 units of Lumber from "trains" to "planes"
 * 3. "trains" delivers 6 units to Warehouse 2
 * 4. "planes" delivers 3 units to Warehouse 3
 *
 * The test presumes the existence of an account named "test.alice", "test.bob",
 * "test.trains", and "test.planes".
 */
void SupplyChain::testShipAndDeliver2() {
    ///
    /// Initialize Test
    ///
    // Require the contract's authority to run tests
    requireAuthorization(ownerAccount());
    BAL::Log("\n\nTesting Ship and Deliver 2");

    // Initialize values for the test
    BAL::AccountName manager1 = "test.alice"_N;
    const std::string warehouse1Name = "Alice's Test Warehouse 1";
    BAL::AccountName manager2 = "test.bob"_N;
    const std::string warehouse2Name = "Bob's Test Warehouse 1";
    BAL::AccountName manager3 = manager2;
    const std::string warehouse3Name = "Bob's Test Warehouse 2";
    BAL::AccountName trains = "test.trains"_N;
    BAL::AccountName planes = "test.planes"_N;

    // Verify the existence of blockchain accounts needed for the test
    BAL::Verify(accountExists(manager1), "Test manager account 1 does not exist");
    BAL::Verify(accountExists(manager2), "Test manager account 2 does not exist");
    BAL::Verify(accountExists(manager3), "Test manager account 3 does not exist");
    BAL::Verify(accountExists(trains), "Test carrier does not exist");
    BAL::Verify(accountExists(planes), "Test carrier does not exist");

    // Ensure that the warehouses do not exist
    {
        Warehouses warehouses = getTable<Warehouses>(GLOBAL_SCOPE);

        const Warehouse* w1 = seekWarehouse(warehouses, warehouse1Name);
        BAL::Verify(w1 == nullptr, "The warehouse should not exist at the start of the test!");

        const Warehouse* w2 = seekWarehouse(warehouses, warehouse2Name);
        BAL::Verify(w2 == nullptr, "The warehouse should not exist at the start of the test!");

        const Warehouse* w3 = seekWarehouse(warehouses, warehouse3Name);
        BAL::Verify(w3 == nullptr, "The warehouse should not exist at the start of the test!");
    }


    // Ensure that the carrier has no cargo
    {
        // Ensure that the carrier has no manifests
        Manifests manifests = getTable<Manifests>(trains);
        BAL::Verify(manifests.begin() == manifests.end(),
            "No carrier manifests should exist at the start of the test!");

        // Ensure that the carrier has no cargo stock
        CargoStock cargoStock = getTable<CargoStock>(trains);
        BAL::Verify(cargoStock.begin() == cargoStock.end(),
            "No carrier cargo stock should exist at the start of the test!");
    }

    // Ensure that the carrier has no cargo
    {
        // Ensure that the carrier has no manifests
        Manifests manifests = getTable<Manifests>(planes);
        BAL::Verify(manifests.begin() == manifests.end(),
            "No carrier manifests should exist at the start of the test!");

        // Ensure that the carrier has no cargo stock
        CargoStock cargoStock = getTable<CargoStock>(planes);
        BAL::Verify(cargoStock.begin() == cargoStock.end(),
            "No carrier cargo stock should exist at the start of the test!");
    }


    ///
    /// Add warehouses
    ///
    BAL::Log("=> Adding Warehouse 1");
    WarehouseId idWarehouse1;
    {
        Warehouses warehouses = getTable<Warehouses>(GLOBAL_SCOPE);
        addWarehouse(manager1, warehouse1Name);

        // Verify the presence of the new warehouse
        const Warehouse* w1 = seekWarehouse(warehouses, warehouse1Name);
        BAL::Verify(w1 != nullptr && w1->manager == manager1, "The new warehouse was not found!");

        idWarehouse1 = w1->id;
    }

    BAL::Log("=> Adding Warehouse 2");
    WarehouseId idWarehouse2;
    {
        Warehouses warehouses = getTable<Warehouses>(GLOBAL_SCOPE);
        addWarehouse(manager2, warehouse2Name);

        // Verify the presence of the new warehouse
        const Warehouse* w2 = seekWarehouse(warehouses, warehouse2Name);
        BAL::Verify(w2 != nullptr && w2->manager == manager2, "The new warehouse was not found!");

        idWarehouse2 = w2->id;
    }

    BAL::Log("=> Adding Warehouse 3");
    WarehouseId idWarehouse3;
    {
        Warehouses warehouses = getTable<Warehouses>(GLOBAL_SCOPE);
        addWarehouse(manager3, warehouse3Name);

        // Verify the presence of the new warehouse
        const Warehouse* w3 = seekWarehouse(warehouses, warehouse3Name);
        BAL::Verify(w3 != nullptr && w3->manager == manager3,"The new warehouse was not found!");

        idWarehouse3 = w3->id;
    }


    ///
    /// Add inventory to Warehouse 1
    ///
    BAL::Log("=> Adding inventory to Warehouse 1");
    const std::string lumberDesc = "Lumber";
    const uint32_t initialQtyLumber = 10;
    const std::string graphiteDesc = "Graphite";
    const uint32_t initialQtyGraphite = 7;
    {
        addInventory(idWarehouse1, manager1, lumberDesc, initialQtyLumber);
        addInventory(idWarehouse1, manager1, graphiteDesc, initialQtyGraphite);
    }

    // Check the inventory
    InventoryId idLumberBatch1, idGraphiteBatch1;
    {
        auto stock1 = getTable<Stock>(idWarehouse1);

        const InventoryId* ptrIdLumberBatch1 = seekInv(stock1, lumberDesc, initialQtyLumber);
        BAL::Verify(ptrIdLumberBatch1 != nullptr,
            "Lumber inventory is missing from the warehouse");
        idLumberBatch1 = *ptrIdLumberBatch1;

        const InventoryId* ptrIdGraphiteBatch1 = seekInv(stock1, graphiteDesc, initialQtyGraphite);
        BAL::Verify(ptrIdGraphiteBatch1 != nullptr,
            "Graphite inventory is missing from the warehouse");
        idGraphiteBatch1 = *ptrIdGraphiteBatch1;
    }


    ///
    /// 1. Ship 9 units out of 10 of a single inventory item from Warehouse 1 to a Carrier A
    ///
    BAL::Log("=> Shipping inventory out of Warehouse 1");
    const std::string manifestDesc = "wrhs1-lumber-wrhs2-and-wrhs3";
    const uint32_t shipmentQtyLumber = 9;
    {
        PickList manifest;
        manifest.insert(std::make_pair(idLumberBatch1, shipmentQtyLumber));

        const bool deleteConsumed = true;
        shipInventory(idWarehouse1, manager1, trains, manifest, deleteConsumed, manifestDesc);
    }

    // Find the manifest ID
    ManifestId idManifest1;
    {
        Manifests manifests = getTable<Manifests>(trains);
        const ManifestId* ptrIdManifest = seekManifest(manifests, idWarehouse1, manifestDesc);
        BAL::Verify(ptrIdManifest != nullptr, "The newly created manifest was not found!");
        idManifest1 = *ptrIdManifest;
    }

    // Check the removal from Warehouse 1
    {
        Stock stock1 = getTable<Stock>(idWarehouse1);

        const InventoryId* ptrIdLumberBatch1 = seekInv(stock1, lumberDesc, initialQtyLumber - shipmentQtyLumber);
        BAL::Verify(ptrIdLumberBatch1 != nullptr,
            "Lumber inventory is missing from Warehouse 1!");

        // The other inventory should still be present
        const InventoryId* ptrIdGraphiteBatch1 = seekInv(stock1, graphiteDesc, initialQtyGraphite);
        BAL::Verify(ptrIdGraphiteBatch1 != nullptr,
            "Graphite inventory is missing from the warehouse");
    }

    // Check the addition to the carrier's cargo
    CargoId idCargoManifest1;
    {
        CargoStock carrierStock = getTable<CargoStock>(trains);

        BAL::Verify(isCargoInManifest(lumberDesc, shipmentQtyLumber, carrierStock, idManifest1),
            "Did not find the lumber in the carrier's cargo manifest");

        const vector<CargoId> manifestCargo = seekManifestCargo(carrierStock, idManifest1);
        BAL::Verify(manifestCargo.size() == 1,
            "Only a single item should be found in the cargo manifest");
        idCargoManifest1 = manifestCargo[0];
    }


    ///
    /// 2. Transfer some of the cargo to another carrier
    ///
    const std::string manifest2Desc = "trains-lumber-wrhs3";
    const uint32_t qtyCarrierTransferLumber = 3;
    {
        CargoManifest subManifest;
        subManifest.insert(std::make_pair(idCargoManifest1, qtyCarrierTransferLumber));

        transferCargo(trains, planes, idManifest1, subManifest, manifest2Desc);
    }

    // Find the manifest ID
    ManifestId idManifest2;
    {
        Manifests manifests = getTable<Manifests>(planes);
        const ManifestId* ptrIdManifest = seekManifest(manifests, idWarehouse1, manifest2Desc);
        BAL::Verify(ptrIdManifest != nullptr, "The newly created manifest was not found!");
        idManifest2 = *ptrIdManifest;
    }

    // Check the addition to the destination carrier's cargo
    CargoId idCargoInManifest2;
    {
        CargoStock carrierStock = getTable<CargoStock>(planes);

        BAL::Verify(isCargoInManifest(lumberDesc, qtyCarrierTransferLumber, carrierStock, idManifest2),
            "Did not find the lumber in the carrier's cargo manifest");

        const vector<CargoId> manifestCargo = seekManifestCargo(carrierStock, idManifest2);
        BAL::Verify(manifestCargo.size() == 1, "Only a single item should be found in the cargo manifest");
        idCargoInManifest2 = manifestCargo[0];
    }

    // Check the reduction of the source carrier's cargo
    {
        CargoStock carrierStock = getTable<CargoStock>(trains);

        BAL::Verify(isCargoInManifest(lumberDesc, shipmentQtyLumber - qtyCarrierTransferLumber, carrierStock, idManifest1),
            "Did not find the lumber in the carrier's cargo manifest");

        const vector<CargoId> manifestCargo = seekManifestCargo(carrierStock, idManifest1);
        BAL::Verify(manifestCargo.size() == 1,
            "Only a single item should be found in the cargo manifest");
    }


    ///
    /// 3. Deliver Manifest 1 cargo to Warehouse 2
    ///
    BAL::Log("=> Deliver Manifest 1 to Warehouse 2");
    {
        CargoManifest subManifest; // Empty sub-manifest; deliver everything

        deliverCargo(trains, idWarehouse2, manager2, idManifest1, subManifest, "Test delivery");
    }

    // Check the reduction to the carrier's cargo
    {
        CargoStock carrierStock = getTable<CargoStock>(trains);

        const vector<CargoId> manifestCargo = seekManifestCargo(carrierStock, idManifest1);
        BAL::Verify(manifestCargo.size() == 0,
            "No items should have been found in the cargo manifest");
    }

   // Check the addition to the destination warehouse inventory
   {
        Stock stock2 = getTable<Stock>(idWarehouse2);

        const InventoryId* ptrIdLumberBatch1 = seekInv(stock2, lumberDesc, shipmentQtyLumber - qtyCarrierTransferLumber);
        BAL::Verify(ptrIdLumberBatch1 != nullptr,
            "Lumber inventory is missing from Warehouse 2");
    }


    ///
    /// 4. Deliver Manifest 2 cargo to Warehouse 3
    ///
    BAL::Log("=> Deliver Manifest 2 to Warehouse 3");
    {
        CargoManifest subManifest; // Empty sub-manifest; deliver everything

        deliverCargo(planes, idWarehouse3, manager3, idManifest2, subManifest, "Test delivery");
    }

    // Check the reduction to the carrier's cargo
    {
        CargoStock carrierStock = getTable<CargoStock>(planes);

        const vector<CargoId> manifestCargo = seekManifestCargo(carrierStock, idManifest2);
        BAL::Verify(manifestCargo.size() == 0,
            "No items should have been found in the cargo manifest");
    }

   // Check the addition to the destination warehouse inventory
   {
        Stock stock = getTable<Stock>(idWarehouse3);

        const InventoryId* ptrIdLumberSplit = seekInv(stock, lumberDesc, qtyCarrierTransferLumber);
        BAL::Verify(ptrIdLumberSplit != nullptr,
            "Lumber inventory is missing from Warehouse 3");
    }


    ///
    /// Clean the test artifacts
    ///
    const bool removeInventory = true;
    deleteWarehouse(idWarehouse1, manager1, removeInventory, "Unit test");
    deleteWarehouse(idWarehouse2, manager2, removeInventory, "Unit test");
    deleteWarehouse(idWarehouse3, manager3, removeInventory, "Unit test");
    clean(trains);
    clean(planes);

    BAL::Log("Test: PASSED");
}


/**
 * Test delivery of inventory from Warehouse 1 to Warehouse 2 and 3
 * by transferring sub-manifests between carriers,
 * delivering sub-manifests, and removing cargo.
 *
 * 1. Ship 9 units of Lumber and 5 units of Graphite from Warehouse 1 to "trains" carrier
 * 2. Transfer 3 of 9 Lumber units and 2 of 5 Graphite units from "trains" to "planes"
 * 3. "trains" delivers 2 of 6 Lumber units and 2 of 3 Graphite units to Warehouse 2
 * 4. "trains" delivers 4 of 4 Lumber units and 1 of 1 Graphite units to Warehouse 3 (deplete Lumber units)
 * 5. "planes" removes 1 of 3 Lumber units
 * 6. "planes" removes 2 of 2 Lumber units (deplete Lumber units)
 * 7. "planes" delivers 2 of 2 Graphite units to Warehouse 3 (deplete Graphite units)
 *
 * The test presumes the existence of an account named "test.alice", "test.bob",
 * "test.trains", and "test.planes".
 */
void SupplyChain::testShipAndDeliver3() {
    ///
    /// Initialize Test
    ///
    // Require the contract's authority to run tests
    requireAuthorization(ownerAccount());
    BAL::Log("\n\nTesting Ship and Deliver 3");

    // Initialize values for the test
    BAL::AccountName manager1 = "test.alice"_N;
    const std::string warehouse1Name = "Alice's Test Warehouse 1";
    BAL::AccountName manager2 = "test.bob"_N;
    const std::string warehouse2Name = "Bob's Test Warehouse 1";
    BAL::AccountName manager3 = manager2;
    const std::string warehouse3Name = "Bob's Test Warehouse 2";
    BAL::AccountName trains = "test.trains"_N;
    BAL::AccountName planes = "test.planes"_N;

    // Verify the existence of blockchain accounts needed for the test
    BAL::Verify(accountExists(manager1), "Test manager account 1 does not exist");
    BAL::Verify(accountExists(manager2), "Test manager account 2 does not exist");
    BAL::Verify(accountExists(manager3), "Test manager account 3 does not exist");
    BAL::Verify(accountExists(trains), "Test carrier does not exist");
    BAL::Verify(accountExists(planes), "Test carrier does not exist");

    // Ensure that the warehouses do not exist
    {
        Warehouses warehouses = getTable<Warehouses>(GLOBAL_SCOPE);

        const Warehouse* w1 = seekWarehouse(warehouses, warehouse1Name);
        BAL::Verify(w1 == nullptr, "The warehouse should not exist at the start of the test!");

        const Warehouse* w2 = seekWarehouse(warehouses, warehouse2Name);
        BAL::Verify(w2 == nullptr, "The warehouse should not exist at the start of the test!");

        const Warehouse* w3 = seekWarehouse(warehouses, warehouse3Name);
        BAL::Verify(w3 == nullptr, "The warehouse should not exist at the start of the test!");
    }


    // Ensure that the carrier has no cargo
    {
        // Ensure that the carrier has no manifests
        Manifests manifests = getTable<Manifests>(trains);
        BAL::Verify(manifests.begin() == manifests.end(),
            "No carrier manifests should exist at the start of the test!");

        // Ensure that the carrier has no cargo stock
        CargoStock cargoStock = getTable<CargoStock>(trains);
        BAL::Verify(cargoStock.begin() == cargoStock.end(),
            "No carrier cargo stock should exist at the start of the test!");
    }

    // Ensure that the carrier has no cargo
    {
        // Ensure that the carrier has no manifests
        Manifests manifests = getTable<Manifests>(planes);
        BAL::Verify(manifests.begin() == manifests.end(),
            "No carrier manifests should exist at the start of the test!");

        // Ensure that the carrier has no cargo stock
        CargoStock cargoStock = getTable<CargoStock>(planes);
        BAL::Verify(cargoStock.begin() == cargoStock.end(),
            "No carrier cargo stock should exist at the start of the test!");
    }


    ///
    /// Add warehouses
    ///
    BAL::Log("=> Adding Warehouse 1");
    WarehouseId idWarehouse1;
    {
        Warehouses warehouses = getTable<Warehouses>(GLOBAL_SCOPE);
        addWarehouse(manager1, warehouse1Name);

        // Verify the presence of the new warehouse
        const Warehouse* w1 = seekWarehouse(warehouses, warehouse1Name);
        BAL::Verify(w1 != nullptr && w1->manager == manager1, "The new warehouse was not found!");

        idWarehouse1 = w1->id;
    }

    BAL::Log("=> Adding Warehouse 2");
    WarehouseId idWarehouse2;
    {
        Warehouses warehouses = getTable<Warehouses>(GLOBAL_SCOPE);
        addWarehouse(manager2, warehouse2Name);

        // Verify the presence of the new warehouse
        const Warehouse* w2 = seekWarehouse(warehouses, warehouse2Name);
        BAL::Verify(w2 != nullptr && w2->manager == manager2, "The new warehouse was not found!");

        idWarehouse2 = w2->id;
    }

    BAL::Log("=> Adding Warehouse 3");
    WarehouseId idWarehouse3;
    {
        Warehouses warehouses = getTable<Warehouses>(GLOBAL_SCOPE);
        addWarehouse(manager3, warehouse3Name);

        // Verify the presence of the new warehouse
        const Warehouse* w3 = seekWarehouse(warehouses, warehouse3Name);
        BAL::Verify(w3 != nullptr && w3->manager == manager3,"The new warehouse was not found!");

        idWarehouse3 = w3->id;
    }


    ///
    /// Add inventory to Warehouse 1
    ///
    BAL::Log("=> Adding inventory to Warehouse 1");
    const std::string descLumber = "Lumber";
    const uint32_t initialQtyLumber = 10;
    const std::string descGraphite = "Graphite";
    const uint32_t initialQtyGraphite = 7;
    {
        addInventory(idWarehouse1, manager1, descLumber, initialQtyLumber);
        addInventory(idWarehouse1, manager1, descGraphite, initialQtyGraphite);
    }

    // Check the inventory
    InventoryId idLumberBatch1, idGraphiteBatch1;
    {
        auto stock1 = getTable<Stock>(idWarehouse1);

        const InventoryId* ptrIdLumberBatch1 =
                           seekInv(stock1, descLumber, initialQtyLumber);
        BAL::Verify(ptrIdLumberBatch1 != nullptr,
            "Lumber inventory is missing from the warehouse");
        idLumberBatch1 = *ptrIdLumberBatch1;

        const InventoryId* ptrIdGraphiteBatch1 =
                           seekInv(stock1, descGraphite, initialQtyGraphite);
        BAL::Verify(ptrIdGraphiteBatch1 != nullptr,
            "Graphite inventory is missing from the warehouse");
        idGraphiteBatch1 = *ptrIdGraphiteBatch1;
    }

    ///
    /// 1. Ship 9 units of Lumber and 5 units of Graphite from Warehouse 1 to "trains" carrier
    ///
    BAL::Log("=> Shipping inventory out of Warehouse 1");
    const std::string manifestDesc = "wrhs1-lumber-wrhs2-and-wrhs3";
    const uint32_t shipmentQtyLumber = 9;
    const uint32_t shipmentQtyGraphite = 5;
    {
        PickList manifest;
        manifest.insert(std::make_pair(idLumberBatch1, shipmentQtyLumber));
        manifest.insert(std::make_pair(idGraphiteBatch1, shipmentQtyGraphite));

        const bool deleteConsumed = true;
        shipInventory(idWarehouse1, manager1, trains, manifest, deleteConsumed, manifestDesc);
    }

    // Find the manifest ID
    ManifestId idManifest1;
    {
        Manifests manifests = getTable<Manifests>(trains);
        const ManifestId* ptrIdManifest = seekManifest(manifests, idWarehouse1, manifestDesc);
        BAL::Verify(ptrIdManifest != nullptr, "The newly created manifest was not found!");
        idManifest1 = *ptrIdManifest;
    }

    // Check the removal from Warehouse 1
    {
        Stock stock1 = getTable<Stock>(idWarehouse1);

        const InventoryId* ptrIdLumberBatch1 =
                           seekInv(stock1, descLumber,
                                    initialQtyLumber - shipmentQtyLumber);
        BAL::Verify(ptrIdLumberBatch1 != nullptr,
            "Lumber inventory is missing from Warehouse 1!");

        // The other inventory should still be present
        const InventoryId* ptrIdGraphiteBatch1 =
                           seekInv(stock1, descGraphite,
                                    initialQtyGraphite - shipmentQtyGraphite);
        BAL::Verify(ptrIdGraphiteBatch1 != nullptr,
            "Graphite inventory is missing from the warehouse");
    }

    // Check the addition to the carrier's cargo
    CargoId idCargoManifest1Lumber, idCargoManifest1Graphite;
    {
        CargoStock carrierStock = getTable<CargoStock>(trains);

        const CargoId* ptrIdCargoLumber =
                       seekManifestCargo(carrierStock, idManifest1, descLumber,
                                           shipmentQtyLumber);
        BAL::Verify(ptrIdCargoLumber != nullptr,
                    "Did not find the lumber in the carrier's cargo manifest");
        idCargoManifest1Lumber = *ptrIdCargoLumber;

        const CargoId* ptrIdCargoGraphite =
                       seekManifestCargo(carrierStock, idManifest1, descGraphite,
                                           shipmentQtyGraphite);
        BAL::Verify(ptrIdCargoGraphite != nullptr,
                    "Did not find the graphite in the carrier's cargo manifest");
        idCargoManifest1Graphite = *ptrIdCargoGraphite;
    }


    ///
    /// 2. Transfer 3 of 9 Lumber units and 2 of 5 Graphite units from "trains" to "planes"
    ///
    const std::string manifest2Desc = "trains-multipleItems-multipleDestinations";
    const uint32_t qtyCarrierTransferLumber = 3;
    const uint32_t qtyCarrierTransferGraphite = 2;
    {
        CargoManifest subManifest;
        subManifest.insert(std::make_pair(idCargoManifest1Lumber, qtyCarrierTransferLumber));
        subManifest.insert(std::make_pair(idCargoManifest1Graphite, qtyCarrierTransferGraphite));

        transferCargo(trains, planes, idManifest1, subManifest, manifest2Desc);
    }

    // Find the manifest ID
    ManifestId idManifest2;
    {
        Manifests manifests = getTable<Manifests>(planes);
        const ManifestId* ptrIdManifest = seekManifest(manifests, idWarehouse1, manifest2Desc);
        BAL::Verify(ptrIdManifest != nullptr, "The newly created manifest was not found!");
        idManifest2 = *ptrIdManifest;
    }

    // Check the addition to the destination carrier's cargo
    CargoId idCargoInManifest2Lumber, idCargoInManifest2Graphite;
    {
        CargoStock carrierStock = getTable<CargoStock>(planes);

        const vector<CargoId> manifestCargo = seekManifestCargo(carrierStock, idManifest2);
        BAL::Verify(manifestCargo.size() == 2, "Two items should be found in the Cargo Manifest 2");

        const CargoId* ptrIdCargoInManifest2Lumber =
                       seekManifestCargo(carrierStock, idManifest2, descLumber,
                                           qtyCarrierTransferLumber);
        BAL::Verify(ptrIdCargoInManifest2Lumber != nullptr,
                    "Did not find the lumber in Cargo Manifest 2");
        idCargoInManifest2Lumber = *ptrIdCargoInManifest2Lumber;

        const CargoId* ptrIdCargoInManifest2Graphite =
                       seekManifestCargo(carrierStock, idManifest2, descGraphite,
                                           qtyCarrierTransferGraphite);
        BAL::Verify(ptrIdCargoInManifest2Graphite != nullptr,
                    "Did not find the graphite in Cargo Manifest 2");
        idCargoInManifest2Graphite = *ptrIdCargoInManifest2Graphite;

    }

    // Check the reduction of the source carrier's cargo
    {
        CargoStock carrierStock = getTable<CargoStock>(trains);

        BAL::Verify(isCargoInManifest(descLumber,
                                         shipmentQtyLumber - qtyCarrierTransferLumber,
                                         carrierStock, idManifest1),
                    "Did not find the lumber in Cargo Manifest 1");
        BAL::Verify(isCargoInManifest(descGraphite,
                                         shipmentQtyGraphite - qtyCarrierTransferGraphite,
                                         carrierStock, idManifest1),
                    "Did not find the graphite in Cargo Manifest 1");

        const vector<CargoId> manifestCargo = seekManifestCargo(carrierStock, idManifest1);
        BAL::Verify(manifestCargo.size() == 2, "Two items should be found in Cargo Manifest 1");
    }


    ///
    /// 3. "trains" delivers 2 of 6 Lumber units and 2 of 3 Graphite units to Warehouse 2
    ///
    BAL::Log("=> Deliver a sub-manifest of Manifest 1 to Warehouse 2");
    const uint32_t qtyDelivery1Lumber = 2;
    const uint32_t qtyDelivery1Graphite = 2;
    {
        CargoManifest subManifest;
        subManifest.insert(std::make_pair(idCargoManifest1Lumber, qtyDelivery1Lumber));
        subManifest.insert(std::make_pair(idCargoManifest1Graphite, qtyDelivery1Graphite));

        deliverCargo(trains, idWarehouse2, manager2, idManifest1, subManifest, "Test delivery");
    }

    // Check the reduction to the carrier's cargo
    {
        CargoStock carrierStock = getTable<CargoStock>(trains);

        const vector<CargoId> manifestCargo = seekManifestCargo(carrierStock, idManifest1);
        BAL::Verify(manifestCargo.size() == 2,
            "Two items should have been found in the cargo manifest");

        BAL::Verify(isCargoInManifest(descLumber,
                                         shipmentQtyLumber - qtyCarrierTransferLumber - qtyDelivery1Lumber,
                                         carrierStock,
                                         idManifest1),
                    "Did not find the lumber in the carrier's cargo manifest");

        BAL::Verify(isCargoInManifest(descGraphite,
                                         shipmentQtyGraphite - qtyCarrierTransferGraphite - qtyDelivery1Graphite,
                                         carrierStock,
                                         idManifest1),
                    "Did not find the graphite in the carrier's cargo manifest");
    }

   // Check the addition to the destination warehouse inventory
   {
        Stock stock2 = getTable<Stock>(idWarehouse2);

        const InventoryId* ptrIdLumberWarehouse2 = seekInv(stock2, descLumber, qtyDelivery1Lumber);
        BAL::Verify(ptrIdLumberWarehouse2 != nullptr,
                    "Lumber inventory is missing from Warehouse 2");

        const InventoryId* ptrIdGraphiteWarehouse2 = seekInv(stock2, descLumber, qtyDelivery1Graphite);
        BAL::Verify(ptrIdGraphiteWarehouse2 != nullptr,
                    "Graphite inventory is missing from Warehouse 2");
    }


    ///
    /// 4. "trains" delivers 4 of 4 Lumber units and 1 of 1 Graphite units to Warehouse 3 (deplete Lumber units)
    ///
    BAL::Log("=> Deliver a sub-manifest (and deplete) of Manifest 1 to Warehouse 3");
    const uint32_t qtyDelivery2Lumber = 4;
    const uint32_t qtyDelivery2Graphite = 1;
    {
        CargoManifest subManifest; // Empty sub-manifest; deliver everything
        subManifest.insert(std::make_pair(idCargoManifest1Lumber, qtyDelivery2Lumber));
        subManifest.insert(std::make_pair(idCargoManifest1Graphite, qtyDelivery2Graphite));

        deliverCargo(trains, idWarehouse3, manager3, idManifest1, subManifest,
                     "Test delivery that depletes the a cargo manifest");
    }

    // Check the reduction to the carrier's cargo
    {
        CargoStock carrierStock = getTable<CargoStock>(trains);

        const vector<CargoId> manifestCargo = seekManifestCargo(carrierStock, idManifest1);
        BAL::Verify(manifestCargo.size() == 0,
            "No items should have been found in the cargo manifest");
    }

   // Check the addition to the destination warehouse inventory
   {
        Stock stock = getTable<Stock>(idWarehouse3);

        const InventoryId* ptrIdLumberSplit = seekInv(stock, descLumber, qtyDelivery2Lumber);
        BAL::Verify(ptrIdLumberSplit != nullptr, "Lumber inventory is missing from Warehouse 3");

        const InventoryId* ptrIdGraphiteSplit = seekInv(stock, descGraphite, qtyDelivery2Graphite);
        BAL::Verify(ptrIdGraphiteSplit != nullptr, "Graphite inventory is missing from Warehouse 3");
    }


    ///
    /// 5. "planes" removes 1 of 3 Lumber units
    ///
    BAL::Log("=> planes removes 1 of 3 Lumber units");
    const uint32_t qtyGraphiteRemoval1 = 1;
    {
        removeCargo(planes, idManifest2, idCargoInManifest2Lumber, qtyGraphiteRemoval1,
                    "Testing");
    }

    // Check the reduction to the carrier's cargo
    {
        CargoStock carrierStock = getTable<CargoStock>(planes);

        const CargoId* ptrIdCargoLumber =
                       seekManifestCargo(carrierStock, idManifest2, descLumber,
                                           qtyCarrierTransferLumber - qtyGraphiteRemoval1);
        BAL::Verify(ptrIdCargoLumber != nullptr,
            "Did not find the expected lumber in Manifest 2");

        const CargoId* ptrIdCargoGraphite =
                       seekManifestCargo(carrierStock, idManifest2, descGraphite,
                                           qtyCarrierTransferGraphite);
        BAL::Verify(ptrIdCargoGraphite != nullptr,
            "Did not find the expected graphite in Manifest 2");
    }


    ///
    /// 6. "planes" removes 2 of 2 Lumber units (deplete Lumber units)
    ///
    {
        removeCargo(planes, idManifest2, idCargoInManifest2Lumber, 2,
                    "Testing");
    }

    // Check the reduction to the carrier's cargo
    {
        CargoStock carrierStock = getTable<CargoStock>(planes);

         const vector<CargoId> manifestCargo = seekManifestCargo(carrierStock, idManifest2);
        BAL::Verify(manifestCargo.size() == 1,
                    "Only one item should have been found in the cargo manifest");

        const CargoId* ptrIdCargoLumber =
                       seekManifestCargo(carrierStock, idManifest2, descLumber,
                                           0);
        BAL::Verify(ptrIdCargoLumber == nullptr,
                    "Should not have found lumber in Manifest 2!");

        const CargoId* ptrIdCargoGraphite =
                       seekManifestCargo(carrierStock, idManifest2, descGraphite,
                                           qtyCarrierTransferGraphite);
        BAL::Verify(ptrIdCargoGraphite != nullptr,
                    "Did not find the expected graphite in Manifest 2!");
    }


    ///
    /// 7. "planes" delivers 2 of 2 Graphite units to Warehouse 3 (deplete Graphite units)
    ///
    BAL::Log("=> Deliver a sub-manifest of Manifest 2 to Warehouse 3");
    const uint32_t qtyDelivery3Graphite = 2;
    {
        CargoManifest subManifest;
        subManifest.insert(std::make_pair(idCargoInManifest2Graphite, qtyDelivery3Graphite));

        deliverCargo(planes, idWarehouse3, manager3, idManifest2, subManifest, "Test delivery");
    }

    // Check the reduction to the carrier's cargo
    {
        CargoStock carrierStock = getTable<CargoStock>(planes);

        const vector<CargoId> manifestCargo = seekManifestCargo(carrierStock, idManifest2);
        BAL::Verify(manifestCargo.size() == 0,
                    "No items should not have be found in the cargo manifest");
    }

    // Check the addition to the destination warehouse inventory
    {
        Stock stock = getTable<Stock>(idWarehouse3);

        const InventoryId* ptrIdGraphite = seekInv(stock, descGraphite, qtyDelivery3Graphite);
        BAL::Verify(ptrIdGraphite != nullptr, "Graphite inventory is missing from Warehouse 3");
    }


    ///
    /// Clean the test artifacts
    ///
    const bool removeInventory = true;
    deleteWarehouse(idWarehouse1, manager1, removeInventory, "Unit test");
    deleteWarehouse(idWarehouse2, manager2, removeInventory, "Unit test");
    deleteWarehouse(idWarehouse3, manager3, removeInventory, "Unit test");
    clean(trains);
    clean(planes);

    BAL::Log("Test: PASSED");
}
