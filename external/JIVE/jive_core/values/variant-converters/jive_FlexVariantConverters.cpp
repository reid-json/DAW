#include "jive_FlexVariantConverters.h"

namespace juce
{
    // Helper: normalize American spelling to the canonical spelling used by this JIVE build.
    static var normalizeCentreAliases(const var& v)
    {
        if (v.isString() && v.toString() == "center")
            return var("centre");

        return v;
    }

    const Array<var> VariantConverter<FlexBox::AlignContent>::options = {
        "stretch",
        "flex-start",
        "flex-end",
        "centre",
        "space-between",
        "space-around"
    };

    FlexBox::AlignContent VariantConverter<FlexBox::AlignContent>::fromVar(const var& v)
    {
        if (v.isVoid())
            return FlexBox{}.alignContent;

        const auto normalized = normalizeCentreAliases(v);

        jassert(options.contains(normalized));
        return static_cast<FlexBox::AlignContent>(options.indexOf(normalized));
    }

    var VariantConverter<FlexBox::AlignContent>::toVar(FlexBox::AlignContent justification)
    {
        const auto index = static_cast<int>(justification);

        jassert(options.size() >= index);
        return options[index];
    }

    const Array<var> VariantConverter<FlexBox::AlignItems>::options = {
        "stretch",
        "flex-start",
        "flex-end",
        "centre"
    };

    FlexBox::AlignItems VariantConverter<FlexBox::AlignItems>::fromVar(const var& v)
    {
        if (v.isVoid())
            return FlexBox{}.alignItems;

        const auto normalized = normalizeCentreAliases(v);

        jassert(options.contains(normalized));
        return static_cast<FlexBox::AlignItems>(options.indexOf(normalized));
    }

    var VariantConverter<FlexBox::AlignItems>::toVar(FlexBox::AlignItems alignment)
    {
        const auto index = static_cast<int>(alignment);

        jassert(options.size() >= index);
        return options[index];
    }

    const Array<var> VariantConverter<FlexBox::Direction>::options = {
        "row",
        "row-reverse",
        "column",
        "column-reverse"
    };

    FlexBox::Direction VariantConverter<FlexBox::Direction>::fromVar(const var& v)
    {
        if (v.isVoid())
            return FlexBox{}.flexDirection;

        jassert(options.contains(v));
        return static_cast<FlexBox::Direction>(options.indexOf(v));
    }

    var VariantConverter<FlexBox::Direction>::toVar(FlexBox::Direction direction)
    {
        const auto index = static_cast<int>(direction);

        jassert(options.size() >= index);
        return options[index];
    }

    const Array<var> VariantConverter<FlexBox::JustifyContent>::options = {
        "flex-start",
        "flex-end",
        "centre",
        "space-between",
        "space-around"
    };

    FlexBox::JustifyContent VariantConverter<FlexBox::JustifyContent>::fromVar(const var& v)
    {
        if (v.isVoid())
            return FlexBox{}.justifyContent;

        const auto normalized = normalizeCentreAliases(v);

        jassert(options.contains(normalized));
        return static_cast<FlexBox::JustifyContent>(options.indexOf(normalized));
    }

    var VariantConverter<FlexBox::JustifyContent>::toVar(FlexBox::JustifyContent justification)
    {
        const auto index = static_cast<int>(justification);

        jassert(options.size() >= index);
        return options[index];
    }

    const Array<var> VariantConverter<FlexBox::Wrap>::options = {
        "nowrap",
        "wrap",
        "wrap-reverse"
    };

    FlexBox::Wrap VariantConverter<FlexBox::Wrap>::fromVar(const var& v)
    {
        if (v.isVoid())
            return FlexBox{}.flexWrap;

        jassert(options.contains(v));
        return static_cast<FlexBox::Wrap>(options.indexOf(v));
    }

    var VariantConverter<FlexBox::Wrap>::toVar(FlexBox::Wrap wrap)
    {
        const auto index = static_cast<int>(wrap);

        jassert(options.size() >= index);
        return options[index];
    }

    const Array<var> VariantConverter<FlexItem::AlignSelf>::options = {
        "auto",
        "flex-start",
        "flex-end",
        "centre",
        "stretch"
    };

    FlexItem::AlignSelf VariantConverter<FlexItem::AlignSelf>::fromVar(const var& v)
    {
        if (v.isVoid())
            return FlexItem{}.alignSelf;

        const auto normalized = normalizeCentreAliases(v);

        jassert(options.contains(normalized));
        return static_cast<FlexItem::AlignSelf>(options.indexOf(normalized));
    }

    var VariantConverter<FlexItem::AlignSelf>::toVar(FlexItem::AlignSelf alignSelf)
    {
        const auto index = static_cast<int>(alignSelf);

        jassert(options.size() >= index);
        return options[index];
    }
} // namespace juce