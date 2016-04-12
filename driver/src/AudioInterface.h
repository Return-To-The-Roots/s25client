// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#include "AudioType.h"

class Sound;

/// Interface for audio drivers (required for use across DLL boundaries)
class IAudioDriver
{
public:
    virtual ~IAudioDriver() = 0;

    /// Funktion zum Auslesen des Treibernamens.
    virtual const char* GetName() const = 0;

    /// Treiberinitialisierungsfunktion.
    virtual bool Initialize() = 0;

    /// Treiberaufräumfunktion.
    virtual void CleanUp() = 0;

    virtual Sound* LoadEffect(AudioType data_type, const unsigned char* data, unsigned long size) = 0;
    virtual Sound* LoadMusic (AudioType data_type, const unsigned char* data, unsigned long size) = 0;

    /// Spielt Sound ab
    virtual unsigned int PlayEffect(Sound* sound, const unsigned char volume, const bool loop) = 0;
    /// Spielt Midi ab
    virtual void PlayMusic(Sound* sound, const unsigned repeats) = 0;
    /// Stoppt die Musik.
    virtual void StopMusic() = 0;
    /// Stoppt einen Sound
    virtual void StopEffect(const unsigned int play_id) = 0;
    /// Wird ein Sound (noch) abgespielt?
    virtual bool IsEffectPlaying(const unsigned play_id) = 0;
    /// Verändert die Lautstärke von einem abgespielten Sound (falls er noch abgespielt wird)
    virtual void ChangeVolume(const unsigned play_id, const unsigned char volume) = 0;

    virtual void SetMasterEffectVolume(unsigned char volume) = 0;
    virtual void SetMasterMusicVolume(unsigned char volume) = 0;

    /// prüft auf Initialisierung.
    virtual bool IsInitialized() = 0;

};

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#undef DRIVERDLLAPI
#ifdef _WIN32
#   if defined _USRDLL || defined _LIB || defined BUILD_DLL
#       define DRIVERDLLAPI extern "C" __declspec(dllexport)
#   else
#       define DRIVERDLLAPI extern "C" __declspec(dllimport)
#   endif // !_USRDLL
#else
#   define DRIVERDLLAPI extern "C"
#endif // !_WIN32

class AudioDriverLoaderInterface;

/// Instanzierungsfunktion der Treiber.
DRIVERDLLAPI IAudioDriver* CreateAudioInstance(AudioDriverLoaderInterface* CallBack, void* device_dependent);
DRIVERDLLAPI void FreeAudioInstance(IAudioDriver* driver);

///
typedef IAudioDriver* (*PDRIVER_CREATEAUDIOINSTANCE)(AudioDriverLoaderInterface*, void* );
typedef void (*PDRIVER_FREEAUDIOINSTANCE)(IAudioDriver*);

#endif // !AUDIOINTERFACE_H_INCLUDED
