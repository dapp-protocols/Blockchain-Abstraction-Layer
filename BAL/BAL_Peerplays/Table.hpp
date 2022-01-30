#pragma once

#include <BAL/FcSerialization.hpp>
#include <BAL/Types.hpp>
#include <BAL/IndexHelpers.hpp>
#include <BAL/Declarations.hpp>

#include <graphene/chain/database.hpp>

#include <fc/io/raw_fwd.hpp>

#include <boost/iterator/transform_iterator.hpp>
#include <boost/range/adaptor/reversed.hpp>

#include <type_traits>

namespace BAL {

/*
 * The Table abstraction on Peerplays is notably more complex than its counterpart on EOSIO, because while EOSIO
 * defines the concept of table Scope natively, Peerplays does not and thus the Table must support the abstraction,
 * but define it as a no-op. This isn't higly complex, but it isn't trivial either. Here's how it works:
 *
 * The table doesn't store the Object type directly, but instead it stores a Scope object which inherits Object and
 * adds a scope field. The indexes are then all augmented to implicitly include the scope field in their keys.
 *
 * It involves some template arithmetic that isn't necessary on EOSIO, but once you understand what's going on, it
 * should be fairly straightforward to follow.
 */

using Scope = uint64_t;

// Tag type for the primary index of a table
struct ById;

// Forward declare Table template
template<typename>
class Table;

// Peerplays has no notion of scope built in, so we must define it by storing an Object with a scope field built
// in and embedding it into every key by turning those keys into composite keys with the scope as the first field.
// This struct inherits Object and adds the scope field to it, and thus forms a suitable type for storage.
template<typename Object>
struct ScopedObject : public graphene::db::abstract_object<ScopedObject<Object>> {
    using ObjectType = Object;
    using Contract = typename Object::Contract;

    constexpr static const uint8_t& space_id = Contract::OBJECT_SPACE_ID;
    constexpr static const uint8_t& type_id =
            Util::TypeList::indexOf<typename ContractDeclarations<Contract>::Tables, Table<Object>>();

    Object object;
    Scope scope;

    static const ScopedObject& fromObject(const Object& o) {
        static ScopedObject s;
        std::size_t offset = reinterpret_cast<std::size_t>(&s.object) - reinterpret_cast<std::size_t>(&s);
        return *reinterpret_cast<const ScopedObject*>(reinterpret_cast<const char*>(&o) - offset);
    }

    auto primary_key() const { return object.primary_key(); }
    template<typename SecondaryIndex>
    auto secondary_key() const { auto getter = SecondaryIndex::KeyGetter; return (object.*getter)(); }
};

// This type transforms a class conforming with the SecondaryIndex interface into a boost multi_index type
struct toMultiIndex {
    template<typename Idx>
    struct transform {
        using Object = typename Idx::ObjectType;
        using Scoped = ScopedObject<Object>;
        using type =
            boost::multi_index::ordered_non_unique<boost::multi_index::tag<NameTag<Idx::Tag>>,
                                                   boost::multi_index::composite_key<Scoped,
                                                    boost::multi_index::member<Scoped, Scope, &Scoped::scope>,
                                                    boost::multi_index::const_mem_fun<Scoped, typename Idx::FieldType,
                                                                              &Scoped::template secondary_key<Idx>>>>;
    };
};

template<typename ScopedObject>
struct ScopedToObject {
    typename ScopedObject::ObjectType& operator()(ScopedObject& s) const { return s.object; }
    const typename ScopedObject::ObjectType& operator()(const ScopedObject& s) const { return s.object; }
};

// This template provides a high-level interface to access a Peerplays table described by the provided object.
template<typename Object>
class Table {
public:
    using Row = Object;
    using PrimaryKey = decltype(std::declval<Object>().primary_key());
    using Scoped = ScopedObject<Object>;

protected:
    using GrapheneIndex =
        boost::multi_index::ordered_unique<boost::multi_index::tag<graphene::db::by_id>,
                                           boost::multi_index::member<graphene::db::object,
                                                                      graphene::db::object_id_type,
                                                                      &graphene::db::object::id>>;
    using PrimaryIndex =
        boost::multi_index::ordered_unique<boost::multi_index::tag<ById>,
                                           boost::multi_index::composite_key<Scoped,
                                              boost::multi_index::member<Scoped, Scope, &Scoped::scope>,
                                              boost::multi_index::const_mem_fun<Scoped, PrimaryKey,
                                                                                &Scoped::primary_key>>>;

    // Given a variadic of secondary indexes, return the fully expanded boost::multi_index_container type
    template<typename... Secondaries>
    using GetBoostTable =
        boost::multi_index_container<Scoped, boost::multi_index::indexed_by<GrapheneIndex, PrimaryIndex,
                                                                            Secondaries...>>;
    template<typename... Secondaries>
    using GetGrapheneTable =
        graphene::chain::generic_index<Scoped, GetBoostTable<Secondaries...>>;

public:
    using BoostTable = Util::TypeList::apply<Util::TypeList::transform<SecondaryIndexes<Object>, toMultiIndex>,
                                             GetBoostTable>;
    using GrapheneTable = Util::TypeList::apply<Util::TypeList::transform<SecondaryIndexes<Object>, toMultiIndex>,
                                                GetGrapheneTable>;

protected:
    graphene::chain::database& db;
    const GrapheneTable& table;
    const typename boost::multi_index::index<BoostTable, ById>::type& primaryIndex =
            table.indices().template get<ById>();
    Scope _scope;

