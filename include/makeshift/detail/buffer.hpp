﻿
#ifndef INCLUDED_MAKESHIFT_DETAIL_BUFFER_HPP_
#define INCLUDED_MAKESHIFT_DETAIL_BUFFER_HPP_


#include <array>
#include <cstddef>     // for size_t, ptrdiff_t
#include <algorithm>   // for copy()
#include <type_traits> // for integral_constant<>

#include <gsl/gsl-lite.hpp> // for Expects()

#include <makeshift/macros.hpp> // for MAKESHIFT_NODISCARD, MAKESHIFT_CONSTEXPR17


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
    using const_pointer = T const*;
    using reference = T&;
    using const_reference = T const&;

    using iterator = T*;
    using const_iterator = T const*;

private:
    MAKESHIFT_CONSTEXPR17 T* _data(void) noexcept { return static_cast<DerivedT*>(this)->data(); }
    MAKESHIFT_CONSTEXPR17 T const* _data(void) const noexcept { return static_cast<DerivedT const*>(this)->data(); }
    MAKESHIFT_CONSTEXPR17 std::size_t _size(void) const noexcept { return static_cast<DerivedT const*>(this)->size(); }

public:
    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR17 iterator begin(void) noexcept
    {
        return _data();
    }
    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR17 const_iterator begin(void) const noexcept
    {
        return _data();
    }
    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR17 iterator end(void) noexcept
    {
        return _data() + _size();
    }
    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR17 const_iterator end(void) const noexcept
    {
        return _data() + _size();
    }

    MAKESHIFT_NODISCARD constexpr bool empty(void) const noexcept
    {
        return _size() == 0;
    }

    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR17 reference at(size_type i)
    {
        Expects(i < _size());
        return _data()[i];
    }
    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR17 const_reference at(size_type i) const
    {
        Expects(i < _size());
        return _data()[i];
    }
    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR17 reference operator [](size_type i)
    {
        return _data()[i];
    }
    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR17 const_reference operator [](size_type i) const
    {
        return _data()[i];
    }

    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR17 reference front(void) noexcept
    {
        Expects(!empty());
        return _data()[0];
    }
    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR17 const_reference front(void) const noexcept
    {
        Expects(!empty());
        return _data()[0];
    }
    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR17 reference back(void) noexcept
    {
        Expects(!empty());
        return _data()[_size() - 1];
    }
    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR17 const_reference back(void) const noexcept
    {
        Expects(!empty());
        return _data()[_size() - 1];
    }
};

template <typename T, std::ptrdiff_t Extent, std::ptrdiff_t MaxExtent = -1>
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

    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR17 iterator begin(void) noexcept
    {
        return data_.begin();
    }
    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR17 const_iterator begin(void) const noexcept
    {
        return data_.begin();
    }
    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR17 iterator end(void) noexcept
    {
        return data_.end();
    }
    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR17 const_iterator end(void) const noexcept
    {
        return data_.end();
    }

    MAKESHIFT_NODISCARD constexpr std::size_t size(void) const noexcept
    {
        return Extent;
    }

    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR17 T* data(void) noexcept
    {
        return data_.data();
    }
    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR17 T const* data(void) const noexcept
    {
        return data_.data();
    }
};
template <typename T, std::ptrdiff_t MaxExtent>
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

    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR17 T* data(void) noexcept
    {
        return data_.data();
    }
    MAKESHIFT_NODISCARD MAKESHIFT_CONSTEXPR17 T const* data(void) const noexcept
    {
        return data_.data();
    }
};

