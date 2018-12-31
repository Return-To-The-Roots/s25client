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

#include "commonDefines.h" // IWYU pragma: keep
#include "AudioSDL.h"
#include "SoundSDL_Effect.h"
#include "SoundSDL_Music.h"
#include "driver/AudioInterface.h"
#include "driver/IAudioDriverCallback.h"
#include <SDL.h>
#include <SDL_mixer.h>
#include <iostream>

static AudioSDL* nthis = NULL;

/**
 *  Instanzierungsfunktion von @p AudioSDL.
 *
 *  @return liefert eine Instanz des jeweiligen Treibers
 */
DRIVERDLLAPI IAudioDriver* CreateAudioInstance(IAudioDriverCallback* adli, void* /*device_dependent*/)
{
    nthis = new AudioSDL(adli);
    return nthis;
}

DRIVERDLLAPI void FreeAudioInstance(IAudioDriver* driver)
{
    delete driver;
}

DRIVERDLLAPI const char* GetDriverName()
{
    return "(SDL) Audio via SDL_mixer-Library";
}

/** @class AudioSDL
 *
 *  Klasse für den SDL-Audiotreiber.
 */

AudioSDL::AudioSDL(IAudioDriverCallback* adli) : AudioDriver(adli), master_effects_volume(255), master_music_volume(255) {}

AudioSDL::~AudioSDL()
{
    CleanUp();
}

/**
 *  Funktion zum Auslesen des Treibernamens.
 *
 *  @return liefert den Treibernamen zurück
 */
const char* AudioSDL::GetName() const
{
    return GetDriverName();
}

/**
 *  Treiberinitialisierungsfunktion.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 */
