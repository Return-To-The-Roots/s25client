// $Id: AudioDriverWrapper.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef AUDIODRIVERWRAPPER_H_INCLUDED
#define AUDIODRIVERWRAPPER_H_INCLUDED


#include "Singleton.h"
#include "DriverWrapper.h"
#include "../driver/src/AudioInterface.h"

class AudioDriver;

#define MAX_DRIVER_COUNT 20
const char* const EMPTY_STR = "";

///////////////////////////////////////////////////////////////////////////////
// DriverWrapper
class AudioDriverWrapper : public Singleton<AudioDriverWrapper>, public AudioDriverLoaderInterface
{
    public:
        /// Konstruktor von @p DriverWrapper
        AudioDriverWrapper();

        ~AudioDriverWrapper();

        /// Läd den Treiber
        bool LoadDriver(void);

        /// Lädt einen Sound.
        Sound* LoadEffect(unsigned int data_type, unsigned char* data, unsigned int size);
        Sound* LoadMusic(unsigned int data_type, unsigned char* data, unsigned int size);

        /// Spielt einen Sound
        unsigned PlayEffect(Sound* sound, const unsigned char volume, const bool loop);
        /// Stoppt einen Sound
        void StopEffect(const unsigned int play_id);

        /// Spielt Midi ab
        void PlayMusic(Sound* sound, const unsigned repeats)
        { if(audiodriver) audiodriver->PlayMusic(sound, repeats); }
        /// Stoppt die Musik.
        void StopMusic(void)
        { if(audiodriver) audiodriver->StopMusic();  }
        /// Wird ein Sound (noch) abgespielt?
        bool IsEffectPlaying(const unsigned play_id)
        { if(audiodriver) return audiodriver->IsEffectPlaying(play_id); else return false; }
        /// Verändert die Lautstärke von einem abgespielten Sound (falls er noch abgespielt wird)
        void ChangeVolume(const unsigned play_id, const unsigned char volume)
        { if(audiodriver) audiodriver->ChangeVolume(play_id, volume); }

        void SetMasterEffectVolume(unsigned char volume)
        { if(audiodriver) audiodriver->SetMasterEffectVolume(volume); }
        void SetMasterMusicVolume(unsigned char volume)
        { if(audiodriver) audiodriver->SetMasterMusicVolume(volume); }

        const char* GetName(void) const { if(audiodriver) return audiodriver->GetName();    return EMPTY_STR; }
    private:

        void Msg_MusicFinished();

    private:

        DriverWrapper driver_wrapper;
        AudioDriver* audiodriver;
};

#endif
