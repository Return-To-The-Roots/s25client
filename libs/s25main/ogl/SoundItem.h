// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "drivers/SoundHandle.h"

/// Base class for all sound items
class SoundItem
{
public:
    virtual ~SoundItem() = default;
    // Get the type of the sound item. Must be loaded first
    driver::SoundType getLoadedType() const { return handle.getType(); }

protected:
    /// Load the sound item into the driver
    virtual SoundHandle Load() = 0;
    /// Return the handle loading it if required
    SoundHandle& GetSoundHandle()
    {
        if(!handle)
            handle = Load();
        return handle;
    }

private:
    /// Handle to the sound, managed by driver, hence safe to copy
    SoundHandle handle;
};
