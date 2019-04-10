
#ifndef INCLUDED_MAKESHIFT_DETAIL_SERIALIZERS_STREAM_REFLECT_HPP_
#define INCLUDED_MAKESHIFT_DETAIL_SERIALIZERS_STREAM_REFLECT_HPP_


#include <iosfwd>
#include <string_view>
#include <utility>  // for move(), forward<>()

#include <makeshift/serialize.hpp> // for metadata_tag_of_serializer<>, parse_error
#include <makeshift/reflect.hpp>   // for member_accessor()
#include <makeshift/tuple.hpp>     // for tuple_foreach()

#include <makeshift/detail/export.hpp>         // for MAKESHIFT_PUBLIC
#include <makeshift/detail/metadata.hpp>       // for type_flags_of<>
#include <makeshift/detail/string_compare.hpp>


namespace makeshift
{

namespace detail
{


MAKESHIFT_PUBLIC void raw_string_to_stream(std::ostream& stream, std::string_view string);
MAKESHIFT_PUBLIC void name_to_stream(std::ostream& stream, std::string_view name);
MAKESHIFT_PUBLIC void name_to_stream(std::ostream& stream, std::string_view name, std::string_view nameIndicator);

template <typename T, typename SerializerT>
    void compound_to_stream(std::ostream& stream, const T& value, SerializerT& serializer, const any_compound_serialization_options& compoundOptions)
{
    using MetadataTag = metadata_tag_of_serializer_t<std::decay_t<SerializerT>>;
    constexpr auto members = members_of<T, MetadataTag>();
    constexpr bool isCompoundValue = has_flag(type_flags_of_v<T, MetadataTag>, type_flag::value);
    const auto& options = isCompoundValue ? compoundOptions.compound_value : compoundOptions.compound;

    raw_string_to_stream(stream, options.opening_delimiter);
    bool first = true;
    tuple_foreach(members, [&](auto&& member)
    {
        if (first)
            first = false;
        else
            raw_string_to_stream(stream, options.element_delimiter);

        auto accessor = member_accessor(member);
        auto theName = get_or_default<std::string_view>(member.attributes);
        if (options.with_member_names)
        {
            name_to_stream(stream, theName, options.name_indicator);
            raw_string_to_stream(stream, options.name_value_separator);
        }
        stream << streamable(accessor(value), serializer);
    });
    raw_string_to_stream(stream, options.closing_delimiter);
}
class stream_compound_member_deserializer_base
{
public:
    virtual bool by_index(std::istream& stream, std::size_t index) = 0;
    virtual bool by_name(std::istream& stream, std::string_view name, std::size_t& index) = 0;
    virtual ~stream_compound_member_deserializer_base(void) { } // not strictly necessary but let us risk no analyzer warnings about this
};
template <typename T, typename SerializerT>
    class stream_compound_member_deserializer final : public stream_compound_member_deserializer_base
{
    using MetadataTag = metadata_tag_of_serializer_t<std::decay_t<SerializerT>>;
    static constexpr auto members_ = members_of<T, MetadataTag>();

private:
    T& value_;
    SerializerT serializer_;
    string_comparison memberNameComparison_;

public:
    stream_compound_member_deserializer(T& _value, SerializerT& _serializer, string_comparison _memberNameComparison)
        : value_(_value), serializer_(std::forward<SerializerT>(_serializer)), memberNameComparison_(_memberNameComparison)
    {
    }

    bool by_index(std::istream& stream, std::size_t index) override
    {
        std::size_t i = 0;
        return tuple_any(members_, [&](auto&& member)
        {
            if (i == index)
            {
                auto accessor = member_accessor(member);
                using Member = typename std::decay_t<decltype(accessor)>::value_type;
                Member memberValue{ accessor(value_) }; // initialize with original value as the type may not be default-constructible
                stream >> streamable(memberValue, serializer_);
                accessor(value_, std::move(memberValue));
                return true;
            }
            ++i;
            return false;
        });
    }
    bool by_name(std::istream& stream, std::string_view name, std::size_t& index) override
    {
        auto comparer = string_comparer(string_comparer_options{ memberNameComparison_ });
        std::size_t i = 0;
        return tuple_any(members_, [&](auto&& member)
        {
            auto memberName = get_or_default<std::string_view>(member.attributes);
            if (!memberName.empty() && equal_to(comparer)(memberName, name))
            {
                auto accessor = member_accessor(member);
                using Member = typename std::decay_t<decltype(accessor)>::value_type;
                Member memberValue{ accessor(value_) }; // initialize with original value as the type may not be default-constructible
                stream >> streamable(memberValue, serializer_);
                accessor(value_, std::move(memberValue));
                index = i;
                return true;
            }
            ++i;
            return false;
        });
    }
};
template <typename T, typename SerializerT>
    stream_compound_member_deserializer(T&, SerializerT&, string_comparison) -> stream_compound_member_deserializer<T, SerializerT>;

MAKESHIFT_PUBLIC void compound_from_stream(std::istream& stream, stream_compound_member_deserializer_base& memberDeserializer, const compound_serialization_options& options);

template <typename T, typename SerializerT>
    void compound_from_stream(std::istream& stream, T& value, SerializerT& serializer, const any_compound_serialization_options& compoundOptions)
{
    using MetadataTag = metadata_tag_of_serializer_t<std::decay_t<SerializerT>>;

    constexpr bool isCompoundValue = has_flag(type_flags_of_v<T, MetadataTag>, type_flag::value);
    const auto& options = isCompoundValue ? compoundOptions.compound_value : compoundOptions.compound;
    stream_compound_member_deserializer memberDeserializer{ value, serializer, options.member_name_comparison_mode };
    compound_from_stream(stream, memberDeserializer, options);
}


} // namespace detail

} // namespace makeshift


#endif // INCLUDED_MAKESHIFT_DETAIL_SERIALIZERS_STREAM_REFLECT_HPP_
