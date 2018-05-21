
#ifndef MAKESHIFT_TYPES_HPP_
#define MAKESHIFT_TYPES_HPP_


#include <type_traits> // for declval<>(), decay_t<>, reference_wrapper<>
#include <utility>     // for forward<>()
#include <cstdint>     // for uint32_t
#include <cstddef>     // for size_t


namespace makeshift
{

inline namespace types
{


    // Inherit from define_flags<> to define a flag enum type:
    // 
    //     struct Vegetable : define_flags<Vegetable>
    //     {
    //         static constexpr flag tomato { 1 };
    //         static constexpr flag onion { 2 };
    //         static constexpr flag eggplant { 4 };
    //         static constexpr flag garlic { 8 };
    //     };
    //     using Vegetables = Vegetable::flags;
template <typename FlagsT, typename UnderlyingTypeT = unsigned>
    struct define_flags
{
    enum class flags : UnderlyingTypeT { none = 0 };
    using flag = flags; // alias for declaring flag constants
    
    friend constexpr flags operator |(flags lhs, flags rhs) noexcept { return flags(UnderlyingTypeT(lhs) | UnderlyingTypeT(rhs)); }
    friend constexpr flags operator &(flags lhs, flags rhs) noexcept { return flags(UnderlyingTypeT(lhs) & UnderlyingTypeT(rhs)); }
    friend constexpr flags operator ^(flags lhs, flags rhs) noexcept { return flags(UnderlyingTypeT(lhs) ^ UnderlyingTypeT(rhs)); }
    friend constexpr flags operator ~(flags arg) noexcept { return flags(~UnderlyingTypeT(arg)); }
    friend constexpr flags& operator |=(flags& lhs, flags rhs) noexcept { lhs = lhs | rhs; return lhs; }
    friend constexpr flags& operator &=(flags& lhs, flags rhs) noexcept { lhs = lhs & rhs; return lhs; }
    friend constexpr flags& operator ^=(flags& lhs, flags rhs) noexcept { lhs = lhs ^ rhs; return lhs; }
    friend constexpr bool hasFlag(flags _flags, flag _flag) noexcept { return (UnderlyingTypeT(_flags) & UnderlyingTypeT(_flag)) != 0; }
    friend constexpr bool hasAnyOf(flags _flags, flags desiredFlags) noexcept { return (UnderlyingTypeT(_flags) & UnderlyingTypeT(desiredFlags)) != 0; }
    friend constexpr bool hasAllOf(flags _flags, flags desiredFlags) noexcept { return flags(UnderlyingTypeT(_flags) & UnderlyingTypeT(desiredFlags)) == desiredFlags; }
};


    // Helper for type dispatching.
template <typename T = void>
    struct tag
{
    using type = T;
};

} // inline namespace types


namespace detail
{

    // adapted from Mark Adler's post at https://stackoverflow.com/a/27950866
static constexpr std::uint32_t crc32c(std::uint32_t crc, const char* buf, std::size_t len) noexcept
{
        // CRC-32 (Ethernet, ZIP, etc.) polynomial in reversed bit order.
    constexpr std::uint32_t poly = 0xedb88320u;

    crc = ~crc;
    while (len--)
    {
        crc ^= (unsigned char) *buf++;
        for (int k = 0; k < 8; k++)
            crc = crc & 1 ? (crc >> 1) ^ poly : crc >> 1;
    }
    return ~crc;
}

enum class key_crc : std::uint32_t { };

struct key_name
{
    const char* data;
    std::size_t size;

    constexpr operator key_crc(void) const noexcept
    {
        return key_crc(crc32c(0, data, size));
    }
};

constexpr inline key_crc operator +(key_crc lhs, key_name rhs) noexcept
{
    return key_crc(crc32c(std::uint32_t(lhs), rhs.data, rhs.size));
}
constexpr inline key_crc operator +(key_name lhs, key_name rhs) noexcept
{
    return key_crc(lhs) + rhs;
}
constexpr inline key_crc operator /(key_crc lhs, key_name rhs) noexcept
{
    char sep[] = { '/' };
    auto sc = key_crc(crc32c(std::uint32_t(lhs), sep, 1));
    return sc + rhs;
}
constexpr inline key_crc operator /(key_name lhs, key_name rhs) noexcept
{
    return key_crc(lhs) / rhs;
}

} // namespace detail


inline namespace types
{


    // Named object wrapper.
    // Use with the ""_kn literal defined below:
    //
    //     using NamedInt = named<int, "width"_kn>;
    //
    // Construct an object of a named type either with the explicit constructor, or by using name<>
    // with assignment syntax:
    //
    //     NamedInt val1 { 42 };
    //     NamedInt val2 = { name<"width"_kn> = 42 };
    //
    // This is currently implemented using CRC-32 to work around the inability to pass strings as template
    // arguments. This may change in C++20, cf. P0732. I hope to be able to switch to a P0732-based
    // implementation while maintaining source compatibility.
template <typename T, makeshift::detail::key_crc KeyCRC>
    struct named
{
    T value;

    explicit constexpr named(const T& _value)
    noexcept(noexcept(T(_value)))
        : value(_value)
    {
    }
    explicit constexpr named(T&& _value)
    noexcept(noexcept(T(std::move(_value))))
        : value(std::move(_value))
    {
    }
};

} // inline namespace types

namespace detail
{

template <key_crc KeyCRC>
    struct key
{
    static constexpr key_crc value = KeyCRC;