template <typename T, std::ptrdiff_t BufExtent>
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
    constexpr dynamic_buffer_base(dynamic_buffer_base const& rhs)
        : dynamic_buffer_base(rhs.size_)
    {
        // TODO: we could use non-initializing allocation and unitialized_copy() to optimize this further
        std::copy(rhs.begin(), rhs.end(), data_);
    }
    constexpr dynamic_buffer_base& operator =(dynamic_buffer_base const& rhs)
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
    MAKESHIFT_NODISCARD constexpr T const* data(void) const noexcept
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
    constexpr dynamic_buffer_base(dynamic_buffer_base const& rhs)
        : dynamic_buffer_base(rhs.size_)
    {
        // TODO: we could use non-initializing allocation and unitialized_copy() to optimize this further
        std::copy(rhs.begin(), rhs.end(), data_);
    }
    constexpr dynamic_buffer_base& operator =(dynamic_buffer_base const& rhs)
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
    MAKESHIFT_NODISCARD constexpr T const* data(void) const noexcept
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

static constexpr memory_location determine_memory_location(std::ptrdiff_t bufExtent, std::ptrdiff_t maxStaticBufferExtent) noexcept
{
    if (maxStaticBufferExtent < 0) return bufExtent < 0 ? memory_location::never_on_stack : memory_location::always_on_stack;
    if (maxStaticBufferExtent == 0) return memory_location::never_on_stack;
    if (bufExtent < 0) return memory_location::dynamic;
    if (bufExtent <= maxStaticBufferExtent) return memory_location::always_on_stack;
    return memory_location::never_on_stack;
}


template <typename T, std::ptrdiff_t Extent, std::ptrdiff_t MaxStaticBufferExtent, memory_location MemoryLocation>
    class buffer_base;
template <typename T, std::ptrdiff_t Extent, std::ptrdiff_t MaxStaticBufferExtent>
    class buffer_base<T, Extent, MaxStaticBufferExtent, memory_location::always_on_stack>
        : public static_buffer_base<T, Extent>
{
private:
    using base_ = static_buffer_base<T, Extent>;

public:
    using base_::base_;
};
template <typename T, std::ptrdiff_t Extent, std::ptrdiff_t MaxStaticBufferExtent>
    class buffer_base<T, Extent, MaxStaticBufferExtent, memory_location::dynamic>
        : public dynamic_buffer_base<T, MaxStaticBufferExtent>
{
private:
    using base_ = dynamic_buffer_base<T, MaxStaticBufferExtent>;

public:
    using base_::base_;
};
template <typename T, std::ptrdiff_t Extent, std::ptrdiff_t MaxStaticBufferExtent>
    class buffer_base<T, Extent, MaxStaticBufferExtent, memory_location::never_on_stack>
        : public dynamic_buffer_base<T, 0>
{
private:
    using base_ = dynamic_buffer_base<T, 0>;

public:
    using base_::base_;
};

template <typename ExtentT>
    constexpr std::ptrdiff_t buffer_extent_from_constval(ExtentT)
{
    return -1;
}
template <typename T, T V>
    constexpr std::ptrdiff_t buffer_extent_from_constval(std::integral_constant<T, V>)
{
    return V;
}

template <typename C>
    constexpr inline void check_buffer_extents(std::true_type /*dynamicExtent*/, std::ptrdiff_t /*expectedExtent*/, C actualExtent)
{
    Expects(actualExtent >= 0);
}
template <typename C>
    constexpr inline void check_buffer_extents(std::false_type /*dynamicExtent*/, std::ptrdiff_t expectedExtent, C actualExtent)
{
    Expects(actualExtent == expectedExtent);
}
template <typename C>
    constexpr inline void check_fixed_buffer_extents(std::true_type /*dynamicExtent*/, std::ptrdiff_t /*expectedExtent*/, C actualExtent, std::ptrdiff_t maxBufferExtent)
{
    Expects(actualExtent >= 0 && actualExtent <= maxBufferExtent);
}
template <typename C>
    constexpr inline void check_fixed_buffer_extents(std::false_type /*dynamicExtent*/, std::ptrdiff_t expectedExtent, C actualExtent, std::ptrdiff_t /*maxBufferExtent*/)
{
    Expects(actualExtent == expectedExtent);
}


} // namespace detail

} // namespace makeshift


#endif // INCLUDED_MAKESHIFT_DETAIL_BUFFER_HPP_
