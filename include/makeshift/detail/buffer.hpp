﻿
#ifndef INCLUDED_MAKESHIFT_DETAIL_BUFFER_HPP_
#define INCLUDED_MAKESHIFT_DETAIL_BUFFER_HPP_


#include <array>
#include <cstddef>     // for size_t
#include <iterator>    // for move_iterator<>
#include <type_traits> // for integral_constant<>, is_convertible<>

#include <gsl/gsl_assert> // for Expects()

#include <makeshift/utility2.hpp> // for dim
#include <makeshift/constval.hpp> // for constval_extract(), constval_assert(), constval_transform()


namespace makeshift
{

namespace detail
{


template <typename T, typename DerivedT>
    class buffer_interface_mixin
{
public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;

    using iterator = T*;
    using const_iterator = const T*;

private:
    MAKESHIFT_CONSTEXPR_CXX17 T* _data(void) noexcept { return static_cast<DerivedT*>(this)->data(); }
    MAKESHIFT_CONSTEXPR_CXX17 const T* _data(void) const noexcept { return static_cast<const DerivedT*>(this)->data(); }
    MAKESHIFT_CONSTEXPR_CXX17 std::size_t _size(void) const noexcept { return static_cast<const DerivedT*>(this)->size(); }

public:
    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR_CXX17 iterator begin(void) noexcept
    {
        return _data();
    }
    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR_CXX17 const_iterator begin(void) const noexcept
    {
        return _data();
    }
    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR_CXX17 iterator end(void) noexcept
    {
        return _data() + _size();
    }
    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR_CXX17 const_iterator end(void) const noexcept
    {
        return _data() + _size();
    }

    MAKESHIFT_NODISCARD constexpr bool empty(void) const noexcept
    {
        return _size() == 0;
    }

    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR_CXX17 reference at(size_type i)
    {
        Expects(i < _size());
        return _data()[i];
    }
    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR_CXX17 const_reference at(size_type i) const
    {
        Expects(i < _size());
        return _data()[i];
    }
    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR_CXX17 reference operator [](size_type i)
    {
        return _data()[i];
    }
    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR_CXX17 const_reference operator [](size_type i) const
    {
        return _data()[i];
    }

    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR_CXX17 reference front(void) noexcept
    {
        Expects(!empty());
        return _data()[0];
    }
    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR_CXX17 const_reference front(void) const noexcept
    {
        Expects(!empty());
        return _data()[0];
    }
    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR_CXX17 reference back(void) noexcept
    {
        Expects(!empty());
        return _data()[_size() - 1];
    }
    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR_CXX17 const_reference back(void) const noexcept
    {
        Expects(!empty());
        return _data()[_size() - 1];
    }
};

template <typename T, dim Extent, dim MaxExtent = -1>
    class static_buffer_base : public buffer_interface_mixin<T, static_buffer_base<T, Extent, MaxExtent>>
{
    static_assert(Extent >= 0, "buffer extent must be non-negative");

private:
    std::array<T, Extent> data_;

public:
    using iterator = typename std::array<T, Extent>::iterator;
    using const_iterator = typename std::array<T, Extent>::const_iterator;

    constexpr static_buffer_base(std::size_t)
        : data_{ }
    {
    }

    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR_CXX17 iterator begin(void) noexcept
    {
        return data_.begin();
    }
    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR_CXX17 const_iterator begin(void) const noexcept
    {
        return data_.begin();
    }
    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR_CXX17 iterator end(void) noexcept
    {
        return data_.end();
    }
    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR_CXX17 const_iterator end(void) const noexcept
    {
        return data_.end();
    }

    MAKESHIFT_NODISCARD constexpr std::size_t size(void) const noexcept
    {
        return Extent;
    }

    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR_CXX17 T* data(void) noexcept
    {
        return data_.data();
    }
    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR_CXX17 const T* data(void) const noexcept
    {
        return data_.data();
    }
};
template <typename T, dim MaxExtent>
    class static_buffer_base<T, -1, MaxExtent> : public buffer_interface_mixin<T, static_buffer_base<T, -1, MaxExtent>>
{
private:
    std::array<T, MaxExtent> data_;
    std::size_t size_;

public:
    constexpr static_buffer_base(std::size_t _size)
        : data_{ }, size_(_size)
    {
    }

