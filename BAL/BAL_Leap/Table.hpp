#pragma once

#include <BAL/IndexHelpers.hpp>

#include <eosio/eosio.hpp>
#include <type_traits>

namespace BAL {

// This type transforms a class conforming with the SecondaryIndex interface into an eosio::indexed_by type
struct toEosioIndex {
    template<typename Idx>
    struct transform {
        using type = eosio::indexed_by<eosio::name(Idx::Tag.value),
                                       eosio::const_mem_fun<typename Idx::ObjectType,
                                                            typename Idx::FieldType, Idx::KeyGetter>>;
    };
};

// This template provides a high-level interface to access an EOSIO table described by the provided object.
template<typename Object>
class Table {
    // Given a variadic of eosio::indexed_by<> secondary indexes, return the fully expanded eosio::multi_index type
    template<typename... Secondaries>
    using GetEosioTable = eosio::multi_index<eosio::name(Object::TableName.value), Object, Secondaries...>;

public:
    using EosioTable = Util::TypeList::apply<
                          Util::TypeList::transform<SecondaryIndexes<Object>, toEosioIndex>,
                          GetEosioTable>;

protected:
    EosioTable table;

    template<Name::raw Tag, typename EosioIndex>
    class SecondaryIndex {
        EosioIndex index;

        template<typename Index>
        struct TagMatches { constexpr static bool value = Index::Tag == Tag; };
        static_assert(Util::TypeList::length<Util::TypeList::filter<SecondaryIndexes<Object>, TagMatches>>() == 1,
                      "Requested secondary index tag does not match exactly one secondary index");
        using IndexDescriptor = Util::TypeList::first<Util::TypeList::filter<SecondaryIndexes<Object>, TagMatches>>;
        using Key = typename IndexDescriptor::FieldType;

    public:
        // All iterators are const! Modification cannot be done by direct iterator access.
        using iterator = typename EosioIndex::const_iterator;
        using const_iterator = iterator;
        using reverse_iterator = typename EosioIndex::const_reverse_iterator;
        using const_reverse_iterator = reverse_iterator;

        SecondaryIndex(EosioIndex&& index) : index(std::move(index)) {}

        iterator begin() const { return index.begin(); }
        iterator end() const { return index.end(); }
        reverse_iterator rbegin() const { return index.rbegin(); }
        reverse_iterator rend() const { return index.rend(); }

        const Object& get(const Key& key, const char* error = "Couldn't find key") const {
            return index.get(key, error);
        }
        iterator find(const Key& key) const { return index.find(key); }
        bool contains(const Key& key) const { return find(key) != index.end(); }
        iterator lowerBound(const Key& key) const { return index.lower_bound(key); }
        iterator upperBound(const Key& key) const { return index.upper_bound(key); }
        std::pair<iterator, iterator> equalRange(const Key& key) const {
            return std::make_pair(lowerBound(key), upperBound(key));
        }
        std::pair<iterator, iterator> range(const Key& lowest, const Key& highest) const {
            return std::make_pair(lowerBound(lowest), upperBound(highest));
        }

        template<typename Constructor>
        const Object& create(Constructor&& ctor) { return *index.emplace(index.get_code(), ctor); }
        template<typename Constructor>
        const Object& create(AccountName payer, Constructor&& ctor) { return *index.emplace(payer, ctor); }
        template<typename Constructor>
        const Object& create(AccountId payer, Constructor&& ctor) { return *index.emplace(Name(payer), ctor); }
        template<typename Modifier>
        void modify(iterator itr, Modifier&& modifier) { index.modify(itr, index.get_code(), std::move(modifier)); }
        template<typename Modifier>
        void modify(iterator itr, AccountName payer, Modifier&& modifier) { index.modify(itr, payer, std::move(modifier)); }
        template<typename Modifier>
        void modify(iterator itr, AccountId payer, Modifier&& modifier) { index.modify(itr, Name(payer), std::move(modifier)); }
        template<typename Modifier>
        void modify(const Object& obj, Modifier&& modifier) {
            index.modify(index.iterator_to(obj), index.get_code(), std::move(modifier));
        }
        template<typename Modifier>
        void modify(const Object& obj, AccountName payer, Modifier&& modifier) {
            index.modify(index.iterator_to(obj), payer, std::move(modifier));
        }
        template<typename Modifier>
        void modify(const Object& obj, AccountId payer, Modifier&& modifier) {
            index.modify(index.iterator_to(obj), Name(payer), std::move(modifier));
        }
        iterator erase(iterator itr) { return index.erase(itr); }
        void erase(const Object& obj) { index.erase(index.iterator_to(obj)); }
    };

public:
    using PrimaryKey = decltype(std::declval<Object>().primary_key());

