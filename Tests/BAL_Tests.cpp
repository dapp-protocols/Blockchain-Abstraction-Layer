#include "BAL_Tests.hpp"

// ================================
// Compile-time tests of reflection
// ================================
namespace TL = Util::TypeList;
using std::is_same_v;
using std::string_view;
using std::literals::operator""sv;

using GI = BALTests::GroceryItem;
using GIRefl = Util::Reflector<BALTests::GroceryItem>;
// Test GroceryItem reflection
static_assert(GIRefl::IsDefined::value, "GroceryItem's reflector not defined");
static_assert(is_same_v<GIRefl::Type, GI>, "GroceryItem's reflector has wrong type");
static_assert(TL::length<GIRefl::Members>() == 4, "GroceryItem's member list wrong size");
static_assert(TL::length<GIRefl::NativeMembers>() == 4, "GroceryItem's member list wrong size");
static_assert(TL::length<GIRefl::InheritedMembers>() == 0, "GroceryItem's member list wrong size");
static_assert(TL::length<GIRefl::BaseClasses>() == 0, "GroceryItem's base class list wrong size");
static_assert(GIRefl::name() == string_view("GroceryItem"), "GroceryItem's reflected name incorrect");

// Test GroceryItem's member reflections
template<typename... Refls>
using TestContainers = std::conjunction<std::conjunction<std::is_same<typename Refls::Container, GI>, std::is_same<typename Refls::NativeContainer, GI>>...>;
static_assert(TL::apply<GIRefl::Members, TestContainers>::value, "Field reflections' Container or NativeContainer not as expected");

struct ToType { template<typename Refl> struct transform { using type = typename Refl::Type; }; };
static_assert(is_same_v<TL::transform<GIRefl::Members, ToType>, TL::List<GroceryId, string, string, uint32_t>>, "Reflected member types not as expected");

template<typename... Refls>
using AllNative = std::conjunction<std::bool_constant<Refls::isNative>...>;
static_assert(TL::apply<GIRefl::Members, AllNative>::value, "Not all members reflected as native");

template<typename... Refls>
struct NamesAndPointers {
    constexpr static auto get() {
        return std::make_tuple(std::make_tuple(Refls::name, Refls::pointer)...);
    }
};
using GetNamesAndPointers = TL::apply<GIRefl::Members, NamesAndPointers>;
static_assert(GetNamesAndPointers::get() == std::make_tuple(std::make_tuple("id"sv, &GI::id), std::make_tuple("name"sv, &GI::name), std::make_tuple("sku"sv, &GI::sku), std::make_tuple("net_weight_grams"sv, &GI::net_weight_grams)), "Reflected names and pointers not as expected");

// Declare some new structs and test reflection works with inherited and empty classes
struct Base {
    int a;
    char b;

    BAL_REFLECT(Base, (a)(b))
};
struct Derived1 : public Base {
    // No members
    BAL_REFLECT_EMPTY(Derived1, Base)
};
struct Derived2 : public Derived1 {
    bool c;
    BAL_REFLECT(Derived2, (c), Derived1)
};
struct Base2 {
    double z;

    BAL_REFLECT(Base2, (z))
};
struct Derived3: public Derived2, public Base2 {
    BAL_REFLECT_EMPTY(Derived3, Derived2, Base2)
};

static_assert(TL::length<Util::Reflector<Derived2>::Members>() == 3, "Member count wrong");
static_assert(TL::length<Util::Reflector<Derived2>::NativeMembers>() == 1, "Native member count wrong");
static_assert(TL::length<Util::Reflector<Derived2>::InheritedMembers>() == 2, "Inherited member count wrong");
static_assert(is_same_v<Util::Reflector<Derived2>::BaseClasses, TL::List<Derived1>>, "Base class list wrong");
static_assert(is_same_v<TL::at<Util::Reflector<Derived2>::Members, 0>::Container, Derived2>, "Container wrong");
static_assert(is_same_v<TL::at<Util::Reflector<Derived2>::Members, 1>::Container, Derived2>, "Container wrong");
static_assert(is_same_v<TL::at<Util::Reflector<Derived2>::Members, 2>::Container, Derived2>, "Container wrong");
static_assert(is_same_v<TL::at<Util::Reflector<Derived2>::Members, 0>::NativeContainer, Base>, "NativeContainer wrong");
static_assert(is_same_v<TL::at<Util::Reflector<Derived2>::Members, 1>::NativeContainer, Base>, "NativeContainer wrong");
static_assert(is_same_v<TL::at<Util::Reflector<Derived2>::Members, 2>::NativeContainer, Derived2>, "NativeContainer wrong");
static_assert(!TL::at<Util::Reflector<Derived2>::Members, 0>::isNative, "isNative wrong");
static_assert(!TL::at<Util::Reflector<Derived2>::Members, 1>::isNative, "isNative wrong");
static_assert(TL::at<Util::Reflector<Derived2>::Members, 2>::isNative, "isNative wrong");
static_assert(TL::at<Util::Reflector<Derived2>::Members, 0>::name == "a", "name wrong");
static_assert(TL::at<Util::Reflector<Derived2>::Members, 1>::name == "b", "name wrong");
static_assert(TL::at<Util::Reflector<Derived2>::Members, 2>::name == "c", "name wrong");

static_assert(TL::length<Util::Reflector<Derived3>::Members>() == 4, "Member count wrong");
static_assert(TL::length<Util::Reflector<Derived3>::NativeMembers>() == 0, "Native member count wrong");
static_assert(TL::length<Util::Reflector<Derived3>::InheritedMembers>() == 4, "Inherited member count wrong");
static_assert(is_same_v<Util::Reflector<Derived3>::BaseClasses, TL::List<Derived2, Base2>>, "Base class list wrong");
static_assert(is_same_v<TL::at<Util::Reflector<Derived3>::Members, 0>::Container, Derived3>, "Container wrong");
static_assert(is_same_v<TL::at<Util::Reflector<Derived3>::Members, 1>::Container, Derived3>, "Container wrong");
static_assert(is_same_v<TL::at<Util::Reflector<Derived3>::Members, 2>::Container, Derived3>, "Container wrong");
static_assert(is_same_v<TL::at<Util::Reflector<Derived3>::Members, 3>::Container, Derived3>, "Container wrong");
static_assert(is_same_v<TL::at<Util::Reflector<Derived3>::Members, 0>::NativeContainer, Base>, "NativeContainer wrong");
static_assert(is_same_v<TL::at<Util::Reflector<Derived3>::Members, 1>::NativeContainer, Base>, "NativeContainer wrong");
static_assert(is_same_v<TL::at<Util::Reflector<Derived3>::Members, 2>::NativeContainer, Derived2>, "NativeContainer wrong");
static_assert(is_same_v<TL::at<Util::Reflector<Derived3>::Members, 3>::NativeContainer, Base2>, "NativeContainer wrong");
static_assert(!TL::at<Util::Reflector<Derived3>::Members, 0>::isNative, "isNative wrong");
static_assert(!TL::at<Util::Reflector<Derived3>::Members, 1>::isNative, "isNative wrong");
static_assert(!TL::at<Util::Reflector<Derived3>::Members, 2>::isNative, "isNative wrong");
static_assert(!TL::at<Util::Reflector<Derived3>::Members, 3>::isNative, "isNative wrong");
static_assert(TL::at<Util::Reflector<Derived3>::Members, 0>::name == "a", "name wrong");
static_assert(TL::at<Util::Reflector<Derived3>::Members, 1>::name == "b", "name wrong");
static_assert(TL::at<Util::Reflector<Derived3>::Members, 2>::name == "c", "name wrong");
static_assert(TL::at<Util::Reflector<Derived3>::Members, 3>::name == "z", "name wrong");

// =============
// Runtime tests
// =============
void BALTests::runTests() {
    testReflection();
    testScope();
    testIteration1();
    testEraseAndIterate2A();
    testEraseAndIterate2B();
    testEraseAndIterate2C();
    testEraseAndIterate2D();
    testEraseAndIterate3A();
    testEraseAndIterate3B();
    testEraseAndIterate3C();
    testEraseAndIterate3D();
    testEraseAndIterate4A();
    testEraseAndIterate4B();
    testEraseAndIterate4C();
    testEraseAndIterate4D();
    testBoundsAndFind_DatasetB_1();
    testModify1A();
    testModify1B();
    testModify1C();
    testModify1D();

    BAL::Log("ALL Tests PASSED");
}


/**
 * Dataset A: The sequence of the objects in the primary index
 * differs from the secondary index (by net weight).
 *
 * The primary index sequence is:
 * Apple, Net wt.: 25
 * Banana, Net wt.: 35
 * Carrot, Net wt.: 60
 * Date, Net wt.: 45
 *
 * The secondary index sequence is:
 * Apple, Net wt.: 25
 * Banana, Net wt.: 35
 * Date, Net wt.: 45
 * Carrot, Net wt.: 60
 */
void BALTests::populateDatasetA(GroceryItems& items) {
    // Add items
    items.create([](GroceryItem& g) {
        g.id = GroceryId{0};
        g.name = "Apple";
        g.sku = "A1";
        g.net_weight_grams = 25;
    });
    items.create([](GroceryItem& g) {
        g.id = GroceryId{1};
        g.name = "Banana";
        g.sku = "B2";
        g.net_weight_grams = 35;
    });
    items.create([](GroceryItem& g) {
        g.id = GroceryId{2};
        g.name = "Carrot";
        g.sku = "C3";
        g.net_weight_grams = 60;
    });
    items.create([](GroceryItem& g) {
        g.id = GroceryId{3};
        g.name = "Date";
        g.sku = "D4";
        g.net_weight_grams = 45;
    });
}


/**
 * Dataset B: The sequence of the objects in the primary and secondary indices match.
 *
 * Apple, Net wt.: 10
 * Banana, Net wt.: 20
 * Carrot, Net wt.: 30
 * Date, Net wt.: 40
 */
void BALTests::populateDatasetB(GroceryItems& items) {
    // Add items
    items.create([](GroceryItem& g) {
        g.id = GroceryId{0};
        g.name = "Apple";
        g.sku = "A1000";
        g.net_weight_grams = 10;
    });
    items.create([](GroceryItem& g) {
        g.id = GroceryId{1};
        g.name = "Banana";
        g.sku = "B2000";
        g.net_weight_grams = 20;
    });
    items.create([](GroceryItem& g) {
        g.id = GroceryId{2};
        g.name = "Carrot";
        g.sku = "C3000";
        g.net_weight_grams = 30;
    });
    items.create([](GroceryItem& g) {
        g.id = GroceryId{3};
        g.name = "Date";
        g.sku = "D4000";
        g.net_weight_grams = 40;
    });
}


auto ClearGroceries = [](BALTests::GroceryItems& groceries) {
    {
        auto itr = groceries.begin();
        while (itr != groceries.end()) itr = groceries.erase(itr);
    }
};


// Name of test tables
constexpr static auto TestGroceriesTableName = "testing"_N;

/**
 * Runtime tests of BAL reflection
 */
void BALTests::testReflection() {
    GroceryItem item {GroceryId{7}, "Blueberries", "b100be1212135", 250};
    const GroceryItem& constItem = item;

    BAL::Verify(TL::at<GIRefl::Members, 0>::get(item).value == 7, "Bad ID");
    TL::at<GIRefl::Members, 0>::get(item) = GroceryId{10};
    BAL::Verify(TL::at<GIRefl::Members, 0>::get(constItem).value == 10, "Bad ID");

    BAL::Verify(TL::at<GIRefl::Members, 1>::get(item) == "Blueberries", "Bad name");
    TL::at<GIRefl::Members, 1>::get(item) = "Raspberries";
    BAL::Verify(TL::at<GIRefl::Members, 1>::get(constItem) == "Raspberries", "Bad name");

    BAL::Verify(TL::at<GIRefl::Members, 2>::get(item) == "b100be1212135", "Bad SKU");
    TL::at<GIRefl::Members, 2>::get(item) = "boiled skuid";
    BAL::Verify(TL::at<GIRefl::Members, 2>::get(constItem) == "boiled skuid", "Bad SKU");

    BAL::Verify(TL::at<GIRefl::Members, 3>::get(item) == 250, "Bad weight");
    TL::at<GIRefl::Members, 3>::get(item) = 80000;
    BAL::Verify(TL::at<GIRefl::Members, 3>::get(constItem) == 80000, "Bad weight");
    BAL::Log("Reflection test 1: PASSED");

    Derived3 d3;
    Base& b1 = d3;
    Derived2& d2 = d3;
    Base2& b2 = d3;
    using D3Members = Util::Reflector<Derived3>::Members;
    auto& a = TL::at<D3Members, 0>::get(d3);
    auto& b = TL::at<D3Members, 1>::get(d3);
    auto& c = TL::at<D3Members, 2>::get(d3);
    auto& z = TL::at<D3Members, 3>::get(d3);
    BAL::Verify(&a == &d3.a && &a == &b1.a && &b == &d3.b && &b == &b1.b && &c == &d3.c && &c == &d2.c && &z == &d3.z && &z == &b2.z, "Pointers don't match");
    BAL::Log("Reflection test 2: PASSED");
}

/**
 * Test that differently scoped tables are distinct from each other
 */
