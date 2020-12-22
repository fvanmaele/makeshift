
#ifndef INCLUDED_MAKESHIFT_DETAIL_RANGES_HPP_
#define INCLUDED_MAKESHIFT_DETAIL_RANGES_HPP_


#include <cstddef>     // for size_t, ptrdiff_t
#include <utility>     // for move(), integer_sequence<>
#include <iterator>    // for iterator_traits<>, random_access_iterator_tag
#include <type_traits> // for is_base_of<>, declval<>(), integral_constant<>

#include <gsl-lite/gsl-lite.hpp> // for index, gsl_Expects(), gsl_NODISCARD

#include <makeshift/detail/macros.hpp> // for MAKESHIFT_DETAIL_FORCEINLINE


namespace makeshift {

namespace gsl = ::gsl_lite;

namespace detail {


template <typename It, typename EndIt, typename IteratorTagT, std::ptrdiff_t Extent>
class range_base;
template <typename It, typename EndIt, typename IteratorTagT>
class range_base<It, EndIt, IteratorTagT, -1>
{
private:
    It first_;
    EndIt last_;

public:
    constexpr range_base(It first, EndIt last)
        : first_(std::move(first)), last_(std::move(last))
    {
    }

    gsl_NODISCARD constexpr It const& begin(void) const noexcept { return first_; }
    gsl_NODISCARD constexpr EndIt const& end(void) const noexcept { return last_; }
};
template <typename It, typename EndIt>
class range_base<It, EndIt, std::random_access_iterator_tag, -1>
{
private:
    It first_;
    EndIt last_;

public:
    constexpr range_base(It first, EndIt last)
        : first_(std::move(first)), last_(std::move(last))
    {
    }

    gsl_NODISCARD constexpr It const& begin(void) const noexcept { return first_; }
    gsl_NODISCARD constexpr EndIt const& end(void) const noexcept { return last_; }
    gsl_NODISCARD constexpr std::size_t size(void) const noexcept
    {
        return last_ - first_;
    }
    gsl_NODISCARD constexpr decltype(auto) operator [](std::size_t i) const noexcept
    {
        gsl_Expects(i < last_ - first_);
        return first_[i];
    }
};
template <typename It, std::ptrdiff_t Extent>
class range_base<It, It, std::random_access_iterator_tag, Extent>
{
    static_assert(Extent >= 0, "range extent must be non-negative");

private:
    It first_;

public:
    constexpr range_base(It first, It /*last*/)
        : first_(std::move(first))
    {
    }

    gsl_NODISCARD constexpr It const& begin(void) const noexcept { return first_; }
    gsl_NODISCARD constexpr It const& end(void) const noexcept { return first_ + Extent; }
    gsl_NODISCARD constexpr std::size_t size(void) const noexcept
    {
        return Extent;
    }
    gsl_NODISCARD constexpr decltype(auto) operator [](std::size_t i) const noexcept
    {
        gsl_Expects(i < std::size_t(Extent));
        return first_[i];
    }
};

template <typename It> using range_iterator_tag = std::conditional_t<
    std::is_base_of<std::random_access_iterator_tag, typename std::iterator_traits<It>::iterator_category>::value, // TODO: in C++20 we should use concepts to also permit non-legacy random-access iterators
    std::random_access_iterator_tag,
    std::input_iterator_tag>;

template <typename ExtentC>
constexpr std::ptrdiff_t range_extent_from_constval(ExtentC)
{
    return -1;
}
template <typename T, T V>
constexpr std::ptrdiff_t range_extent_from_constval(std::integral_constant<T, V>)
{
    return V;
}

template <typename T>
constexpr void check_buffer_extents(std::true_type /*dynamicExtent*/, std::size_t expectedExtent, std::size_t actualExtent)
{
    gsl_Expects(expectedExtent == actualExtent);
}
template <typename T>
constexpr void check_buffer_extents(std::false_type /*dynamicExtent*/, std::size_t /*expectedExtent*/, std::size_t /*actualExtent*/)
{
}

    // Implement tuple-like protocol for `range<>`.
template <class T>
constexpr T const& as_const(T& t) noexcept
{
    return t;
}
template <std::size_t I, typename It, std::ptrdiff_t Extent>
gsl_NODISCARD constexpr std::enable_if_t<(Extent >= 0), decltype(*std::declval<It>())>
get(range_base<It, It, std::random_access_iterator_tag, Extent>& range) noexcept
{
    static_assert(I < std::size_t(Extent), "index out of range");
    return range[I];
}
template <std::size_t I, typename It, std::ptrdiff_t Extent>
gsl_NODISCARD constexpr std::enable_if_t<(Extent >= 0), decltype(detail::as_const(*std::declval<It>()))>
get(range_base<It, It, std::random_access_iterator_tag, Extent> const& range) noexcept
{
    static_assert(I < std::size_t(Extent), "index out of range");
    return range[I];
}


class contiguous_index_range
{
private:
    gsl::index first_;
    gsl::index last_;

public:
    class const_iterator
    {
        friend class contiguous_index_range;

    private:
        gsl::index i_;

        explicit constexpr const_iterator(gsl::index _i) noexcept
            : i_(_i)
        {
        }

    public:
        MAKESHIFT_DETAIL_FORCEINLINE constexpr gsl::index
        operator *(void) const
        {
            return i_;
        }

        MAKESHIFT_DETAIL_FORCEINLINE constexpr const_iterator&
        operator ++(void)
        {
            ++i_;
            return *this;
        }

