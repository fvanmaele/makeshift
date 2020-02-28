﻿
#ifndef INCLUDED_MAKESHIFT_ALGORITHM_HPP_
#define INCLUDED_MAKESHIFT_ALGORITHM_HPP_


#include <cstddef>     // for ptrdiff_t
#include <utility>     // for forward<>(), swap()
#include <iterator>    // for iterator_traits<>
#include <type_traits> // for integral_constant<>, decay<>

#include <gsl-lite/gsl-lite.hpp> // for gsl_Expects()

#include <makeshift/detail/algorithm.hpp>
#include <makeshift/detail/range-index.hpp> // for identity_transform_t, all_of_pred, none_of_pred


namespace makeshift {

namespace gsl = ::gsl_lite;


    //
    // Similar to `std::shuffle()`, but support iterators with proxy reference types such as `std::vector<bool>` or `soa_span<>`
    // (which cannot implement LegacyRandomAccessIterator even though they may be random-access), and permits passing a
    // user-defined integer distribution.
    //ᅟ
    //ᅟ    shuffle(v.begin(), v.end(), rng,
    //ᅟ        std::uniform_int_distribution<std::ptrdiff_t>{ });
    //
template <typename RandomIt, typename URBG, typename UniformIntDistributionT>
constexpr void
shuffle(RandomIt first, RandomIt last, URBG&& rng, UniformIntDistributionT dist)
{
    // Implementation taken from https://en.cppreference.com/w/cpp/algorithm/shuffle.

    using Diff = typename std::iterator_traits<RandomIt>::difference_type;
    using Param = typename UniformIntDistributionT::param_type;
 
    Diff length = last - first;
    for (Diff i = length - 1; i > 0; --i)
    {
        Diff j = dist(rng, Param(0, i));
        if (i != j)
        {
            using std::swap;
            swap(first[i], first[j]);
        }
    }
}


    //
    // Given a list of ranges, returns a range of tuples. 
    //ᅟ
    //ᅟ    for (auto&& [i, val] : range_zip(range_index, std::array{ 1, 2, 3 })) {
    //ᅟ        std::cout << "array[" << i << "]: " << val << '\n';
    //ᅟ    }
    //ᅟ    // prints "array[0]: 1\narray[1]: 2\narray[2]: 3\n"
    //
template <typename... Rs>
constexpr auto
range_zip(Rs&&... ranges)
{
    auto mergedSize = detail::merge_sizes(detail::range_size(ranges)...);
    static_assert(!std::is_same<decltype(mergedSize), detail::dim_constant<detail::unknown_size>>::value, "no range argument and no size given");

    return detail::make_zip_common_range(mergedSize, std::forward<Rs>(ranges)...);
}


// TODO: define iota_view(), sub_view()


    //
    // Takes a scalar procedure (i.e. a function of non-range arguments which returns nothing) and calls the procedure for every
    // set of elements in the given ranges.
    //ᅟ
    //ᅟ    range_for(
    //ᅟ        [](gsl::index i, int val) { std::cout << "array[" << i << "]: " << val << '\n'; },
    //ᅟ        range_index, std::array{ 1, 2, 3 });
    //ᅟ    // prints "array[0]: 1\narray[1]: 2\narray[2]: 3\n"
    //
template <typename F, typename... Rs>
constexpr void
range_for(F&& func, Rs&&... ranges)
{
    static_assert(!gsl::conjunction_v<std::is_same<std::decay_t<Rs>, detail::range_index_t>...>, "no range argument and no size given");

    auto mergedSize = detail::merge_sizes(detail::range_size(ranges)...);
    auto it = detail::make_zip_begin_iterator(mergedSize, ranges...);
    auto end = detail::make_zip_iterator_sentinel(mergedSize);
    for (; it != end; ++it)
    {
        it.apply(func);
    }
}


    //
    // Takes an initial value, a reducer, a transformer, and a list of ranges and reduces them to a scalar value.
    //ᅟ
    //ᅟ    range_transform_reduce(
    //ᅟ        gsl::index(0),
    //ᅟ        std::plus<>{ },
    //ᅟ        [](auto&& str) { return str.length(); },
    //ᅟ        std::array{ "Hello, "sv, "World!"sv });
    //ᅟ    // returns 13
    //
template <typename T, typename ReduceFuncT, typename TransformFuncT, typename... Rs>
gsl_NODISCARD constexpr std::decay_t<T>
range_transform_reduce(T&& initialValue, ReduceFuncT&& reduce, TransformFuncT&& transform, Rs&&... ranges)
{
    static_assert(!gsl::conjunction_v<std::is_same<std::decay_t<Rs>, detail::range_index_t>...>, "no range argument and no size given");

    auto mergedSize = detail::merge_sizes(detail::range_size(ranges)...);
    auto result = std::forward<T>(initialValue);
    auto it = detail::make_zip_begin_iterator(mergedSize, ranges...);
    auto end = detail::make_zip_iterator_sentinel(mergedSize);
    for (; it != end; ++it)
    {
        result = reduce(std::move(result), it.apply(transform));
    }
    return result;
}


