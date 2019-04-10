
#ifndef INCLUDED_MAKESHIFT_DETAIL_SERIALIZERS_HINT_REFLECT_HPP_
#define INCLUDED_MAKESHIFT_DETAIL_SERIALIZERS_HINT_REFLECT_HPP_


#include <iosfwd>
#include <sstream>

#include <makeshift/serializers/hint.hpp>
#include <makeshift/type_traits.hpp>      // for tag<>
#include <makeshift/serialize.hpp>        // for metadata_tag_of_serializer<>
#include <makeshift/reflect.hpp>          // for member_accessor()
#include <makeshift/metadata.hpp>         // for have_metadata<>, metadata_of<>, caption_metadata
#include <makeshift/tuple.hpp>            // for tuple_foreach()

#include <makeshift/detail/export.hpp> // for MAKESHIFT_PUBLIC


namespace makeshift
{

namespace detail
{

    // defined in serializer_stream.cpp
MAKESHIFT_PUBLIC void raw_string_to_stream(std::ostream& stream, std::string_view string);
MAKESHIFT_PUBLIC void name_to_stream(std::ostream& stream, std::string_view name);

template <typename MemberT, typename SerializerT>
    void compound_member_type_hint_to_stream(SerializerT&, std::ostream& stream, const compound_hint_options& options)
{
    using MetadataTag = metadata_tag_of_serializer_t<std::decay_t<SerializerT>>;
    if constexpr (!have_metadata_v<MemberT, MetadataTag>)
        name_to_stream(stream, options.unnamed_member_placeholder);
    else
    {
        auto slug = get_or_default<std::string_view>(metadata_of<MemberT, MetadataTag>.attributes);
        if (!slug.empty())
            name_to_stream(stream, slug);
        else
        {
            auto caption = get_or_default<caption_metadata>(metadata_of<MemberT, MetadataTag>.attributes);
            if (!caption.value.empty())
                name_to_stream(stream, caption.value);
            else
                name_to_stream(stream, options.unnamed_member_placeholder);
        }
    }
}

template <typename T, typename SerializerT>
    std::string get_compound_hint(SerializerT& serializer, const any_compound_hint_options& compoundOptions)
{
    using MetadataTag = metadata_tag_of_serializer_t<std::decay_t<SerializerT>>;
    constexpr auto members = members_of<T, MetadataTag>();
    constexpr bool isCompoundValue = has_flag(type_flags_of_v<T, MetadataTag>, type_flag::value);
    const auto& options = isCompoundValue ? compoundOptions.compound_value : compoundOptions.compound;

    std::ostringstream sstr;
    raw_string_to_stream(sstr, options.opening_delimiter);
    bool first = true;
    tuple_foreach(members, [&](auto&& member)
    {
        using Accessor = decltype(member_accessor(member));
        using Member = typename Accessor::value_type;

        if (first)
            first = false;
        else
            raw_string_to_stream(sstr, options.element_delimiter);

        std::string_view theName = get_or_default<std::string_view>(member.attributes);
        if (!theName.empty())
            name_to_stream(sstr, theName);
        else
            compound_member_type_hint_to_stream<Member>(serializer, sstr, options);
    });
    raw_string_to_stream(sstr, options.closing_delimiter);
    return sstr.str();
}


} // namespace detail

} // namespace makeshift


#endif // INCLUDED_MAKESHIFT_DETAIL_SERIALIZERS_HINT_REFLECT_HPP_