void BALTests::testScope() {
    ///
    /// Initialize Test
    ///
    // Require the contract's authority to run tests
    requireAuthorization(ownerAccount());
    BAL::Log("\n\nTesting Scope");

    GroceryItems table1 = getTable<GroceryItems>(TestGroceriesTableName);
    GroceryItems table2 = getTable<GroceryItems>("other"_N);

    // Ensure no items are present
    {
        BAL::Verify(table1.begin() == table1.end(), "The test should begin with an empty set of grocery items");
        BAL::Verify(table2.begin() == table2.end(), "The test should begin with an empty set of grocery items");
    }

    BAL::Scope scope1 = table1.scope();
    BAL::Verify(scope1 == TestGroceriesTableName.value, "An unexpected scope was found");

    BAL::Scope scope2 = table2.scope();
    BAL::Verify(scope2 == "other"_N.value, "An unexpected scope was found");

    BAL::Verify(scope1 != scope2, "Two identical scopes should not have been found");


    /// Populate the tables with different items
    populateDatasetA(table1);
    populateDatasetB(table2);


    ///
    /// Confirm that the two differently scoped tables have different entries
    /// Both tables coincidentally have 4 items, the same IDs, and the same names.
    /// However, their SKUs and net weights differ.
    ///
    {
        const GroceryItem& item1 = table1.getId(GroceryId{0});
        const GroceryItem& item2 = table2.getId(GroceryId{0});
        BAL::Verify(item1.name == item2.name, "The two items from different scopes should have intentionally had the same");
        BAL::Verify(item1.sku != item2.sku, "The two items from different scopes should have had different SKUs");
        BAL::Verify(item1.net_weight_grams != item2.net_weight_grams, "The two items from different scopes should have had different net weights");
    }

    {
        const GroceryItem& item1 = table1.getId(GroceryId{1});
        const GroceryItem& item2 = table2.getId(GroceryId{1});
        BAL::Verify(item1.name == item2.name, "The two items from different scopes should have intentionally had the same");
        BAL::Verify(item1.sku != item2.sku, "The two items from different scopes should have had different SKUs");
        BAL::Verify(item1.net_weight_grams != item2.net_weight_grams, "The two items from different scopes should have had different net weights");
    }

    {
        const GroceryItem& item1 = table1.getId(GroceryId{2});
        const GroceryItem& item2 = table2.getId(GroceryId{2});
        BAL::Verify(item1.name == item2.name, "The two items from different scopes should have intentionally had the same");
        BAL::Verify(item1.sku != item2.sku, "The two items from different scopes should have had different SKUs");
        BAL::Verify(item1.net_weight_grams != item2.net_weight_grams, "The two items from different scopes should have had different net weights");
    }

    {
        const GroceryItem& item1 = table1.getId(GroceryId{3});
        const GroceryItem& item2 = table2.getId(GroceryId{3});
        BAL::Verify(item1.name == item2.name, "The two items from different scopes should have intentionally had the same");
        BAL::Verify(item1.sku != item2.sku, "The two items from different scopes should have had different SKUs");
        BAL::Verify(item1.net_weight_grams != item2.net_weight_grams, "The two items from different scopes should have had different net weights");
    }


    ///
    /// Clean the test artifacts
    ///
    ClearGroceries(table1);
    ClearGroceries(table2);

    BAL::Log("Test: PASSED");
}


/**
 * Test iterating over a fixed content table
 *
 * Dataset A: The sequence of the secondary index differs from the primary index
 */
void BALTests::testIteration1() {
    ///
    /// Initialize Test
    ///
    // Require the contract's authority to run tests
    requireAuthorization(ownerAccount());
    BAL::Log("\n\nTesting Iteration 1");

    GroceryItems grocery_items = getTable<GroceryItems>(TestGroceriesTableName);

    // Ensure no items are present
    {
        BAL::Verify(grocery_items.begin() == grocery_items.end(), "The test should begin with an empty set of grocery items");
    }

    // Add items
    populateDatasetA(grocery_items);


    ///
    /// Test forward iteration of primary index
    /// The expected forward order is: Apple, Banana, Carrot, Date
    ///
    {
        BAL::Log("=> Primary Index, Forward Iteration");

        auto itr = grocery_items.begin();
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr->id == 1, "Did not find the expected ID for Banana");
        BAL::Verify(itr->name == "Banana", "Did not find the expected Banana");

        itr++;
        BAL::Verify(itr->id == 2, "Did not find the expected ID for Carrot");
        BAL::Verify(itr->name == "Carrot", "Did not find the expected Carrot");

        itr++;
        BAL::Verify(itr->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr->name == "Date", "Did not find the expected Date");

        itr++;
        BAL::Verify(itr == grocery_items.end(), "Did not find the expected end");
    }


    ///
    /// Test reverse iteration of primary index
    /// The expected reverse order is: Date, Carrot, Banana, Apple
    ///
    {
        BAL::Log("=> Primary Index, Reverse Iteration");

        auto itr = grocery_items.rbegin();
        BAL::Verify(itr->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr->name == "Date", "Did not find the expected Date");

        itr++;
        BAL::Verify(itr->id == 2, "Did not find the expected ID for Carrot");
        BAL::Verify(itr->name == "Carrot", "Did not find the expected Carrot");

        itr++;
        BAL::Verify(itr->id == 1, "Did not find the expected ID for Banana");
        BAL::Verify(itr->name == "Banana", "Did not find the expected Carrot");

        itr++;
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr == grocery_items.rend(), "Did not find the expected end");
    }


    ///
    /// Test forward iteration of secondary index
    /// By net weight, the expected forward order is: Apple, Banana, Date, Carrot
    ///
    {
        BAL::Log("=> Secondary Index, Forward Iteration");

        auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        auto itr = groceriesByNetWeight.begin();
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr->id == 1, "Did not find the expected ID for Banana");
        BAL::Verify(itr->name == "Banana", "Did not find the expected Banana");

        itr++;
        BAL::Verify(itr->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr->name == "Date", "Did not find the expected Date");

        itr++;
        BAL::Verify(itr->id == 2, "Did not find the expected ID for Carrot");
        BAL::Verify(itr->name == "Carrot", "Did not find the expected Carrot");

        itr++;
        BAL::Verify(itr == groceriesByNetWeight.end(), "Did not find the expected end");
    }

    ///
    /// Test reverse iteration of secondary index
    /// By net weight, the expected reverse order is: Carrot, Date, Banana, Apple
    ///
    {
        BAL::Log("=> Secondary Index, Reverse Iteration");

        auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        auto itr = groceriesByNetWeight.rbegin();
        BAL::Verify(itr->id == 2, "Did not find the expected ID for Carrot");
        BAL::Verify(itr->name == "Carrot", "Did not find the expected Carrot");

        itr++;
        BAL::Verify(itr->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr->name == "Date", "Did not find the expected Date");

        itr++;
        BAL::Verify(itr->id == 1, "Did not find the expected ID for Banana");
        BAL::Verify(itr->name == "Banana", "Did not find the expected Banana");

        itr++;
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr == groceriesByNetWeight.rend(), "Did not find the expected end");
    }

    ///
    /// Clean the test artifacts
    ///
    ClearGroceries(grocery_items);

    BAL::Log("Test: PASSED");
}


/**
 * Test iterating over a table whose objects are being erased
 * from the "middle" of the secondary index
 * via the primary index's `void erase(const Object& obj)`
 */
void BALTests::testEraseAndIterate2A() {
    BAL::Log("\n\nTesting Iteration 2A");

    GroceryItems grocery_items = getTable<GroceryItems>(TestGroceriesTableName);
    Eraser e = [&items=grocery_items] (const GroceryId& id) {
        const GroceryItem& item = items.getId(id);
        items.erase(item);
    };

    testEraseAndIterate2(e);
}

/**
 * Test iterating over a table whose objects are being erased
 * from the "middle" of the secondary index
 * via the secondary index's `void erase(const Object& obj)`
 */
void BALTests::testEraseAndIterate2B() {
    BAL::Log("\n\nTesting Iteration 2B");

    GroceryItems grocery_items = getTable<GroceryItems>(TestGroceriesTableName);
    auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();
    Eraser e = [&items=grocery_items, &nw=groceriesByNetWeight] (const GroceryId& id) {
        const GroceryItem& item = items.getId(id);
        nw.erase(item);
    };

    testEraseAndIterate2(e);
}

/**
 * Test iterating over a table whose objects are being erased
 * from the "middle" of the secondary index
 * via the primary index's "forward" `iterator erase(iterator itr)`
 */
void BALTests::testEraseAndIterate2C() {
    BAL::Log("\n\nTesting Iteration 2C");

    GroceryItems grocery_items = getTable<GroceryItems>(TestGroceriesTableName);
    Eraser e = [&items=grocery_items] (const GroceryId& id) {
        auto itr = items.findId(id);
        BAL::Verify(itr != items.end(), "The item to be modified was not found");
        items.erase(itr);
    };

    testEraseAndIterate2(e);
}

/**
 * Test iterating over a table whose objects are being erased
 * from the "middle" of the secondary index
 * via the secondary index's "forward" `iterator erase(iterator itr)`
 */
void BALTests::testEraseAndIterate2D() {
    BAL::Log("\n\nTesting Iteration 2D");

    GroceryItems grocery_items = getTable<GroceryItems>(TestGroceriesTableName);
    auto idx2 = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

    Eraser e = [&items=grocery_items, &items2=idx2] (const GroceryId& id) {
        const GroceryItem& item = items.getId(id);

        auto itr2 = items2.find(item.net_weight_grams);
        BAL::Verify(itr2 != items2.end(), "The item to be modified was not found");

        items2.erase(itr2);
    };

    testEraseAndIterate2(e);
}

/**
 * Test iterating over a table whose objects are being progressively erased
 * from the "middle" of the secondary index of Dataset A
 * whose **initial** sequence of the secondary index differs from the primary index.
 *
 * The primary index sequence is:
 * Apple, Net wt.: 25
 * Banana, Net wt.: 35
 * Carrot, Net wt.: 60
 * Date, Net wt.: 45
 *
 * The secondary index sequence is:
 * Apple, Net wt.: 25
 * Banana, Net wt.: 35
 * Date, Net wt.: 45
 * Carrot, Net wt.: 60
 */
