#pragma once

#include <Utils/StaticVariant.hpp>

#include <BAL/IO.hpp>

#include <fc/io/raw_fwd.hpp>

#include <variant>

namespace fc {

// Serialization for StaticVariant
template<typename... Ts>
void to_variant(const Util::StaticVariant<Ts...>& sv, variant& v, uint32_t max_depth) {
    namespace TL = Util::TypeList;
    BAL::Verify(max_depth > 0, "Unable to serialize value: max recursion depth exceeded");
    variant value = TL::runtime::Dispatch(TL::List<Ts...>(), sv.which(), [&sv, max_depth](auto t) {
        using Type = typename decltype(t)::type;
        return variant(sv.template get<Type>(), max_depth-1);
    });
    v = variants{sv.which(), std::move(value)};
}
template<typename... Ts>
void from_variant(const variant& v, Util::StaticVariant<Ts...>& sv, uint32_t max_depth) {
    namespace TL = Util::TypeList;
    BAL::Verify(max_depth > 0, "Unable to deserialize value: max recursion depth exceeded");
    auto list = v.get_array();
    sv = TL::runtime::Dispatch(TL::List<Ts...>(), list[0].as_uint64(),
            [&v=list[1], max_depth] (auto t) -> Util::StaticVariant<Ts...> {
        using Type = typename decltype(t)::type;
        return v.as<Type>(max_depth-1);
    });
}

// Serialization for std::variant
template<typename... Ts>
void to_variant(const std::variant<Ts...>& sv, variant& v, uint32_t max_depth) {
    namespace TL = Util::TypeList;
    BAL::Verify(max_depth > 0, "Unable to serialize value: max recursion depth exceeded");
    variant value = TL::runtime::Dispatch(TL::makeSequence<sizeof...(Ts)>(), sv.index(), [&sv, max_depth](auto t) {
        constexpr auto Index = typename decltype(t)::type();
        return variant(std::get<Index>(sv), max_depth-1);
    });
    v = variants{sv.index(), std::move(value)};
}
template<typename... Ts>
void from_variant(const variant& v, std::variant<Ts...>& sv, uint32_t max_depth) {
    namespace TL = Util::TypeList;
    BAL::Verify(max_depth > 0, "Unable to deserialize value: max recursion depth exceeded");
    auto list = v.get_array();
    sv = TL::runtime::Dispatch(TL::index<TL::List<Ts...>>(), list[0].as_uint64(),
            [&v=list[1], max_depth] (auto t) {
        constexpr auto Index = TL::first<typename decltype(t)::type>();
        using Type = TL::last<typename decltype(t)::type>;
        return std::variant<Ts...>(std::in_place_index_t<Index>(), v.as<Type>(max_depth-1));
    });
}

namespace raw {

template<typename Stream, typename... Ts>
void pack(Stream& s, const Util::StaticVariant<Ts...> v, uint32_t max_depth) {
    namespace TL = Util::TypeList;
    BAL::Verify(max_depth > 0, "Unable to pack value: max recursion depth exceeded");
    unsigned_int discriminant = v.which();
    pack(s, discriminant, max_depth-1);
    TL::runtime::Dispatch(TL::List<Ts...>(), discriminant.value, [&s, &v, max_depth] (auto t) {
        using Type = typename decltype(t)::type;
        pack(s, v.template get<Type>(), max_depth-1);
    });
}
template<typename Stream, typename... Ts>
void unpack(Stream& s, Util::StaticVariant<Ts...> v, uint32_t max_depth) {
    namespace TL = Util::TypeList;
    BAL::Verify(max_depth > 0, "Unable to unpack value: max recursion depth exceeded");
    unsigned_int discriminant;
    unpack(s, discriminant, max_depth-1);
    TL::runtime::Dispatch(TL::List<Ts...>(), discriminant.value, [&s, &v, max_depth] (auto t) {
        using Type = typename decltype(t)::type;
        Type value;
        unpack(s, value, max_depth-1);
        v = value;
    });
}

template<typename Stream, typename... Ts>
void pack(Stream& s, const std::variant<Ts...> v, uint32_t max_depth) {
    namespace TL = Util::TypeList;
    BAL::Verify(max_depth > 0, "Unable to pack value: max recursion depth exceeded");
    unsigned_int discriminant = v.index();
    pack(s, discriminant, max_depth-1);
    TL::runtime::Dispatch(TL::makeSequence<sizeof...(Ts)>(), discriminant.value, [&s, &v, max_depth] (auto t) {
        constexpr auto Index = typename decltype(t)::type();
        pack(s, std::get<Index>(v), max_depth-1);
    });
}
template<typename Stream, typename... Ts>
void unpack(Stream& s, std::variant<Ts...> v, uint32_t max_depth) {
    namespace TL = Util::TypeList;
    BAL::Verify(max_depth > 0, "Unable to unpack value: max recursion depth exceeded");
    unsigned_int discriminant;
    unpack(s, discriminant, max_depth-1);
    TL::runtime::Dispatch(TL::List<Ts...>(), discriminant.value, [&s, &v, max_depth] (auto t) {
        using Type = typename decltype(t)::type;
        Type value;
        unpack(s, value, max_depth-1);
        v = value;
    });
    TL::runtime::Dispatch(TL::index<TL::List<Ts...>>(), discriminant.value, [&s, &v, max_depth] (auto t) {
        constexpr auto Index = TL::first<typename decltype(t)::type>();
        using Type = TL::last<typename decltype(t)::type>;
        Type value;
        unpack(s, value, max_depth-1);
        v.template emplace<Index>(value);
    });
}
} // namespace fc::raw

// Serialization and typename for std::optional
template<typename T>
struct get_typename<std::optional<T>> {
    static const char* name() {
        static auto theName = std::string("std::optional<") + get_typename<T>::name() + ">";
        return theName.c_str();
    }
};

template<typename T>
void from_variant(const variant& v, std::optional<T>& o, uint32_t max_depth) {
    if (v.is_null())
        o.reset();
    else
        o = v.as<T>(max_depth);
}
template<typename T>
void to_variant(const std::optional<T>& o, variant& v, uint32_t max_depth) {
    if (o.has_value())
        v = variant(*o, max_depth);
    else
        v.clear();
}

namespace raw {

template<typename Stream, typename T>
void pack(Stream& s, const std::optional<T>& o, uint32_t max_depth) {
    pack(s, o.has_value(), max_depth);
    if (o.has_value())
        pack(s, *o, max_depth);
}
template<typename Stream, typename T>
void unpack(Stream& s, std::optional<T>& o, uint32_t max_depth) {
    bool present;
    unpack(s, present, max_depth);
    if (present) {
        T value;
        unpack(s, value, max_depth);
        o = value;
    } else {
        o.reset();
    }
}

} } // namespace fc::raw
