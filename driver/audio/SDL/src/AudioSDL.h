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
#ifndef SDL_H_INCLUDED
#define SDL_H_INCLUDED

#include <AudioDriver.h>
class AudioDriverLoaderInterface;
class Sound;

/// Klasse für den SDL Audiotreiber.
class AudioSDL : public AudioDriver
{
    private:

        /// Welche Sounds werden in den Channels gerade gespielt?
        unsigned channels[CHANNEL_COUNT];
        /// Lautstärke der Effekte.
        unsigned char master_effects_volume;
        /// Lautstärke der Musik.
        unsigned char master_music_volume;

    public:
        AudioSDL(AudioDriverLoaderInterface* adli);

        ~AudioSDL() override;

        /// Funktion zum Auslesen des Treibernamens.
        const char* GetName() const override;

        /// Treiberinitialisierungsfunktion.
        bool Initialize() override;

        /// Treiberaufräumfunktion.
        void CleanUp() override;

        Sound* LoadEffect(AudioType data_type, const unsigned char* data, unsigned long size) override;
        Sound* LoadMusic(AudioType data_type, const unsigned char* data, unsigned long size) override;

        /// Spielt Sound ab
        unsigned int PlayEffect(Sound* sound, const unsigned char volume, const bool loop) override;
        /// Spielt Midi ab
        void PlayMusic(Sound* sound, const unsigned repeats) override;
        /// Stoppt die Musik.
        void StopMusic() override;
        /// Wird der Sound (noch) abgespielt?
        bool IsEffectPlaying(const unsigned play_id) override;
        /// Stoppt einen Sound
        void StopEffect(const unsigned play_id) override;
        /// Verändert die Lautstärke von einem abgespielten Sound (falls er noch abgespielt wird)
        void ChangeVolume(const unsigned play_id, const unsigned char volume) override;

        void SetMasterEffectVolume(unsigned char volume) override;
        void SetMasterMusicVolume(unsigned char volume) override;

    private:

        /// Callback für Audiotreiber
        static void MusicFinished();
};

#endif // !SDL_H_INCLUDED