    template <typename T>
        constexpr named<std::decay_t<T>, value> operator =(T&& rhs) const
        noexcept(noexcept(std::decay_t<T>(std::forward<T>(rhs))))
    {
        return named<std::decay_t<T>, value> { std::forward<T>(rhs) };
    }
};

} // namespace detail

inline namespace types
{

template <makeshift::detail::key_crc KeyCRC>
    constexpr inline makeshift::detail::key<KeyCRC> name { };

} // inline namespace types

inline namespace literals
{

constexpr inline makeshift::detail::key_name operator ""_kn(const char* data, std::size_t size) noexcept
{
    return { data, size };
}

} // inline namespace literals

namespace detail
{

struct default_overload_tag { };

template <typename... Fs>
    struct overload_base : Fs...
{
    constexpr overload_base(Fs&&... fs) : Fs(std::move(fs))... { }
    using Fs::operator ()...;
    template <typename T>
        constexpr decltype(auto) operator()(std::reference_wrapper<T> arg)
        noexcept(noexcept(std::declval<overload_base>()(arg.get())))
    {
        return (*this)(arg.get());
    }
    template <typename T>
        constexpr decltype(auto) operator()(std::reference_wrapper<T> arg) const
        noexcept(noexcept(std::declval<overload_base>()(arg.get())))
    {
        return (*this)(arg.get());
    }
};

template <typename F>
    struct default_overload_wrapper : F
{
    constexpr default_overload_wrapper(F&& func)
        : F(std::move(func))
    {
    }
    template <typename... Ts>
#ifdef MAKESHIFT_FANCY_DEFAULT
        constexpr decltype(auto) operator ()(default_overload_tag, Ts&&... args) const
#else // MAKESHIFT_FANCY_DEFAULT
        constexpr decltype(auto) operator ()(Ts&&... args) const
#endif // MAKESHIFT_FANCY_DEFAULT
        noexcept(noexcept(F::operator ()(std::forward<Ts>(args)...)))
    {
        return F::operator ()(std::forward<Ts>(args)...);
    }
};
struct ignore_overload_wrapper
{
    template <typename... Ts>
#ifdef MAKESHIFT_FANCY_DEFAULT
        constexpr void operator ()(default_overload_tag, Ts&&...) const noexcept
#else // MAKESHIFT_FANCY_DEFAULT
        constexpr void operator ()(Ts&&...) const noexcept
#endif // MAKESHIFT_FANCY_DEFAULT
    {
    }
};

template <std::size_t N, typename T, std::size_t... Is>
    constexpr std::array<std::remove_cv_t<T>, N> to_array_impl(T array[], std::index_sequence<Is...>)
    noexcept(noexcept(std::remove_cv_t<T>(std::declval<T>())))
{
    return {{ array[Is]... }};
}

template <typename F, template <typename...> class T>
    struct match_template_func : F
{
    constexpr match_template_func(F&& func)
        : F(std::move(func))
    {
    }
    template <typename... Ts>
        constexpr decltype(auto) operator ()(const T<Ts...>& arg)
    {
        return F::operator ()(arg);
    }
    template <typename... Ts>
        constexpr decltype(auto) operator ()(const T<Ts...>& arg) const
    {
        return F::operator ()(arg);
    }
    template <typename... Ts>
        constexpr decltype(auto) operator ()(T<Ts...>& arg)
    {
        return F::operator ()(arg);
    }
    template <typename... Ts>
        constexpr decltype(auto) operator ()(T<Ts...>& arg) const
    {
        return F::operator ()(arg);
    }
    template <typename... Ts>
        constexpr decltype(auto) operator ()(T<Ts...>&& arg)
    {
        return F::operator ()(std::move(arg));
    }
    template <typename... Ts>
        constexpr decltype(auto) operator ()(T<Ts...>&& arg) const
    {
        return F::operator ()(std::move(arg));
    }
};

template <typename T, template <typename...> class U>
    struct is_template_ : std::false_type
{
};
template <template <typename...> class U, typename... Ts>
    struct is_template_<U<Ts...>, U> : std::true_type
{
};

} // namespace detail

inline namespace types
{

template <std::size_t N, typename T>
    constexpr std::array<std::remove_cv_t<T>, N> to_array(T (&array)[N])
    noexcept(noexcept(std::remove_cv_t<T>(std::declval<T>())))
{
    return makeshift::detail::to_array_impl(array, std::make_index_sequence<N>{ });
}

struct ignore_t { };
constexpr inline ignore_t ignore { };

template <typename F>
    constexpr makeshift::detail::default_overload_wrapper<std::decay_t<F>> otherwise(F&& func)
    noexcept(noexcept(F(std::forward<F>(func))))
{
    return { std::forward<F>(func) };
}
constexpr inline makeshift::detail::ignore_overload_wrapper otherwise(ignore_t) noexcept
{
    return { };
}

template <typename... Fs>
    struct overload : makeshift::detail::overload_base<Fs...>
{
    using base = makeshift::detail::overload_base<Fs...>;
    using base::base;

#ifdef MAKESHIFT_FANCY_DEFAULT
private:
    struct test : base
    {
        using base::operator ();
        makeshift::detail::default_overload_tag operator ()(...) const;
    };

public:
    template <typename... Ts>
        constexpr decltype(auto) operator()(Ts&&... args)
    {
        using ResultType = decltype(std::declval<test>()(std::forward<Ts>(args)...));
        constexpr bool isDefaultOverload = std::is_same<ResultType, makeshift::detail::default_overload_tag>::value;
        if constexpr (isDefaultOverload)
            return base::operator ()(makeshift::detail::default_overload_tag{ }, std::forward<Ts>(args)...);
        else
            return base::operator ()(std::forward<Ts>(args)...);
    }
    template <typename... Ts>
        constexpr decltype(auto) operator()(Ts&&... args) const
    {
        using ResultType = decltype(std::declval<const test>()(std::forward<Ts>(args)...));
        constexpr bool isDefaultOverload = std::is_same<ResultType, makeshift::detail::default_overload_tag>::value;
        if constexpr (isDefaultOverload)
            return base::operator ()(makeshift::detail::default_overload_tag{ }, std::forward<Ts>(args)...);
        else
            return base::operator ()(std::forward<Ts>(args)...);
    }
#else // MAKESHIFT_FANCY_DEFAULT
    using base::operator ();
#endif // MAKESHIFT_FANCY_DEFAULT
};

template <template <typename...> class T, typename F>
    constexpr makeshift::detail::match_template_func<std::decay_t<F>, T> match_template(F&& func)
{
    return { std::forward<F>(func) };
}

template <typename T, template <typename...> class U>
    using is_template_t = makeshift::detail::is_template_<T, U>;
template <typename T, template <typename...> class U>
    constexpr bool is_template = is_template_t<T, U>::value;

} // inline namespace types

namespace detail
{

template <std::size_t... Is, typename TupleT, typename F>
    constexpr void tuple_foreach_impl(std::index_sequence<Is...>, TupleT&& tuple, F&& func)
{
    (func(std::get<Is>(std::forward<TupleT>(tuple))), ...);
}

} // namespace detail

inline namespace types
{

template <typename... Ts, typename F>
    constexpr void tuple_foreach(const std::tuple<Ts...>& tuple, F&& func)
{
    makeshift::detail::tuple_foreach_impl(std::make_index_sequence<sizeof...(Ts)>{ }, tuple,
        std::forward<F>(func));
}
template <typename... Ts, typename F>
    constexpr void tuple_foreach(std::tuple<Ts...>& tuple, F&& func)
{
    makeshift::detail::tuple_foreach_impl(std::make_index_sequence<sizeof...(Ts)>{ }, tuple,
        std::forward<F>(func));
}
template <typename... Ts, typename F>
    constexpr void tuple_foreach(std::tuple<Ts...>&& tuple, F&& func)
{
    makeshift::detail::tuple_foreach_impl(std::make_index_sequence<sizeof...(Ts)>{ }, std::move(tuple),
        std::forward<F>(func));
}

} // inline namespace types

} // namespace makeshift

#endif // MAKESHIFT_TYPES_HPP_
