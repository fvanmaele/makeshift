﻿
#ifndef INCLUDED_MAKESHIFT_METADATA_HPP_
#define INCLUDED_MAKESHIFT_METADATA_HPP_


#include <string_view>
#include <type_traits> // for decay<>, conditional<>
#include <utility>     // for forward<>()
#include <cstddef>     // for size_t
#include <tuple>

#include <makeshift/type_traits.hpp> // for tag<>, can_apply<>, is_flags_enum<>, type_category


namespace makeshift
{

namespace detail
{


struct type_metadata_base { };
struct value_metadata_base { };
struct member_metadata_base { };


} // namespace detail


inline namespace types
{


    //ᅟ
    // Like `std::decay<>` but with additional support for converting plain old literals to modern types.
    //
template <typename T> struct literal_decay { using type = std::decay_t<T>; };
template <std::size_t N> struct literal_decay<const char (&)[N]> { using type = std::string_view; };

    //ᅟ
    // Like `std::decay<>` but with additional support for converting plain old literals to modern types.
    //
template <typename T> using literal_decay_t = typename literal_decay<T>::type;


} // inline namespace types


inline namespace metadata
{


    // Expose the `""sv` literal.
using namespace std::literals::string_view_literals;


    //ᅟ
    // Stores metadata for a type.
    //
template <typename T, typename CategoryC, typename AttributesT>
    struct type_metadata;
template <typename T, type_category Category, typename AttributesT>
    struct type_metadata<T, std::integral_constant<type_category, Category>, AttributesT> : makeshift::detail::type_metadata_base
{
    using type = T;

    static constexpr type_category category = Category;

    AttributesT attributes;

    constexpr type_metadata(AttributesT&& _attributes) : attributes(std::move(_attributes)) { }
};

    //ᅟ
    // Use `type<T>(...)` to declare metadata for a type.
    //
template <typename T, typename... AttrT>
    constexpr type_metadata<T, constant<makeshift::detail::default_type_category<T>()>, std::tuple<literal_decay_t<AttrT>...>> type(AttrT&&... attributes)
{
    return { std::tuple<literal_decay_t<AttrT>...>(std::forward<AttrT>(attributes)...) };
}

    //ᅟ
    // Use `type<T, Category>(...)` to declare metadata for a type with a given type category.
    //
template <typename T, type_category Category, typename... AttrT>
    constexpr type_metadata<T, constant<makeshift::detail::infer_type_category<T>(Category)>, std::tuple<literal_decay_t<AttrT>...>> type(AttrT&&... attributes)
{
    return { std::tuple<literal_decay_t<AttrT>...>(std::forward<AttrT>(attributes)...) };
}


    //ᅟ
    // Stores metadata for a known value of a type.
    //
template <typename ValC, typename AttributesT>
    struct value_metadata;
template <typename ValT, ValT Val, typename AttributesT>
    struct value_metadata<std::integral_constant<ValT, Val>, AttributesT> : constant<Val>, makeshift::detail::value_metadata_base
{
    AttributesT attributes;

    constexpr value_metadata(AttributesT&& _attributes) : attributes(std::move(_attributes)) { }
};

    //ᅟ
    // Use `value<V>(...)` to declare metadata for a known value of a type.
    //
template <auto Val, typename... AttrT, typename = decltype(Val)>
    constexpr value_metadata<constant<Val>, std::tuple<literal_decay_t<AttrT>...>> value(AttrT&&... attributes)
{
    return { std::tuple<literal_decay_t<AttrT>...>(std::forward<AttrT>(attributes)...) };
}


    //ᅟ
    // Stores metadata for members of a type.
    //
template <typename AccessorsC, typename AttributesT>
    struct member_metadata : makeshift::detail::member_metadata_base
{
    using accessors = AccessorsC;

    AttributesT attributes;

    constexpr member_metadata(AttributesT&& _attributes) : attributes(std::move(_attributes)) { }
};

    //ᅟ
    // Use `member<Accessors...>(...)` to declare metadata for members of a type.
    //
template <auto... Accessors, typename... AttrT, typename = type_sequence<decltype(Accessors)...>>
    constexpr member_metadata<type_sequence<constant<Accessors>...>, std::tuple<literal_decay_t<AttrT>...>> member(AttrT&&... attributes)
{
    return { std::tuple<literal_decay_t<AttrT>...>(std::forward<AttrT>(attributes)...) };
}


    //ᅟ
    // Stores metadata for the bitflag type of a flags enum.
    //
template <typename TypeMetadataT>
    struct flags_metadata
{
    using value_type = TypeMetadataT;
    TypeMetadataT value;
};

    //ᅟ
    // When defining metadata for flag enums, use `flags(type<TheFlagsType>(...))` to define metadata for the bitflag type itself.
    //
template <typename TypeMetadataT,
          typename = std::enable_if_t<is_instantiation_of_v<TypeMetadataT, type_metadata>>>
    constexpr flags_metadata<std::decay_t<TypeMetadataT>> flags(TypeMetadataT&& typeMetadata)
{
    return { std::forward<TypeMetadataT>(typeMetadata) };
}


    //ᅟ
    // Encodes a human-readable caption of an entity in metadata.
    //
struct caption_metadata { std::string_view value; };

    //ᅟ
    // Use `caption("the caption")` to encode a human-readable caption of an entity in metadata.
    //
static inline constexpr caption_metadata caption(std::string_view value) { return { value }; }


} // inline namespace metadata


} // namespace makeshift


#endif // INCLUDED_MAKESHIFT_METADATA_HPP_
