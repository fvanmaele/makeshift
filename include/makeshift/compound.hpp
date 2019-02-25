﻿
#ifndef INCLUDED_MAKESHIFT_COMPOUND_HPP_
#define INCLUDED_MAKESHIFT_COMPOUND_HPP_


#include <tuple>       // for tuple_size<>
#include <cstddef>     // for size_t
#include <type_traits> // for decay<>
#include <utility>     // for move(), integer_sequence<>
#include <functional>  // for equal_to<>, less<>

#include <makeshift/reflect2.hpp> // for compound_members<>()
#include <makeshift/tuple2.hpp>   // for tuple_all_of(), tuple_reduce()
#include <makeshift/functional2.hpp> // for hash<>, adapter_base<>
#include <makeshift/version.hpp>  // for MAKESHIFT_NODISCARD


namespace makeshift
{

namespace detail
{


static constexpr inline std::size_t hash_combine(std::size_t seed, std::size_t newHash) noexcept
{
        // taken from boost::hash_combine()
    return seed ^ (newHash + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}


} // namespace detail


inline namespace metadata
{


    //ᅟ
    // Equality comparer for compound types which determines equivalence by comparing members for equality.
    //
template <typename EqualToT = std::equal_to<>, typename CompoundMembersT = compound_members_t>
    struct compound_equal_to : private makeshift::detail::adapter_base<EqualToT, CompoundMembersT>
{
    using makeshift::detail::adapter_base<EqualToT, CompoundMembersT>::adapter_base;

    template <typename T>
        MAKESHIFT_NODISCARD constexpr bool operator ()(const T& lhs, const T& rhs) const noexcept
    {
        auto& equalTo = static_cast<const EqualToT&>(*this);
        auto& compoundMembers = static_cast<const CompoundMembersT&>(*this);
        return tuple_all_of(
            compoundMembers(type_v<T>),
            [&equalTo, &lhs, &rhs](auto&& member)
            {
                return equalTo(get_member_value(lhs, member), get_member_value(rhs, member));
            });
    }
};


    //ᅟ
    // Hasher for compound types which computes a hash by combining the hashes of the members.
    //
template <typename HashT = hash2<>, typename CompoundMembersT = compound_members_t>
    struct compound_hash : private makeshift::detail::adapter_base<HashT, CompoundMembersT>
{
    using makeshift::detail::adapter_base<HashT, CompoundMembersT>::adapter_base;

    template <typename T>
        MAKESHIFT_NODISCARD constexpr std::size_t operator ()(const T& obj) const noexcept
    {
        auto& hash = static_cast<const HashT&>(*this);
        auto& compoundMembers = static_cast<const CompoundMembersT&>(*this);
        return tuple_reduce(
            compoundMembers(type_v<T>),
            std::size_t(0),
            [&hash, &obj](std::size_t seed, auto&& member)
            {
                std::size_t memberValueHash = hash(get_member_value(obj, member));
                return makeshift::detail::hash_combine(seed, memberValueHash);
            });
    }
};


    //ᅟ
    // Ordering comparer for compound types which determines order by lexicographically comparing members.
    //
template <typename LessT = std::less<>, typename CompoundMembersT = compound_members_t>
    struct compound_less : private makeshift::detail::adapter_base<LessT, CompoundMembersT>
{
    using makeshift::detail::adapter_base<LessT, CompoundMembersT>::adapter_base;

private:
    template <typename MembersT, typename T>
        constexpr bool invoke_(std::index_sequence<>, MembersT&&, const T&, const T&) const noexcept
    {
        return false;
    }
    template <std::size_t I0, std::size_t... Is, typename MembersT, typename T>
        constexpr bool invoke_(std::index_sequence<I0, Is...>, MembersT&& members, const T& lhs, const T& rhs) const noexcept
    {
        const auto& lhsMember = get_member_value(lhs, std::get<I0>(members));
        const auto& rhsMember = get_member_value(rhs, std::get<I0>(members));
        const LessT& less = *this;
        if (less(lhsMember, rhsMember)) return true;
        if (less(rhsMember, lhsMember)) return false;
        return invoke_(std::index_sequence<Is...>{ }, members, lhs, rhs);
    }

public:
    template <typename T>
        MAKESHIFT_NODISCARD constexpr bool operator ()(const T& lhs, const T& rhs) const noexcept
    {
        auto& compoundMembers = static_cast<const CompoundMembersT&>(*this);
        auto members = compoundMembers(type_v<T>);
        return invoke_(std::make_index_sequence<std::tuple_size<std::decay_t<decltype(members)>>::value>{ },
            members, lhs, rhs);
    }
};


template <typename CategoryT, typename OperationT = typename CategoryT::default_operation, typename CompoundMembersT = compound_members_t>
    struct compound_operation;

    //ᅟ
    // Equality comparer for compound types which determines equivalence by comparing members for equality.
    //
template <typename EqualToT, typename CompoundMembersT>
    struct compound_operation<equatable, EqualToT, CompoundMembersT> : compound_equal_to<EqualToT, CompoundMembersT>
{
    using compound_equal_to<EqualToT, CompoundMembersT>::compound_equal_to;
};

    //ᅟ
    // Hasher for compound types which computes a hash by combining the hashes of the members.
    //
template <typename HashT, typename CompoundMembersT>
    struct compound_operation<hashable, HashT, CompoundMembersT> : compound_hash<HashT, CompoundMembersT>
{
    using compound_hash<HashT, CompoundMembersT>::compound_hash;
};

    //ᅟ
    // Ordering comparer for compound types which determines order by lexicographically comparing members.
    //
template <typename LessT, typename CompoundMembersT>
    struct compound_operation<comparable, LessT, CompoundMembersT> : compound_less<LessT, CompoundMembersT>
{
    using compound_less<LessT, CompoundMembersT>::compound_less;
};


template <typename... Ts>
    struct compound_base
        : Ts::template interface<compound_operation<Ts>>...
{
};


} // inline namespace types

} // namespace makeshift


#endif // INCLUDED_MAKESHIFT_COMPOUND_HPP_
