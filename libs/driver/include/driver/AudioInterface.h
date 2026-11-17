// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
    virtual const char* getName() const = 0;
    /// True iff the driver is initialized
    virtual bool isInitialized() = 0;
    virtual bool initialize() = 0;
    /// Closes all open handles of the driver (stops and unloads music and effects)
    virtual void cleanUp() = 0;

    virtual RawSoundHandle loadEffect(const std::string& filepath) = 0;
    virtual RawSoundHandle loadEffect(const std::vector<char>& data, const std::string& ext) = 0;
    virtual RawSoundHandle loadMusic(const std::string& filepath) = 0;
    virtual RawSoundHandle loadMusic(const std::vector<char>& data, const std::string& ext) = 0;

    /// Unload the sound
    virtual void unloadSound(RawSoundHandle handle) = 0;
    /// Register this handle pointer so that the driverData is reset when the sound gets unloaded
    virtual void registerForUnload(RawSoundHandle* handlePtr) = 0;

    /// Plays an effect at the given volume. If loop is true, effect is looped indefinitely
    virtual EffectPlayId playEffect(const RawSoundHandle& sound, uint8_t volume, bool loop) = 0;
    /// Plays the given music. Only 1 music will be played.
    /// If repeats is less than 0 it will loop indefinitely, otherwise it will be repeated that many times.
    virtual void playMusic(const RawSoundHandle& sound, int repeats) = 0;
    /// Stop the music
    virtual void stopMusic() = 0;
    /// Stop the effect with the given id (if it is still playing)
    virtual void stopEffect(EffectPlayId play_id) = 0;
    /// Is the effect still being played
    virtual bool isEffectPlaying(EffectPlayId play_id) = 0;
    /// Changes volume [0..256) of a played sound (if it is still playing) relative to the master effect volume
    virtual void changeVolume(EffectPlayId play_id, uint8_t volume) = 0;
    /// Set the master effect volume [0..256) at which all effects will be played. Changing the volume of an effect will
    /// be relative to this
    virtual void setMasterEffectVolume(uint8_t volume) = 0;
    /// Sets the music volume [0..256)
    virtual void setMusicVolume(uint8_t volume) = 0;
};
} // namespace driver

class IAudioDriverCallback;

/// Instanzierungsfunktion der Treiber.
RTTR_DECL driver::IAudioDriver* CreateAudioInstance(IAudioDriverCallback* callback, void* device_dependent);
RTTR_DECL void FreeAudioInstance(driver::IAudioDriver* driver);