    MAKESHIFT_NODISCARD constexpr std::size_t size(void) const noexcept
    {
        return size_;
    }

    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR_CXX17 T* data(void) noexcept
    {
        return data_.data();
    }
    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR_CXX17 const T* data(void) const noexcept
    {
        return data_.data();
    }
};

template <typename T, dim BufExtent>
    class dynamic_buffer_base : public buffer_interface_mixin<T, dynamic_buffer_base<T, BufExtent>>
{
private:
    T* data_;
    std::size_t size_;
    std::array<T, BufExtent> buf_;

public:
    constexpr dynamic_buffer_base(std::size_t _size)
        : size_(_size)
    {
        if (_size <= BufExtent)
            data_ = buf_.data();
        else
            data_ = new T[_size]{ };
    }
    constexpr dynamic_buffer_base(const dynamic_buffer_base& rhs)
        : dynamic_buffer_base(rhs.size_)
    {
        // TODO: we could use non-initializing allocation and unitialized_copy() to optimize this further
        std::copy(rhs.begin(), rhs.end(), data_);
    }
    constexpr dynamic_buffer_base& operator =(const dynamic_buffer_base& rhs)
    {
        *this = dynamic_buffer_base(rhs); // copy & move for exception safety
        return *this;
    }
    dynamic_buffer_base(dynamic_buffer_base&& rhs) noexcept = delete;
    dynamic_buffer_base& operator =(dynamic_buffer_base&& rhs) noexcept = delete;
    ~dynamic_buffer_base(void)
    {
        if (data_ != buf_.data())
            delete[] data_;
    }

    MAKESHIFT_NODISCARD constexpr std::size_t size(void) const noexcept
    {
        return size_;
    }

    MAKESHIFT_NODISCARD constexpr T* data(void) noexcept
    {
        return data_;
    }
    MAKESHIFT_NODISCARD constexpr const T* data(void) const noexcept
    {
        return data_;
    }
};

template <typename T>
    class dynamic_buffer_base<T, 0> : public buffer_interface_mixin<T, dynamic_buffer_base<T, 0>>
{
private:
    T* data_;
    std::size_t size_;

public:
    constexpr dynamic_buffer_base(std::size_t _size)
        : data_(_size > 0 ? new T[_size]{ } : nullptr), size_(_size)
    {
    }
    constexpr dynamic_buffer_base(const dynamic_buffer_base& rhs)
        : dynamic_buffer_base(rhs.size_)
    {
        // TODO: we could use non-initializing allocation and unitialized_copy() to optimize this further
        std::copy(rhs.begin(), rhs.end(), data_);
    }
    constexpr dynamic_buffer_base& operator =(const dynamic_buffer_base& rhs)
    {
        *this = dynamic_buffer_base(rhs); // copy & move for exception safety
        return *this;
    }
    dynamic_buffer_base(dynamic_buffer_base&& rhs) noexcept = delete;
    dynamic_buffer_base& operator =(dynamic_buffer_base&& rhs) noexcept = delete;
    ~dynamic_buffer_base(void)
    {
        delete[] data_;
    }

    MAKESHIFT_NODISCARD constexpr std::size_t size(void) const noexcept
    {
        return size_;
    }

