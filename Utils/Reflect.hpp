/**
 * \file reflect.hpp A simple, macro-based reflection framework
 *
 * This file defines a simple reflection framework which uses macros based on Boost::Preprocessor to define
 * compile-time registries tracking data about structs and their members.
 *
 * The collected data is stored in specializations of the @ref Util::reflector struct template. Member names
 * are accessible via the @ref Util::member_names::member_name template.
 */

#pragma once

#include "TypeList.hpp"

#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>

namespace Util {

/**
 *  @brief defines visit functions for T
 *  Unless this is specialized, visit() will not be defined for T.
 *
 *  @tparam T - the type that will be visited.
 *
 *  The @ref REFLECT(TYPE,MEMBERS) macro is used to specialize this class for your type.
 */
template<typename T>
struct reflector {
    using type = T;
    using is_defined = std::false_type;
    /// A TypeList with a @ref field_reflection for each native member (non-inherited) of the struct
    using native_members = TypeList::List<>;
    /// A TypeList with an @ref inherited_field_reflection for each inherited member of the struct
    using inherited_members = TypeList::List<>;
    /// A TypeList with a @ref field_reflection or @ref inherited_field_reflection for each member of the struct,
    /// starting with inherited members
    using members = TypeList::List<>;
    /// A TypeList of base classes for this type
    using base_classes = TypeList::List<>;

    /// String containing the type's name
    static const char* name() { return "Unknown Type"; }
};

namespace member_names {
/// A template which stores the name of the native member at a given index in a given class
template<typename Class, std::size_t index> struct member_name {
    constexpr static const char* value = "Unknown member";
};
}

/**
 *  @brief A template to store compile-time information about a field in a reflected struct
 *
 *  @tparam Container The type of the struct or class containing the field
 *  @tparam Member The type of the field
 *  @tparam field A pointer-to-member for the reflected field
 */
template<std::size_t Index, typename Container, typename Member, Member Container::*field>
struct field_reflection {
    using container = Container;
    using type = Member;
    using reflector = Util::reflector<type>;
    constexpr static std::size_t index = Index;
    constexpr static bool is_derived = false;
    constexpr static type container::*pointer = field;

    /// @brief Given a reference to the container type, get a reference to the field
    static type& get(container& c) { return c.*field; }
    static const type& get(const container& c) { return c.*field; }
    /// @brief Get the name of the field
    static const char* get_name() { return Util::member_names::member_name<container, index>::value; }
};
/// Basically the same as @ref field_reflection, but for inherited fields
/// Note that inherited field reflections do not have an index field; indexes are for native fields only
template<std::size_t IndexInBase, typename Base, typename Derived, typename Member, Member Base::*field>
struct inherited_field_reflection {
    using container = Derived;
    using field_container = Base;
    using type = Member;
    using reflector = Util::reflector<type>;
    constexpr static std::size_t index_in_base = IndexInBase;
    constexpr static bool is_derived = true;
    constexpr static type field_container::*pointer = field;