bool AudioSDL::Initialize()
{
    if(SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
    {
        std::cerr << SDL_GetError() << std::endl;
        initialized = false;
        return false;
    }

    // open 44.1KHz, signed 16bit, system byte order,
    // stereo audio, using 1024 byte chunks
    if(Mix_OpenAudio(44100, AUDIO_S16LSB, 2, 4096) < 0)
    {
        std::cerr << Mix_GetError() << std::endl;
        initialized = false;
        return false;
    }

    SetNumChannels(Mix_AllocateChannels(MAX_NUM_CHANNELS));
    Mix_SetMusicCMD(NULL);
    Mix_HookMusicFinished(AudioSDL::MusicFinished);

    initialized = true;

    return true;
}

/**
 *  Treiberaufräumfunktion.
 */
void AudioSDL::CleanUp()
{
    // Unload sounds
    AudioDriver::CleanUp();

    Mix_CloseAudio();
    Mix_HookMusicFinished(NULL);
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

/**
 *  Läd einen Effekt.
 *
 *  @param[in] type Typ der Daten
 *  @param[in] data Datenblock
 *  @param[in] size Größe des Datenblocks
 *
 *  @return Sounddeskriptor bei Erfolg, @p NULL bei Fehler
 */
SoundHandle AudioSDL::LoadEffect(const std::string& filepath)
{
    Mix_Chunk* sound = Mix_LoadWAV(filepath.c_str());

    if(sound == NULL)
    {
        std::cerr << Mix_GetError() << std::endl;
        return SoundHandle();
    }

    return CreateSoundHandle(new SoundSDL_Effect(sound));
}

SoundHandle AudioSDL::LoadEffect(const std::vector<char>& data, const std::string& /*ext*/)
{
    SDL_RWops* rwOps = SDL_RWFromConstMem(&data[0], static_cast<int>(data.size()));
    Mix_Chunk* sound = Mix_LoadWAV_RW(rwOps, true); //-V601
    if(sound == NULL)
    {
        std::cerr << Mix_GetError() << std::endl;
        return SoundHandle();
    }

    return CreateSoundHandle(new SoundSDL_Effect(sound));
}

/**
 *  Läd ein Musikstück.

 *  @return Sounddeskriptor bei Erfolg, @p NULL bei Fehler
 */
SoundHandle AudioSDL::LoadMusic(const std::string& filepath)
{
    Mix_Music* music = Mix_LoadMUS(filepath.c_str());

    if(music == NULL)
    {
        std::cerr << Mix_GetError() << std::endl;
        return SoundHandle();
    }

    return CreateSoundHandle(new SoundSDL_Music(music));
}

SoundHandle AudioSDL::LoadMusic(const std::vector<char>& data, const std::string& /*ext*/)
{
    // Need to copy data as it is used by SDL
    SoundSDL_Music* handle = new SoundSDL_Music(data);
    SDL_RWops* rwOps = SDL_RWFromConstMem(&handle->data[0], static_cast<int>(data.size()));
    Mix_Music* music = Mix_LoadMUS_RW(rwOps);
    if(music == NULL)
    {
        SDL_FreeRW(rwOps);
        delete handle;
        std::cerr << Mix_GetError() << std::endl;
        return SoundHandle();
    }
    handle->music = music;

    return CreateSoundHandle(handle);
}

EffectPlayId AudioSDL::PlayEffect(const SoundHandle& sound, uint8_t volume, bool loop)
{
    if(!sound.isValid())
        return -1;
    RTTR_Assert(sound.isEffect());
    if(!sound.isEffect())
        return -1;

    int channel = Mix_PlayChannel(-1, static_cast<SoundSDL_Effect&>(*sound.getDescriptor()).sound, (loop) ? -1 : 0);
    if(channel < 0)
        return -1;
    Mix_Volume(channel, CalcEffectVolume(volume));
    return AddPlayedEffect(channel);
}

void AudioSDL::PlayMusic(const SoundHandle& sound, unsigned repeats)
{
    if(!sound.isValid())
        return;
    RTTR_Assert(sound.isMusic());
    if(!sound.isMusic())
        return;

    int channel = Mix_PlayMusic(static_cast<SoundSDL_Music&>(*sound.getDescriptor()).music, repeats == 0 ? -1 : int(repeats));
    if(channel < 0)
        return;
    // TODO: Can we actually use multiple channels? If yes we might want to store it. Also print error message?
    Mix_VolumeMusic(master_music_volume / 2);
}

/**
 *  Stoppt die Musik.
 */
void AudioSDL::StopMusic()
{
    // Musik anhalten
    Mix_FadeOutMusic(1000);
}

void AudioSDL::StopEffect(EffectPlayId play_id)
{
    int channel = GetEffectChannel(play_id);
    if(channel >= 0)
    {
        Mix_HaltChannel(channel);
        RemoveEffect(play_id);
    }
}

bool AudioSDL::IsEffectPlaying(EffectPlayId play_id)
{
    int channel = GetEffectChannel(play_id);
    if(channel < 0)
        return false;
    if(Mix_Playing(channel))
        return true;
    RemoveEffect(play_id);
    return false;
}

void AudioSDL::ChangeVolume(EffectPlayId play_id, uint8_t volume)
{
    int channel = GetEffectChannel(play_id);
    if(channel >= 0)
        Mix_Volume(channel, CalcEffectVolume(volume));
}

void AudioSDL::SetMasterEffectVolume(uint8_t volume)
{
    master_effects_volume = volume;
}

void AudioSDL::SetMusicVolume(uint8_t volume)
{
    master_music_volume = volume;

    // volume von 0 - 127
    Mix_VolumeMusic(volume / 2);
}

uint8_t AudioSDL::CalcEffectVolume(uint8_t volume) const
{
    // Scale by master_effects_volume and scale down to SDL_Mixer range [0, 127]
    return (static_cast<unsigned>(master_effects_volume) * volume) / 255 / 2;
}

void AudioSDL::MusicFinished()
{
    nthis->driverCallback->Msg_MusicFinished();
}

void AudioSDL::DoUnloadSound(SoundDesc& sound)
{
    if(sound.type_ == SD_EFFECT)
    {
        SoundSDL_Effect& effect = static_cast<SoundSDL_Effect&>(sound);
        Mix_FreeChunk(effect.sound);
        effect.setInvalid();
    } else if(sound.type_ == SD_MUSIC)
    {
        SoundSDL_Music& music = static_cast<SoundSDL_Music&>(sound);
        Mix_FreeMusic(music.music);
        music.setInvalid();
    } else
        RTTR_Assert(false && "Invalid type");
}
