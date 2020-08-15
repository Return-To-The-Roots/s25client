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

#include "driver/RawSoundHandle.h"
#include <memory>
#include <stdexcept>
#include <utility>

/// Reference counted handle to a sound. Sound will be unloaded when the last SoundHandle of it goes out of scope.
/// Additionally it may be reset by the driver when it unloads the sound
class SoundHandle
{
public:
    SoundHandle() = default;
    /// Initialize with a loaded sound.
    /// Will call unloadHandle(ptrHandle) when the last reference goes out of scope and registers the new pointer with
    /// registerForUnload(ptrHandle) which is supposed to make sure the driverData is reset when the driver chooses to unload the sound
    template<typename T, typename U>
    SoundHandle(const driver::RawSoundHandle& rawHandle, T&& unloadHandle, U&& registerForUnload)
        : rawHandle_(new driver::RawSoundHandle(rawHandle), std::forward<T>(unloadHandle))
    {
        registerForUnload(rawHandle_.get());
    }

    /// Return true if the handle is valid
    explicit operator bool() const { return rawHandle_ && rawHandle_->getDriverData(); }
    /// Get underlying raw handle. Requires it to be valid
    const driver::RawSoundHandle& getRawHandle() const
    {
        if(!*this)
            throw std::runtime_error("Invalid handle");
        return *rawHandle_;
    }
    /// Get type of the sound. Requires it to be valid
    driver::SoundType getType() const { return getRawHandle().getType(); }

private:
    std::shared_ptr<driver::RawSoundHandle> rawHandle_;
};

#endif // !SOUNDHANDLE_H_INCLUDED
