// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