void BALTests::testEraseAndIterate2(Eraser& eraser) {
    ///
    /// Initialize Test
    ///
    // Require the contract's authority to run tests
    requireAuthorization(ownerAccount());

    GroceryItems grocery_items = getTable<GroceryItems>(TestGroceriesTableName);

    // Ensure no items are present
    {
        BAL::Verify(grocery_items.begin() == grocery_items.end(), "The test should begin with an empty set of grocery items");
    }

    /// Add items
    populateDatasetA(grocery_items);


    ///
    /// Phase 1: Test erasing an item in the middle of the primary index (Banana)
    ///
    {
        BAL::Log("=> Phase 1");

        // Identify item to erase
        const GroceryItem& item = grocery_items.getId(GroceryId{1}, "Could not find the grocery item");
        BAL::Verify(item.id == 1, "Did not find the expected ID for Banana");
        BAL::Verify(item.name == "Banana", "Did not find the expected Banana");

        // Erase
        eraser(item.id);
    }

    ///
    /// Test forward iteration of primary index
    /// The expected forward order is: Apple, Carrot, Date
    ///
    {
	// Test iterating
        BAL::Log("==> Primary Index, Forward Iteration");

        auto itr = grocery_items.begin();
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr->id == 2, "Did not find the expected ID for Carrot");
        BAL::Verify(itr->name == "Carrot", "Did not find the expected Carrot");

        itr++;
        BAL::Verify(itr->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr->name == "Date", "Did not find the expected Date");

        itr++;
        BAL::Verify(itr == grocery_items.end(), "Did not find the expected end");
    }


    ///
    /// Test reverse iteration of primary index
    /// The expected reverse order is: Date, Carrot, Apple
    ///
    {
        BAL::Log("==> Primary Index, Reverse Iteration");

        auto itr = grocery_items.rbegin();
        BAL::Verify(itr->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr->name == "Date", "Did not find the expected Date");

        itr++;
        BAL::Verify(itr->id == 2, "Did not find the expected ID for Carrot");
        BAL::Verify(itr->name == "Carrot", "Did not find the expected Carrot");

        itr++;
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr == grocery_items.rend(), "Did not find the expected end");
    }


    ///
    /// Test forward iteration of secondary index
    /// By net weight, the expected forward order is: Apple, Date, Carrot
    ///
    {
        BAL::Log("==> Secondary Index, Forward Iteration");

        auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        auto itr = groceriesByNetWeight.begin();
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr->name == "Date", "Did not find the expected Date");

        itr++;
        BAL::Verify(itr->id == 2, "Did not find the expected ID for Carrot");
        BAL::Verify(itr->name == "Carrot", "Did not find the expected Carrot");

        itr++;
        BAL::Verify(itr == groceriesByNetWeight.end(), "Did not find the expected end");
    }

    ///
    /// Test reverse iteration of secondary index
    /// By net weight, the expected reverse order is: Carrot, Date, Apple
    ///
    {
        BAL::Log("==> Secondary Index, Reverse Iteration");

        auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        auto itr = groceriesByNetWeight.rbegin();
        BAL::Verify(itr->id == 2, "Did not find the expected ID for Carrot");
        BAL::Verify(itr->name == "Carrot", "Did not find the expected Carrot");

        itr++;
        BAL::Verify(itr->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr->name == "Date", "Did not find the expected Date");

        itr++;
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr == groceriesByNetWeight.rend(), "Did not find the expected end");
    }


    ///
    /// Phase 2: Test erasing an item in the middle of the primary index (Carrot)
    ///
    {
        BAL::Log("=> Phase 2");

        // Identify item to erase
        const GroceryItem& item = grocery_items.getId(GroceryId{2}, "Could not find the grocery item");
        BAL::Verify(item.id == 2, "Did not find the expected ID for Carrot");
        BAL::Verify(item.name == "Carrot", "Did not find the expected Carrot");

        // Erase
        eraser(item.id);
    }

    ///
    /// Test forward iteration of primary index
    /// The expected forward order is: Apple, Date
    ///
    {
	// Test iterating
        BAL::Log("==> Primary Index, Forward Iteration");

        auto itr = grocery_items.begin();
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr->name == "Date", "Did not find the expected Date");

        itr++;
        BAL::Verify(itr == grocery_items.end(), "Did not find the expected end");
    }


    ///
    /// Test reverse iteration of primary index
    /// The expected reverse order is: Date, Apple
    ///
    {
        BAL::Log("==> Primary Index, Reverse Iteration");

        auto itr = grocery_items.rbegin();
        BAL::Verify(itr->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr->name == "Date", "Did not find the expected Date");

        itr++;
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr == grocery_items.rend(), "Did not find the expected end");
    }


    ///
    /// Test forward iteration of secondary index
    /// By net weight, the expected forward order is: Apple, Date
    ///
    {
        BAL::Log("==> Secondary Index, Forward Iteration");

        auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        auto itr = groceriesByNetWeight.begin();
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr->name == "Date", "Did not find the expected Date");

        itr++;
        BAL::Verify(itr == groceriesByNetWeight.end(), "Did not find the expected end");
    }

    ///
    /// Test reverse iteration of secondary index
    /// By net weight, the expected reverse order is: Date, Apple
    ///
    {
        BAL::Log("==> Secondary Index, Reverse Iteration");

        auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        auto itr = groceriesByNetWeight.rbegin();
        BAL::Verify(itr->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr->name == "Date", "Did not find the expected Date");

        itr++;
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr == groceriesByNetWeight.rend(), "Did not find the expected end");
    }


    ///
    /// Phase 3: Test erasing an item at the end of the primary index (Date)
    ///
    {
        BAL::Log("=> Phase 3");

        // Identify item to erase
        auto& item = grocery_items.getId(GroceryId{3}, "Could not find the grocery item");
        BAL::Verify(item.id == 3, "Did not find the expected ID for Date");
        BAL::Verify(item.name == "Date", "Did not find the expected Date");

        // Erase
        eraser(item.id);
    }


    ///
    /// Test forward iteration of primary index
    /// The expected forward order is: Apple
    ///
    {
	// Test iterating
        BAL::Log("==> Primary Index, Forward Iteration");

        auto itr = grocery_items.begin();
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr == grocery_items.end(), "Did not find the expected end");
    }


    ///
    /// Test reverse iteration of primary index
    /// The expected reverse order is: Apple
    ///
    {
        BAL::Log("==> Primary Index, Reverse Iteration");

        auto itr = grocery_items.rbegin();
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr == grocery_items.rend(), "Did not find the expected end");
    }

    ///
    /// Test forward iteration of secondary index
    /// By net weight, the expected forward order is: Apple
    ///
    {
        BAL::Log("==> Secondary Index, Forward Iteration");

        auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        auto itr = groceriesByNetWeight.begin();
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr == groceriesByNetWeight.end(), "Did not find the expected end");
    }

    ///
    /// Test reverse iteration of secondary index
    /// By net weight, the expected reverse order is: Apple
    ///
    {
        BAL::Log("==> Secondary Index, Reverse Iteration");

        auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        auto itr = groceriesByNetWeight.rbegin();
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr == groceriesByNetWeight.rend(), "Did not find the expected end");
    }


    ///
    /// Phase 4: Test erasing the last remaining item (Apple)
    ///
    {
        BAL::Log("=> Phase 4");

        // Identify item to erase
        auto& item = grocery_items.getId(GroceryId{0}, "Could not find the grocery item");
        BAL::Verify(item.id == 0, "Did not find the expected ID for Date");
        BAL::Verify(item.name == "Apple", "Did not find the expected Date");

        // Erase
        eraser(item.id);
    }


    ///
    /// Test forward iteration of primary index
    /// There should be nothing to iterate
    ///
    {
	// Test iterating
        BAL::Log("==> Primary Index, Forward Iteration");

        auto itr = grocery_items.begin();
        BAL::Verify(itr == grocery_items.end(), "Did not find the expected end");
    }


    ///
    /// Test reverse iteration of primary index
    /// There should be nothing to iterate
    ///
    {
        BAL::Log("==> Primary Index, Reverse Iteration");

        auto itr = grocery_items.rbegin();
        BAL::Verify(itr == grocery_items.rend(), "Did not find the expected end");
    }

    ///
    /// Test forward iteration of secondary index
    /// There should be nothing to iterate
    ///
    {
        BAL::Log("==> Secondary Index, Forward Iteration");

        auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        auto itr = groceriesByNetWeight.begin();
        BAL::Verify(itr == groceriesByNetWeight.end(), "Did not find the expected end");
    }

    ///
    /// Test reverse iteration of secondary index
    /// There should be nothing to iterate
    ///
    {
        BAL::Log("==> Secondary Index, Reverse Iteration");

        auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        auto itr = groceriesByNetWeight.rbegin();
        BAL::Verify(itr == groceriesByNetWeight.rend(), "Did not find the expected end");
    }

    ///
    /// Clean the test artifacts
    ///
    ClearGroceries(grocery_items);

    BAL::Log("Test: PASSED");
}


/**
 * Test iterating over a table whose objects are being erased
 * from the "begin" of the secondary index
 * via the primary index's `void erase(const Object& obj)`
 */
void BALTests::testEraseAndIterate3A() {
    BAL::Log("\n\nTesting Iteration 3A");

    GroceryItems grocery_items = getTable<GroceryItems>(TestGroceriesTableName);
    Eraser e = [&items=grocery_items] (const GroceryId& id) {
        const GroceryItem& item = items.getId(id);
        items.erase(item);
    };

    testEraseAndIterate3(e);
}

/**
 * Test iterating over a table whose objects are being erased
 * from the "begin" of the secondary index
 * via the secondary index's `void erase(const Object& obj)`
 */
void BALTests::testEraseAndIterate3B() {
    BAL::Log("\n\nTesting Iteration 3B");

    GroceryItems grocery_items = getTable<GroceryItems>(TestGroceriesTableName);
    auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();
    Eraser e = [&items=grocery_items, &nw=groceriesByNetWeight] (const GroceryId& id) {
        const GroceryItem& item = items.getId(id);
        nw.erase(item);
    };

    testEraseAndIterate3(e);
}

/**
 * Test iterating over a table whose objects are being erased
 * from the "begin" of the secondary index
 * via the primary index's "forward" `iterator erase(iterator itr)`
 */
void BALTests::testEraseAndIterate3C() {
    BAL::Log("\n\nTesting Iteration 3C");

    GroceryItems grocery_items = getTable<GroceryItems>(TestGroceriesTableName);
    Eraser e = [&items=grocery_items] (const GroceryId& id) {
        auto itr = items.findId(id);
        BAL::Verify(itr != items.end(), "The item to be modified was not found");
        items.erase(itr);
    };

    testEraseAndIterate3(e);
}

/**
 * Test iterating over a table whose objects are being erased
 * from the "begin" of the secondary index
 * via the secondary index's "forward" `iterator erase(iterator itr)`
 */
void BALTests::testEraseAndIterate3D() {
    BAL::Log("\n\nTesting Iteration 3D");

    GroceryItems grocery_items = getTable<GroceryItems>(TestGroceriesTableName);
    auto idx2 = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

    Eraser e = [&items=grocery_items, &items2=idx2] (const GroceryId& id) {
        const GroceryItem& item = items.getId(id);

        auto itr2 = items2.find(item.net_weight_grams);
        BAL::Verify(itr2 != items2.end(), "The item to be modified was not found");

        items2.erase(itr2);
    };

    testEraseAndIterate3(e);
}


/**
 * Test iterating over a table whose objects are being progressively erased
 * from the "begin" of the secondary index of Dataset A
 * whose **initial** sequence of the secondary index differs from the primary index.
 *
 * The **initial** primary index sequence is:
 * Apple, Net wt.: 25
 * Banana, Net wt.: 35
 * Carrot, Net wt.: 60
 * Date, Net wt.: 45
 *
 * The **initial** secondary index sequence is:
 * Apple, Net wt.: 25
 * Banana, Net wt.: 35
 * Date, Net wt.: 45
 * Carrot, Net wt.: 60
 */
