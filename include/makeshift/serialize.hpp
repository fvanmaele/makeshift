﻿
#ifndef INCLUDED_MAKESHIFT_SERIALIZE_EHPP_
#define INCLUDED_MAKESHIFT_SERIALIZE_EHPP_


#include <tuple>
#include <string>
#include <utility>     // for move(), forward<>()
#include <stdexcept>   // for runtime_error
#include <type_traits> // for decay<>, is_base_of<>

#include <makeshift/type_traits.hpp> // for tag<>, type_sequence<>, type_sequence_cat<>

#include <makeshift/detail/cfg.hpp>               // for MAKESHIFT_DLLFUNC
#include <makeshift/detail/string_compare.hpp>    // for string_comparison
#include <makeshift/detail/utility_chainable.hpp>


namespace makeshift
{

inline namespace serialize
{


    //ᅟ
    // Options for serializing and deserializing enums and flag enums with metadata.
    //
struct enum_serialization_options
{
        //ᅟ
        // Determines comparison mode for string representations of enum values.
        //
    string_comparison enum_string_comparison_mode = string_comparison::ordinal_ignore_case;

        //ᅟ
        // Determines separator for flags enum values.
        //
    std::string_view flags_separator = ", ";

    constexpr enum_serialization_options(void) = default;
};


    //ᅟ
    // Base class for serializers with argument class `SerializerArgsT`. Permits chaining of serializers.
    //ᅟ
    //ᅟ    struct MySerializerArgs { ... };
    //ᅟ    template <typename BaseT = void>
    //ᅟ        struct MySerializer : define_serializer<MySerializer, BaseT, MySerializerArgs>
    //ᅟ    {
    //ᅟ        using base = define_serializer<MySerializer, BaseT, MySerializerArgs>;
    //ᅟ        using base::base;
    //ᅟ        ...
    //ᅟ    };
    //
template <template <typename...> class SerializerT, typename BaseT, typename SerializerArgsT = void, typename... Ts>
    using define_serializer = define_chainable<SerializerT, BaseT, SerializerArgsT, makeshift::detail::serializer_base, Ts...>;


    //ᅟ
    // Use this class with `chain()` to inject a metadata tag into a metadata-based serializer.
    //
template <typename MetadataTagT = serialization_metadata_tag, typename BaseT = void>
    struct metadata_tag_for_serializer : define_serializer<metadata_tag_for_serializer, BaseT, void, MetadataTagT>
{
    using base = define_serializer<makeshift::metadata_tag_for_serializer, BaseT, void, MetadataTagT>;
    using base::base;

    using metadata_tag = MetadataTagT;
};
template <typename MetadataTagT = serialization_metadata_tag> constexpr metadata_tag_for_serializer<MetadataTagT> metadata_tag_for_serializer_v { };


    //ᅟ
    // Retrieves the metadata tag to be used for metadata-based serializers.
    // Defaults to `serialization_metadata_tag` if the user did not override the tag by chaining with a `metadata_tag_for_serializer<>`.
    //
template <typename SerializerT> struct metadata_tag_of_serializer : std::conditional_t<can_apply_v<makeshift::detail::metadata_tag_rt, SerializerT>, makeshift::detail::metadata_tag_r<SerializerT>, tag<serialization_metadata_tag>> { };

    //ᅟ
    // Retrieves the metadata tag to be used for metadata-based serializers.
    // Defaults to `serialization_metadata_tag` if the user did not override the tag by chaining with a `metadata_tag_for_serializer<>`.
    //
template <typename SerializerT> using metadata_tag_of_serializer_t = typename metadata_tag_of_serializer<SerializerT>::type;


    //ᅟ
    // Exception class used to signal format errors during deserialization.
    //
class parse_error : public std::runtime_error
{
private:
    std::string error_;
    std::string context_;
    std::size_t column_;

public:
    MAKESHIFT_DLLFUNC parse_error(const std::string& _error);
    MAKESHIFT_DLLFUNC parse_error(const std::string& _error, const std::string& _context, std::size_t _column);

    const std::string& error(void) const noexcept { return error_; }
    const std::string& context(void) const noexcept { return context_; }
    std::size_t column(void) const noexcept { return column_; }
};


} // inline namespace serialize

} // namespace makeshift


#endif // INCLUDED_MAKESHIFT_SERIALIZE_EHPP_
