// Copyright (c) 2005 - 2019 Settlers Freaks (sf-team at siedler25.org)
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
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#ifndef libs_driver_include_driver_AudioDriver_h
#define libs_driver_include_driver_AudioDriver_h

#include "AudioInterface.h"

#include <array>
#include <vector>

class IAudioDriverCallback;

/// Base class for audio drivers
class AudioDriver : public IAudioDriver
{
public:
    explicit
    AudioDriver(IAudioDriverCallback* driverCallback);
    ~AudioDriver() override;

    /// prüft auf Initialisierung.
    bool IsInitialized() override { return initialized; }
    void CleanUp() override;

protected:
    /// Maximum number of channels
    static constexpr unsigned MAX_NUM_CHANNELS = 64;
    /// Sets the actual number of channels used. Must be called before using channels and numChannels <= MAX_NUM_CHANNELS
    void SetNumChannels(unsigned numChannels);
    /// Adds an effect to a channel
    EffectPlayId AddPlayedEffect(int channel);
    /// Get the channel an effect is being played at or -1 if not found
    int GetEffectChannel(EffectPlayId playId);
    /// Removes the effect from the channel list
    void RemoveEffect(EffectPlayId playId);
    /// Creates a handle for the given sound and adds it to the sounds list
    /// When the last reference to the handle is lost, DoUnloadSound will be called unless the IsValid flag was set to false
    SoundHandle CreateSoundHandle(SoundDesc* sound);
    /// Called for a still loaded sound (IsValid() == true) and should unload the sound and set IsValid to false)
    virtual void DoUnloadSound(SoundDesc& sound) = 0;

    IAudioDriverCallback* driverCallback;
    bool initialized; /// Is initialized?

private:
    /// Callback for unloading a sound
    static void UnloadSound(AudioDriver& driver, SoundDesc* sound);
    /// Generates a play id. -1 for invalid
    int GeneratePlayID();
    /// Next play id
    EffectPlayId nextPlayID_;
    std::vector<SoundDesc*> sounds_;
    unsigned numChannels_;
    /// Which effect is played on which channel
    std::array<EffectPlayId, MAX_NUM_CHANNELS> channels_; //-V730_NOINIT
};

#endif // !libs_driver_include_driver_AudioDriver_h
