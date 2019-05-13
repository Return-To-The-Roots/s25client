// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef SOUNDHANDLE_H_INCLUDED
#define SOUNDHANDLE_H_INCLUDED

#include <boost/noncopyable.hpp>
#include <memory>
#include <utility>

enum SoundType
{
    SD_UNKNOWN = 0,
    SD_MUSIC,
    SD_EFFECT
};

/// Base class for a sound descriptor managed by the driver
struct SoundDesc : boost::noncopyable
{
    SoundDesc() : type_(SD_UNKNOWN), isValid_(false) {}
    virtual ~SoundDesc() = default;
    const SoundType type_;
    bool isValid() const { return isValid_; }

protected:
    explicit SoundDesc(SoundType type) : type_(type), isValid_(true) {}
    bool isValid_;
};

/// Handle to a sound. Safe to copy around
class SoundHandle
{
public:
    using Descriptor = std::shared_ptr<SoundDesc>;

    explicit SoundHandle(Descriptor descriptor = Descriptor()) : descriptor_(std::move(descriptor)) {}
    SoundType getType() const { return isValid() ? descriptor_->type_ : SD_UNKNOWN; }
    bool isMusic() const { return getType() == SD_MUSIC; }
    bool isEffect() const { return getType() == SD_EFFECT; }
    /// Return true if the sound is still valid/loaded
    bool isValid() const { return (!descriptor_) ? false : descriptor_->isValid(); }
    /// Function for the driver to query the descriptor
    const Descriptor& getDescriptor() const { return descriptor_; }

private:
    Descriptor descriptor_;
};

#endif // !SOUNDHANDLE_H_INCLUDED
