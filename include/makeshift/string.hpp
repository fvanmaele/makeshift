﻿
#ifndef INCLUDED_MAKESHIFT_STRING_HPP_
#define INCLUDED_MAKESHIFT_STRING_HPP_


#include <string>      // for string, to_string<>
#include <string_view>
#include <type_traits> // for decay<>, is_enum<>
#include <utility>     // for move()
#include <algorithm>   // for equal()

#include <makeshift/type_traits.hpp> // for tag<>
#include <makeshift/metadata.hpp>
#include <makeshift/serialize.hpp>   // for define_serializer<>

#include <makeshift/detail/cfg.hpp>  // for MAKESHIFT_DLLFUNC
#include <makeshift/detail/string_compare.hpp>
#include <makeshift/detail/serialize_enum.hpp>


namespace makeshift
{

namespace detail
{


inline std::string scalar_to_string(std::string s) { return std::move(s); }
inline std::string scalar_to_string(int val) { return std::to_string(val); }
inline std::string scalar_to_string(unsigned val) { return std::to_string(val); }
inline std::string scalar_to_string(long val) { return std::to_string(val); }
inline std::string scalar_to_string(unsigned long val) { return std::to_string(val); }
inline std::string scalar_to_string(long long val) { return std::to_string(val); }
inline std::string scalar_to_string(unsigned long long val) { return std::to_string(val); }
inline std::string scalar_to_string(float val) { return std::to_string(val); }
inline std::string scalar_to_string(double val) { return std::to_string(val); }
inline std::string scalar_to_string(long double val) { return std::to_string(val); }

inline std::string scalar_from_string(tag<std::string>, const std::string& s) { return s; }
inline int scalar_from_string(tag<int>, const std::string& string) { return std::stoi(string); }
MAKESHIFT_DLLFUNC unsigned scalar_from_string(tag<unsigned>, const std::string& string);
inline long scalar_from_string(tag<long>, const std::string& string) { return std::stol(string); }
inline unsigned long scalar_from_string(tag<unsigned long>, const std::string& string) { return std::stoul(string); }
inline long long scalar_from_string(tag<long long>, const std::string& string) { return std::stoll(string); }
inline unsigned long long scalar_from_string(tag<unsigned long long>, const std::string& string) { return std::stoull(string); }
inline float scalar_from_string(tag<float>, const std::string& string) { return std::stof(string); }
inline double scalar_from_string(tag<double>, const std::string& string) { return std::stod(string); }
inline long double scalar_from_string(tag<long double>, const std::string& string) { return std::stold(string); }


template <typename MetadataTagT, typename T> struct have_string_conversion : is_enum_with_metadata<MetadataTagT, T> { };
template <typename MetadataTagT> struct have_string_conversion<MetadataTagT, std::string> : std::true_type { };
template <typename MetadataTagT> struct have_string_conversion<MetadataTagT, int> : std::true_type { };
template <typename MetadataTagT> struct have_string_conversion<MetadataTagT, unsigned> : std::true_type { };
template <typename MetadataTagT> struct have_string_conversion<MetadataTagT, long> : std::true_type { };
template <typename MetadataTagT> struct have_string_conversion<MetadataTagT, unsigned long> : std::true_type { };
template <typename MetadataTagT> struct have_string_conversion<MetadataTagT, long long> : std::true_type { };
template <typename MetadataTagT> struct have_string_conversion<MetadataTagT, unsigned long long> : std::true_type { };
template <typename MetadataTagT> struct have_string_conversion<MetadataTagT, float> : std::true_type { };
template <typename MetadataTagT> struct have_string_conversion<MetadataTagT, double> : std::true_type { };
template <typename MetadataTagT> struct have_string_conversion<MetadataTagT, long double> : std::true_type { };
template <typename MetadataTagT, typename T> constexpr bool have_string_conversion_v = have_string_conversion<MetadataTagT, T>::value;


} // namespace detail


inline namespace serialize
{


    // To customize string serialization for arbitrary types, define your own serializer type along with `to_string_impl()`, `from_string_impl()`
    // overloads in the same namespace.
    //
    // To override parts of the behavior of an existing serializer, have your serializer inherit from the existing serializer. Do not inherit from
    // serializers with orthogonal concerns and try to keep the scope of a serializer as small as possible to permit unambiguous combination.
    //
    // Orthogonal serializers can be combined with `combine()`:
    //ᅟ
    //ᅟ    auto serializer = combine(
    //ᅟ        string_serializer,
    //ᅟ        stream_serializer
    //ᅟ    );



    //ᅟ
    // Runtime arguments for string serializers.
    //
struct string_serializer_args
{
    enum_serialization_options_t enum_options;

    constexpr string_serializer_args(void) noexcept = default;
    constexpr string_serializer_args(enum_serialization_options_t _enum_options) noexcept : enum_options(_enum_options) { }
};


    //ᅟ
    // String serializer for common scalar types (built-in types and `std::string`).
    //
template <typename BaseT = void>
    struct string_serializer_t : define_serializer<string_serializer_t, BaseT, string_serializer_args>
{
    using base = define_serializer<makeshift::string_serializer_t, BaseT, string_serializer_args>; // TODO: report VC++ bug
    using base::base;
    
    template <typename T, typename SerializerT/*,
              typename = std::enable_if_t<makeshift::detail::have_string_conversion_v<serializer_metadata_tag_t<std::decay_t<SerializerT>>, std::decay_t<T>>>*/>
        friend std::enable_if_t<makeshift::detail::have_string_conversion_v<serializer_metadata_tag_t<std::decay_t<SerializerT>>, std::decay_t<T>>, std::string>
        to_string_impl(const T& value, const string_serializer_t& stringSerializer, SerializerT&)
    {
        (void) stringSerializer;
        using D = std::decay_t<T>;
        if constexpr (std::is_enum<D>::value)
            return to_string_impl(value, makeshift::detail::serialization_data<D, serializer_metadata_tag_t<std::decay_t<SerializerT>>>, stringSerializer.enum_options);
        else
            return makeshift::detail::scalar_to_string(value);
    }
    template <typename T, typename SerializerT/*,
              typename = std::enable_if_t<makeshift::detail::have_string_conversion_v<serializer_metadata_tag_t<std::decay_t<SerializerT>>, T>>*/>
        friend std::enable_if_t<makeshift::detail::have_string_conversion_v<serializer_metadata_tag_t<std::decay_t<SerializerT>>, T>, T>
        from_string_impl(tag<T>, const std::string& string, const string_serializer_t& stringSerializer, SerializerT&)
    {
        (void) stringSerializer;
        if constexpr (std::is_enum<T>::value)
            return from_string_impl(tag_v<T>, string, makeshift::detail::serialization_data<T, serializer_metadata_tag_t<std::decay_t<SerializerT>>>, stringSerializer.enum_options);
        else
            return makeshift::detail::scalar_from_string(tag_v<T>, string);
    }
};

    //ᅟ
    // String serializer for common scalar types (built-in types and `std::string`).
    //
constexpr string_serializer_t<> string_serializer{ };


    //ᅟ
    // Serializes the given value as string using the provided serializer.
    //ᅟ
    //ᅟ    std::string s = to_string(42, string_serializer); // returns "42"s
    //
template <typename T, typename SerializerT>
    std::string to_string(const T& value, SerializerT& serializer)
{
    return to_string_impl(value, serializer, serializer);
}


    //ᅟ
    // Serializes the given value as string using `string_serializer`.
    //ᅟ
    //ᅟ    std::string s = to_string(42); // returns "42"s
    //
template <typename T>
    std::string to_string(const T& value)
{
    return to_string(value, string_serializer);
}


    //ᅟ
    // Deserializes the given value from a string using the provided serializer.
    //ᅟ
    //ᅟ    int i = from_string(tag_v<int>, "42", string_serializer); // returns 42
    //
template <typename T, typename SerializerT>
    T from_string(tag<T>, const std::string& string, SerializerT& serializer)
{
    return from_string_impl(tag_v<T>, string, serializer, serializer);
}


    //ᅟ
    // Deserializes the given value from a string using `string_serializer`.
    //ᅟ
    //ᅟ    int i = from_string(tag_v<int>, "42"); // returns 42
    //
template <typename T>
    T from_string(tag<T>, const std::string& string)
{
    return from_string(tag_v<T>, string, string_serializer);
}


} // inline namespace serialize

} // namespace makeshift


#endif // INCLUDED_MAKESHIFT_STRING_HPP_