void BALTests::testEraseAndIterate3(Eraser& eraser) {
    ///
    /// Initialize Test
    ///
    // Require the contract's authority to run tests
    requireAuthorization(ownerAccount());

    GroceryItems grocery_items = getTable<GroceryItems>(TestGroceriesTableName);

    // Ensure no items are present
    {
        BAL::Verify(grocery_items.begin() == grocery_items.end(), "The test should begin with an empty set of grocery items");
    }

    /// Add items
    populateDatasetA(grocery_items);

    ///
    /// Phase 1: Test erasing the first item of the secondary index (Apple)
    ///
    {
        BAL::Log("=> Phase 1");

       // Identify item to erase
        auto& item = grocery_items.getId(GroceryId{0}, "Could not find the grocery item");
        BAL::Verify(item.id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(item.name == "Apple", "Did not find the expected Apple");

        // Erase
        eraser(item.id);
    }


    ///
    /// Test forward iteration of primary index
    /// The expected forward order is: Banana, Carrot, Date
    ///
    {
	// Test iterating
        BAL::Log("==> Primary Index, Forward Iteration");

        auto itr = grocery_items.begin();
        BAL::Verify(itr->id == 1, "Did not find the expected ID for Banana");
        BAL::Verify(itr->name == "Banana", "Did not find the expected Banana");

        itr++;
        BAL::Verify(itr->id == 2, "Did not find the expected ID for Carrot");
        BAL::Verify(itr->name == "Carrot", "Did not find the expected Carrot");

        itr++;
        BAL::Verify(itr->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr->name == "Date", "Did not find the expected Date");

        itr++;
        BAL::Verify(itr == grocery_items.end(), "Did not find the expected end");
    }


    ///
    /// Test reverse iteration of primary index
    /// The expected reverse order is: Date, Carrot, Banana
    ///
    {
        BAL::Log("==> Primary Index, Reverse Iteration");

        auto itr = grocery_items.rbegin();
        BAL::Verify(itr->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr->name == "Date", "Did not find the expected Date");

        itr++;
        BAL::Verify(itr->id == 2, "Did not find the expected ID for Carrot");
        BAL::Verify(itr->name == "Carrot", "Did not find the expected Carrot");

        itr++;
        BAL::Verify(itr->id == 1, "Did not find the expected ID for Banana");
        BAL::Verify(itr->name == "Banana", "Did not find the expected Banana");

        itr++;
        BAL::Verify(itr == grocery_items.rend(), "Did not find the expected end");
    }


    ///
    /// Test forward iteration of secondary index
    /// By net weight, the expected forward order is: Banana, Date, Carrot
    ///
    {
        BAL::Log("==> Secondary Index, Forward Iteration");

        auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        auto itr = groceriesByNetWeight.begin();
        BAL::Verify(itr->id == 1, "Did not find the expected ID for Banana");
        BAL::Verify(itr->name == "Banana", "Did not find the expected Banana");

        itr++;
        BAL::Verify(itr->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr->name == "Date", "Did not find the expected Date");

        itr++;
        BAL::Verify(itr->id == 2, "Did not find the expected ID for Carrot");
        BAL::Verify(itr->name == "Carrot", "Did not find the expected Carrot");

        itr++;
        BAL::Verify(itr == groceriesByNetWeight.end(), "Did not find the expected end");
    }


    ///
    /// Test reverse iteration of secondary index
    /// By net weight, the expected reverse order is: Carrot, Date, Banana
    ///
    {
        BAL::Log("==> Secondary Index, Reverse Iteration");

        auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        auto itr = groceriesByNetWeight.rbegin();
        BAL::Verify(itr->id == 2, "Did not find the expected ID for Carrot");
        BAL::Verify(itr->name == "Carrot", "Did not find the expected Carrot");

        itr++;
        BAL::Verify(itr->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr->name == "Date", "Did not find the expected Date");

        itr++;
        BAL::Verify(itr->id == 1, "Did not find the expected ID for Banana");
        BAL::Verify(itr->name == "Banana", "Did not find the expected Banana");

        itr++;
        BAL::Verify(itr == groceriesByNetWeight.rend(), "Did not find the expected end");
    }

    ///
    /// Phase 2: Test erasing the first item of the secondary index (Banana)
    ///
    {
        BAL::Log("=> Phase 2");

        // Identify item to erase
        auto& item = grocery_items.getId(GroceryId{1}, "Could not find the last grocery item");
        BAL::Verify(item.id == 1, "Did not find the expected ID for Banana");
        BAL::Verify(item.name == "Banana", "Did not find the expected Banana");

        // Erase
        eraser(item.id);
    }

    ///
    /// Test forward iteration of primary index
    /// The expected forward order is: Carrot, Date
    ///
    {
	// Test iterating
        BAL::Log("==> Primary Index, Forward Iteration");

        auto itr = grocery_items.begin();
        BAL::Verify(itr->id == 2, "Did not find the expected ID for Carrot");
        BAL::Verify(itr->name == "Carrot", "Did not find the expected Carrot");

        itr++;
        BAL::Verify(itr->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr->name == "Date", "Did not find the expected Date");

        itr++;
        BAL::Verify(itr == grocery_items.end(), "Did not find the expected end");
    }


    ///
    /// Test reverse iteration of primary index
    /// The expected reverse order is: Date, Carrot
    ///
    {
        BAL::Log("==> Primary Index, Reverse Iteration");

        auto itr = grocery_items.rbegin();
        BAL::Verify(itr->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr->name == "Date", "Did not find the expected Date");

        itr++;
        BAL::Verify(itr->id == 2, "Did not find the expected ID for Carrot");
        BAL::Verify(itr->name == "Carrot", "Did not find the expected Carrot");

        itr++;
        BAL::Verify(itr == grocery_items.rend(), "Did not find the expected end");
    }


    ///
    /// Test forward iteration of secondary index
    /// By net weight, the expected forward order is: Date, Carrot
    ///
    {
        BAL::Log("==> Secondary Index, Forward Iteration");

        auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        auto itr = groceriesByNetWeight.begin();
        BAL::Verify(itr->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr->name == "Date", "Did not find the expected Date");

        itr++;
        BAL::Verify(itr->id == 2, "Did not find the expected ID for Carrot");
        BAL::Verify(itr->name == "Carrot", "Did not find the expected Carrot");

        itr++;
        BAL::Verify(itr == groceriesByNetWeight.end(), "Did not find the expected end");
    }


    ///
    /// Test reverse iteration of secondary index
    /// By net weight, the expected reverse order is: Carrot, Date
    ///
    {
        BAL::Log("==> Secondary Index, Reverse Iteration");

        auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        auto itr = groceriesByNetWeight.rbegin();
        BAL::Verify(itr->id == 2, "Did not find the expected ID for Carrot");
        BAL::Verify(itr->name == "Carrot", "Did not find the expected Carrot");

        itr++;
        BAL::Verify(itr->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr->name == "Date", "Did not find the expected Date");

        itr++;
        BAL::Verify(itr == groceriesByNetWeight.rend(), "Did not find the expected end");
    }



    ///
    /// Phase 3: Test erasing the first item of the secondary index (Date)
    ///
    {
        BAL::Log("=> Phase 3");

        // Identify item to erase
        auto& item = grocery_items.getId(GroceryId{3}, "Could not find the last grocery item");
        BAL::Verify(item.id == 3, "Did not find the expected ID for Date");
        BAL::Verify(item.name == "Date", "Did not find the expected Date");

        // Erase
        eraser(item.id);
    }

    ///
    /// Test forward iteration of primary index
    /// The expected forward order is: Carrot
    ///
    {
	// Test iterating
        BAL::Log("==> Primary Index, Forward Iteration");

        auto itr = grocery_items.begin();
        BAL::Verify(itr->id == 2, "Did not find the expected ID for Carrot");
        BAL::Verify(itr->name == "Carrot", "Did not find the expected Carrot");

        itr++;
        BAL::Verify(itr == grocery_items.end(), "Did not find the expected end");
    }


    ///
    /// Test reverse iteration of primary index
    /// The expected reverse order is: Carrot
    ///
    {
        BAL::Log("==> Primary Index, Reverse Iteration");

        auto itr = grocery_items.rbegin();
        BAL::Verify(itr->id == 2, "Did not find the expected ID for Carrot");
        BAL::Verify(itr->name == "Carrot", "Did not find the expected Carrot");

        itr++;
        BAL::Verify(itr == grocery_items.rend(), "Did not find the expected end");
    }


    ///
    /// Test forward iteration of secondary index
    /// By net weight, the expected forward order is: Carrot
    ///
    {
        BAL::Log("==> Secondary Index, Forward Iteration");

        auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        auto itr = groceriesByNetWeight.begin();
        BAL::Verify(itr->id == 2, "Did not find the expected ID for Carrot");
        BAL::Verify(itr->name == "Carrot", "Did not find the expected Carrot");

        itr++;
        BAL::Verify(itr == groceriesByNetWeight.end(), "Did not find the expected end");
    }


    ///
    /// Test reverse iteration of secondary index
    /// By net weight, the expected reverse order is: Carrot
    ///
    {
        BAL::Log("==> Secondary Index, Reverse Iteration");

        auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        auto itr = groceriesByNetWeight.rbegin();
        BAL::Verify(itr->id == 2, "Did not find the expected ID for Carrot");
        BAL::Verify(itr->name == "Carrot", "Did not find the expected Carrot");

        itr++;
        BAL::Verify(itr == groceriesByNetWeight.rend(), "Did not find the expected end");
    }


    ///
    /// Phase 4: Test erasing the first item of the secondary index (Carrot)
    ///
    {
        BAL::Log("=> Phase 4");

        // Identify item to erase
        auto& item = grocery_items.getId(GroceryId{2}, "Could not find the last grocery item");
        BAL::Verify(item.id == 2, "Did not find the expected ID for Carrot");
        BAL::Verify(item.name == "Carrot", "Did not find the expected Carrot");

        // Erase
        eraser(item.id);
    }

    ///
    /// Test forward iteration of primary index
    /// There should be nothing to iterate
    ///
    {
	// Test iterating
        BAL::Log("==> Primary Index, Forward Iteration");

        auto itr = grocery_items.begin();
        BAL::Verify(itr == grocery_items.end(), "Did not find the expected end");
    }


    ///
    /// Test reverse iteration of primary index
    /// There should be nothing to iterate
    ///
    {
        BAL::Log("==> Primary Index, Reverse Iteration");

        auto itr = grocery_items.rbegin();
        BAL::Verify(itr == grocery_items.rend(), "Did not find the expected end");
    }


    ///
    /// Test forward iteration of secondary index
    /// There should be nothing to iterate
    ///
    {
        BAL::Log("==> Secondary Index, Forward Iteration");

        auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        auto itr = groceriesByNetWeight.begin();
        BAL::Verify(itr == groceriesByNetWeight.end(), "Did not find the expected end");
    }


    ///
    /// Test reverse iteration of secondary index
    /// There should be nothing to iterate
    ///
    {
        BAL::Log("==> Secondary Index, Reverse Iteration");

        auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        auto itr = groceriesByNetWeight.rbegin();
        BAL::Verify(itr == groceriesByNetWeight.rend(), "Did not find the expected end");
    }


    ///
    /// Clean the test artifacts
    ///
    ClearGroceries(grocery_items);

    BAL::Log("Test: PASSED");
}


/**
 * Test iterating over a table whose objects are being erased
 * from the "end" of the secondary index
 * via the primary index's `void erase(const Object& obj)`
 */
void BALTests::testEraseAndIterate4A() {
    BAL::Log("\n\nTesting Iteration 4A");

    GroceryItems grocery_items = getTable<GroceryItems>(TestGroceriesTableName);
    Eraser e = [&items=grocery_items] (const GroceryId& id) {
        const GroceryItem& item = items.getId(id);
        items.erase(item);
    };

    testEraseAndIterate4(e);
}

/**
 * Test iterating over a table whose objects are being erased
 * from the "end" of the secondary index
 * via the secondary index's `void erase(const Object& obj)`
 */
void BALTests::testEraseAndIterate4B() {
    BAL::Log("\n\nTesting Iteration 4B");

    GroceryItems grocery_items = getTable<GroceryItems>(TestGroceriesTableName);
    auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();
    Eraser e = [&items=grocery_items, &nw=groceriesByNetWeight] (const GroceryId& id) {
        const GroceryItem& item = items.getId(id);
        nw.erase(item);
    };

    testEraseAndIterate4(e);
}

/**
 * Test iterating over a table whose objects are being erased
 * from the "end" of the secondary index
 * via the primary index's "forward" `iterator erase(iterator itr)`
 */
void BALTests::testEraseAndIterate4C() {
    BAL::Log("\n\nTesting Iteration 4C");

    GroceryItems grocery_items = getTable<GroceryItems>(TestGroceriesTableName);
    Eraser e = [&items=grocery_items] (const GroceryId& id) {
        auto itr = items.findId(id);
        BAL::Verify(itr != items.end(), "The item to be modified was not found");
        items.erase(itr);
    };

    testEraseAndIterate4(e);
}

/**
 * Test iterating over a table whose objects are being erased
 * from the "end" of the secondary index
 * via the secondary index's "forward" `iterator erase(iterator itr)`
 */
void BALTests::testEraseAndIterate4D() {
    BAL::Log("\n\nTesting Iteration 4D");

    GroceryItems grocery_items = getTable<GroceryItems>(TestGroceriesTableName);
    auto idx2 = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

    Eraser e = [&items=grocery_items, &items2=idx2] (const GroceryId& id) {
        const GroceryItem& item = items.getId(id);

        auto itr2 = items2.find(item.net_weight_grams);
        BAL::Verify(itr2 != items2.end(), "The item to be modified was not found");

        items2.erase(itr2);
    };

    testEraseAndIterate4(e);
}

/**
 * Test iterating over a table whose objects are being progressively erased
 * from the "end" of the secondary index of Dataset A
 * whose **initial** sequence of the secondary index differs from the primary index.
 *
 * The **initial** primary index sequence is:
 * Apple, Net wt.: 25
 * Banana, Net wt.: 35
 * Carrot, Net wt.: 60
 * Date, Net wt.: 45
 *
 * The **initial** secondary index sequence is:
 * Apple, Net wt.: 25
 * Banana, Net wt.: 35
 * Date, Net wt.: 45
 * Carrot, Net wt.: 60
 */
void BALTests::testEraseAndIterate4(Eraser& eraser) {
    ///
    /// Initialize Test
    ///
    // Require the contract's authority to run tests
    requireAuthorization(ownerAccount());

    GroceryItems grocery_items = getTable<GroceryItems>(TestGroceriesTableName);

    // Ensure no items are present
    {
        BAL::Verify(grocery_items.begin() == grocery_items.end(), "The test should begin with an empty set of grocery items");
    }


    /// Add items
    populateDatasetA(grocery_items);


    ///
    /// Phase 1: Test erasing the last item of the secondary index (Carrot)
    ///
    {
        BAL::Log("=> Phase 1");

       // Identify item to erase
        auto& item = grocery_items.getId(GroceryId{2}, "Could not find the grocery item");
        BAL::Verify(item.id == 2, "Did not find the expected ID for Carrot");
        BAL::Verify(item.name == "Carrot", "Did not find the expected Carrot");

        // Erase
        grocery_items.erase(item);
    }


    ///
    /// Test forward iteration of primary index
    /// The expected forward order is: Apple, Banana, Date
    ///
    {
	// Test iterating
        BAL::Log("==> Primary Index, Forward Iteration");

        auto itr = grocery_items.begin();
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr->id == 1, "Did not find the expected ID for Banana");
        BAL::Verify(itr->name == "Banana", "Did not find the expected Banana");

        itr++;
        BAL::Verify(itr->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr->name == "Date", "Did not find the expected Date");

        itr++;
        BAL::Verify(itr == grocery_items.end(), "Did not find the expected end");
    }


    ///
    /// Test reverse iteration of primary index
    /// The expected reverse order is: Date, Banana, Apple
    ///
    {
        BAL::Log("==> Primary Index, Reverse Iteration");

        auto itr = grocery_items.rbegin();
        BAL::Verify(itr->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr->name == "Date", "Did not find the expected Date");

        itr++;
        BAL::Verify(itr->id == 1, "Did not find the expected ID for Banana");
        BAL::Verify(itr->name == "Banana", "Did not find the expected Banana");

        itr++;
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr == grocery_items.rend(), "Did not find the expected end");
    }


    ///
    /// Test forward iteration of secondary index
    /// By net weight, the expected forward order is: Apple, Banana, Date
    ///
    {
        BAL::Log("==> Secondary Index, Forward Iteration");

        auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        auto itr = groceriesByNetWeight.begin();
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr->id == 1, "Did not find the expected ID for Banana");
        BAL::Verify(itr->name == "Banana", "Did not find the expected Banana");

        itr++;
        BAL::Verify(itr->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr->name == "Date", "Did not find the expected Date");

        itr++;
        BAL::Verify(itr == groceriesByNetWeight.end(), "Did not find the expected end");
    }


    ///
    /// Test reverse iteration of secondary index
    /// By net weight, the expected reverse order is: Date, Banana, Apple
    ///
    {
        BAL::Log("==> Secondary Index, Reverse Iteration");

        auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        auto itr = groceriesByNetWeight.rbegin();
        BAL::Verify(itr->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr->name == "Date", "Did not find the expected Date");

        itr++;
        BAL::Verify(itr->id == 1, "Did not find the expected ID for Banana");
        BAL::Verify(itr->name == "Banana", "Did not find the expected Banana");

        itr++;
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr == groceriesByNetWeight.rend(), "Did not find the expected end");
    }

    ///
    /// Phase 2: Test erasing the last item of the secondary index (Date)
    ///
    {
        BAL::Log("=> Phase 2");

        // Identify item to erase
        auto& item = grocery_items.getId(GroceryId{3}, "Could not find the last grocery item");
        BAL::Verify(item.id == 3, "Did not find the expected ID for Date");
        BAL::Verify(item.name == "Date", "Did not find the expected Date");

        // Erase
        grocery_items.erase(item);
    }

    ///
    /// Test forward iteration of primary index
    /// The expected forward order is: Apple, Banana
    ///
    {
	// Test iterating
        BAL::Log("==> Primary Index, Forward Iteration");

        auto itr = grocery_items.begin();
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr->id == 1, "Did not find the expected ID for Banana");
        BAL::Verify(itr->name == "Banana", "Did not find the expected Banana");

        itr++;
        BAL::Verify(itr == grocery_items.end(), "Did not find the expected end");
    }


    ///
    /// Test reverse iteration of primary index
    /// The expected reverse order is: Apple, Banana
    ///
    {
        BAL::Log("==> Primary Index, Reverse Iteration");

        auto itr = grocery_items.rbegin();
        BAL::Verify(itr->id == 1, "Did not find the expected ID for Banana");
        BAL::Verify(itr->name == "Banana", "Did not find the expected Banana");

        itr++;
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr == grocery_items.rend(), "Did not find the expected end");
    }



    ///
    /// Test forward iteration of secondary index
    /// By net weight, the expected forward order is: Apple, Banana
    ///
    {
        BAL::Log("==> Secondary Index, Forward Iteration");

        auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        auto itr = groceriesByNetWeight.begin();
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr->id == 1, "Did not find the expected ID for Banana");
        BAL::Verify(itr->name == "Banana", "Did not find the expected Banana");

        itr++;
        BAL::Verify(itr == groceriesByNetWeight.end(), "Did not find the expected end");
    }


    ///
    /// Test reverse iteration of secondary index
    /// By net weight, the expected reverse order is: Banana, Apple
    ///
    {
        BAL::Log("==> Secondary Index, Reverse Iteration");

        auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        auto itr = groceriesByNetWeight.rbegin();
        BAL::Verify(itr->id == 1, "Did not find the expected ID for Banana");
        BAL::Verify(itr->name == "Banana", "Did not find the expected Banana");

        itr++;
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr == groceriesByNetWeight.rend(), "Did not find the expected end");
    }


    ///
    /// Phase 3: Test erasing the last item of the secondary index (Banana)
    ///
    {
        BAL::Log("=> Phase 3");

        // Identify item to erase
        auto& item = grocery_items.getId(GroceryId{1}, "Could not find the last grocery item");
        BAL::Verify(item.id == 1, "Did not find the expected ID for Banana");
        BAL::Verify(item.name == "Banana", "Did not find the expected Banana");

        // Erase
        grocery_items.erase(item);
    }

    ///
    /// Test forward iteration of primary index
    /// The expected forward order is: Apple
    ///
    {
	// Test iterating
        BAL::Log("==> Primary Index, Forward Iteration");

        auto itr = grocery_items.begin();
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr == grocery_items.end(), "Did not find the expected end");
    }


    ///
    /// Test reverse iteration of primary index
    /// The expected reverse order is: Apple
    ///
    {
        BAL::Log("==> Primary Index, Reverse Iteration");

        auto itr = grocery_items.rbegin();
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr == grocery_items.rend(), "Did not find the expected end");
    }



    ///
    /// Test forward iteration of secondary index
    /// By net weight, the expected forward order is: Apple
    ///
    {
        BAL::Log("==> Secondary Index, Forward Iteration");

        auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        auto itr = groceriesByNetWeight.begin();
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr == groceriesByNetWeight.end(), "Did not find the expected end");
    }


    ///
    /// Test reverse iteration of secondary index
    /// By net weight, the expected reverse order is: Apple
    ///
    {
        BAL::Log("==> Secondary Index, Reverse Iteration");

        auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        auto itr = groceriesByNetWeight.rbegin();
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr == groceriesByNetWeight.rend(), "Did not find the expected end");
    }


    ///
    /// Phase 4: Test erasing the last item of the secondary index (Apple)
    ///
    {
        BAL::Log("=> Phase 4");

        // Identify item to erase
        auto& item = grocery_items.getId(GroceryId{0}, "Could not find the last grocery item");
        BAL::Verify(item.id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(item.name == "Apple", "Did not find the expected Apple");

        // Erase
        grocery_items.erase(item);
    }

    ///
    /// Test forward iteration of primary index
    /// There should be nothing to iterate
    ///
    {
	// Test iterating
        BAL::Log("==> Primary Index, Forward Iteration");

        auto itr = grocery_items.begin();
        BAL::Verify(itr == grocery_items.end(), "Did not find the expected end");
    }


    ///
    /// Test reverse iteration of primary index
    /// There should be nothing to iterate
    ///
    {
        BAL::Log("==> Primary Index, Reverse Iteration");

        auto itr = grocery_items.rbegin();
        BAL::Verify(itr == grocery_items.rend(), "Did not find the expected end");
    }



    ///
    /// Test forward iteration of secondary index
    /// There should be nothing to iterate
    ///
    {
        BAL::Log("==> Secondary Index, Forward Iteration");

        auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        auto itr = groceriesByNetWeight.begin();
        BAL::Verify(itr == groceriesByNetWeight.end(), "Did not find the expected end");
    }


    ///
    /// Test reverse iteration of secondary index
    /// There should be nothing to iterate
    ///
    {
        BAL::Log("==> Secondary Index, Reverse Iteration");

        auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        auto itr = groceriesByNetWeight.rbegin();
        BAL::Verify(itr == groceriesByNetWeight.rend(), "Did not find the expected end");
    }


    ///
    /// Clean the test artifacts
    ///
    ClearGroceries(grocery_items);

    BAL::Log("Test: PASSED");
}


