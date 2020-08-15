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

#ifndef SoundItem_h__
#define SoundItem_h__

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

#endif // SoundItem_h__
