﻿
#ifndef INCLUDED_MAKESHIFT_DETAIL_UTILITY_HPP_
#define INCLUDED_MAKESHIFT_DETAIL_UTILITY_HPP_


#include <makeshift/detail/type_traits.hpp> // for try_index_of_type<>


namespace makeshift
{


template <typename T>
struct type_tag;


namespace detail
{


template <typename T, typename TypeSeqT> struct try_index_of_type_in;
template <typename T, template <typename...> class TypeSeqT, typename... Ts> struct try_index_of_type_in<T, TypeSeqT<Ts...>> : try_index_of_type<T, Ts...> { };


template <typename T>
struct type_base : constval_tag
{
    using type = T;
};


template <typename RSeqT, typename... Ts> struct type_sequence_cat_;
template <typename RSeqT> struct type_sequence_cat_<RSeqT> { using type = RSeqT; };
template <template <typename...> class TypeSeq1T, template <typename...> class TypeSeq2T, typename... RSeqT, typename... NSeqT, typename... Ts>
struct type_sequence_cat_<TypeSeq1T<RSeqT...>, TypeSeq2T<NSeqT...>, Ts...>
    : type_sequence_cat_<TypeSeq1T<RSeqT..., NSeqT...>, Ts...> 
{
};


template <typename T>
struct type_tag_proxy
{
    constexpr operator type_tag<T> const&(void) const noexcept;
    constexpr type_tag<T> const* operator &(void) const noexcept
    {
        return &static_cast<type_tag<T> const&>(*this);
    }
};


} // namespace detail

} // namespace makeshift


#endif // INCLUDED_MAKESHIFT_DETAIL_UTILITY_HPP_
