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
inline uint64_t Decompose(const Util::StaticVariant<Ts...>& id) {
    static_assert(sizeof...(Ts) < 0b1000, "Variant has too many types to be decomposed.");
    uint64_t value;
    Util::TypeList::runtime::Dispatch(Util::TypeList::List<Ts...>(), id.which(), [&id, &value](auto v) {
        using T = typename decltype(v)::type;
        static_assert(std::is_same_v<decltype(uint64_t(std::declval<T>())), uint64_t>,
                      "All types in a decomposed variant must be convertible to uint64.");
        value = uint64_t(id.template get<T>());
    });
    Verify(value < (1ull << 61), "Variant value is too large to be decomposed. Please report this error");
    return value | (uint64_t(id.which()) << 61);
}
// Get the variant which will decompose to the smallest possible value
template<typename V>
constexpr V Decompose_MIN() { return Util::TypeList::first<typename V::List>{0}; }
// Get the variant which will decompose to the greatest possible value.
// NOTE: This value should not be stored in the database; if more types are added to the variant in an update, the
// Decompose_MAX value of that variant will change. This value is intended only for bounding searches on an index.
template<typename V>
inline constexpr V Decompose_MAX() {
    using T = Util::TypeList::last<typename V::List>;
    return T{~0ull};
}

// Combine several fields into a single value in order to use them all in a composite key index.
inline UInt128 MakeCompositeKey(uint64_t a, uint64_t b) { return (UInt128(a) << 64) + b; }
inline UInt256 MakeCompositeKey(uint64_t a, uint64_t b, uint64_t c) {
    return UInt256(std::array<uint64_t, 3>{a, b, c});
}
template<typename... Args>
inline auto MakeCompositeKey(Args... args) { return MakeCompositeKey(uint64_t(args)...); }

// Make a key out of up to 32 bytes of string
inline UInt256 MakeStringKey(std::string_view str) {
    UInt256 key;
    strncpy((char*)key.data(), str.data(), std::min<uint64_t>(str.size(), 256/8));
    return key;
}

} // namespace BAL