/**
 * Test bounding and finding over a table whose contents are progressively deleted over four phases
 *
 * Dataset B: The sequence of the secondary index matches the primary index
 * -
 * Apple, Net wt.: 10
 * Banana, Net wt.: 20
 * Carrot, Net wt.: 30
 * Date, Net wt.: 40
 */
void BALTests::testBoundsAndFind_DatasetB_1() {
    ///
    /// Initialize Test
    ///
    // Require the contract's authority to run tests
    requireAuthorization(ownerAccount());
    BAL::Log("\n\nTesting Bounds 1 on Dataset B");

    GroceryItems grocery_items = getTable<GroceryItems>(TestGroceriesTableName);

    // Ensure no items are present
    {
        BAL::Verify(grocery_items.begin() == grocery_items.end(), "The test should begin with an empty set of grocery items");
    }


    /// Add items
    populateDatasetB(grocery_items);


    ///
    /// Check table bounds
    /// The expected forward order is: Apple, Banana, Carrot, Date
    ///
    {
        BAL::Log("=> Checking Table Bounds");

        BAL::Verify(grocery_items.contains(GroceryId{0}), "Item 0 should be present");
        auto itr0 = grocery_items.findId(GroceryId{0});
        BAL::Verify(itr0 != grocery_items.end(), "Did not find Item 0");
        BAL::Verify(itr0->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr0->name == "Apple", "Did not find the expected Apple");

        BAL::Verify(grocery_items.contains(GroceryId{1}), "Item 1 should be present");
        auto itr1 = grocery_items.findId(GroceryId{1});
        BAL::Verify(itr1 != grocery_items.end(), "Did not find Item 1");
        BAL::Verify(itr1->id == 1, "Did not find the expected ID for Banana");
        BAL::Verify(itr1->name == "Banana", "Did not find the expected Banana");

        BAL::Verify(grocery_items.contains(GroceryId{2}), "Item 2 should be present");
        auto itr2 = grocery_items.findId(GroceryId{2});
        BAL::Verify(itr2 != grocery_items.end(), "Did not find Item 2");
        BAL::Verify(itr2->id == 2, "Did not find the expected ID for Carrot");
        BAL::Verify(itr2->name == "Carrot", "Did not find the expected Carrot");

        BAL::Verify(grocery_items.contains(GroceryId{3}), "Item 3 should be present");
        auto itr3 = grocery_items.findId(GroceryId{3});
        BAL::Verify(itr3 != grocery_items.end(), "Did not find Item 3");
        BAL::Verify(itr3->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr3->name == "Date", "Did not find the expected Date");

        BAL::Verify(!grocery_items.contains(GroceryId{4}), "An item of unknown origin was found");

        auto lb0 = grocery_items.lowerBound(GroceryId{0});
        auto ub0 = grocery_items.upperBound(GroceryId{0});
        BAL::Verify(lb0 == itr0, "Lower bound for Item 0 was not correctly identified");
        BAL::Verify(ub0 == itr1, "Upper bound for Item 0 was not correctly identified");

        auto lb1 = grocery_items.lowerBound(GroceryId{1});
        auto ub1 = grocery_items.upperBound(GroceryId{1});
        BAL::Verify(lb1 == itr1, "Lower bound for Item 1 was not correctly identified");
        BAL::Verify(ub1 == itr2, "Upper bound for Item 1 was not correctly identified");

        auto lb2 = grocery_items.lowerBound(GroceryId{2});
        auto ub2 = grocery_items.upperBound(GroceryId{2});
        BAL::Verify(lb2 == itr2, "Lower bound for Item 2 was not correctly identified");
        BAL::Verify(ub2 == itr3, "Upper bound for Item 2 was not correctly identified");

        auto lb3 = grocery_items.lowerBound(GroceryId{3});
        auto ub3 = grocery_items.upperBound(GroceryId{3});
        BAL::Verify(lb3 == itr3, "Lower bound for Item 3 was not correctly identified");
        BAL::Verify(ub3 == grocery_items.end(), "Upper bound for Item 3 was not correctly identified");
    }


    ///
    /// Test secondary index bound for net weight
    /// The expected forward order is: Apple, Banana, Carrot, Date
    ///
    {
        BAL::Log("=> Checking Secondary Index Bounds");

        auto by_netweight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        BAL::Verify(by_netweight.contains(10), "Item 0 should be present");
        auto itr0 = by_netweight.find(10);
        BAL::Verify(itr0 != by_netweight.end(), "Did not find Item 0");
        BAL::Verify(itr0->id == 0, "Did not find the expected ID for Item 0");
        BAL::Verify(itr0->name == "Apple", "Did not find the expected Item 0");

        BAL::Verify(by_netweight.contains(20), "Item 1 should be present");
        auto itr1 = by_netweight.find(20);
        BAL::Verify(itr1 != by_netweight.end(), "Did not find Item 1");
        BAL::Verify(itr1->id == 1, "Did not find the expected ID for Item 1");
        BAL::Verify(itr1->name == "Banana", "Did not find the expected Item 1");

        BAL::Verify(by_netweight.contains(30), "Item 2 should be present");
        auto itr2 = by_netweight.find(30);
        BAL::Verify(itr2 != by_netweight.end(), "Did not find Item 2");
        BAL::Verify(itr2->id == 2, "Did not find the expected ID for Item 2");
        BAL::Verify(itr2->name == "Carrot", "Did not find the expected Item 2");

        BAL::Verify(by_netweight.contains(40), "Item 3 should be present");
        auto itr3 = by_netweight.find(40);
        BAL::Verify(itr3 != by_netweight.end(), "Did not find Item 3");
        BAL::Verify(itr3->id == 3, "Did not find the expected ID for Item 3");
        BAL::Verify(itr3->name == "Date", "Did not find the expected Item 3");

        // Peform exact searches
        auto lb0 = by_netweight.lowerBound(10);
        auto ub0 = by_netweight.upperBound(10);
        BAL::Verify(lb0 == itr0, "Lower bound for Item 0 was not correctly identified");
        BAL::Verify(ub0 == itr1, "Upper bound for Item 0 was not correctly identified");

        auto lb1 = by_netweight.lowerBound(20);
        auto ub1 = by_netweight.upperBound(20);
        BAL::Verify(lb1 == itr1, "Lower bound for Item 1 was not correctly identified");
        BAL::Verify(ub1 == itr2, "Upper bound for Item 1 was not correctly identified");

        auto lb2 = by_netweight.lowerBound(30);
        auto ub2 = by_netweight.upperBound(30);
        BAL::Verify(lb2 == itr2, "Lower bound for Item 2 was not correctly identified");
        BAL::Verify(ub2 == itr3, "Upper bound for Item 2 was not correctly identified");

        auto lb3 = by_netweight.lowerBound(40);
        auto ub3 = by_netweight.upperBound(40);
        BAL::Verify(lb3 == itr3, "Lower bound for Item 3 was not correctly identified");
        BAL::Verify(ub3 == by_netweight.end(), "Upper bound for Item 3 was not correctly identified");

        // Peform inexact searches
        auto lbBelow0 = by_netweight.lowerBound(5);
        auto ubBelow0 = by_netweight.upperBound(5);
        BAL::Verify(lbBelow0 == itr0, "Lower bound for below Item 0 was not correctly identified");
        BAL::Verify(ubBelow0 == itr0, "Upper bound for below Item 0 was not correctly identified");

        auto lbBetween0and1 = by_netweight.lowerBound(15);
        auto ubBetween0and1 = by_netweight.upperBound(15);
        BAL::Verify(lbBetween0and1 == itr1, "Lower bound for between Item 0 and 1 was not correctly identified");
        BAL::Verify(ubBetween0and1 == itr1, "Upper bound for between Item 0 and 1 was not correctly identified");

        auto lbBetween1and2 = by_netweight.lowerBound(25);
        auto ubBetween1and2 = by_netweight.upperBound(25);
        BAL::Verify(lbBetween1and2 == itr2, "Lower bound for between Item 1 and 2 was not correctly identified");
        BAL::Verify(ubBetween1and2 == itr2, "Upper bound for between Item 1 and 2 was not correctly identified");

        auto lbBetween2and3 = by_netweight.lowerBound(35);
        auto ubBetween2and3 = by_netweight.upperBound(35);
        BAL::Verify(lbBetween2and3 == itr3, "Lower bound for between Item 2 and 3 was not correctly identified");
        BAL::Verify(ubBetween2and3 == itr3, "Upper bound for between Item 2 and 3 was not correctly identified");

        auto lbAbove40 = by_netweight.lowerBound(45);
        auto ubAbove40 = by_netweight.upperBound(45);
        BAL::Verify(lbAbove40 == by_netweight.end(), "Lower bound for above last Item was not correctly identified");
        BAL::Verify(ubAbove40 == by_netweight.end(), "Upper bound for above last Item was not correctly identified");

    }


    ///
    /// Phase 1: Remove an entry
    ///
    {
        BAL::Log("=> Removing Apple");
        auto& item = grocery_items.getId(GroceryId{0}, "Could not find the grocery item");
        grocery_items.erase(item);
    }


    ///
    /// Check table bounds
    /// The expected forward order is: Banana, Carrot, Date
    ///
    {
        BAL::Log("=> Checking Table Bounds");


        BAL::Verify(!grocery_items.contains(GroceryId{0}), "Item 0 should be missing");

        BAL::Verify(grocery_items.contains(GroceryId{1}), "Item 1 should be present");
        auto itr1 = grocery_items.findId(GroceryId{1});
        BAL::Verify(itr1 != grocery_items.end(), "Did not find Item 1");
        BAL::Verify(itr1->id == 1, "Did not find the expected ID for Banana");
        BAL::Verify(itr1->name == "Banana", "Did not find the expected Banana");

        BAL::Verify(grocery_items.contains(GroceryId{2}), "Item 2 should be present");
        auto itr2 = grocery_items.findId(GroceryId{2});
        BAL::Verify(itr2 != grocery_items.end(), "Did not find Item 2");
        BAL::Verify(itr2->id == 2, "Did not find the expected ID for Carrot");
        BAL::Verify(itr2->name == "Carrot", "Did not find the expected Carrot");

        BAL::Verify(grocery_items.contains(GroceryId{3}), "Item 3 should be present");
        auto itr3 = grocery_items.findId(GroceryId{3});
        BAL::Verify(itr3 != grocery_items.end(), "Did not find Item 3");
        BAL::Verify(itr3->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr3->name == "Date", "Did not find the expected Date");

        BAL::Verify(!grocery_items.contains(GroceryId{4}), "An item of unknown origin was found");

        auto lb0 = grocery_items.lowerBound(GroceryId{0});
        auto ub0 = grocery_items.upperBound(GroceryId{0});
        BAL::Verify(lb0 == itr1, "Lower bound for Item 0 was not correctly identified");
        BAL::Verify(ub0 == itr1, "Upper bound for Item 0 was not correctly identified");

        auto lb1 = grocery_items.lowerBound(GroceryId{1});
        auto ub1 = grocery_items.upperBound(GroceryId{1});
        BAL::Verify(lb1 == itr1, "Lower bound for Item 1 was not correctly identified");
        BAL::Verify(ub1 == itr2, "Upper bound for Item 1 was not correctly identified");

        auto lb2 = grocery_items.lowerBound(GroceryId{2});
        auto ub2 = grocery_items.upperBound(GroceryId{2});
        BAL::Verify(lb2 == itr2, "Lower bound for Item 2 was not correctly identified");
        BAL::Verify(ub2 == itr3, "Upper bound for Item 2 was not correctly identified");

        auto lb3 = grocery_items.lowerBound(GroceryId{3});
        auto ub3 = grocery_items.upperBound(GroceryId{3});
        BAL::Verify(lb3 == itr3, "Lower bound for Item 3 was not correctly identified");
        BAL::Verify(ub3 == grocery_items.end(), "Upper bound for Item 3 was not correctly identified");
    }


    ///
    /// Test secondary index bound for net weight
    /// The expected forward order is: Banana, Carrot, Date
    ///
    {
        BAL::Log("=> Checking Secondary Index Bounds");

        auto by_netweight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        BAL::Verify(!by_netweight.contains(10), "Item 0 should be missing");
        auto itr0 = by_netweight.find(10);
        BAL::Verify(itr0 == by_netweight.end(), "Item 0 should be missing");

        BAL::Verify(by_netweight.contains(20), "Item 1 should be present");
        auto itr1 = by_netweight.find(20);
        BAL::Verify(itr1 != by_netweight.end(), "Did not find Item 1");
        BAL::Verify(itr1->id == 1, "Did not find the expected ID for Item 1");
        BAL::Verify(itr1->name == "Banana", "Did not find the expected Item 1");

        BAL::Verify(by_netweight.contains(30), "Item 2 should be present");
        auto itr2 = by_netweight.find(30);
        BAL::Verify(itr2 != by_netweight.end(), "Did not find Item 2");
        BAL::Verify(itr2->id == 2, "Did not find the expected ID for Item 2");
        BAL::Verify(itr2->name == "Carrot", "Did not find the expected Item 2");

        BAL::Verify(by_netweight.contains(40), "Item 3 should be present");
        auto itr3 = by_netweight.find(40);
        BAL::Verify(itr3 != by_netweight.end(), "Did not find Item 3");
        BAL::Verify(itr3->id == 3, "Did not find the expected ID for Item 3");
        BAL::Verify(itr3->name == "Date", "Did not find the expected Item 3");

        // Peform exact searches
        auto lb0 = by_netweight.lowerBound(10);
        auto ub0 = by_netweight.upperBound(10);
        BAL::Verify(lb0 == itr1, "Lower bound for Item 0 was not correctly identified");
        BAL::Verify(ub0 == itr1, "Upper bound for Item 0 was not correctly identified");

        auto lb1 = by_netweight.lowerBound(20);
        auto ub1 = by_netweight.upperBound(20);
        BAL::Verify(lb1 == itr1, "Lower bound for Item 1 was not correctly identified");
        BAL::Verify(ub1 == itr2, "Upper bound for Item 1 was not correctly identified");

        auto lb2 = by_netweight.lowerBound(30);
        auto ub2 = by_netweight.upperBound(30);
        BAL::Verify(lb2 == itr2, "Lower bound for Item 2 was not correctly identified");
        BAL::Verify(ub2 == itr3, "Upper bound for Item 2 was not correctly identified");

        auto lb3 = by_netweight.lowerBound(40);
        auto ub3 = by_netweight.upperBound(40);
        BAL::Verify(lb3 == itr3, "Lower bound for Item 3 was not correctly identified");
        BAL::Verify(ub3 == by_netweight.end(), "Upper bound for Item 3 was not correctly identified");

        // Peform inexact searches
        auto lbBelow0 = by_netweight.lowerBound(5);
        auto ubBelow0 = by_netweight.upperBound(5);
        BAL::Verify(lbBelow0 == itr1, "Lower bound for below Item 0 was not correctly identified");
        BAL::Verify(ubBelow0 == itr1, "Upper bound for below Item 0 was not correctly identified");

        auto lbBetween0and1 = by_netweight.lowerBound(15);
        auto ubBetween0and1 = by_netweight.upperBound(15);
        BAL::Verify(lbBetween0and1 == itr1, "Lower bound for between Item 0 and 1 was not correctly identified");
        BAL::Verify(ubBetween0and1 == itr1, "Upper bound for between Item 0 and 1 was not correctly identified");

        auto lbBetween1and2 = by_netweight.lowerBound(25);
        auto ubBetween1and2 = by_netweight.upperBound(25);
        BAL::Verify(lbBetween1and2 == itr2, "Lower bound for between Item 1 and 2 was not correctly identified");
        BAL::Verify(ubBetween1and2 == itr2, "Upper bound for between Item 1 and 2 was not correctly identified");

        auto lbBetween2and3 = by_netweight.lowerBound(35);
        auto ubBetween2and3 = by_netweight.upperBound(35);
        BAL::Verify(lbBetween2and3 == itr3, "Lower bound for between Item 2 and 3 was not correctly identified");
        BAL::Verify(ubBetween2and3 == itr3, "Upper bound for between Item 2 and 3 was not correctly identified");

        auto lbAbove40 = by_netweight.lowerBound(45);
        auto ubAbove40 = by_netweight.upperBound(45);
        BAL::Verify(lbAbove40 == by_netweight.end(), "Lower bound for above last Item was not correctly identified");
        BAL::Verify(ubAbove40 == by_netweight.end(), "Upper bound for above last Item was not correctly identified");
    }


    ///
    /// Phase 2: Remove an entry
    ///
    {
        BAL::Log("=> Removing Banana");
        auto& item = grocery_items.getId(GroceryId{1}, "Could not find the grocery item");
        grocery_items.erase(item);
    }


    ///
    /// Check table bounds
    /// The expected forward order is: Carrot, Date
    ///
    {
        BAL::Log("=> Checking Table Bounds");


        BAL::Verify(!grocery_items.contains(GroceryId{0}), "Item 0 should be missing");
        BAL::Verify(grocery_items.findId(GroceryId{0}) == grocery_items.end(), "Item 0 should be missing");

        BAL::Verify(!grocery_items.contains(GroceryId{1}), "Item 1 should be missing");
        BAL::Verify(grocery_items.findId(GroceryId{1}) == grocery_items.end(), "Item 1 should be missing");

        BAL::Verify(grocery_items.contains(GroceryId{2}), "Item 2 should be present");
        auto itr2 = grocery_items.findId(GroceryId{2});
        BAL::Verify(itr2 != grocery_items.end(), "Did not find Item 2");
        BAL::Verify(itr2->id == 2, "Did not find the expected ID for Carrot");
        BAL::Verify(itr2->name == "Carrot", "Did not find the expected Carrot");

        BAL::Verify(grocery_items.contains(GroceryId{3}), "Item 3 should be present");
        auto itr3 = grocery_items.findId(GroceryId{3});
        BAL::Verify(itr3 != grocery_items.end(), "Did not find Item 3");
        BAL::Verify(itr3->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr3->name == "Date", "Did not find the expected Date");

        BAL::Verify(!grocery_items.contains(GroceryId{4}), "An item of unknown origin was found");

        auto lb0 = grocery_items.lowerBound(GroceryId{0});
        auto ub0 = grocery_items.upperBound(GroceryId{0});
        BAL::Verify(lb0 == itr2, "Lower bound for Item 0 was not correctly identified");
        BAL::Verify(ub0 == itr2, "Upper bound for Item 0 was not correctly identified");

        auto lb1 = grocery_items.lowerBound(GroceryId{1});
        auto ub1 = grocery_items.upperBound(GroceryId{1});
        BAL::Verify(lb1 == itr2, "Lower bound for Item 1 was not correctly identified");
        BAL::Verify(ub1 == itr2, "Upper bound for Item 1 was not correctly identified");

        auto lb2 = grocery_items.lowerBound(GroceryId{2});
        auto ub2 = grocery_items.upperBound(GroceryId{2});
        BAL::Verify(lb2 == itr2, "Lower bound for Item 2 was not correctly identified");
        BAL::Verify(ub2 == itr3, "Upper bound for Item 2 was not correctly identified");

        auto lb3 = grocery_items.lowerBound(GroceryId{3});
        auto ub3 = grocery_items.upperBound(GroceryId{3});
        BAL::Verify(lb3 == itr3, "Lower bound for Item 3 was not correctly identified");
        BAL::Verify(ub3 == grocery_items.end(), "Upper bound for Item 3 was not correctly identified");
    }


    ///
    /// Test secondary index bound for net weight
    /// The expected forward order is: Carrot, Date
    ///
    {
        BAL::Log("=> Checking Secondary Index Bounds");

        auto by_netweight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        BAL::Verify(!by_netweight.contains(10), "Item 0 should be missing");
        auto itr0 = by_netweight.find(10);
        BAL::Verify(itr0 == by_netweight.end(), "Item 0 should be missing");

        BAL::Verify(!by_netweight.contains(20), "Item 1 should be missing");
        auto itr1 = by_netweight.find(20);
        BAL::Verify(itr1 == by_netweight.end(), "Item 1 should be missing");

        BAL::Verify(by_netweight.contains(30), "Item 2 should be present");
        auto itr2 = by_netweight.find(30);
        BAL::Verify(itr2 != by_netweight.end(), "Did not find Item 2");
        BAL::Verify(itr2->id == 2, "Did not find the expected ID for Item 2");
        BAL::Verify(itr2->name == "Carrot", "Did not find the expected Item 2");

        BAL::Verify(by_netweight.contains(40), "Item 3 should be present");
        auto itr3 = by_netweight.find(40);
        BAL::Verify(itr3 != by_netweight.end(), "Did not find Item 3");
        BAL::Verify(itr3->id == 3, "Did not find the expected ID for Item 3");
        BAL::Verify(itr3->name == "Date", "Did not find the expected Item 3");

        // Peform exact searches
        auto lb0 = by_netweight.lowerBound(10);
        auto ub0 = by_netweight.upperBound(10);
        BAL::Verify(lb0 == itr2, "Lower bound for Item 0 was not correctly identified");
        BAL::Verify(ub0 == itr2, "Upper bound for Item 0 was not correctly identified");

        auto lb1 = by_netweight.lowerBound(20);
        auto ub1 = by_netweight.upperBound(20);
        BAL::Verify(lb1 == itr2, "Lower bound for Item 1 was not correctly identified");
        BAL::Verify(ub1 == itr2, "Upper bound for Item 1 was not correctly identified");

        auto lb2 = by_netweight.lowerBound(30);
        auto ub2 = by_netweight.upperBound(30);
        BAL::Verify(lb2 == itr2, "Lower bound for Item 2 was not correctly identified");
        BAL::Verify(ub2 == itr3, "Upper bound for Item 2 was not correctly identified");

        auto lb3 = by_netweight.lowerBound(40);
        auto ub3 = by_netweight.upperBound(40);
        BAL::Verify(lb3 == itr3, "Lower bound for Item 3 was not correctly identified");
        BAL::Verify(ub3 == by_netweight.end(), "Upper bound for Item 3 was not correctly identified");

        // Peform inexact searches
        auto lbBelow0 = by_netweight.lowerBound(5);
        auto ubBelow0 = by_netweight.upperBound(5);
        BAL::Verify(lbBelow0 == itr2, "Lower bound for below Item 0 was not correctly identified");
        BAL::Verify(ubBelow0 == itr2, "Upper bound for below Item 0 was not correctly identified");

        auto lbBetween0and1 = by_netweight.lowerBound(15);
        auto ubBetween0and1 = by_netweight.upperBound(15);
        BAL::Verify(lbBetween0and1 == itr2, "Lower bound for between Item 0 and 1 was not correctly identified");
        BAL::Verify(ubBetween0and1 == itr2, "Upper bound for between Item 0 and 1 was not correctly identified");

        auto lbBetween1and2 = by_netweight.lowerBound(25);
        auto ubBetween1and2 = by_netweight.upperBound(25);
        BAL::Verify(lbBetween1and2 == itr2, "Lower bound for between Item 1 and 2 was not correctly identified");
        BAL::Verify(ubBetween1and2 == itr2, "Upper bound for between Item 1 and 2 was not correctly identified");

        auto lbBetween2and3 = by_netweight.lowerBound(35);
        auto ubBetween2and3 = by_netweight.upperBound(35);
        BAL::Verify(lbBetween2and3 == itr3, "Lower bound for between Item 2 and 3 was not correctly identified");
        BAL::Verify(ubBetween2and3 == itr3, "Upper bound for between Item 2 and 3 was not correctly identified");

        auto lbAbove40 = by_netweight.lowerBound(45);
        auto ubAbove40 = by_netweight.upperBound(45);
        BAL::Verify(lbAbove40 == by_netweight.end(), "Lower bound for above last Item was not correctly identified");
        BAL::Verify(ubAbove40 == by_netweight.end(), "Upper bound for above last Item was not correctly identified");
    }


    ///
    /// Phase 3: Remove an entry
    ///
    {
        BAL::Log("=> Removing Carrot");
        auto& item = grocery_items.getId(GroceryId{2}, "Could not find the grocery item");
        grocery_items.erase(item);
    }


    ///
    /// Check table bounds
    /// The expected forward order is: Date
    ///
    {
        BAL::Log("=> Checking Table Bounds");


        BAL::Verify(!grocery_items.contains(GroceryId{0}), "Item 0 should be missing");
        BAL::Verify(grocery_items.findId(GroceryId{0}) == grocery_items.end(), "Item 0 should be missing");

        BAL::Verify(!grocery_items.contains(GroceryId{1}), "Item 1 should be missing");
        BAL::Verify(grocery_items.findId(GroceryId{1}) == grocery_items.end(), "Item 1 should be missing");

        BAL::Verify(!grocery_items.contains(GroceryId{2}), "Item 2 should be missing");
        BAL::Verify(grocery_items.findId(GroceryId{2}) == grocery_items.end(), "Item 2 should be missing");

        BAL::Verify(grocery_items.contains(GroceryId{3}), "Item 3 should be present");
        auto itr3 = grocery_items.findId(GroceryId{3});
        BAL::Verify(itr3 != grocery_items.end(), "Did not find Item 3");
        BAL::Verify(itr3->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr3->name == "Date", "Did not find the expected Date");

        BAL::Verify(!grocery_items.contains(GroceryId{4}), "An item of unknown origin was found");

        auto lb0 = grocery_items.lowerBound(GroceryId{0});
        auto ub0 = grocery_items.upperBound(GroceryId{0});
        BAL::Verify(lb0 == itr3, "Lower bound for Item 0 was not correctly identified");
        BAL::Verify(ub0 == itr3, "Upper bound for Item 0 was not correctly identified");

        auto lb1 = grocery_items.lowerBound(GroceryId{1});
        auto ub1 = grocery_items.upperBound(GroceryId{1});
        BAL::Verify(lb1 == itr3, "Lower bound for Item 1 was not correctly identified");
        BAL::Verify(ub1 == itr3, "Upper bound for Item 1 was not correctly identified");

        auto lb2 = grocery_items.lowerBound(GroceryId{2});
        auto ub2 = grocery_items.upperBound(GroceryId{2});
        BAL::Verify(lb2 == itr3, "Lower bound for Item 2 was not correctly identified");
        BAL::Verify(ub2 == itr3, "Upper bound for Item 2 was not correctly identified");

        auto lb3 = grocery_items.lowerBound(GroceryId{3});
        auto ub3 = grocery_items.upperBound(GroceryId{3});
        BAL::Verify(lb3 == itr3, "Lower bound for Item 3 was not correctly identified");
        BAL::Verify(ub3 == grocery_items.end(), "Upper bound for Item 3 was not correctly identified");
    }


    ///
    /// Test secondary index bound for net weight
    /// The expected forward order is: Date
    ///
    {
        BAL::Log("=> Checking Secondary Index Bounds");

        auto by_netweight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        BAL::Verify(!by_netweight.contains(10), "Item 0 should be missing");
        auto itr0 = by_netweight.find(10);
        BAL::Verify(itr0 == by_netweight.end(), "Item 0 should be missing");

        BAL::Verify(!by_netweight.contains(20), "Item 1 should be missing");
        auto itr1 = by_netweight.find(20);
        BAL::Verify(itr1 == by_netweight.end(), "Item 1 should be missing");

        BAL::Verify(!by_netweight.contains(30), "Item 2 should be missing");
        auto itr2 = by_netweight.find(30);
        BAL::Verify(itr2 == by_netweight.end(), "Item 2 should be missing");

        BAL::Verify(by_netweight.contains(40), "Item 3 should be present");
        auto itr3 = by_netweight.find(40);
        BAL::Verify(itr3 != by_netweight.end(), "Did not find Item 3");
        BAL::Verify(itr3->id == 3, "Did not find the expected ID for Item 3");
        BAL::Verify(itr3->name == "Date", "Did not find the expected Item 3");

        // Peform exact searches
        auto lb0 = by_netweight.lowerBound(10);
        auto ub0 = by_netweight.upperBound(10);
        BAL::Verify(lb0 == itr3, "Lower bound for Item 0 was not correctly identified");
        BAL::Verify(ub0 == itr3, "Upper bound for Item 0 was not correctly identified");

        auto lb1 = by_netweight.lowerBound(20);
        auto ub1 = by_netweight.upperBound(20);
        BAL::Verify(lb1 == itr3, "Lower bound for Item 1 was not correctly identified");
        BAL::Verify(ub1 == itr3, "Upper bound for Item 1 was not correctly identified");

        auto lb2 = by_netweight.lowerBound(30);
        auto ub2 = by_netweight.upperBound(30);
        BAL::Verify(lb2 == itr3, "Lower bound for Item 2 was not correctly identified");
        BAL::Verify(ub2 == itr3, "Upper bound for Item 2 was not correctly identified");

        auto lb3 = by_netweight.lowerBound(40);
        auto ub3 = by_netweight.upperBound(40);
        BAL::Verify(lb3 == itr3, "Lower bound for Item 3 was not correctly identified");
        BAL::Verify(ub3 == by_netweight.end(), "Upper bound for Item 3 was not correctly identified");

        // Peform inexact searches
        auto lbBelow0 = by_netweight.lowerBound(5);
        auto ubBelow0 = by_netweight.upperBound(5);
        BAL::Verify(lbBelow0 == itr3, "Lower bound for below Item 0 was not correctly identified");
        BAL::Verify(ubBelow0 == itr3, "Upper bound for below Item 0 was not correctly identified");

        auto lbBetween0and1 = by_netweight.lowerBound(15);
        auto ubBetween0and1 = by_netweight.upperBound(15);
        BAL::Verify(lbBetween0and1 == itr3, "Lower bound for between Item 0 and 1 was not correctly identified");
        BAL::Verify(ubBetween0and1 == itr3, "Upper bound for between Item 0 and 1 was not correctly identified");

        auto lbBetween1and2 = by_netweight.lowerBound(25);
        auto ubBetween1and2 = by_netweight.upperBound(25);
        BAL::Verify(lbBetween1and2 == itr3, "Lower bound for between Item 1 and 2 was not correctly identified");
        BAL::Verify(ubBetween1and2 == itr3, "Upper bound for between Item 1 and 2 was not correctly identified");

        auto lbBetween2and3 = by_netweight.lowerBound(35);
        auto ubBetween2and3 = by_netweight.upperBound(35);
        BAL::Verify(lbBetween2and3 == itr3, "Lower bound for between Item 2 and 3 was not correctly identified");
        BAL::Verify(ubBetween2and3 == itr3, "Upper bound for between Item 2 and 3 was not correctly identified");

        auto lbAbove40 = by_netweight.lowerBound(45);
        auto ubAbove40 = by_netweight.upperBound(45);
        BAL::Verify(lbAbove40 == by_netweight.end(), "Lower bound for above last Item was not correctly identified");
        BAL::Verify(ubAbove40 == by_netweight.end(), "Upper bound for above last Item was not correctly identified");
    }


    ///
    /// Phase 4: Remove an entry
    ///
    {
        BAL::Log("=> Removing Date");
        auto& item = grocery_items.getId(GroceryId{3}, "Could not find the grocery item");
        grocery_items.erase(item);
    }


    ///
    /// Check table bounds
    /// There should be no items
    ///
    {
        BAL::Log("=> Checking Table Bounds");

        BAL::Verify(!grocery_items.contains(GroceryId{0}), "Item 0 should be missing");
        BAL::Verify(grocery_items.findId(GroceryId{0}) == grocery_items.end(), "Item 0 should be missing");

        BAL::Verify(!grocery_items.contains(GroceryId{1}), "Item 1 should be missing");
        BAL::Verify(grocery_items.findId(GroceryId{1}) == grocery_items.end(), "Item 1 should be missing");

        BAL::Verify(!grocery_items.contains(GroceryId{2}), "Item 2 should be missing");
        BAL::Verify(grocery_items.findId(GroceryId{2}) == grocery_items.end(), "Item 2 should be missing");

        BAL::Verify(!grocery_items.contains(GroceryId{3}), "Item 3 should be missing");
        BAL::Verify(grocery_items.findId(GroceryId{3}) == grocery_items.end(), "Item 3 should be missing");

        BAL::Verify(!grocery_items.contains(GroceryId{4}), "An item of unknown origin was found");

        auto lb0 = grocery_items.lowerBound(GroceryId{0});
        auto ub0 = grocery_items.upperBound(GroceryId{0});
        BAL::Verify(lb0 == grocery_items.end(), "Lower bound for Item 0 was not correctly identified");
        BAL::Verify(ub0 == grocery_items.end(), "Upper bound for Item 0 was not correctly identified");

        auto lb1 = grocery_items.lowerBound(GroceryId{1});
        auto ub1 = grocery_items.upperBound(GroceryId{1});
        BAL::Verify(lb1 == grocery_items.end(), "Lower bound for Item 1 was not correctly identified");
        BAL::Verify(ub1 == grocery_items.end(), "Upper bound for Item 1 was not correctly identified");

        auto lb2 = grocery_items.lowerBound(GroceryId{2});
        auto ub2 = grocery_items.upperBound(GroceryId{2});
        BAL::Verify(lb2 == grocery_items.end(), "Lower bound for Item 2 was not correctly identified");
        BAL::Verify(ub2 == grocery_items.end(), "Upper bound for Item 2 was not correctly identified");

        auto lb3 = grocery_items.lowerBound(GroceryId{3});
        auto ub3 = grocery_items.upperBound(GroceryId{3});
        BAL::Verify(lb3 == grocery_items.end(), "Lower bound for Item 3 was not correctly identified");
        BAL::Verify(ub3 == grocery_items.end(), "Upper bound for Item 3 was not correctly identified");
    }


    ///
    /// Test secondary index bound for net weight
    /// There should be no items
    ///
    {
        BAL::Log("=> Checking Secondary Index Bounds");

        auto by_netweight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        BAL::Verify(!by_netweight.contains(10), "Item 0 should be missing");
        auto itr0 = by_netweight.find(10);
        BAL::Verify(itr0 == by_netweight.end(), "Item 0 should be missing");

        BAL::Verify(!by_netweight.contains(20), "Item 1 should be missing");
        auto itr1 = by_netweight.find(20);
        BAL::Verify(itr1 == by_netweight.end(), "Item 1 should be missing");

        BAL::Verify(!by_netweight.contains(30), "Item 2 should be missing");
        auto itr2 = by_netweight.find(30);
        BAL::Verify(itr2 == by_netweight.end(), "Item 2 should be missing");

        BAL::Verify(!by_netweight.contains(40), "Item 3 should be missing");
        auto itr3 = by_netweight.find(40);
        BAL::Verify(itr3 == by_netweight.end(), "Item 3 should be missing");

        // Peform exact searches
        auto lb0 = by_netweight.lowerBound(10);
        auto ub0 = by_netweight.upperBound(10);
        BAL::Verify(lb0 == by_netweight.end(), "Lower bound for Item 0 was not correctly identified");
        BAL::Verify(ub0 == by_netweight.end(), "Upper bound for Item 0 was not correctly identified");

        auto lb1 = by_netweight.lowerBound(20);
        auto ub1 = by_netweight.upperBound(20);
        BAL::Verify(lb1 == by_netweight.end(), "Lower bound for Item 1 was not correctly identified");
        BAL::Verify(ub1 == by_netweight.end(), "Upper bound for Item 1 was not correctly identified");

        auto lb2 = by_netweight.lowerBound(30);
        auto ub2 = by_netweight.upperBound(30);
        BAL::Verify(lb2 == by_netweight.end(), "Lower bound for Item 2 was not correctly identified");
        BAL::Verify(ub2 == by_netweight.end(), "Upper bound for Item 2 was not correctly identified");

        auto lb3 = by_netweight.lowerBound(40);
        auto ub3 = by_netweight.upperBound(40);
        BAL::Verify(lb3 == by_netweight.end(), "Lower bound for Item 3 was not correctly identified");
        BAL::Verify(ub3 == by_netweight.end(), "Upper bound for Item 3 was not correctly identified");

        // Peform inexact searches
        auto lbBelow0 = by_netweight.lowerBound(5);
        auto ubBelow0 = by_netweight.upperBound(5);
        BAL::Verify(lbBelow0 == by_netweight.end(), "Lower bound for below Item 0 was not correctly identified");
        BAL::Verify(ubBelow0 == by_netweight.end(), "Upper bound for below Item 0 was not correctly identified");

        auto lbBetween0and1 = by_netweight.lowerBound(15);
        auto ubBetween0and1 = by_netweight.upperBound(15);
        BAL::Verify(lbBetween0and1 == by_netweight.end(), "Lower bound for between Item 0 and 1 was not correctly identified");
        BAL::Verify(ubBetween0and1 == by_netweight.end(), "Upper bound for between Item 0 and 1 was not correctly identified");

        auto lbBetween1and2 = by_netweight.lowerBound(25);
        auto ubBetween1and2 = by_netweight.upperBound(25);
        BAL::Verify(lbBetween1and2 == by_netweight.end(), "Lower bound for between Item 1 and 2 was not correctly identified");
        BAL::Verify(ubBetween1and2 == by_netweight.end(), "Upper bound for between Item 1 and 2 was not correctly identified");

        auto lbBetween2and3 = by_netweight.lowerBound(35);
        auto ubBetween2and3 = by_netweight.upperBound(35);
        BAL::Verify(lbBetween2and3 == by_netweight.end(), "Lower bound for between Item 2 and 3 was not correctly identified");
        BAL::Verify(ubBetween2and3 == by_netweight.end(), "Upper bound for between Item 2 and 3 was not correctly identified");

        auto lbAbove40 = by_netweight.lowerBound(45);
        auto ubAbove40 = by_netweight.upperBound(45);
        BAL::Verify(lbAbove40 == by_netweight.end(), "Lower bound for above last Item was not correctly identified");
        BAL::Verify(ubAbove40 == by_netweight.end(), "Upper bound for above last Item was not correctly identified");
    }


    ///
    /// Clean the test artifacts
    ///
    ClearGroceries(grocery_items);

    BAL::Log("Test: PASSED");
}


