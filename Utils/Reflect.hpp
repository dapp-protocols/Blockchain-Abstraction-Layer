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

#include <bluegrass/meta/preprocessor.hpp>

#include <tuple>

namespace Util {
/**
 *  @brief Stores compile-time data about a type T
 *
 *  @tparam T - the type (generally a class or struct) that information is stored about
 *
 *  The @ref REFLECT(TYPE,MEMBERS,BASES) macro is used to record the reflection information. Most classes should
 *  not specialize this template directly, but it can be done if absolutely necessary.
 */
template<typename T, typename=void>
struct Reflector {
    using Type = T;
    using IsDefined = std::false_type;
    /// A TypeList with a @ref FieldReflection for each native member (non-inherited) of the struct
    using NativeMembers = TypeList::List<>;
    /// A TypeList with an @ref FieldReflection for each inherited member of the struct
    using InheritedMembers = TypeList::List<>;
    /// A TypeList with a @ref FieldReflection for each member of the struct, starting with inherited members
    using Members = TypeList::List<>;
    /// A TypeList of base classes for this type
    using BaseClasses = TypeList::List<>;

    /// String containing the type's name
    constexpr static const std::string_view name() { return "Unknown Type"; }
};

/**
 * @brief An example field reflection record
 *
 * This is an example of a record that would be found in a Reflector's members lists. This example demonstrates an inherited field.
 *
 * Note that the actual records in those lists are defined ad-hoc, unrelated to this example struct. Although included as valid code, this example is merely documentative and is irrelevant to actual reflections.
 */
struct FieldReflection {
    // Some dummy types to exemplify the reflection record.
    // In a real field reflection, these would not exist.
    struct Dummy{int field;};
    struct Dummy2 : public Dummy {char field2;};

    /// The type of the container the reflected field is a member of. Always matches the Type of the containing Reflector
    using Container = Dummy2;
    /// Relevant only to inherited fields, the type of the base class the field was natively a member of. For native fields, this is identical to Container
    using NativeContainer = Dummy;
    /// The type of the reflected field
    using Type = int;
    /// A field is native if it originates in Container rather than having been inherited from a base class
    constexpr static bool isNative = false;
    /// A pointer-to-member referring to the reflected field within its containing class
    constexpr static Type Container::*pointer = &Dummy::field;
    /// The name of the field as a string
    constexpr static const std::string_view name = "field";

    /// @brief Given a reference to the container type, get a reference to the field
    static Type& get(Container& c) { return c.*pointer; }
    /// @brief Given a const reference to the container type, get a const reference to the field
    static const Type& get(const Container& c) { return c.*pointer; }
};

namespace impl {
namespace TL = TypeList;

/// A compound metafunction which takes a ReflectionData and produces from it the various FieldReflection lists used in the Reflector.
template<typename ReflectionData>
class FieldReflectionGenerator {
    // Transforms a type like List<FieldType, Index> to a full FieldReflection struct
    struct TransformToReflection {
        template<typename IndexedType> struct transform;
        template<std::size_t Index, typename FieldType>
        struct transform<TL::List<std::integral_constant<std::size_t, Index>, FieldType>> {
            // The true FieldReflection struct to add to the Reflector's members list
            struct _internal {
                using Container = typename ReflectionData::ClassType;
                using NativeContainer = Container;
                using Type = FieldType;
                constexpr static bool isNative = true;
                constexpr static Type Container::*pointer = std::get<Index>(ReflectionData::fieldPointers);
                constexpr static const std::string_view name = ReflectionData::fieldNames[Index];
                static Type& get(Container& c) { return c.*pointer; }
                static const Type& get(const Container& c) { return c.*pointer; }
            };
            using type = _internal;
        };
    };

    // Transforms a base class's field reflections into derived class field reflections
    template<typename Derived>
    struct TransformToInheritedReflection {
        template<typename BaseMember>
        struct transform {
            // The true FieldReflection struct, when the reflected field is inherited rather than native
            struct _internal {
                using Container = typename ReflectionData::ClassType;
                using NativeContainer = typename BaseMember::NativeContainer;
                using Type = typename BaseMember::Type;
                constexpr static bool isNative = false;
                constexpr static Type Container::*pointer = BaseMember::pointer;
                constexpr static const std::string_view name = BaseMember::name;
                static Type& get(Container& c) { return c.*pointer; }
                static const Type& get(const Container& c) { return c.*pointer; }
            };
            using type = _internal;
        };
    };

