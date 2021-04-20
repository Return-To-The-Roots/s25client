// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

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
    /// registerForUnload(ptrHandle) which is supposed to make sure the driverData is reset when the driver chooses to
    /// unload the sound
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
