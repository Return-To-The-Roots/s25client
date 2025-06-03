// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/config.hpp>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <stdexcept>
#include <string>
#include <utility>

namespace boost::filesystem {
class path;
} // namespace boost::filesystem

#ifdef _MSC_VER
#    define RTTR_SAFEBUF __declspec(safebuffers)
#else
#    define RTTR_SAFEBUF
#endif

/// Identifies a resource file
/// This is the lowercase filename without any extension
class ResourceId
{
    // Chosen so size of the struct is 16 bytes
    static constexpr uint8_t maxLength = 15;
    uint8_t length_;
    char name_[maxLength];

    static constexpr bool isValid(const char* name, unsigned length);

    // Constexpr valid assert like macro
#ifdef NDEBUG
#    define RTTR_ResId_Assert(cond)
#else
#    define RTTR_ResId_Assert(cond) \
        if(!(cond))                 \
        throw std::logic_error("Invalid resource id")
#endif

    // Init name directly instead of first zero initializing and then overwriting it
    template<size_t Size, std::size_t... I>
    constexpr BOOST_FORCEINLINE ResourceId(const char (&name)[Size], std::index_sequence<I...>)
        : length_(Size - 1), name_{name[I]...}
    {
        static_assert(sizeof...(I) == Size - 1, "Invalid call");
        static_assert(Size - 1 <= maxLength, "Name to long");
        RTTR_ResId_Assert(isValid(name, Size - 1));
    }

public:
    template<size_t Size>
    constexpr BOOST_FORCEINLINE ResourceId(const char (&name)[Size])
        : ResourceId(name, std::make_index_sequence<Size - 1>())
    {}

    /// Create from string (unchecked)
    explicit BOOST_FORCEINLINE ResourceId(const std::string& name)
        : length_(name.length() <= maxLength ? name.length() : maxLength)
    {
        RTTR_ResId_Assert(name.length() <= maxLength && isValid(name.c_str(), name.length()));
        for(int i = 0; i < length_; i++)
            name_[i] = name[i];
    }
#undef RTTR_ResId_Assert

    constexpr bool operator==(const ResourceId& other) const noexcept;
    constexpr bool operator!=(const ResourceId& other) const noexcept { return !(*this == other); }
    constexpr bool operator<(const ResourceId& other) const noexcept;

    /// Convert a path into a resource id. Throws if the file name is invalid
    static ResourceId make(const boost::filesystem::path& filepath);
    /// Same but allows paths with unresolved folders
    static ResourceId fromPath(const std::string& filepath);
    /// Create a resource id from a string directly. Throws if the name is invalid
    static ResourceId make(const std::string& name);
    /// Can also create a copy of an existing resource id
    static constexpr ResourceId make(const ResourceId& resId) noexcept { return resId; }

    friend std::ostream& operator<<(std::ostream& os, const ResourceId& resId);
};

constexpr bool ResourceId::isValid(const char* name, unsigned length)
{
    if(length == 0)
        return false;
    // Must be a valid Id: All lowercase alpha-numeric
    const int iLen = length;
    for(int i = 0; i < iLen; i++)
    {
        const char c = name[i];
        if(!(('0' <= c && c <= '9') || ('a' <= c && c <= 'z') || c == '_'))
            return false;
    }
    return true;
}

constexpr bool ResourceId::operator==(const ResourceId& other) const noexcept
{
    if(length_ != other.length_)
        return false;
    for(int i = 0; i < length_; i++)
    {
        if(name_[i] != other.name_[i])
            return false;
    }
    return true;
}

constexpr bool ResourceId::operator<(const ResourceId& other) const noexcept
{
    if(length_ != other.length_)
        return length_ < other.length_;
    for(int i = 0; i < length_; i++)
    {
        if(name_[i] != other.name_[i])
            return name_[i] < other.name_[i];
    }
    return false;
}

// Note: Using the constructor is usually more efficient than UDLs, especially on MSVC, due to Size being statically
// known However UDLs could be made to compile-time check values with N3599 (available in GCC/Clang as an extension)

#undef RTTR_SAFEBUF
