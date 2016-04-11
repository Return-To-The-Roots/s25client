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
#ifndef AUDIODRIVERWRAPPER_H_INCLUDED
#define AUDIODRIVERWRAPPER_H_INCLUDED


#include "Singleton.h"
#include "DriverWrapper.h"
#include "driver/src/AudioDriverLoaderInterface.h"
#include "driver/src/AudioType.h"

class IAudioDriver;
class Sound;

#define MAX_DRIVER_COUNT 20

///////////////////////////////////////////////////////////////////////////////
// DriverWrapper
class AudioDriverWrapper : public Singleton<AudioDriverWrapper, SingletonPolicies::WithLongevity>, public AudioDriverLoaderInterface
{
    public:
        BOOST_STATIC_CONSTEXPR unsigned Longevity = 30;

        AudioDriverWrapper();

        ~AudioDriverWrapper() override;

        /// Läd den Treiber
        bool LoadDriver();

        /// Lädt einen Sound.
        Sound* LoadEffect(AudioType data_type, const unsigned char* data, unsigned int size);
        Sound* LoadMusic(AudioType data_type, const unsigned char* data, unsigned int size);

        /// Spielt einen Sound
        unsigned PlayEffect(Sound* sound, const unsigned char volume, const bool loop);
        /// Stoppt einen Sound
        void StopEffect(const unsigned int play_id);

        /// Spielt Midi ab
        void PlayMusic(Sound* sound, const unsigned repeats);

        /// Stoppt die Musik.
        void StopMusic();

        /// Wird ein Sound (noch) abgespielt?
        bool IsEffectPlaying(const unsigned play_id);

        /// Verändert die Lautstärke von einem abgespielten Sound (falls er noch abgespielt wird)
        void ChangeVolume(const unsigned play_id, const unsigned char volume);

        void SetMasterEffectVolume(unsigned char volume);

        void SetMasterMusicVolume(unsigned char volume);

        std::string GetName() const;

    private:

        void Msg_MusicFinished() override;

    private:

        DriverWrapper driver_wrapper;
        IAudioDriver* audiodriver;
};

#define AUDIODRIVER AudioDriverWrapper::inst()

#endif
