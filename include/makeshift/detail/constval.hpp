﻿
#ifndef INCLUDED_MAKESHIFT_DETAIL_CONSTVAL_HPP_
#define INCLUDED_MAKESHIFT_DETAIL_CONSTVAL_HPP_


#include <array>
#include <utility>     // for forward<>()
#include <type_traits> // for is_empty<>, is_default_constructible<>, integral_constant<>, declval<>(), is_base_of<>, disjunction<>

#include <makeshift/type_traits2.hpp> // for constval_tag
#include <makeshift/utility2.hpp>     // for array_constant<> // TODO: shouldn't we avoid pulling in the entire header?
#include <makeshift/version.hpp>      // for MAKESHIFT_NODISCARD


namespace makeshift
{

namespace detail
{


template <typename T> using type_member_r = typename T::type;


    // Workaround for non-default-constructible lambdas in C++17.
    // Does not rely on UB (as far as I can tell). Works with GCC 8.2, Clang 7.0, MSVC 19.20, and ICC 19.0 (also `constexpr` evaluation).
    // Idea taken from http://pfultz2.com/blog/2014/09/02/static-lambda/ and modified to avoid casts.
    // This construct becomes unnecessary with C++20 which permits stateless lambdas to be default-constructed.
template <typename F>
    struct stateless_functor_wrapper
{
    static_assert(std::is_empty<F>::value, "lambda must be empty");

private:
#ifdef MAKESHIFT_CXX17
    union impl
    {
        F obj;
        char dummy;

        constexpr impl(void) noexcept : dummy(0) { }
    };
#endif // MAKESHIFT_CXX17

public:
    template <typename... Ts>
        constexpr decltype(auto) operator ()(Ts&&... args) const
    {
#ifdef MAKESHIFT_CXX17
        return impl{ }.obj(std::forward<Ts>(args)...); // we can legally use `obj` even if it wasn't initialized because it is empty
#else // MAKESHIFT_CXX17
        return F{ }.obj(std::forward<Ts>(args)...); // C++14 doesn't support this trick, which is one of the reasons why lambdas cannot be used to make constexpr values (the other reason being that they cannot be constexpr)
#endif // MAKESHIFT_CXX17
    }
};
template <typename F, bool IsDefaultConstructible> struct stateless_functor_0_;
template <typename F> struct stateless_functor_0_<F, true> { using type = F; };
template <typename F> struct stateless_functor_0_<F, false> { using type = stateless_functor_wrapper<F>; };
template <typename F> using stateless_functor_t = typename stateless_functor_0_<F, std::is_default_constructible<F>::value>::type;
template <typename F> constexpr stateless_functor_t<F> stateless_functor_v = { };

template <typename F>
    struct constval_functor_wrapper : constval_tag
{
    static_assert(std::is_empty<F>::value, "lambda must be empty");

private:
#ifdef MAKESHIFT_CXX17
    union impl
    {
        F obj;
        char dummy;

        constexpr impl(void) noexcept : dummy(0) { }
    };
#endif // MAKESHIFT_CXX17

public:
    using value_type = decltype(std::declval<F>()());
    MAKESHIFT_NODISCARD constexpr value_type operator ()(void) const
    {
#ifdef MAKESHIFT_CXX17
        return impl{ }.obj(); // we can legally use `obj` even if it wasn't initialized because it is empty
#else // MAKESHIFT_CXX17
        return F{ }.obj(); // C++14 doesn't support this trick, which is one of the reasons why lambdas cannot be used to make constexpr values (the other reason being that they cannot be constexpr)
#endif // MAKESHIFT_CXX17
    }
#if defined(_MSC_VER ) && !defined(__INTELLISENSE__)
    MAKESHIFT_NODISCARD constexpr operator auto(void) const -> value_type // workaround for VC++ bug, cf. https://developercommunity.visualstudio.com/content/problem/149701/c2833-with-operator-decltype.html#reply-152822
#else // defined(_MSC_VER ) && !defined(__INTELLISENSE__)
    MAKESHIFT_NODISCARD constexpr operator value_type(void) const
#endif // defined(_MSC_VER ) && !defined(__INTELLISENSE__)
    {
        return (*this)();
    }
};

template <typename C, bool IsConstval> struct constval_functor_0_;
template <typename C> struct constval_functor_0_<C, true> { using type = C; };
template <typename C> struct constval_functor_0_<C, false> { using type = constval_functor_wrapper<C>; };
template <typename C> using constval_functor_t = typename constval_functor_0_<C, is_constval_<C>::value>::type;

