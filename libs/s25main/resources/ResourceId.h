// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include "RTTR_Assert.h"
#include <boost/filesystem/path.hpp>
#include <iosfwd>
#include <string>

/// Identifies a resource file
/// This is the lowercase filename without any extension
/// For simplicity this is implicitely convertible to and from string. TODO: Make explicit
class ResourceId
{
    std::string name_;

    static bool isValid(const std::string& name);
    struct UncheckedTag
    {};
    ResourceId(const std::string& name, UncheckedTag) noexcept : name_(name) {}

public:
    template<size_t Size>
    ResourceId(const char (&name)[Size]) noexcept : name_(name, Size - 1)
    {
        RTTR_Assert(isValid(name_));
    }
    ResourceId(const char* name) noexcept : name_(name) { RTTR_Assert(isValid(name_)); }
    ResourceId(const std::string& name) noexcept : name_(name) { RTTR_Assert(isValid(name_)); }

    bool operator==(const ResourceId& other) const noexcept { return name_ == other.name_; }
    bool operator!=(const ResourceId& other) const noexcept { return name_ != other.name_; }
    bool operator<(const ResourceId& other) const noexcept { return name_ < other.name_; }

    /// Create a resource id from a path. Throws if the file name is invalid
    static ResourceId make(const boost::filesystem::path& filepath);
    /// Can also create a copy of an existing resource id
    static ResourceId make(const ResourceId& resId) { return resId; }

    friend std::ostream& operator<<(std::ostream& os, const ResourceId& resId);
};

inline ResourceId operator"" _res(const char* str, std::size_t) noexcept
{
    return ResourceId(str);
}

