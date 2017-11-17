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
#ifndef AUDIOINTERFACE_H_INCLUDED
#define AUDIOINTERFACE_H_INCLUDED

#include "EffectPlayId.h"
#include "SoundHandle.h"
#include <stdint.h>
#include <string>

/// Interface for audio drivers (required for use across DLL boundaries)
class IAudioDriver
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

    virtual SoundHandle LoadEffect(const std::string& filepath) = 0;
    virtual SoundHandle LoadMusic(const std::string& filepath) = 0;

    /// Plays an effect at the given volume. If loop is true, effect is looped indefinitely
    virtual EffectPlayId PlayEffect(const SoundHandle& sound, uint8_t volume, bool loop) = 0;
    /// Plays the given music. Only 1 music will be played. If Repeats is 0 it will loop indefinitely,
    /// otherwise it loops the many times. TODO: What about not looping it (e.g. playing only once?)
    virtual void PlayMusic(const SoundHandle& sound, unsigned repeats) = 0;
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

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#undef DRIVERDLLAPI
#ifdef _WIN32
#ifdef BUILD_DLL
#define DRIVERDLLAPI extern "C" __declspec(dllexport)
#else
#define DRIVERDLLAPI extern "C" __declspec(dllimport)
#endif // !_USRDLL
#else
#define DRIVERDLLAPI extern "C"
#endif // !_WIN32

class IAudioDriverCallback;

/// Instanzierungsfunktion der Treiber.
DRIVERDLLAPI IAudioDriver* CreateAudioInstance(IAudioDriverCallback* CallBack, void* device_dependent);
DRIVERDLLAPI void FreeAudioInstance(IAudioDriver* driver);

///
typedef IAudioDriver* (*PDRIVER_CREATEAUDIOINSTANCE)(IAudioDriverCallback*, void*);
typedef void (*PDRIVER_FREEAUDIOINSTANCE)(IAudioDriver*);

#endif // !AUDIOINTERFACE_H_INCLUDED