    // Rather nifty metafunction that looks up each base class's member list, transforms it to an inherited member list, and concatenates all of those sub-lists into a single inherited member list for the derived class.
    template<typename... BaseClasses>
    struct ConcatenateInheritedReflections {
        static_assert(std::conjunction_v<typename Reflector<BaseClasses>::IsDefined...>,
                      "All reflected base classes must themselves be reflected.");
        using type = TL::concat<TL::transform<typename Reflector<BaseClasses>::Members,
                                              TransformToInheritedReflection<typename ReflectionData::ClassType>>...>;
    };

public:
    /// A list of native field reflections for the fields native to the reflected class
    using NativeFields = TL::transform<TL::index<typename ReflectionData::FieldTypes>,
                                       TransformToReflection>;
    /// A list of inherited field reflections for the fields inherited by the reflected class
    using InheritedFields = typename TL::apply<typename ReflectionData::BaseClasses,
                                               ConcatenateInheritedReflections>::type;
    /// A comprehensive list of all field reflections for the reflected class, both native and inherited
    using AllFields = TL::concat<InheritedFields, NativeFields>;
};
} // namespace impl

// Now we specialize Reflector for all types which have a T::ReflectionData type, generally those which use the REFLECT(...) macro.
template<typename T>
struct Reflector<T, std::void_t<typename T::ReflectionData>> {
private:
    // An alias for the FRG, colloquially dubbed "fridge"
    using Fridge = impl::FieldReflectionGenerator<typename T::ReflectionData>;
    // Now going completely off the deep end, we also colloquially dub the ReflectionData "pantry", just for giggles
    using Pantry = typename T::ReflectionData;

public:
    using Type = T;
    using IsDefined = std::true_type;
    using NativeMembers = typename Fridge::NativeFields;
    using InheritedMembers = typename Fridge::InheritedFields;
    using Members = typename Fridge::AllFields;
    using BaseClasses = typename Pantry::BaseClasses;

    constexpr static const std::string_view name() { return Pantry::className; }
};

#define DECLTYPE_OF(Class, Field) decltype(Class::Field)
#define STRINGIFY_NAME(unused, Field) #Field
#define POINTER_TO_MEMBER_VALUE(Class, Field) &Class::Field
#define MAKE_MACRO_INVOCATION(CALL, ...) CALL(__VA_ARGS__)

} // namespace Util

/**
 * @brief Adds reflection information to a class
 *
 * @param TYPE - fully qualified name of the class being reflected
 * @param MEMBERS - a sequence of member names.  (field1)(field2)(field3)
 * @param Bases... - variadic list of base class names
 */
#define REFLECT(TYPE, MEMBERS, /*Base classes*/...) \
    struct ReflectionData { \
        constexpr static const std::string_view className = #TYPE; \
        inline static constexpr const std::array<std::string_view, BLUEGRASS_META_SEQ_SIZE(MEMBERS)> fieldNames = \
            {MAKE_MACRO_INVOCATION(BLUEGRASS_META_FOREACH, STRINGIFY_NAME, unused, BLUEGRASS_META_SEQ_ENUM(MEMBERS))}; \
        constexpr static const auto fieldPointers = std::make_tuple(MAKE_MACRO_INVOCATION(BLUEGRASS_META_FOREACH, POINTER_TO_MEMBER_VALUE, TYPE, BLUEGRASS_META_SEQ_ENUM(MEMBERS))); \
        using ClassType = TYPE; \
        using FieldTypes = Util::TypeList::List<MAKE_MACRO_INVOCATION(BLUEGRASS_META_FOREACH, DECLTYPE_OF, TYPE, BLUEGRASS_META_SEQ_ENUM(MEMBERS))>; \
        using BaseClasses = Util::TypeList::List<__VA_ARGS__>; \
    };

/**
 * @brief An alternative to @ref REFLECT for classes with no members
 */
#define REFLECT_EMPTY(TYPE, /*Base classes*/...) \
    struct ReflectionData { \
        constexpr static const std::string_view className = #TYPE; \
        inline static constexpr const std::array<std::string_view, 0> fieldNames = {}; \
        constexpr static const auto fieldPointers = std::tuple<>(); \
        using ClassType = TYPE; \
        using FieldTypes = Util::TypeList::List<>; \
        using BaseClasses = Util::TypeList::List<__VA_ARGS__>; \
    };
