#pragma once

#include <BAL/IO.hpp>
#include <BAL/Name.hpp>

#include <limits>
#include <cstdint>
#include <type_traits>

namespace BAL {

// A tagged ID, so IDs with different tags have different types
template<typename TagType>
struct ID {
    using Tag = TagType;
    uint64_t value = 0;

    ID incremented() const {
        Verify(value < std::numeric_limits<uint64_t>::max(), "Unable to increment ID: ID is at maximum value");
        return ID{value+1};
    }

    operator uint64_t() const { return value; }

    friend bool operator==(const ID& a, const ID& b) { return a.value == b.value; }
    friend bool operator!=(const ID& a, const ID& b) { return a.value != b.value; }
    friend bool operator<(const ID& a, const ID& b) { return a.value < b.value; }
    friend bool operator<=(const ID& a, const ID& b) { return a.value <= b.value; }
    friend bool operator>(const ID& a, const ID& b) { return a.value > b.value; }
    friend bool operator>=(const ID& a, const ID& b) { return a.value >= b.value; }
};

// A convenient class for making ID tags from numbers
template<uint64_t Number>
struct NumberTag : public std::integral_constant<uint64_t, Number> {};

// This struct converts any Name value into a unique type, suitable for use as a Tag on an ID
template<Name::raw tag>
struct NameTag {
    constexpr static Name::raw toRaw = tag;
    constexpr static Name toName = Name(tag);
};

namespace impl {
inline uint64_t ParseTag(std::string idString, const std::string& magic) {
    BAL::Verify(idString.size() > magic.size()+1, "Cannot deserialize ID: ID is too short");
    BAL::Verify(idString.substr(0, magic.size()) == magic, "Cannot deserialize ID: wrong magic");
    BAL::Verify(idString[idString.size()-1] == '}', "Cannot deserialize ID: wrong magic");
    idString.erase(0, magic.size());
    idString.erase(idString.size()-1, 1);
    return stoull(idString);
}
} // namespace BAL::Impl

} // namespace BAL

// Define print helpers for IDs
#ifdef BAL_PLATFORM_EOSIO
namespace eosio {
template<typename Tag>
void print(BAL::ID<Tag> id) { print("TaggedID{", id.value, "}"); }
template<BAL::Name::raw Tag>
void print(BAL::ID<BAL::NameTag<Tag>> id) {  print("TaggedID<", BAL::Name(Tag), ">{", id.value, "}"); }
template<uint64_t Tag>
void print(BAL::ID<BAL::NumberTag<Tag>> id) {  print("TaggedID<", Tag, ">{", id.value, "}"); }
}
#endif
#ifdef BAL_PLATFORM_PEERPLAYS
#include <fc/variant.hpp>
namespace fc {
template<typename Tag>
void to_variant(const BAL::ID<Tag>& id, variant& v, uint32_t) {
    v = std::string("TaggedID{") + std::to_string(id.value) + "}";
}
template<BAL::Name::raw Tag>
void to_variant(const BAL::ID<BAL::NameTag<Tag>>& id, variant& v, uint32_t) {
    v = std::string("TaggedID<") + BAL::Name(Tag).to_string() + ">{" + std::to_string(id.value) + "}";
}
template<uint64_t Tag>
void to_variant(const BAL::ID<BAL::NumberTag<Tag>>& id, variant& v, uint32_t) {
    v = std::string("TaggedID<") + Tag + ">{" + std::to_string(id.value) + "}";
}

template<uint64_t Tag>
void from_variant(const variant& v, BAL::ID<BAL::NumberTag<Tag>>& id, uint32_t) {
    if (v.is_integer()) {
        id.value = v.as_uint64();
        return;
    }

    const static std::string prefix = std::string("TaggedID<") + std::to_string(Tag) + ">{";
    id.value = BAL::impl::ParseTag(v.as_string(), prefix);
}
template<BAL::Name::raw Tag>
void from_variant(const variant& v, BAL::ID<BAL::NameTag<Tag>>& id, uint32_t) {
    if (v.is_integer()) {
        id.value = v.as_uint64();
        return;
    }

    const static std::string prefix = std::string("TaggedID<") + BAL::Name(Tag).to_string() + ">{";
    id.value = BAL::impl::ParseTag(v.as_string(), prefix);
}
template<typename Tag>
void from_variant(const variant& v, BAL::ID<Tag>& id, uint32_t) {
    if (v.is_integer()) {
        id.value = v.as_uint64();
        return;
    }

    const static std::string prefix("TaggedID{");
    id.value = BAL::impl::ParseTag(v.as_string(), prefix);
}
}
#endif

