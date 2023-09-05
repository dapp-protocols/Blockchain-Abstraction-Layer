/** This source adapted from https://github.com/kmicklas/variadic-variant.
 *
 * Copyright (C) 2013 Kenneth Micklas
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **/
#pragma once

/// StaticVariant -- a variant containing one of a statically-defined list of types.
/// Yes, this is 100% redundant with std::variant, but it has a much more pleasant interface

#include "TypeList.hpp"

#include <string>
#include <array>
#include <functional>
#include <stdexcept>
#include <typeinfo>
#include <optional>

namespace Util {

// Implementation details, the user should not import this:
namespace impl {
template<typename...>
struct MaxSize;
template<>
struct MaxSize<> : std::integral_constant<size_t, 0> {};
template<typename T>
struct MaxSize<T> : std::integral_constant<size_t, sizeof(T)> {};
template<typename T, typename... Ts>
struct MaxSize<T, Ts...> : std::integral_constant<size_t, std::max(sizeof(T), MaxSize<Ts...>::value)> {};
} // namespace impl

template<typename... Types>
class StaticVariant {
public:
    using Tag = int64_t;
    using List = TypeList::List<Types...>;

protected:
    static_assert(TypeList::length<TypeList::filter<List, std::is_reference>>() == 0,
                  "Reference types are not permitted in StaticVariant.");
    static_assert(TypeList::length<TypeList::concatUnique<List>>() == TypeList::length<List>(),
                  "StaticVariant type arguments contain duplicate types.");

    template<typename X>
    using TypeInTypelist = std::enable_if_t<TypeList::indexOf<List, X>() != -1>;

    Tag _tag;
    std::array<uint8_t, impl::MaxSize<Types...>::value> _storage;

    template<typename X, typename = TypeInTypelist<X>>
    void init(const X& x) {
        _tag = TypeList::indexOf<List, X>();
        new(_storage.data()) X(x);
    }

    template<typename X, typename = TypeInTypelist<X>>
    void init(X&& x) {
        _tag = TypeList::indexOf<List, X>();
        new(_storage.data()) X( std::move(x) );
    }

    void initFromTag(Tag tag) {
        if (tag < 0 || size_t(tag) >= count())
#if __cpp_exceptions
            throw std::bad_cast();
#else
   std::abort();
#endif
        _tag = tag;
        TypeList::Runtime::Dispatch(List(), tag, [this](auto t) {
            using T = typename decltype(t)::type;
            new(reinterpret_cast<T*>(_storage.data())) T();
        });
    }

    void clean() {
        TypeList::Runtime::Dispatch(List(), _tag, [data=_storage.data()](auto t) {
            using T = typename decltype(t)::type;
            reinterpret_cast<T*>(data)->~T();
        });
    }

    template<typename T, typename = void>
    struct ImportHelper {
        static StaticVariant construct(const T&) {
#if __cpp_exceptions
            throw std::bad_cast();
#else
            std::abort();
#endif
        }
        static StaticVariant construct(T&&) {
#if __cpp_exceptions
            throw std::bad_cast();
#else
            std::abort();
#endif
        }
    };
    template<typename T>
    struct ImportHelper<T, TypeInTypelist<T>> {
        static StaticVariant construct(const T& t) { return StaticVariant(t); }
        static StaticVariant construct(T&& t) { return StaticVariant(std::move(t)); }
    };

public:
    template<typename X, typename = TypeInTypelist<X>>
    struct TagOf {
       static constexpr Tag value = TypeList::indexOf<List, X>();
    };

    struct type_lt {
       bool operator()(const StaticVariant& a, const StaticVariant& b) const { return a.which() < b.which(); }
    };
    struct type_eq {
       bool operator()(const StaticVariant& a, const StaticVariant& b) const { return a.which() == b.which(); }
    };