    //
    // Takes an initial value, a reducer, and a range and reduces it to a scalar value.
    //ᅟ
    //ᅟ    range_reduce(
    //ᅟ        std::string{ },
    //ᅟ        std::plus<>{ },
    //ᅟ        std::array{ "Hello, "s, "World!"s });
    //ᅟ    // returns "Hello, World!"s;
    //
template <typename T, typename ReduceFuncT, typename R>
gsl_NODISCARD constexpr auto
range_reduce(T&& initialValue, ReduceFuncT&& reduce, R&& range)
{
    static_assert(!std::is_same<std::decay_t<R>, detail::range_index_t>::value, "no range argument and no size given");

    auto result = std::forward<T>(initialValue);
    for (auto&& elem : range)
    {
        result = reduce(std::move(result), std::forward<decltype(elem)>(elem));
    }
    return result;
}


    //
    // Takes a predicate and a list of ranges and counts the sets of range elements for which the predicate applies.
    //ᅟ
    //ᅟ    range_count_if(
    //ᅟ        [](auto&& str) { return !str.empty(); },
    //ᅟ        std::array{ "Hello, "sv, "World!"sv });
    //ᅟ    // returns 2
    //
template <typename PredicateT, typename... Rs>
gsl_NODISCARD constexpr std::ptrdiff_t
range_count_if(PredicateT&& predicate, Rs&&... ranges)
{
    static_assert(!gsl::conjunction_v<std::is_same<std::decay_t<Rs>, detail::range_index_t>...>, "no range argument and no size given");

    auto mergedSize = detail::merge_sizes(detail::range_size(ranges)...);
    auto it = detail::make_zip_begin_iterator(mergedSize, ranges...);
    auto end = detail::make_zip_iterator_sentinel(mergedSize);
    auto result = std::ptrdiff_t(0);
    for (; it != end; ++it)
    {
        if (it.apply(predicate)) ++result;
    }
    return result;
}


    //
    // Takes a predicate and a list of ranges and returns whether the predicate is satisfied for all sets of range elements.
    //ᅟ
    //ᅟ    range_all_of(
    //ᅟ        [](auto&& str) { return str.empty(); },
    //ᅟ        std::array{ "Hello, "sv, "World!"sv });
    //ᅟ    // returns false
    //
template <typename PredicateT, typename... Rs>
gsl_NODISCARD constexpr bool
range_all_of(PredicateT&& predicate, Rs&&... ranges)
{
    static_assert(!gsl::conjunction_v<std::is_same<std::decay_t<Rs>, detail::range_index_t>...>, "no range argument and no size given");

    auto mergedSize = detail::merge_sizes(detail::range_size(ranges)...);
    auto it = detail::make_zip_begin_iterator(mergedSize, ranges...);
    auto end = detail::make_zip_iterator_sentinel(mergedSize);
    for (; it != end; ++it)
    {
        if (!it.apply(predicate)) return false;
    }
    return true;
}


    //
    // Takes a predicate and a list of ranges and returns whether the predicate is satisfied for any set of range elements.
    //ᅟ
    //ᅟ    range_any_of(
    //ᅟ        [](auto&& str) { return str.empty(); },
    //ᅟ        std::array{ "Hello, "sv, "World!"sv });
    //ᅟ    // returns false
    //
template <typename PredicateT, typename... Rs>
gsl_NODISCARD constexpr bool
range_any_of(PredicateT&& predicate, Rs&&... ranges)
{
    static_assert(!gsl::conjunction_v<std::is_same<std::decay_t<Rs>, detail::range_index_t>...>, "no range argument and no size given");

    auto mergedSize = detail::merge_sizes(detail::range_size(ranges)...);
    auto it = detail::make_zip_begin_iterator(mergedSize, ranges...);
    auto end = detail::make_zip_iterator_sentinel(mergedSize);
    for (; it != end; ++it)
    {
        if (it.apply(predicate)) return true;
    }
    return false;
}


    //
    // Takes a predicate and a list of ranges and returns whether the predicate is satisfied for no set of range elements.
    //ᅟ
    //ᅟ    range_none_of(
    //ᅟ        [](auto&& str) { return str.empty(); },
    //ᅟ        std::tuple{ "Hello, "sv, "World!"sv });
    //ᅟ    // returns true
    //
template <typename PredicateT, typename... Rs>
gsl_NODISCARD constexpr bool
range_none_of(PredicateT&& predicate, Rs&&... ranges)
{
    static_assert(!gsl::conjunction_v<std::is_same<std::decay_t<Rs>, detail::range_index_t>...>, "no range argument and no size given");

    auto mergedSize = detail::merge_sizes(detail::range_size(ranges)...);
    auto it = detail::make_zip_begin_iterator(mergedSize, ranges...);
    auto end = detail::make_zip_iterator_sentinel(mergedSize);
    for (; it != end; ++it)
    {
        if (it.apply(predicate)) return false;
    }
    return true;
}


} // namespace makeshift


#endif // INCLUDED_MAKESHIFT_ALGORITHM_HPP_
