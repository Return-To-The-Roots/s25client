// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/filesystem/path.hpp>
#include <vector>

/// Resolved archive path. I.e. the individual files or folders in the order they are to be loaded
class ResolvedFile : private std::vector<boost::filesystem::path>
{
public:
    using Parent = std::vector<boost::filesystem::path>;
    using Parent::const_iterator;
    using Parent::value_type;

    ResolvedFile() = default;
    ResolvedFile(std::initializer_list<boost::filesystem::path> values) : Parent(values) {}

    explicit operator bool() const { return !empty(); }

    using Parent::begin;
    using Parent::end;
    using Parent::push_back;
    using Parent::size;
    friend bool operator==(const ResolvedFile& lhs, const ResolvedFile& rhs)
    {
        return static_cast<const Parent&>(lhs) == static_cast<const Parent&>(rhs);
    }
    friend bool operator!=(const ResolvedFile& lhs, const ResolvedFile& rhs) { return !(lhs == rhs); }
};