    /// Import the value from a foreign StaticVariant with types not in this one, and throw if the value is an
    /// incompatible type
    template<typename... Other>
    static StaticVariant ImportFrom(const StaticVariant<Other...>& other) {
        return TypeList::Runtime::Dispatch(TypeList::List<Other...>(), other.which(), [&other](auto t) {
            using other_type = typename decltype(t)::type;
            return ImportHelper<other_type>::construct(other.template get<other_type>());
        });
    }
    /// Import the value from a foreign StaticVariant with types not in this one, and throw if the value is an
    /// incompatible type
    template<typename... Other>
    static StaticVariant ImportFrom(StaticVariant<Other...>&& other) {
        return TypeList::Runtime::Dispatch(TypeList::List<Other...>(), other.which(), [&other](auto t) {
            using other_type = typename decltype(t)::type;
            return ImportHelper<other_type>::construct(std::move(other.template get<other_type>()));
        });
    }

    StaticVariant()
    {
        initFromTag(0);
    }

    template<typename... Other>
    StaticVariant(const StaticVariant<Other...>& cpy)
    {
       TypeList::Runtime::Dispatch(TypeList::List<Other...>(), cpy.which(), [this, &cpy](auto t) mutable {
          this->init(cpy.template get<typename decltype(t)::type>());
       });
    }

    StaticVariant(const StaticVariant& cpy)
    {
       TypeList::Runtime::Dispatch(List(), cpy.which(), [this, &cpy](auto t) mutable {
          this->init(cpy.template get<typename decltype(t)::type>());
       });
    }

    StaticVariant(StaticVariant&& mv)
    {
       TypeList::Runtime::Dispatch(List(), mv.which(), [this, &mv](auto t) mutable {
          this->init(std::move(mv.template get<typename decltype(t)::type>()));
       });
    }

    template<typename... Other>
    StaticVariant(StaticVariant<Other...>&& mv)
    {
       TypeList::Runtime::Dispatch(TypeList::List<Other...>(), mv.which(), [this, &mv](auto t) mutable {
           this->init(std::move(mv.template get<typename decltype(t)::type>()));
       });
    }

    template<typename X, typename = TypeInTypelist<X>>
    StaticVariant(const X& v) { init(v); }
    template<typename X, typename = TypeInTypelist<X>>
    StaticVariant(X&& v) { init(std::move(v)); }

    ~StaticVariant() { clean(); }

    template<typename X, typename = TypeInTypelist<X>>
    StaticVariant& operator=(const X& v) {
        clean();
        init(v);
        return *this;
    }
    StaticVariant& operator=(const StaticVariant& v) {
       if( this == &v ) return *this;
       clean();
       TypeList::Runtime::Dispatch(List(), v.which(), [this, &v](auto t)mutable {
          this->init(v.template get<typename decltype(t)::type>());
       });
       return *this;
    }
    StaticVariant& operator=(StaticVariant&& v) {
       if( this == &v ) return *this;
       clean();
       TypeList::Runtime::Dispatch(List(), v.which(), [this, &v](auto t)mutable {
          this->init(std::move(v.template get<typename decltype(t)::type>()));
       });
       return *this;
    }

    friend bool operator==(const StaticVariant& a, const StaticVariant& b) {
       if (a.which() != b.which())
          return false;
       return TypeList::Runtime::Dispatch(List(), a.which(), [&a, &b](auto t) {
          using Value = typename decltype(t)::type;
          return a.template get<Value>() == b.template get<Value>();
       });
    }
    friend bool operator!=(const StaticVariant& a, const StaticVariant& b) { return !(a == b); }
    friend bool operator<(const StaticVariant& a, const StaticVariant& b) {
        if (a.which() < b.which()) return true;
        if (a.which() > b.which()) return false;
        return TypeList::Runtime::Dispatch(List(), a.which(), [&a, &b](auto t) {
            using T = typename decltype(t)::type;
            return a.template get<T>() < b.template get<T>();
        });
    }

    template<typename X, typename = TypeInTypelist<X>>
    X& get() {
        if(_tag == TypeList::indexOf<List, X>()) {
            return *reinterpret_cast<X*>(_storage.data());
        } else {
#if __cpp_exceptions
            throw std::bad_cast();
#else
            std::abort();
#endif
        }
    }
    template<typename X, typename = TypeInTypelist<X>>
    const X& get() const {
        if(_tag == TypeList::indexOf<List, X>()) {
            return *reinterpret_cast<const X*>(_storage.data());
        } else {
#if __cpp_exceptions
            throw std::bad_cast();
#else
            std::abort();
#endif
        }
    }
    template<typename visitor>
    typename visitor::result_type visit(visitor& v) {
        return visit(_tag, v, (void*) _storage.data());
    }

