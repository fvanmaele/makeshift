
#include <array>
#include <tuple>
#include <type_traits> // for integral_constant<>, is_same<>
#include <functional>  // for plus<>

#include <makeshift/array.hpp>   // for array<>
#include <makeshift/utility.hpp>
#include <makeshift/constval.hpp>

#include <catch2/catch.hpp>


namespace mk = makeshift;


constexpr std::tuple<int, int> getTuple(void)
{
    return { 4, 2 };
}

/*static auto getTupleElement = [](auto indexR)
{
    return std::get<indexR()>(getTuple());
};*/

template <typename... Ts>
    void discard_args(Ts...)
{
}

template <typename T, T V>
    void expect_constval_normalization(std::integral_constant<T, V>)
{
}

template <typename T, T... Vs>
    void expect_array_constval_normalization(mk::array_constant<T, Vs...>)
{
}

template <typename T, T... Vs>
    void expect_nested_array_constval_normalization(mk::array_constant<T, Vs...>)
{
#if MAKESHIFT_CXX >= 17
    (expect_array_constval_normalization(mk::c<T, Vs>), ...);
#else // MAKESHIFT_CXX >= 17
    discard_args((expect_array_constval_normalization(mk::c<T, Vs>), 1)...);
#endif // MAKESHIFT_CXX >= 17
}

template <typename... Cs>
    void expect_array_tuple_constval_normalization(mk::tuple_constant<Cs...>)
{
#if MAKESHIFT_CXX >= 17
    (expect_array_constval_normalization(Cs{ }), ...);
#else // MAKESHIFT_CXX >= 17
    discard_args((expect_array_constval_normalization(Cs{ }), 1)...);
#endif // MAKESHIFT_CXX >= 17
}

template <typename T>
    void expect_type_tag(mk::type<T>)
{
}

template <typename... Ts>
    void expect_type_sequence_tag(mk::type_sequence<Ts...>)
{
}

template <typename C>
    void expect_tuple_like(C c)
{
    (void) c;

#if MAKESHIFT_CXX >= 17
    if constexpr (std::tuple_size_v<C> > 0)
    {
        using std::get;
        std::tuple_element_t<0, C> c0 = get<0>(c);
        (void) c0;
    }
#endif // MAKESHIFT_CXX >= 17
}


struct CustomType
{
    int i;
    float f;
    std::array<int, 2> a;
};

struct SomeClass
{
    static constexpr CustomType ct = { 4, 1.41421f, { 1, 3 } };
    static constexpr std::array<int, 2> ca = { 2, 4 };
};

enum class Color { red, green, blue };


struct C5
{
    constexpr auto operator ()(void) const noexcept
    {
        return 5;
    }
};
struct CClr
{
    constexpr auto operator ()(void) const noexcept
    {
        return Color::red;
    }
};
struct CA
{
    constexpr auto operator ()(void) const noexcept
    {
        return std::array<int, 2>{ 4, 2 };
    }
};
struct CAA
{
    constexpr auto operator ()(void) const noexcept
    {
        return std::array<std::array<int, 1>, 2>{ std::array<int, 1>{ 4 }, std::array<int, 1>{ 2 } };
    }
};
struct CTA
{
    constexpr auto operator ()(void) const noexcept
    {
        return std::make_tuple(std::array<int, 1>{ 3 }, std::array<int, 2>{ 1, 4 });
    }
};
struct CT
{
    constexpr auto operator ()(void) const noexcept
    {
        return mk::type_c<int>;
    }
};
struct CTS
{
    constexpr auto operator ()(void) const noexcept
    {
        return mk::type_sequence<int, float>{ };
    }
};

