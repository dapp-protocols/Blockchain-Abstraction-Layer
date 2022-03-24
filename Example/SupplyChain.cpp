#include "SupplyChain.hpp"

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

void SupplyChain::updateWarehouse(AccountHandle manager, WarehouseId warehouseId,
                                  optional<AccountHandle> newManager, optional<string> newDescription,
                                  string documentation) {
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
    warehouses.modify(warehouse, payer, [this, &newManager, &newDescription](Warehouse& warehouse) {
        if (newManager.has_value())
            warehouse.manager = *newManager;
        if (newDescription.has_value())
            warehouse.description = std::move(*newDescription);
    });

    BAL::Log("Successfully updated warehouse", warehouseId);
}

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

void SupplyChain::createInventory(WarehouseId warehouseId, AccountHandle payer,
                                  string description, uint32_t quantity) {
    auto stock = getTable<Stock>(warehouseId);
    InventoryId newId = stock.nextId();
    stock.create(payer, [newId, &description, quantity, origin=currentTransactionId()](Inventory& item) {
        item.id = newId;
        item.description = std::move(description);
        item.origin = std::move(origin);
        item.quantity = quantity;
    });
}

void SupplyChain::addInventory(WarehouseId warehouseId, AccountHandle manager,
                               string description, uint32_t quantity) {
    // Validity checks
    requireAuthorization(manager);
    BAL::Verify(description.size() <= MAX_DESCRIPTION_SIZE,
                "Description size cannot exceed", MAX_DESCRIPTION_SIZE, "characters.");
    auto warehouses = getTable<Warehouses>(GLOBAL_SCOPE);
    const auto& warehouse = warehouses.getId(warehouseId, "Cannot add inventory: specified warehouse does not exist");
    BAL::Verify(warehouse.manager == manager, "Cannot add inventory: authorizing account", manager,
                "is not the manager for warehouse", warehouseId);

    // Create the record
    createInventory(warehouseId, manager, description, quantity);

    BAL::Log("Successfully added item to inventory of warehouse", warehouseId);
}

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

    // Look up all inventory records
    auto stock = getTable<Stock>(warehouseId);
    auto consumeRecords = processPickList(stock, consume);

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
    auto manifestInventory = processPickList(sourceStock, manifest);

    // Update the warehouse stocks
    auto destinationStock = getTable<Stock>(destinationWarehouseId);
    for (auto& [id, transferred] : manifest) {
        // ID should be the first element, so use constant time lookup if possible
        auto itr = manifestInventory.begin()->first == id? manifestInventory.begin() : manifestInventory.find(id);
        BAL::Verify(itr != manifestInventory.end(), "Cannot find the desired inventory item by ID", id, "from the picked manifest");
        const Inventory& inventory = itr->second;

        // Add to destination stock
        InventoryId newId = destinationStock.nextId();
        destinationStock.create(destinationManager, [this, newId, &inventory, t=transferred](Inventory& newRecord) {
            newRecord.id = newId;
            newRecord.description = inventory.description;
            newRecord.origin = inventory.origin;
            newRecord.quantity = t;
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

void SupplyChain::shipInventory(WarehouseId warehouseId, AccountHandle manager, AccountHandle carrier,
                                PickList manifest, bool deleteConsumed, string documentation) {
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
        BAL::Verify(itr != pickedManifest.end(), "Cannot find the desired inventory item by ID", id, "from the picked manifest");
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
        if (!byManifest.contains(manifestId)) {
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

void SupplyChain::transferCargo(AccountHandle sourceCarrier, AccountHandle destinationCarrier,
                                ManifestId manifestId, CargoManifest submanifest, string documentation) {
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
    // gets rid of it, but only if we get that far. By moving the check here, we allow a carrier to get rid of his
    // empty manifest (should never happen, but if it did...) simply by transferring it to himself.
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
        BAL::Verify(itr != cargoToGo.end(), "Cannot find the desired inventory item by ID", id, "from the cargo-to-go");
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

void SupplyChain::deliverCargo(AccountHandle carrier, WarehouseId warehouseId, AccountHandle manager,
                               ManifestId manifestId, CargoManifest submanifest, string documentation) {
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
        BAL::Verify(itr != cargoToGo.end(), "Cannot find the desired inventory item by ID", id, "from the cargo-to-go");
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
