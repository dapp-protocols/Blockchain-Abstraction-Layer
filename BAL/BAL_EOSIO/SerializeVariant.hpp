#pragma once

#include <Utils/TypeList.hpp>

#include <eosio/check.hpp>

#include <variant>

template<typename Stream, typename... Ts>
Stream& operator<<(Stream& s, const std::variant<Ts...>& v) {
    eosio::check(!v.valueless_by_exception(), "Unable to serialize valueless variant!");
    uint64_t discriminant = v.index();
    s << discriminant;
    Util::TypeList::runtime::Dispatch(Util::TypeList::makeSequence<sizeof...(Ts)>(), discriminant, [&s, &v](auto i) {
        constexpr uint64_t Index = typename decltype(i)::type();
        s << std::get<Index>(v);
    });
    return s;
}
template<typename Stream, typename... Ts>
Stream& operator>>(Stream& s, std::variant<Ts...>& v) {
    uint64_t discriminant;
    s >> discriminant;
    Util::TypeList::runtime::Dispatch(Util::TypeList::makeSequence<sizeof...(Ts)>(), discriminant, [&s, &v](auto i) {
        constexpr uint64_t Index = typename decltype(i)::type();
        Util::TypeList::at<Util::TypeList::List<Ts...>, Index> value;
        s >> value;
        v.emplace<Index>(std::move(value));
    });
    return s;
}
