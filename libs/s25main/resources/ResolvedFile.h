// Copyright (c) 2005 - 2018 Settlers Freaks (sf-team at siedler25.org)
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

#include <boost/filesystem/path.hpp>
#include <vector>

/// Resolved archive path. I.e. the individual files or folders in the order they are to be loaded
class ResolvedFile : private std::vector<boost::filesystem::path>
{
public:
    using Parent = std::vector<boost::filesystem::path>;

    ResolvedFile() = default;
    ResolvedFile(std::initializer_list<boost::filesystem::path> values) : Parent(values) {}

    explicit operator bool() const { return !empty(); }

    using Parent::begin;
    using Parent::end;
    using Parent::push_back;
    friend bool operator==(const ResolvedFile& lhs, const ResolvedFile& rhs)
    {
        return static_cast<const Parent&>(lhs) == static_cast<const Parent&>(rhs);
    }
    friend bool operator!=(const ResolvedFile& lhs, const ResolvedFile& rhs) { return !(lhs == rhs); }
};
