#pragma once

#include <Utils/Reflect.hpp>

#ifdef BAL_PLATFORM_LEAP
#define BAL_REFLECT(...) REFLECT(__VA_ARGS__)
#define BAL_REFLECT_EMPTY(...) REFLECT_EMPTY(__VA_ARGS__)

template<typename Stream_T, typename T,
         std::enable_if_t<Util::Reflector<T>::IsDefined::value, bool> = true>
eosio::datastream<Stream_T>& operator<<(eosio::datastream<Stream_T>& s, const T& t) {
    Util::TypeList::Runtime::ForEach(typename Util::Reflector<T>::Members(), [&s, &t](auto member) {
        using Member = typename decltype(member)::type;
        BAL::Log(Member::name);
        s << Member::get(t);
    });
    return s;
}

template<typename Stream_T, typename T,
         std::enable_if_t<Util::Reflector<T>::IsDefined::value, bool> = true>
eosio::datastream<Stream_T>& operator>>(eosio::datastream<Stream_T>& s, T& t) {
    Util::TypeList::Runtime::ForEach(typename Util::Reflector<T>::Members(), [&s, &t](auto member) {
        using Member = typename decltype(member)::type;
        BAL::Log(Member::name);
        s >> Member::get(t);
    });
    return s;
}

#endif