TEST_CASE("constval")
{
    auto c1 = std::integral_constant<int, 1>{ };
    auto c51 = mk::make_constval(C5{ });
    expect_constval_normalization(c51);
#if MAKESHIFT_CXX >= 17
    auto c52 = mk::make_constval([]{ return 5; });
    expect_constval_normalization(c52);
#endif // MAKESHIFT_CXX >= 17
    auto c42 = 42;

    auto cClr1 = mk::make_constval(CClr{ });
    expect_constval_normalization(cClr1);
#if MAKESHIFT_CXX >= 17
    auto cClr2 = mk::make_constval([]{ return Color::red; });
    expect_constval_normalization(cClr2);
#endif // MAKESHIFT_CXX >= 17

    auto c42R = mk::constval_transform(std::plus<>{ }, c1, c42);
    static_assert(std::is_same<decltype(c42R), int>::value, "wrong type");
    CHECK(c42R == 43);

    auto cA1 = mk::make_constval(CA{ });
    expect_array_constval_normalization<int, 4, 2>(cA1);
    mk::mdarray<int, 2> ncA1 = cA1;
    (void) ncA1;
#if MAKESHIFT_CXX >= 17
    auto cA2 = mk::make_constval([]{ return std::array{ 4, 2 }; });
    expect_array_constval_normalization<int, 4, 2>(cA2);
    mk::mdarray<int, 2> ncA2 = cA2;
    (void) ncA2;
#endif // MAKESHIFT_CXX >= 17

    auto cAA1 = mk::make_constval(CAA{ });
    expect_nested_array_constval_normalization(cAA1);
    mk::mdarray<int, 2, 1> ncAA1 = cAA1;
    (void) ncAA1;
#if MAKESHIFT_CXX >= 17
    auto cAA2 = mk::make_constval([]{ return std::array{ std::array{ 4 }, std::array{ 2 } }; });
    expect_nested_array_constval_normalization(cAA2);
    mk::mdarray<int, 2, 1> ncAA2 = cAA2;
    (void) ncAA2;
#endif // MAKESHIFT_CXX >= 17

    auto cTA1 = mk::make_constval(CTA{ });
    expect_array_tuple_constval_normalization(cTA1);
    std::tuple<std::array<int, 1>, std::array<int, 2>> ncTA1 = cTA1;
    (void) ncTA1;
#if MAKESHIFT_CXX >= 17
    auto cTA2 = mk::make_constval([]{ return std::tuple{ std::array{ 3 }, std::array{ 1, 4 } }; });
    expect_array_tuple_constval_normalization(cTA2);
    std::tuple<std::array<int, 1>, std::array<int, 2>> ncTA2 = cTA2;
    (void) ncTA2;
#endif // MAKESHIFT_CXX >= 17

    auto cT1 = mk::make_constval(CT{ });
    expect_type_tag<int>(cT1);
#if MAKESHIFT_CXX >= 17
    auto cT2 = mk::make_constval([]{ return mk::type_c<int>; });
    expect_type_tag<int>(cT2);
#endif // MAKESHIFT_CXX >= 17

    auto cT3 = mk::type_c<float>;
    expect_type_tag<float>(cT3);

    auto cTS1 = mk::make_constval(CTS{ });
    expect_type_sequence_tag<int, float>(cTS1);
#if MAKESHIFT_CXX >= 17
    auto cTS2 = mk::make_constval([]{ return mk::type_sequence<int, float>{ }; });
    expect_type_sequence_tag<int, float>(cTS2);
#endif // MAKESHIFT_CXX >= 17

    auto cTS3 = mk::type_sequence<float, int>{ };
    expect_type_sequence_tag<float, int>(cTS3);

#if MAKESHIFT_CXX >= 17
    auto cCT = mk::make_constval([]
        {
            return CustomType{
                42,
                13.37f,
                { 4, 2 }
            };
        });
    auto cCTA = mk::constval_transform(
        [](auto customTypeObj)
        {
            return std::array{ customTypeObj, customTypeObj };
        },
        cCT);
    expect_tuple_like(cCTA);

    auto cCTV = mk::constval_transform(
        [](auto customTypeObj)
        {
            return std::tuple{ customTypeObj, customTypeObj };
        },
        cCT);
    expect_tuple_like(cCTV);
#endif // MAKESHIFT_CXX >= 17

    auto cCT1 = mk::c<CustomType const&, SomeClass::ct>;
    static constexpr CustomType c2 = cCT1();
    (void) c2;
    auto cA3 = mk::c<std::array<int, 2> const&, SomeClass::ca>;
    expect_array_constval_normalization(cA3);
#if MAKESHIFT_CXX >= 17
    auto cCT2 = mk::ref_c<SomeClass::ct>;
    static constexpr CustomType c3 = cCT2();
    (void) c3;
    auto cA4 = mk::ref_c<SomeClass::ca>;
    expect_array_constval_normalization(cA4);
#endif // MAKESHIFT_CXX >= 17

#if MAKESHIFT_CXX >= 17
    //auto iCT = mk::ref_c<SomeClass::ct.i>; // this doesn't work because arg doesn't have linkage
    //auto cCT3 = mk::ref_c<c2>; // this doesn't work either, for the same reason
    auto iCT = mk::make_constval([]{ return SomeClass::ct.i; });
    expect_constval_normalization<int>(iCT);
#endif // MAKESHIFT_CXX >= 17
}