/**
 * Test modifying an item such that its sequence in the secondary index changes.
 * Modify via the primary table's `void modify(const Object& obj, Modifier&& modifier)`.
 *
 * Dataset A: The sequence of the secondary index differs from the primary index
 */
void BALTests::testModify1A() {
    BAL::Log("\n\nTesting Modify 1A");

    GroceryItems grocery_items = getTable<GroceryItems>(TestGroceriesTableName);

    Modifier m = [&items=grocery_items] (const GroceryId& id, const uint32_t new_net_weight_grams) {
        const GroceryItem& item = items.getId(id);

        items.modify(item, [new_net_weight_grams](GroceryItem& i) {
            i.net_weight_grams = new_net_weight_grams;
        });
    };

    testModify1(grocery_items, m);
}

/**
 * Test modifying an item such that its sequence in the secondary index changes.
 * Modify via the secondary table's `void modify(const Object& obj, Modifier&& modifier)`
 *
 * Dataset A: The sequence of the secondary index differs from the primary index
 */
void BALTests::testModify1B() {
    BAL::Log("\n\nTesting Modify 1B");

    GroceryItems grocery_items = getTable<GroceryItems>(TestGroceriesTableName);
    auto idx2 = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

    Modifier m = [&items=grocery_items, &items2=idx2] (const GroceryId& id, const uint32_t new_net_weight_grams) {
        const GroceryItem& item = items.getId(id);

        items2.modify(item, [new_net_weight_grams](GroceryItem& i) {
            i.net_weight_grams = new_net_weight_grams;
        });
    };

    testModify1(grocery_items, m);
}