    template<typename visitor>
    typename visitor::result_type visit(const visitor& v) {
        return visit(_tag, v, (void*) _storage.data());
    }

    template<typename visitor>
    typename visitor::result_type visit(visitor& v)const {
        return visit(_tag, v, (const void*) _storage.data());
    }

    template<typename visitor>
    typename visitor::result_type visit(const visitor& v)const {
        return visit(_tag, v, (const void*) _storage.data());
    }

    template<typename visitor>
    static typename visitor::result_type visit(Tag tag, visitor& v, void* data) {
        if (tag < 0 || size_t(tag) >= count())
#if __cpp_exceptions
            throw std::range_error("[StaticVariant] Bad type tag: " + std::to_string(tag));
#else
            std::abort();
#endif
        return TypeList::Runtime::Dispatch(List(), tag, [&v, data](auto t) {
            return v(*reinterpret_cast<typename decltype(t)::type*>(data));
        });
    }

    template<typename visitor>
    static typename visitor::result_type visit(Tag tag, const visitor& v, void* data) {
        if (tag < 0 || size_t(tag) >= count())
#if __cpp_exceptions
            throw std::range_error("[StaticVariant] Bad type tag: " + std::to_string(tag));
#else
            std::abort();
#endif
        return TypeList::Runtime::Dispatch(List(), tag, [&v, data](auto t) {
            return v(*reinterpret_cast<typename decltype(t)::type*>(data));
        });
    }

    template<typename visitor>
    static typename visitor::result_type visit(Tag tag, visitor& v, const void* data) {
        if (tag < 0 || size_t(tag) >= count())
#if __cpp_exceptions
            throw std::range_error("[StaticVariant] Bad type tag: " + std::to_string(tag));
#else
            std::abort();
#endif
        return TypeList::Runtime::Dispatch(List(), tag, [&v, data](auto t) {
            return v(*reinterpret_cast<const typename decltype(t)::type*>(data));
        });
    }

    template<typename visitor>
    static typename visitor::result_type visit(Tag tag, const visitor& v, const void* data) {
        if (tag < 0 || size_t(tag) >= count())
#if __cpp_exceptions
            throw std::range_error("[StaticVariant] Bad type tag: " + std::to_string(tag));
#else
            std::abort();
#endif
        return TypeList::Runtime::Dispatch(List(), tag, [&v, data](auto t) {
            return v(*reinterpret_cast<const typename decltype(t)::type*>(data));
        });
    }

    static constexpr size_t count() { return TypeList::length<List>(); }
    void set_which(Tag tag) {
        if (tag < 0 || size_t(tag) >= count())
#if __cpp_exceptions
            throw std::range_error("[StaticVariant] Bad type tag: " + std::to_string(tag));
#else
            std::abort();
#endif
      clean();
      initFromTag(tag);
    }

    Tag which() const {return _tag;}

    template<typename T>
    bool isType() const { return _tag == TagOf<T>::value; }
};
} // namespace Util

#ifdef BAL_PLATFORM_LEAP
namespace eosio {

template<typename Stream, typename... Ts>
Stream& operator<<(Stream& s, const Util::StaticVariant<Ts...>& v) {
    uint64_t discriminant = v.which();
    s << discriminant;
    Util::TypeList::Runtime::Dispatch(Util::TypeList::List<Ts...>(), discriminant, [&s, &v](auto i) {
        using T = typename decltype(i)::type;
        s << v.template get<T>();
    });
    return s;
}
template<typename Stream, typename... Ts>
Stream& operator>>(Stream& s, Util::StaticVariant<Ts...>& v) {
    uint64_t discriminant;
    s >> discriminant;
    Util::TypeList::Runtime::Dispatch(Util::TypeList::List<Ts...>(), discriminant, [&s, &v](auto i) {
        using T = typename decltype(i)::type;
        T value;
        s >> value;
        v = std::move(value);
    });
    return s;
}

}
#endif