    // determines if a given type makes a valid C++14 non-type template parameter
template <typename T> struct is_valid_nttp_ : std::disjunction<std::is_integral<T>, std::is_enum<T>, std::is_member_pointer<T>, std::is_null_pointer<T>> { };

template <typename T> struct is_tag_type_ : std::integral_constant<bool, std::is_empty<T>::value && std::is_default_constructible<T>::value> { };
template <typename T> struct is_constval_tag_type_ : std::conjunction<is_tag_type_<T>, std::is_base_of<constval_tag, T>> { };

template <typename T> struct can_normalize_constval_ : std::disjunction<is_valid_nttp_<T>, is_constval_tag_type_<T>> { };
template <typename T, std::size_t N> struct can_normalize_constval_<std::array<T, N>> : can_normalize_constval_<T> { };
template <typename... Ts> struct can_normalize_constval_<std::tuple<Ts...>> : std::conjunction<can_normalize_constval_<Ts>...> { };

template <typename T>
    struct normalize_constval_;

template <typename C, std::size_t I>
    struct array_accessor_functor
{
    constexpr auto operator ()(void) const
    {
        return C{ }()[I];
    }
};
template <typename C, std::size_t I>
    struct tuple_accessor_functor
{
    constexpr auto operator ()(void) const
    {
        return std::get<I>(C{ }());
    }
};

template <typename T, typename C, typename Is> struct make_constval_array_;
template <typename T, typename C, std::size_t... Is> struct make_constval_array_<T, C, std::index_sequence<Is...>> { using type = constval_array<T[], typename normalize_constval_<T>::template type<array_accessor_functor<C, Is>>...>; };

template <typename C, typename Is, typename... Ts> struct make_constval_tuple_;
template <typename C, std::size_t... Is, typename... Ts> struct make_constval_tuple_<C, std::index_sequence<Is...>, Ts...> { using type = constval_tuple<typename normalize_constval_<Ts>::template type<tuple_accessor_functor<C, Is>>...>; };

template <typename T, bool IsConstvalTagType>
    struct normalize_constval_0_;
template <typename T>
    struct normalize_constval_0_<T, false>
{
    template <typename C> using type = std::integral_constant<decltype(std::declval<C>()()), C{ }()>;
};
template <typename T>
    struct normalize_constval_0_<T, true>
{
    template <typename C> using type = T;
};
template <typename T>
    struct normalize_constval_ : normalize_constval_0_<T, is_constval_tag_type_<T>::value>
{
};
template <typename T, std::size_t N>
    struct normalize_constval_<std::array<T, N>>
{
    template <typename C> using type = typename make_constval_array_<T, C, std::make_index_sequence<N>>::type;
};
template <typename... Ts>
    struct normalize_constval_<std::tuple<Ts...>>
{
    template <typename C> using type = typename make_constval_tuple_<C, std::make_index_sequence<sizeof...(Ts)>, Ts...>::type;
};

template <bool CanNormalize, typename T, typename C> struct make_constval_1_;
template <typename T, typename C> struct make_constval_1_<true, T, C> { using type = typename normalize_constval_<T>::template type<C>; };
template <typename T, typename C> struct make_constval_1_<false, T, C> { using type = C; };
template <typename T, typename C> struct make_constval_0_ : make_constval_1_<can_normalize_constval_<T>::value, T, constval_functor_t<C>> { };
template <typename C> struct make_constval_ : make_constval_0_<decltype(std::declval<C>()()), C> { };
template <typename T, T V> struct make_constval_<std::integral_constant<T, V>> { using type = std::integral_constant<T, V>; }; // shortcut
template <typename C> using make_constval_t = typename make_constval_<C>::type;


template <typename C> constexpr auto constval_value = C{ }(); // workaround for EDG


    // idea taken from Ben Deane & Jason Turner, "constexpr ALL the things!", C++Now 2017
    // currently not used because VS doesn't reliably support constexpr detection
    // (...and also because we now expect that constexpr values are explicitly marked as such)
//template <typename F> using is_constexpr_functor_r = std::integral_constant<bool, (std::declval<F>()(), true)>;
template <typename F> using is_constexpr_functor_r = decltype(std::declval<F>()());


template <typename C>
    constexpr auto get_hvalue_impl(std::true_type /*constvalArg*/, const C&)
{
    return constval_value<C>;
}
template <typename V>
    constexpr V get_hvalue_impl(std::false_type /*constvalArg*/, const V& arg)
{
    return arg;
}
template <typename H>
    constexpr auto get_hvalue(const H& arg)
{
    return get_hvalue_impl(is_constval_<H>{ }, arg);
}


template <typename F, typename... Cs>
    struct constval_transform_functor
{
    constexpr auto operator ()(void) const
    {
        return stateless_functor_v<F>(constval_value<Cs>...);
    }
};

template <typename F, typename... Cs>
    constexpr auto constval_transform_impl(std::true_type /*constvalArgs*/, const Cs&...) noexcept
{
    return make_constval_t<constval_transform_functor<F, Cs...>>{ };
}
template <typename F, typename... Cs>
    constexpr auto constval_transform_impl(std::false_type /*constvalArgs*/, const Cs&... args)
{
    return stateless_functor_v<F>(makeshift::detail::get_hvalue(args)...);
}


template <typename CF, typename... Cs>
    struct constval_extend_functor
{
    constexpr auto operator ()(void) const
    {
        return stateless_functor_v<CF>(make_constval_t<Cs>{ }...);
    }
};

template <typename CF, typename... Cs>
    constexpr auto constval_extend_impl(std::true_type /*constvalArgs*/, const Cs&...) noexcept
{
    return make_constval_t<constval_extend_functor<CF, Cs...>>{ };
}
template <typename CF, typename... Cs>
    constexpr auto constval_extend_impl(std::false_type /*constvalArgs*/, const Cs&... args)
{
    return stateless_functor_v<CF>(args...);
}


template <typename C>
    constexpr auto constval_extract_impl(std::true_type /*isConstval*/, const C&) noexcept
{
    return constval_value<C>;
}
template <typename C>
    constexpr C constval_extract_impl(std::false_type /*isConstval*/, const C& value)
{
    return value;
}


template <typename BoolC>
    constexpr void constval_assert_impl(std::true_type /*isConstval*/, BoolC arg) noexcept
{
    static_assert(arg(), "constval assertion failed");
}
static constexpr void constval_assert_impl(std::false_type /*isConstval*/, bool arg)
{
    Expects(arg);
}


} // namespace detail

} // namespace makeshift


#endif // INCLUDED_MAKESHIFT_DETAIL_CONSTVAL_HPP_
