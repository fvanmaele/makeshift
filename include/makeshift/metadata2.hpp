﻿
#ifndef INCLUDED_MAKESHIFT_METADATA2_HPP_
#define INCLUDED_MAKESHIFT_METADATA2_HPP_


#include <type_traits> // for decay<>

#include <makeshift/version.hpp> // for MAKESHIFT_NODISCARD

#include <makeshift/detail/string_view.hpp>
#include <makeshift/detail/metadata2.hpp>


namespace makeshift
{

inline namespace metadata
{


constexpr makeshift::detail::parameter<makeshift::detail::name_t> name = { };

template <typename T> constexpr inline makeshift::detail::array_parameter_of<makeshift::detail::values_t, T> values = { };

constexpr inline makeshift::detail::array_parameter<makeshift::detail::value_names_t, makeshift::detail::string_view> value_names = { };

template <typename T> constexpr inline makeshift::detail::array_parameter_of<makeshift::detail::named_values_t, makeshift::detail::named_t<T>> named_values = { };


template <typename... ParamsT>
    MAKESHIFT_NODISCARD constexpr makeshift::detail::metadata_t<ParamsT...> define_metadata(ParamsT... params)
{
    return { params... };
};


} // inline namespace metadata

} // namespace makeshift


#endif // INCLUDED_MAKEHIFT_METADATA2_HPP_