/**
 * Test modifying an item such that its sequence in the secondary index changes.
 * Modify via the primary table's `void modify(iterator itr, Modifier&& modifier)`.
 *
 * Dataset A: The sequence of the secondary index differs from the primary index
 */
void BALTests::testModify1C() {
    BAL::Log("\n\nTesting Modify 1C");

    GroceryItems grocery_items = getTable<GroceryItems>(TestGroceriesTableName);

    Modifier m = [&items=grocery_items] (const GroceryId& id, const uint32_t new_net_weight_grams) {
        auto itr = items.findId(id);
        BAL::Verify(itr != items.end(), "The item to be modified was not found");

        items.modify(itr, [new_net_weight_grams](GroceryItem& i) {
            i.net_weight_grams = new_net_weight_grams;
        });
    };

    testModify1(grocery_items, m);
}

/**
 * Test modifying an item such that its sequence in the secondary index changes.
 * Modify via the secondary index's `void modify(iterator itr, Modifier&& modifier)`.
 *
 * Dataset A: The sequence of the secondary index differs from the primary index
 */
void BALTests::testModify1D() {
    BAL::Log("\n\nTesting Modify 1D");

    GroceryItems grocery_items = getTable<GroceryItems>(TestGroceriesTableName);
    auto idx2 = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

    Modifier m = [&items=grocery_items, &items2=idx2] (const GroceryId& id, const uint32_t new_net_weight_grams) {
        const GroceryItem& item = items.getId(id);

        auto itr2 = items2.find(item.net_weight_grams);
        BAL::Verify(itr2 != items2.end(), "The item to be modified was not found");

        items2.modify(itr2, [new_net_weight_grams](GroceryItem& i) {
            i.net_weight_grams = new_net_weight_grams;
        });
    };

    testModify1(grocery_items, m);
}

