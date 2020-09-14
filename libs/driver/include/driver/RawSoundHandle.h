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

namespace driver {
enum class SoundType
{
    Music,
    Effect
};

/// Handle to a loaded sound.
/// Can be passed over ABI boundaries
struct RawSoundHandle
{
    /// Opaque handle type to be used by the driver
    using DriverData = void*;

private:
    DriverData driverData;
    SoundType type;

public:
    DriverData getDriverData() const { return driverData; }
    SoundType getType() const { return type; }

    // Comparison only compares the driver data
    bool operator==(const RawSoundHandle& rhs) const { return driverData == rhs.driverData; }
    bool operator!=(const RawSoundHandle& rhs) const { return driverData == rhs.driverData; }

private:
    // Only AudioDriver may create this to enforce registration and only it may reset the driverData
    friend class AudioDriver;
    RawSoundHandle(DriverData driverData, SoundType type) : driverData(driverData), type(type) {}
    void invalidate() { driverData = nullptr; }
};
} // namespace driver