    // All iterators are const! Modification cannot be done by direct iterator access.
    using iterator = typename EosioTable::const_iterator;
    using const_iterator = iterator;
    using reverse_iterator = typename EosioTable::const_reverse_iterator;
    using const_reverse_iterator = reverse_iterator;

    Table(AccountName owner, Scope scope) : table(owner, scope) {}

    iterator begin() const { return table.begin(); }
    iterator end() const { return table.end(); }
    reverse_iterator rbegin() const { return table.rbegin(); }
    reverse_iterator rend() const { return table.rend(); }

    Scope scope() const { return table.get_scope(); }
    PrimaryKey nextId() const {
        if (begin() == end())
            return PrimaryKey();
        return (--end())->primary_key().incremented();
    }

    const Object& getId(PrimaryKey id, const char* error = "Couldn't find ID") const {
        return table.get(id, error);
    }
    iterator findId(PrimaryKey id) const { return table.find(id); }
    bool contains(PrimaryKey id) const { return findId(id) != table.end(); }
    iterator lowerBound(PrimaryKey lowest) const { return table.lower_bound(lowest); }
    iterator upperBound(PrimaryKey highest) const { return table.upper_bound(highest); }
    std::pair<iterator, iterator> getRange(PrimaryKey lowest = 0, PrimaryKey highest = ~0) const {
        return std::make_pair(lowerBound(lowest), upperBound(highest));
    }

    template<Name::raw Tag>
    auto getSecondaryIndex() {
        auto index = table.template get_index<eosio::name::raw(Tag)>();
        return SecondaryIndex<Tag, decltype(index)>(std::move(index));
    }

    template<typename Constructor>
    const Object& create(Constructor&& ctor) { return *table.emplace(table.get_code(), ctor); }
    template<typename Constructor>
    const Object& create(AccountName payer, Constructor&& ctor) { return *table.emplace(payer, ctor); }
    template<typename Constructor>
    const Object& create(AccountId payer, Constructor&& ctor) { return *table.emplace(Name(payer), ctor); }
    template<typename Modifier>
    void modify(iterator itr, Modifier&& modifier) { table.modify(itr, table.get_code(), std::move(modifier)); }
    template<typename Modifier>
    void modify(iterator itr, AccountName payer, Modifier&& modifier) { table.modify(itr, payer, std::move(modifier)); }
    template<typename Modifier>
    void modify(iterator itr, AccountId payer, Modifier&& modifier) { table.modify(itr, Name(payer), std::move(modifier)); }
    template<typename Modifier>
    void modify(const Object& obj, Modifier&& modifier) { table.modify(obj, table.get_code(), std::move(modifier)); }
    template<typename Modifier>
    void modify(const Object& obj, AccountName payer, Modifier&& modifier) { table.modify(obj, payer, std::move(modifier)); }
    template<typename Modifier>
    void modify(const Object& obj, AccountId payer, Modifier&& modifier) { table.modify(obj, Name(payer), std::move(modifier)); }
    iterator erase(iterator itr) { return table.erase(itr); }
    void erase(const Object& obj) { table.erase(obj); }
};

} // namespace BAL
