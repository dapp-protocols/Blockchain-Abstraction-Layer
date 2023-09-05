#pragma once

#include <BAL/Types.hpp>

#include <Utils/TypeList.hpp>
#include <Utils/StaticVariant.hpp>

namespace BAL {

// This template stores compile-time information about a secondary index on an object
template<Name::raw tag, typename Object, typename Field, Field (Object::*Getter)()const>
struct SecondaryIndex {
    constexpr static Name Tag = Name(tag);
    using ObjectType = Object;
    using FieldType = Field;
    constexpr static FieldType (ObjectType::*KeyGetter)()const = Getter;
};

// This template defines a list of SecondaryIndex instantiations declaring the secondary indexes on the table whose
// rows are defined by Object
template<typename Object, typename = void>
struct SecondaryIndexes_s { using type = Util::TypeList::List<>; };
template<typename Object>
using SecondaryIndexes = typename SecondaryIndexes_s<Object>::type;
// Automatically specialize SecondaryIndexes_s for any object which internally defines a publi SecondaryIndexes type
template<typename Object>
struct SecondaryIndexes_s<Object, std::void_t<typename Object::SecondaryIndexes>> {
    using type = typename Object::SecondaryIndexes;
};

// Pack a variant type-and-value combination into a uint64_t for use in an index. Assigns 3 bits to type, 61 to value.
template<typename... Ts>
inline uint64_t Decompose(const std::variant<Ts...>& id) {
    static_assert(sizeof...(Ts) < 0b1000, "Variant has too many types to be decomposed.");
    uint64_t value;
    value = std::visit([](auto&& arg) -> uint64_t {return arg;}, id);
    Verify(value < (1ull << 61), "Variant value is too large to be decomposed. Please report this error");
    return value | (uint64_t(id.index()) << 61);
}
// Get the variant which will decompose to the smallest possible value
template<typename V>
constexpr V Decompose_MIN() {
    Verify(std::variant_size_v<V> > 0, "Variant should contain at least one type");
    using T = std::variant_alternative_t<0, V>;
    return T{0};
}
// Get the variant which will decompose to the greatest possible value.
// NOTE: This value should not be stored in the database; if more types are added to the variant in an update, the
// Decompose_MAX value of that variant will change. This value is intended only for bounding searches on an index.
template<typename V>
inline constexpr V Decompose_MAX() {
    Verify(std::variant_size_v<V> > 0, "Variant should contain at least one type");
    using T = std::variant_alternative_t<std::variant_size_v<V> - 1, V>;
    return T{~0ull>>3};
}

// Convert various types into a uint64_t for use in a key
template<typename T>
inline uint64_t KeyCast(const T& subkey) { return subkey; }
inline uint64_t KeyCast(const Name& subkey) { return subkey.value; };
inline uint64_t KeyCast(const Name::raw& subkey) { return (uint64_t)subkey; };

// Combine several fields into a single value in order to use them all in a composite key index.
inline UInt128 MakeCompositeKey(uint64_t a, uint64_t b) { return (UInt128(a) << 64) + b; }
inline UInt256 MakeCompositeKey(uint64_t a, uint64_t b, uint64_t c) {
    return UInt256(std::array<uint64_t, 3>{a, b, c});
}
template<typename... Args>
inline auto MakeCompositeKey(Args... args) {
    return MakeCompositeKey(KeyCast(args)...);
}

// Make a key out of up to 32 bytes of string
inline UInt256 MakeStringKey(std::string_view str) {
    UInt256 key;
    strncpy((char*)key.data(), str.data(), std::min<uint64_t>(str.size(), 256/8));
    return key;
}

} // namespace BAL
