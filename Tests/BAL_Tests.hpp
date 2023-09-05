#pragma once

#include <BAL/BAL.hpp>
#include <BAL/Reflect.hpp>

// Import some common names
using BAL::Name;
using BAL::SecondaryIndex;
using BAL::Table;
using BAL::UInt128;
using std::string;

// Give names to all of the tables in our contract
constexpr static auto GroceriesTableName = "groceries"_N;

// Declare unique ID types for all table types, so different ID types can't be cross-assigned
constexpr Name GLOBAL = "global"_N;
constexpr Name BY_NET_WEIGHT = "by.netweight"_N;

using GroceryId = BAL::ID<BAL::NameTag<GroceriesTableName>>;

class [[eosio::contract("baltests")]] BALTests : public BAL::Contract {
public:
    using BAL::Contract::Contract;

    struct [[eosio::table("groceries")]] GroceryItem {
        // Make some declarations to tell BAL about our table
        using Contract = BALTests;
        constexpr static Name TableName = GroceriesTableName;

        GroceryId id;
        string name;
        string sku;
        uint32_t net_weight_grams = 0;

        // Getter for the primary key (this is required)
        GroceryId primary_key() const { return id; }
        UInt128 net_weight_key() const { return net_weight_grams; }

        using SecondaryIndexes = Util::TypeList::List<SecondaryIndex<BY_NET_WEIGHT, GroceryItem, UInt128, &GroceryItem::net_weight_key>>;

        // Reflect the table fields
        BAL_REFLECT(GroceryItem, (id)(name)(sku)(net_weight_grams))
    };
    using GroceryItems = Table<GroceryItem>;
    using Tables = Util::TypeList::List<GroceryItems>;

    // Test actions
    [[eosio::action("tests.run")]]
    void runTests();

    using Actions = Util::TypeList::List<DESCRIBE_ACTION("tests.run"_N, BALTests::runTests)>;

private:
    using Eraser = std::function<void(const GroceryId&)>;
    using Modifier = std::function<void(const GroceryId&, const uint32_t new_net_weight_grams)>;

    void populateDatasetA(GroceryItems& items);
    void populateDatasetB(GroceryItems& items);

    void testReflection();

    void testScope();

    void testIteration1();

    void testEraseAndIterate2A();
    void testEraseAndIterate2B();
    void testEraseAndIterate2C();
    void testEraseAndIterate2D();
    void testEraseAndIterate2(Eraser& eraser);
    void testEraseAndIterate3A();
    void testEraseAndIterate3B();
    void testEraseAndIterate3C();
    void testEraseAndIterate3D();
    void testEraseAndIterate3(Eraser& eraser);
    void testEraseAndIterate4A();
    void testEraseAndIterate4B();
    void testEraseAndIterate4C();
    void testEraseAndIterate4D();
    void testEraseAndIterate4(Eraser& eraser);

    void testBoundsAndFind_DatasetB_1();

    void testModify1A();
    void testModify1B();
    void testModify1C();
    void testModify1D();
    void testModify1(GroceryItems& grocery_items, Modifier& modifier);
};

