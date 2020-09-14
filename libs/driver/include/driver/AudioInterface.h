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

#include "EffectPlayId.h"
#include "RawSoundHandle.h"
#include "exportImport.h"
#include <cstdint>
#include <string>
#include <vector>

namespace driver {

/// Interface for audio drivers (required for use across DLL boundaries)
class BOOST_SYMBOL_VISIBLE IAudioDriver
{
public:
    virtual ~IAudioDriver() = 0;

    /// Return the name of the driver
    virtual const char* GetName() const = 0;
    /// True iff the driver is initialized
    virtual bool IsInitialized() = 0;
    virtual bool Initialize() = 0;
    /// Closes all open handles of the driver (stops and unloads music and effects)
    virtual void CleanUp() = 0;

    virtual RawSoundHandle LoadEffect(const std::string& filepath) = 0;
    virtual RawSoundHandle LoadEffect(const std::vector<char>& data, const std::string& ext) = 0;
    virtual RawSoundHandle LoadMusic(const std::string& filepath) = 0;
    virtual RawSoundHandle LoadMusic(const std::vector<char>& data, const std::string& ext) = 0;

    /// Unload the sound
    virtual void unloadSound(RawSoundHandle handle) = 0;
    /// Register this handle pointer so that the driverData is reset when the sound gets unloaded
    virtual void registerForUnload(RawSoundHandle* handlePtr) = 0;

    /// Plays an effect at the given volume. If loop is true, effect is looped indefinitely
    virtual EffectPlayId PlayEffect(const RawSoundHandle& sound, uint8_t volume, bool loop) = 0;
    /// Plays the given music. Only 1 music will be played.
    /// If repeats is less than 0 it will loop indefinitely, otherwise it will be repeated that many times.
    virtual void PlayMusic(const RawSoundHandle& sound, int repeats) = 0;
    /// Stop the music
    virtual void StopMusic() = 0;
    /// Stop the effect with the given id (if it is still playing)
    virtual void StopEffect(EffectPlayId play_id) = 0;
    /// Is the effect still being played
    virtual bool IsEffectPlaying(EffectPlayId play_id) = 0;
    /// Changes volume [0..256) of a played sound (if it is still playing) relative to the master effect volume
    virtual void ChangeVolume(EffectPlayId play_id, uint8_t volume) = 0;
    /// Set the master effect volume [0..256) at which all effects will be played. Changing the volume of an effect will be relative to this
    virtual void SetMasterEffectVolume(uint8_t volume) = 0;
    /// Sets the music volume [0..256)
    virtual void SetMusicVolume(uint8_t volume) = 0;
};
} // namespace driver

class IAudioDriverCallback;

/// Instanzierungsfunktion der Treiber.
RTTR_DECL driver::IAudioDriver* CreateAudioInstance(IAudioDriverCallback* callback, void* device_dependent);
RTTR_DECL void FreeAudioInstance(driver::IAudioDriver* driver);