    auto boostKey(const PrimaryKey& key) const { return std::make_tuple(_scope, key); }

    template<Name::raw Tag, typename BoostIndex>
    class SecondaryIndex {
        graphene::chain::database& db;
        const BoostIndex& index;
        Scope scope;

        template<typename Index>
        struct TagMatches { constexpr static bool value = Index::Tag == Tag; };
        static_assert(Util::TypeList::length<Util::TypeList::filter<SecondaryIndexes<Object>, TagMatches>>() == 1,
                      "Requested secondary index tag does not match exactly one secondary index");
        using IndexDescriptor = Util::TypeList::first<Util::TypeList::filter<SecondaryIndexes<Object>, TagMatches>>;
        using Key = typename IndexDescriptor::FieldType;

        auto boostKey(const Key& key) const { return boost::make_tuple(scope, key); }

    public:
        // Iterators transform the Scoped to its object member upon reference
        // All iterators are const! Modification cannot be done by direct iterator access.
        using iterator = boost::iterators::transform_iterator<ScopedToObject<Scoped>,
                                                              typename BoostIndex::const_iterator>;
        using const_iterator = iterator;
        using reverse_iterator = boost::iterators::transform_iterator<ScopedToObject<Scoped>,
                                                                      typename BoostIndex::const_reverse_iterator>;
        using const_reverse_iterator = reverse_iterator;

        SecondaryIndex(graphene::chain::database& db, const BoostIndex& index, Scope scope)
            : db(db), index(index), scope(scope) {}

        iterator begin() const { return iterator(index.equal_range(scope).first); }
        iterator end() const { return iterator(index.equal_range(scope).second); }
        reverse_iterator rbegin() const {
            return reverse_iterator((index.equal_range(scope) | boost::adaptors::reversed).begin());
        }
        reverse_iterator rend() const {
            return reverse_iterator((index.equal_range(scope) | boost::adaptors::reversed).end());
        }

        const Object& get(const Key& key, const char* error = "Couldn't find key") const {
            // Take note that result here is a table iterator, not our wrapped iterator
            auto result = index.find(boostKey(key));
            Verify(result != index.end(), error);
            return result->object;
        }
        iterator find(const Key& key) const { return iterator(index.find(boostKey(key))); }
        bool contains(const Key& key) const { return index.find(boostKey(key)) != index.end(); }
        iterator lowerBound(const Key& key) const { return iterator(index.lower_bound(boostKey(key))); }
        iterator upperBound(const Key& key) const { return iterator(index.upper_bound(boostKey(key))); }
        std::pair<iterator, iterator> equalRange(const Key& key) const {
            return std::make_pair(lowerBound(key), upperBound(key));
        }
        std::pair<iterator, iterator> range(const Key& lowest, const Key& highest) const {
            return std::make_pair(lowerBound(lowest), upperBound(highest));
        }

        template<typename Constructor>
        const Object& create(Constructor&& ctor) {
            auto wrappedCtor = [ctor=std::move(ctor), s=scope](Scoped& scoped) {
                ctor(scoped.object);
                scoped.scope = s;
            };
            return db.create<Scoped>(std::move(wrappedCtor)).object;
        }
        template<typename Constructor>
        const Object& create(AccountName, Constructor&& ctor) { return create(std::forward<Constructor>(ctor)); }
        template<typename Constructor>
        const Object& create(AccountId, Constructor&& ctor) { return create(std::forward<Constructor>(ctor)); }
        template<typename Modifier>
        void modify(iterator itr, Modifier&& modifier) {
            auto wrappedMod = [mod = std::move(modifier)](Scoped& scoped) { mod(scoped.object); };
            db.modify<Scoped>(*itr.base(), std::move(wrappedMod));
        }
        template<typename Modifier>
        void modify(iterator itr, AccountName, Modifier&& modifier) { modify(itr, std::forward<Modifier>(modifier)); }
        template<typename Modifier>
        void modify(iterator itr, AccountId, Modifier&& modifier) { modify(itr, std::forward<Modifier>(modifier)); }
        template<typename Modifier>
        void modify(const Object& obj, Modifier&& modifier) {
            auto wrappedMod = [mod = std::move(modifier)](Scoped& scoped) { mod(scoped.object); };
            db.modify<Scoped>(index.iterator_to(Scoped::fromObject(obj)), std::move(wrappedMod));
        }
        template<typename Modifier>
        void modify(const Object& obj, AccountName, Modifier&& modifier) { modify(obj, std::forward<Modifier>(modifier)); }
        template<typename Modifier>
        void modify(const Object& obj, AccountId, Modifier&& modifier) { modify(obj, std::forward<Modifier>(modifier)); }
        iterator erase(iterator itr) { auto ret = itr; ++ret; db.remove(*itr.base()); return ret; }
        void erase(const Object& obj) { db.remove(Scoped::fromObject(obj)); }
    };

public:
    // Iterators transform the Scoped to its object member upon reference
    // All iterators are const! Modification cannot be done by direct iterator access.
    using iterator = boost::iterators::transform_iterator<ScopedToObject<Scoped>,
                                                       typename std::decay_t<decltype(primaryIndex)>::const_iterator>;
    using const_iterator = iterator;
    using reverse_iterator = boost::iterators::transform_iterator<ScopedToObject<Scoped>,
                                               typename std::decay_t<decltype(primaryIndex)>::const_reverse_iterator>;
    using const_reverse_iterator = reverse_iterator;