    static type& get(container& c) {
        // And we need a distinct inherited_field_reflection type because this conversion can't be done statically
        type container::* derived_field = field;
        return c.*derived_field;
    }
    static const type& get(const container& c) {
        type container::* derived_field = field;
        return c.*derived_field;
    }
    static const char* get_name() {
        using Reflector = typename Util::reflector<Base>::native_members::template at<IndexInBase>;
        return Reflector::get_name();
    }
};

namespace impl {
/// Helper template to create a @ref field_reflection without any commas (makes it macro-friendly)
template<typename Container>
struct Reflect_type {
    template<typename Member>
    struct with_field_type {
        template<std::size_t Index>
        struct at_index {
            template<Member Container::*field>
            struct with_field_pointer {
                using type = field_reflection<Index, Container, Member, field>;
            };
        };
    };
};
/// Template to make a transformer of a @ref field_reflection from a base class to a derived class
template<typename Derived>
struct Derivation_reflection_transformer {
    template<typename> struct transform;
    template<std::size_t IndexInBase, typename BaseContainer, typename Member, Member BaseContainer::*field>
    struct transform<field_reflection<IndexInBase, BaseContainer, Member, field>> {
        using type = inherited_field_reflection<IndexInBase, BaseContainer, Derived, Member, field>;
    };
    template<std::size_t IndexInBase, typename BaseContainer, typename IntermediateContainer,
             typename Member, Member BaseContainer::*field>
    struct transform<inherited_field_reflection<IndexInBase, BaseContainer, IntermediateContainer, Member, field>> {
        using type = inherited_field_reflection<IndexInBase, BaseContainer, Derived, Member, field>;
    };
};
} // namespace impl

/// Macro to transform reflected fields of a base class to a derived class and concatenate them to a type list
#define REFLECT_CONCAT_BASE_MEMBER_REFLECTIONS(r, derived, base) \
    ::add_list<TypeList::transform<reflector<base>::members, impl::Derivation_reflection_transformer<derived>>>
/// Macro to concatenate a new @ref field_reflection to a TypeList
#define REFLECT_CONCAT_MEMBER_REFLECTION(r, container, idx, member) \
    ::add<typename impl::Reflect_type<container>::template with_field_type<decltype(container::member)> \
                                                ::template at_index<idx> \
                                                ::template with_field_pointer<&container::member>::type>
#define REFLECT_MEMBER_NAME(r, container, idx, member) \
    template<> struct member_name<container, idx> { constexpr static const char* value = BOOST_PP_STRINGIZE(member); };
#define REFLECT_TEMPLATE_MEMBER_NAME(r, data, idx, member) \
    template<BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_ELEM(0, data))> struct member_name<BOOST_PP_SEQ_ELEM(1, data), idx> { \
    constexpr static const char* value = BOOST_PP_STRINGIZE(member); };
/// Macro to concatenate a new type to a TypeList
#define REFLECT_CONCAT_TYPE(r, x, TYPE) ::add<TYPE>

} // namespace Util

/**
 *  @def REFLECT_DERIVED(TYPE,INHERITS,MEMBERS)
 *
 *  @brief Specializes Util::reflector for TYPE where
 *         type inherits other reflected classes
 *
 *  @param INHERITS - a sequence of base class names (basea)(baseb)(basec)
 *  @param MEMBERS - a sequence of member names.  (field1)(field2)(field3)
 */
#define REFLECT_DERIVED( TYPE, INHERITS, MEMBERS ) \
namespace Util {  \
template<> struct reflector<TYPE> {\
    using type = TYPE; \
    using is_defined = std::true_type; \
    using native_members = \
       typename TypeList::builder<>::type \
          BOOST_PP_SEQ_FOR_EACH_I( REFLECT_CONCAT_MEMBER_REFLECTION, TYPE, MEMBERS ) ::finalize; \
    using inherited_members = \
       typename TypeList::builder<>::type \
          BOOST_PP_SEQ_FOR_EACH( REFLECT_CONCAT_BASE_MEMBER_REFLECTIONS, TYPE, INHERITS ) ::finalize; \
    using members = typename TypeList::concat<inherited_members, native_members>::type; \
    using base_classes = typename TypeList::builder<>::type \
          BOOST_PP_SEQ_FOR_EACH( REFLECT_CONCAT_TYPE, x, INHERITS ) ::finalize; \
    static const char* name()  { return BOOST_PP_STRINGIZE(TYPE);  } \
}; \
namespace member_names { \
BOOST_PP_SEQ_FOR_EACH_I( REFLECT_MEMBER_NAME, TYPE, MEMBERS ) \
} }

/**
 *  @def REFLECT(TYPE,MEMBERS)
 *  @brief Specializes Util::reflector for TYPE
 *
 *  @param MEMBERS - a sequence of member names.  (field1)(field2)(field3)
 *
 *  @see REFLECT_DERIVED
 */
#define REFLECT( TYPE, MEMBERS ) \
    REFLECT_DERIVED( TYPE, BOOST_PP_SEQ_NIL, MEMBERS )