/**
 * Dataset A: The sequence of the secondary index differs from the primary index
 *
 * Use the same GroceryItems that was instantiated during the initialization of the test
 * to avoid differences in "cached views" that can persist among different instances.
 */
void BALTests::testModify1(GroceryItems& grocery_items, Modifier& modifier) {
    ///
    /// Initialize Test
    ///
    // Require the contract's authority to run tests
    requireAuthorization(ownerAccount());

    // Ensure no items are present
    {
        BAL::Verify(grocery_items.begin() == grocery_items.end(), "The test should begin with an empty set of grocery items");
    }

    /// Add items
    populateDatasetA(grocery_items);


    /// Modify an item
    /// The initial sequence in the secondary index is:
    /// Apple, Banana, Date, Carrot
    ///
    /// Modify the secondary key of the Date object such the secondary sequence becomes
    /// Apple, Date, Banana, Carrot
    ///
    /// The sequence in the primary index should be unaffected.
    const GroceryItem& banana = grocery_items.getId(GroceryId{1}, "Could not find the grocery item");
    BAL::Verify(banana.id == 1, "Did not find the expected ID for the item");
    BAL::Verify(banana.name == "Banana", "Did not find the expected name for the item");
    BAL::Verify(banana.net_weight_grams == 35, "Did not find the expected net weight for the item");


    // Modify
    modifier(banana.id, 50);


    ///
    /// Test whether the item's content has changed
    ///
    BAL::Verify(banana.id == 1, "Did not find the expected ID for the item");
    BAL::Verify(banana.name == "Banana", "Did not find the expected name for the item");
    BAL::Verify(banana.net_weight_grams == 50, "Did not find the expected net weight for the item");

    const GroceryItem& banana2 = grocery_items.getId(GroceryId{1}, "Could not find the grocery item");
    BAL::Verify(banana2.id == 1, "Did not find the expected ID for the item");
    BAL::Verify(banana2.name == "Banana", "Did not find the expected name for the item");
    BAL::Verify(banana2.net_weight_grams == 50, "Did not find the expected net weight for the item");


    ///
    /// Test forward iteration of primary index
    /// The expected forward order is: Apple, Banana, Carrot, Date
    ///
    {
        BAL::Log("=> Primary Index, Forward Iteration");

        auto itr = grocery_items.begin();
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr->id == 1, "Did not find the expected ID for Banana");
        BAL::Verify(itr->name == "Banana", "Did not find the expected Banana");

        itr++;
        BAL::Verify(itr->id == 2, "Did not find the expected ID for Carrot");
        BAL::Verify(itr->name == "Carrot", "Did not find the expected Carrot");

        itr++;
        BAL::Verify(itr->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr->name == "Date", "Did not find the expected Date");

        itr++;
        BAL::Verify(itr == grocery_items.end(), "Did not find the expected end");
    }


    ///
    /// Test reverse iteration of primary index
    /// The expected reverse order is: Apple, Banana, Carrot, Date
    ///
    {
        BAL::Log("=> Primary Index, Reverse Iteration");

        auto itr = grocery_items.rbegin();
        BAL::Verify(itr->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr->name == "Date", "Did not find the expected Date");

        itr++;
        BAL::Verify(itr->id == 2, "Did not find the expected ID for Carrot");
        BAL::Verify(itr->name == "Carrot", "Did not find the expected Carrot");

        itr++;
        BAL::Verify(itr->id == 1, "Did not find the expected ID for Banana");
        BAL::Verify(itr->name == "Banana", "Did not find the expected Carrot");

        itr++;
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr == grocery_items.rend(), "Did not find the expected end");
    }


    ///
    /// Test forward iteration of secondary index
    /// By net weight, the expected forward order is: Apple, Date, Banana, Carrot
    ///
    {
        BAL::Log("=> Secondary Index, Forward Iteration");

        auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        auto itr = groceriesByNetWeight.begin();
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr->name == "Date", "Did not find the expected Date");

        itr++;
        BAL::Verify(itr->id == 1, "Did not find the expected ID for Banana");
        BAL::Verify(itr->name == "Banana", "Did not find the expected Banana");

        itr++;
        BAL::Verify(itr->id == 2, "Did not find the expected ID for Carrot");
        BAL::Verify(itr->name == "Carrot", "Did not find the expected Carrot");

        itr++;
        BAL::Verify(itr == groceriesByNetWeight.end(), "Did not find the expected end");
    }

    ///
    /// Test reverse iteration of secondary index
    /// By net weight, the expected reverse order is: Carrot, Banana, Date, Apple
    ///
    {
        BAL::Log("=> Secondary Index, Reverse Iteration");

        auto groceriesByNetWeight = grocery_items.getSecondaryIndex<BY_NET_WEIGHT>();

        auto itr = groceriesByNetWeight.rbegin();
        BAL::Verify(itr->id == 2, "Did not find the expected ID for Carrot");
        BAL::Verify(itr->name == "Carrot", "Did not find the expected Carrot");

        itr++;
        BAL::Verify(itr->id == 1, "Did not find the expected ID for Banana");
        BAL::Verify(itr->name == "Banana", "Did not find the expected Banana");

        itr++;
        BAL::Verify(itr->id == 3, "Did not find the expected ID for Date");
        BAL::Verify(itr->name == "Date", "Did not find the expected Date");

        itr++;
        BAL::Verify(itr->id == 0, "Did not find the expected ID for Apple");
        BAL::Verify(itr->name == "Apple", "Did not find the expected Apple");

        itr++;
        BAL::Verify(itr == groceriesByNetWeight.rend(), "Did not find the expected end");
    }


    ///
    /// Clean the test artifacts
    ///
    ClearGroceries(grocery_items);

    BAL::Log("Test: PASSED");
}
