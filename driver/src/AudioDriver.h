// $Id: AudioDriver.h 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef AUDIODRIVER_H_INCLUDED
#define AUDIODRIVER_H_INCLUDED

#include "Interface.h"
#include "Sound.h"
#include <vector>


#include "../../src/AudioDriverLoaderInterface.h"

/// Basisklasse für einen Audiotreiber.
class AudioDriver
{
    public:
        /// Konstruktor von @p AudioDriver.
        AudioDriver(AudioDriverLoaderInterface* adli);

        /// Destruktor von @p AudioDriver.
        virtual ~AudioDriver(void);

        /// Funktion zum Auslesen des Treibernamens.
        virtual const char* GetName(void) const;

        /// Treiberinitialisierungsfunktion.
        virtual bool Initialize(void) = 0;

        /// Treiberaufräumfunktion.
        virtual void CleanUp(void) = 0;

        virtual Sound* LoadEffect(unsigned int data_type, unsigned char* data, unsigned long size) = 0;
        virtual Sound* LoadMusic(unsigned int data_type, unsigned char* data, unsigned long size) = 0;

        /// Spielt Sound ab
        virtual unsigned int PlayEffect(Sound* sound, const unsigned char volume, const bool loop) = 0;
        /// Spielt Midi ab
        virtual void PlayMusic(Sound* sound, const unsigned repeats) = 0;
        /// Stoppt die Musik.
        virtual void StopMusic(void) = 0;
        /// Stoppt einen Sound
        virtual void StopEffect(const unsigned int play_id) = 0;
        /// Wird ein Sound (noch) abgespielt?
        virtual bool IsEffectPlaying(const unsigned play_id) = 0;
        /// Verändert die Lautstärke von einem abgespielten Sound (falls er noch abgespielt wird)
        virtual void ChangeVolume(const unsigned play_id, const unsigned char volume) = 0;

        virtual void SetMasterEffectVolume(unsigned char volume) = 0;
        virtual void SetMasterMusicVolume(unsigned char volume) = 0;

        /// prüft auf Initialisierung.
        bool IsInitialized() { return initialized; }

    protected:

        /// "Generiert" eine Play-ID
        unsigned GeneratePlayID();

    private:

        /// Counter für Play-IDs
        unsigned play_id_counter;


    public:
        enum DataType
        {
            AD_UNKNOWN = 0,
            AD_WAVE,
            AD_MIDI,
            AD_OTHER
            // AD_MP3, usw
        };


    protected:

        AudioDriverLoaderInterface* adli;

        ///< Das DriverCallback für Rückmeldungen.

        bool initialized; ///< Initialisierungsstatus.

        std::vector<Sound*> sounds;

        /// Anzahl Channels, die reserviert werden können (für Effekte!)
        static const unsigned CHANNEL_COUNT = 64;


};

#endif // !AUDIODRIVER_H_INCLUDED
