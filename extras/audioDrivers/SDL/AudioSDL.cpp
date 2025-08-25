// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AudioSDL.h"
#include "RTTR_Assert.h"
#include "driver/AudioInterface.h"
#include "driver/IAudioDriverCallback.h"
#include "driver/Interface.h"
#include "helpers/CIUtils.h"
#include "helpers/LSANUtils.h"
#include <SDL.h>
#include <SDL_mixer.h>
#include <iostream>
#include <memory>

/**
 *  Instanzierungsfunktion von @p AudioSDL.
 *
 *  @return liefert eine Instanz des jeweiligen Treibers
 */
driver::IAudioDriver* CreateAudioInstance(IAudioDriverCallback* callback, void* /*device_dependent*/)
{
    return new AudioSDL(callback);
}

void FreeAudioInstance(driver::IAudioDriver* driver)
{
    delete driver;
}

const char* GetAudioDriverName()
{
    return "(SDL2) Audio via SDL2_mixer-Library";
}

struct SDLMusicData
{
    Mix_Music* music;             /// Music handle
    const std::vector<char> data; /// Music data if loaded from a stream
    explicit SDLMusicData(Mix_Music* music) : music(music) {}
    explicit SDLMusicData(std::vector<char> data) : music(nullptr), data(std::move(data)) {}
};

/** @class AudioSDL
 *
 *  Klasse für den SDL-Audiotreiber.
 */

AudioSDL::AudioSDL(IAudioDriverCallback* adli) : AudioDriver(adli), master_effects_volume(255), master_music_volume(255)
{}

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
    return GetAudioDriverName();
}

static AudioSDL* currentInstance = nullptr;

bool AudioSDL::Initialize()
{
    initialized = false;
    // SDL_thread leaks a mutex on regular runs and SDL_mixer a lot on initialization error
    rttr::ScopedLeakDisabler _;
    if(SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
    {
        std::cerr << SDL_GetError() << std::endl;
        return false;
    }

    // open 44.1KHz, signed 16bit, system byte order,
    // stereo audio, using 1024 byte chunks
    if(Mix_OpenAudio(44100, AUDIO_S16LSB, 2, 4096) < 0)
    {
        if(rttr::isRunningOnCI())
        {
            initialized = true;
            return true;
        }
        std::cerr << Mix_GetError() << std::endl;
        return false;
    }
    SetNumChannels(Mix_AllocateChannels(DEFAULT_NUM_CHANNELS));
    Mix_SetMusicCMD(nullptr);
    currentInstance = this;
    Mix_HookMusicFinished(AudioSDL::MusicFinished);

    initialized = true;

    return true;
}

void AudioSDL::MusicFinished()
{
    if(currentInstance)
        currentInstance->driverCallback->Msg_MusicFinished();
}

/**
 *  Treiberaufräumfunktion.
 */
void AudioSDL::CleanUp()
{
    // Unload sounds
    AudioDriver::CleanUp();

    Mix_CloseAudio();
    currentInstance = nullptr;
    Mix_HookMusicFinished(nullptr);
    Mix_Quit();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

/**
 *  Läd einen Effekt.
 *
 *  @param[in] type Typ der Daten
 *  @param[in] data Datenblock
 *  @param[in] size Größe des Datenblocks
 *
 *  @return Sounddeskriptor bei Erfolg, @p nullptr bei Fehler
 */
driver::RawSoundHandle AudioSDL::LoadEffect(const std::string& filepath)
{
    Mix_Chunk* sound = Mix_LoadWAV(filepath.c_str());

    if(sound == nullptr)
        std::cerr << Mix_GetError() << std::endl;

    return createRawSoundHandle(sound, driver::SoundType::Effect);
}

driver::RawSoundHandle AudioSDL::LoadEffect(const std::vector<char>& data, const std::string& /*ext*/)
{
    SDL_RWops* rwOps = SDL_RWFromConstMem(data.data(), static_cast<int>(data.size()));
    Mix_Chunk* sound = Mix_LoadWAV_RW(rwOps, true); //-V601

    if(sound == nullptr)
        std::cerr << Mix_GetError() << std::endl;

    return createRawSoundHandle(sound, driver::SoundType::Effect);
}

/**
 *  Läd ein Musikstück.

 *  @return Sounddeskriptor bei Erfolg, @p nullptr bei Fehler
 */
driver::RawSoundHandle AudioSDL::LoadMusic(const std::string& filepath)
{
    Mix_Music* music = Mix_LoadMUS(filepath.c_str());

    if(music == nullptr)
    {
        std::cerr << Mix_GetError() << std::endl;
        return createRawSoundHandle(nullptr, driver::SoundType::Music);
    }

    return createRawSoundHandle(new SDLMusicData(music), driver::SoundType::Music);
}

driver::RawSoundHandle AudioSDL::LoadMusic(const std::vector<char>& data, const std::string& /*ext*/)
{
    // Need to copy data as it is used by SDL
    auto handle = std::make_unique<SDLMusicData>(data);
    SDL_RWops* rwOps = SDL_RWFromConstMem(handle->data.data(), static_cast<int>(data.size()));
    Mix_Music* music = Mix_LoadMUS_RW(rwOps, true);
    if(music == nullptr)
    {
        SDL_FreeRW(rwOps);
        std::cerr << Mix_GetError() << std::endl;
        return createRawSoundHandle(nullptr, driver::SoundType::Music);
    }
    handle->music = music;

    return createRawSoundHandle(handle.release(), driver::SoundType::Music);
}

int AudioSDL::doPlayEffect(driver::RawSoundHandle::DriverData driverData, uint8_t volume, bool loop)
{
    int channel = Mix_PlayChannel(-1, static_cast<Mix_Chunk*>(driverData), (loop) ? -1 : 0);
    if(channel < 0)
        return -1;
    Mix_Volume(channel, CalcEffectVolume(volume));
    return channel;
}

void AudioSDL::PlayMusic(const driver::RawSoundHandle& sound, int repeats)
{
    if(!sound.getDriverData())
        return;
    RTTR_Assert(sound.getType() == driver::SoundType::Music);
    if(sound.getType() != driver::SoundType::Music)
        return;

    int channel =
      Mix_PlayMusic(static_cast<SDLMusicData*>(sound.getDriverData())->music, repeats < 0 ? -1 : repeats + 1);
    if(channel < 0)
        return;
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

void AudioSDL::doStopEffect(int channel)
{
    Mix_HaltChannel(channel);
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

void AudioSDL::doUnloadSound(driver::RawSoundHandle sound)
{
    switch(sound.getType())
    {
        case driver::SoundType::Effect:
        {
            auto* effect = static_cast<Mix_Chunk*>(sound.getDriverData());
            Mix_FreeChunk(effect);
            break;
        }
        case driver::SoundType::Music:
        {
            auto* musicData = static_cast<SDLMusicData*>(sound.getDriverData());
            Mix_FreeMusic(musicData->music);
            delete musicData;
            break;
        }
    }
}