    Table(graphene::chain::database& db, Scope scope)
        : db(db), table(db.get_index_type<GrapheneTable>()), _scope(scope) {}

    iterator begin() const { return iterator(primaryIndex.equal_range(_scope).first); }
    iterator end() const { return iterator(primaryIndex.equal_range(_scope).second); }
    reverse_iterator rbegin() const {
        return reverse_iterator((primaryIndex.equal_range(_scope) | boost::adaptors::reversed).begin());
    }
    reverse_iterator rend() const {
        return reverse_iterator((primaryIndex.equal_range(_scope) | boost::adaptors::reversed).end());
    }

    Scope scope() const { return _scope; }
    PrimaryKey nextId() const {
        if (begin() == end())
            return PrimaryKey();
        return (--end())->primary_key().incremented();
    }

    const Object& getId(PrimaryKey id, const char* error = "Couldn't find ID") const {
        // Take note that result here is a table iterator, not our wrapped iterator
        auto result = primaryIndex.find(boostKey(id));
        Verify(result != primaryIndex.end(), error);
        return result->object;
    }
    iterator findId(PrimaryKey id) const { return iterator(primaryIndex.find(boostKey(id))); }
    bool contains(PrimaryKey id) const { return findId(id) != end(); }
    iterator lowerBound(PrimaryKey lowest) const { return iterator(primaryIndex.lower_bound(boostKey(lowest))); }
    iterator upperBound(PrimaryKey highest) const { return iterator(primaryIndex.upper_bound(boostKey(highest))); }
    std::pair<iterator, iterator> getRange(PrimaryKey lowest = 0, PrimaryKey highest = ~0) const {
        return std::make_pair(lowerBound(lowest), upperBound(highest));
    }

    template<Name::raw Tag>
    auto getSecondaryIndex() {
        auto& index = table.indices().template get<NameTag<Tag>>();
        return SecondaryIndex<Tag, std::decay_t<decltype(index)>>(db, index, scope());
    }

    template<typename Constructor>
    const Object& create(Constructor&& ctor) {
        auto wrapped = [ctor=std::move(ctor), s=scope()](Scoped& scoped) {
            ctor(scoped.object);
            scoped.scope = s;
        };
        return db.create<Scoped>(std::move(wrapped)).object;
    }
    template<typename Constructor>
    const Object& create(AccountName, Constructor&& ctor) { return create(std::forward<Constructor>(ctor)); }
    template<typename Constructor>
    const Object& create(AccountId, Constructor&& ctor) { return create(std::forward<Constructor>(ctor)); }
    template<typename Modifier>
    void modify(iterator itr, Modifier&& modifier) {
        auto wrapped = [mod=std::move(modifier)](Scoped& scoped) { mod(scoped.object); };
        db.modify<Scoped>(*itr.base(), std::move(wrapped));
    }
    template<typename Modifier>
    void modify(iterator itr, AccountName, Modifier&& modifier) { modify(itr, std::forward<Modifier>(modifier)); }
    template<typename Modifier>
    void modify(iterator itr, AccountId, Modifier&& modifier) { modify(itr, std::forward<Modifier>(modifier)); }
    template<typename Modifier>
    void modify(const Object& obj, Modifier&& modifier) {
        auto wrapped = [mod=std::move(modifier)](Scoped& scoped) { mod(scoped.object); };
        db.modify<Scoped>(Scoped::fromObject(obj), std::move(wrapped));
    }
    template<typename Modifier>
    void modify(const Object& obj, AccountName, Modifier&& modifier) { modify(obj, std::forward<Modifier>(modifier)); }
    template<typename Modifier>
    void modify(const Object& obj, AccountId, Modifier&& modifier) { modify(obj, std::forward<Modifier>(modifier)); }
    iterator erase(iterator itr) { auto ret = itr; ++ret; db.remove(*itr.base()); return ret; }
    void erase(const Object& obj) { db.remove(Scoped::fromObject(obj)); }
};

} // namespace BAL

FC_REFLECT_DERIVED_TEMPLATE((typename Object), BAL::ScopedObject<Object>, (graphene::db::object), (object)(scope))