    MAKESHIFT_NODISCARD constexpr T* data(void) noexcept
    {
        return data_;
    }
    MAKESHIFT_NODISCARD constexpr const T* data(void) const noexcept
    {
        return data_;
    }
};


enum class memory_location
{
    always_on_stack,
    dynamic,
    never_on_stack
};

static constexpr memory_location determine_memory_location(dim bufExtent, dim maxStaticBufferExtent) noexcept
{
    if (maxStaticBufferExtent < 0)
        return bufExtent < 0 ? memory_location::never_on_stack : memory_location::always_on_stack;
    if (maxStaticBufferExtent == 0)
        return memory_location::never_on_stack;
    if (bufExtent < 0)
        return memory_location::dynamic;
    if (bufExtent <= maxStaticBufferExtent)
        return memory_location::always_on_stack;
    return memory_location::never_on_stack;
}


template <typename T, dim Extent, dim MaxStaticBufferExtent, memory_location MemoryLocation>
    class buffer_base;
template <typename T, dim Extent, dim MaxStaticBufferExtent>
    class buffer_base<T, Extent, MaxStaticBufferExtent, memory_location::always_on_stack>
        : public static_buffer_base<T, Extent>
{
private:
    using _base = static_buffer_base<T, Extent>;

public:
    using _base::_base;
};
template <typename T, dim Extent, dim MaxStaticBufferExtent>
    class buffer_base<T, Extent, MaxStaticBufferExtent, memory_location::dynamic>
        : public dynamic_buffer_base<T, MaxStaticBufferExtent>
{
private:
    using _base = dynamic_buffer_base<T, MaxStaticBufferExtent>;

public:
    using _base::_base;
};
template <typename T, dim Extent, dim MaxStaticBufferExtent>
    class buffer_base<T, Extent, MaxStaticBufferExtent, memory_location::never_on_stack>
        : public dynamic_buffer_base<T, 0>
{
private:
    using _base = dynamic_buffer_base<T, 0>;

public:
    using _base::_base;
};

template <typename ArrayT, dim Extent = -1, dim MaxStaticBufferExtent = -1>
    class buffer
        : public buffer_base<typename array_element_type_<ArrayT>::type, Extent, MaxStaticBufferExtent, makeshift::detail::determine_memory_location(Extent, MaxStaticBufferExtent)>
{
private:
    using _base = buffer_base<typename array_element_type_<ArrayT>::type, Extent, MaxStaticBufferExtent, makeshift::detail::determine_memory_location(Extent, MaxStaticBufferExtent)>;

    struct check_size_functor
    {
        constexpr bool operator ()(dim size) const noexcept
        {
            return size >= 0
                && (Extent == -1 || size == Extent);
        }
    };

public:
    template <typename C>
        constexpr buffer(C _size)
            : _base(makeshift::constval_extract(_size))
    {
        makeshift::constval_assert(makeshift::constval_transform(check_size_functor{ }, _size));
    }
    template <dim RExtent, typename U>
        constexpr buffer(U (&&array)[RExtent])
            : _base(RExtent)
    {
        static_assert(Extent == -1 || RExtent == Extent, "array extent does not match");
        static_assert(std::is_convertible<U, typename array_element_type_<ArrayT>::type>::value, "incompatible array element types");
        std::copy(std::make_move_iterator(array), std::make_move_iterator(array + RExtent), this->begin());
    }
    template <dim RExtent, typename U>
        constexpr buffer& operator =(U (&&array)[RExtent])
    {
        static_assert(Extent == -1 || RExtent == Extent, "array extent does not match");
        static_assert(std::is_convertible<U, typename array_element_type_<ArrayT>::type>::value, "incompatible array element types");
        Expects(RExtent == this->size());
        std::copy(std::make_move_iterator(array), std::make_move_iterator(array + RExtent), this->begin());
        return *this;
    }
};

template <typename ArrayT, dim Extent, dim MaxBufferExtent>
    class fixed_buffer
        : public static_buffer_base<typename array_element_type_<ArrayT>::type, Extent, MaxBufferExtent>
{
    static_assert(MaxBufferExtent >= 0, "invalid maximal buffer extent");
    static_assert(Extent <= MaxBufferExtent, "size exceeds buffer extent");

private:
    using _base = static_buffer_base<typename array_element_type_<ArrayT>::type, Extent, MaxBufferExtent>;

    struct check_size_functor
    {
        constexpr bool operator ()(dim size) const noexcept
        {
            return size >= 0
                && (Extent == -1 || size == Extent)
                && size <= MaxBufferExtent;
        }
    };

public:
    template <typename C>
        constexpr fixed_buffer(C _size)
            : _base(makeshift::constval_extract(_size))
    {
        makeshift::constval_assert(makeshift::constval_transform(check_size_functor{ }, _size));
    }
    template <dim RExtent, typename U>
        constexpr fixed_buffer(U (&&array)[RExtent])
            : _base(RExtent)
    {
        static_assert(Extent == -1 || RExtent == Extent, "array extent does not match");
        static_assert(std::is_convertible<U, typename array_element_type_<ArrayT>::type>::value, "incompatible array element types");
        std::copy(std::make_move_iterator(array), std::make_move_iterator(array + RExtent), this->begin());
    }
    template <dim RExtent, typename U>
        constexpr fixed_buffer& operator =(U (&&array)[RExtent])
    {
        static_assert(Extent == -1 || RExtent == Extent, "array extent does not match");
        static_assert(std::is_convertible<U, typename array_element_type_<ArrayT>::type>::value, "incompatible array element types");
        Expects(RExtent == this->size());
        std::copy(std::make_move_iterator(array), std::make_move_iterator(array + RExtent), this->begin());
        return *this;
    }
};

template <typename C>
    constexpr dim static_dim_impl(std::true_type /*isConstval*/)
{
    return constval_value<C>;
}
template <typename C>
    constexpr dim static_dim_impl(std::false_type /*isConstval*/)
{
    return -1;
}
template <typename C>
    constexpr dim static_dim(void)
{
    return makeshift::detail::static_dim_impl<C>(makeshift::detail::is_constval_<C>{ });
}


} // namespace detail

} // namespace makeshift


namespace std
{


template <typename T, makeshift::dim Extent, makeshift::dim MaxStaticBufferExtent> struct tuple_size<makeshift::detail::buffer<T[], Extent, MaxStaticBufferExtent>> : std::integral_constant<std::size_t, Extent> { };
template <typename T, makeshift::dim MaxStaticBufferExtent> struct tuple_size<makeshift::detail::buffer<T[], -1, MaxStaticBufferExtent>>; // undefined

template <typename T, makeshift::dim Extent, makeshift::dim MaxBufferExtent> struct tuple_size<makeshift::detail::fixed_buffer<T[], Extent, MaxBufferExtent>> : std::integral_constant<std::size_t, Extent> { };
template <typename T, makeshift::dim MaxBufferExtent> struct tuple_size<makeshift::detail::fixed_buffer<T[], -1, MaxBufferExtent>>; // undefined


} // namespace std


#endif // INCLUDED_MAKESHIFT_DETAIL_BUFFER_HPP_