        MAKESHIFT_DETAIL_FORCEINLINE gsl_NODISCARD constexpr bool
        friend operator ==(const_iterator lhs, const_iterator rhs) noexcept
        {
            return lhs.i_ == rhs.i_;
        }
        MAKESHIFT_DETAIL_FORCEINLINE gsl_NODISCARD bool
        friend operator !=(const_iterator lhs, const_iterator rhs) noexcept
        {
            return !(lhs == rhs);
        }


        // Additional *LegacyIterator* requirements:
        // https://en.cppreference.com/w/cpp/named_req/Iterator

        using difference_type = gsl::diff;
        using value_type = gsl::index;
        using pointer = void;
        using reference = gsl::index;

            // We only satisfy the *LegacyInputIterator* requirement (see below).
        using iterator_category = std::input_iterator_tag;

            // However, we do satisfy the `std::random_access_iterator<>` concept.
        using iterator_concept = std::random_access_iterator_tag;

        MAKESHIFT_DETAIL_FORCEINLINE constexpr const_iterator
        operator ++(int)
        {
            auto result = *this;
            ++*this;
            return result;
        }


        // Additional *LegacyInputIterator* requirements:
        // https://en.cppreference.com/w/cpp/named_req/InputIterator

    private:
        struct ArrowProxy  // cf. https://quuxplusone.github.io/blog/2019/02/06/arrow-proxy/.
        {
            gsl::index i;

            MAKESHIFT_DETAIL_FORCEINLINE constexpr gsl::index*
            operator ->() && noexcept
            {
                return &i;
            }
        };

    public:
        MAKESHIFT_DETAIL_FORCEINLINE gsl_NODISCARD constexpr ArrowProxy
        operator ->(void)
        {
            return { i_ };
        }


        // We cannot satisfy the *LegacyForwardIterator* requirements:
        // https://en.cppreference.com/w/cpp/named_req/ForwardIterator
        // because our `reference` is neither `value_type&` nor `value_type const&`. However, we do already satisfy the
        // `std::forward_iterator<>` concept:
        // https://en.cppreference.com/w/cpp/iterator/forward_iterator


        // Additional `std::bidirectional_iterator<>` concept requirements:
        // https://en.cppreference.com/w/cpp/iterator/bidirectional_iterator

        MAKESHIFT_DETAIL_FORCEINLINE constexpr const_iterator&
        operator --()
        {
            --i_;
            return *this;
        }

        MAKESHIFT_DETAIL_FORCEINLINE constexpr const_iterator
        operator --(int)
        {
            auto result = *this;
            --*this;
            return result;
        }
        

        // Additional `std::random_access_iterator<>` concept requirements:
        // https://en.cppreference.com/w/cpp/iterator/random_access_iterator

        MAKESHIFT_DETAIL_FORCEINLINE constexpr const_iterator&
        operator +=(difference_type offset)
        {
            i_ += offset;
            return *this;
        }
        MAKESHIFT_DETAIL_FORCEINLINE constexpr const_iterator&
        operator -=(difference_type offset)
        {
            i_ -= offset;
            return *this;
        }

        MAKESHIFT_DETAIL_FORCEINLINE gsl_NODISCARD friend constexpr const_iterator
        operator +(const_iterator it, difference_type offset)
        {
            it += offset;
            return it;
        }
        MAKESHIFT_DETAIL_FORCEINLINE gsl_NODISCARD friend constexpr const_iterator
        operator +(difference_type offset, const_iterator it)
        {
            it += offset;
            return it;
        }
        MAKESHIFT_DETAIL_FORCEINLINE gsl_NODISCARD friend constexpr const_iterator
        operator -(const_iterator it, difference_type offset)
        {
            it -= offset;
            return it;
        }
        MAKESHIFT_DETAIL_FORCEINLINE gsl_NODISCARD friend constexpr difference_type
        operator -(const_iterator lhs, const_iterator rhs) noexcept
        {
            return lhs.i_ - rhs.i_;
        }

        MAKESHIFT_DETAIL_FORCEINLINE gsl_NODISCARD friend constexpr bool
        operator <(const_iterator lhs, const_iterator rhs) noexcept
        {
            return lhs.i_ < rhs.i_;
        }
        MAKESHIFT_DETAIL_FORCEINLINE gsl_NODISCARD friend constexpr bool
        operator <=(const_iterator lhs, const_iterator rhs) noexcept
        {
            return !(rhs < lhs);
        }
        MAKESHIFT_DETAIL_FORCEINLINE gsl_NODISCARD friend constexpr bool
        operator >(const_iterator lhs, const_iterator rhs) noexcept
        {
            return rhs < lhs;
        }
        MAKESHIFT_DETAIL_FORCEINLINE gsl_NODISCARD friend constexpr bool
        operator >=(const_iterator lhs, const_iterator rhs) noexcept
        {
            return !(lhs < rhs);
        }

        MAKESHIFT_DETAIL_FORCEINLINE gsl_NODISCARD constexpr gsl::index
        operator [](difference_type offset)
        {
            return *(*this + offset);
        }
    };
    using iterator = const_iterator;

    constexpr contiguous_index_range(gsl::index _first, gsl::index _last) noexcept
        : first_(_first), last_(_last)
    {
    }
    const_iterator
    begin(void) const noexcept
    {
        return const_iterator(first_);
    }
    const_iterator
    end(void) const noexcept
    {
        return const_iterator(last_);
    }
};


} // namespace detail

} // namespace makeshift


#endif // INCLUDED_MAKESHIFT_DETAIL_RANGES_HPP_
