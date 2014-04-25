// $Id: SDL.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"
#include "SDL.h"

#include "SoundSDL_Effect.h"
#include "SoundSDL_Music.h"

#include <AudioInterface.h>


///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static AudioSDL* nthis = NULL;

///////////////////////////////////////////////////////////////////////////////
/**
 *  Instanzierungsfunktion von @p AudioSDL.
 *
 *  @return liefert eine Instanz des jeweiligen Treibers
 *
 *  @author FloSoft
 */
DRIVERDLLAPI AudioDriver* CreateAudioInstance(AudioDriverLoaderInterface* adli, void* device_dependent)
{
    nthis = new AudioSDL(adli);
    return nthis;
}

DRIVERDLLAPI const char* GetDriverName(void)
{
    return "(SDL) Audio via SDL_mixer-Library";
}

///////////////////////////////////////////////////////////////////////////////
/** @class AudioSDL
 *
 *  Klasse für den SDL-Audiotreiber.
 *
 *  @author FloSoft
 */

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p AudioSDL.
 *
 *  @author FloSoft
 */
AudioSDL::AudioSDL(AudioDriverLoaderInterface* adli) : AudioDriver(adli), master_effects_volume(0xFF)
{
    for(unsigned i = 0; i < CHANNEL_COUNT; ++i)
        channels[i] = 0xFFFFFFFF;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Destruktor von @p AudioSDL.
 *
 *  @author FloSoft
 */
AudioSDL::~AudioSDL(void)
{
    CleanUp();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Funktion zum Auslesen des Treibernamens.
 *
 *  @return liefert den Treibernamen zurück
 *
 *  @author FloSoft
 */
const char* AudioSDL::GetName(void) const
{
    return GetDriverName();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Treiberinitialisierungsfunktion.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 *
 *  @author FloSoft
 */
bool AudioSDL::Initialize(void)
{
    if( SDL_InitSubSystem( SDL_INIT_AUDIO ) < 0 )
    {
        fprintf(stderr, "%s\n", SDL_GetError());
        initialized = false;
        return false;
    }

    // open 44.1KHz, signed 16bit, system byte order,
    // stereo audio, using 1024 byte chunks
    if(Mix_OpenAudio(44100, AUDIO_S16LSB, 2, 4096) < 0)
    {
        fprintf(stderr, "%s\n", Mix_GetError());
        initialized = false;
        return false;
    }

    Mix_AllocateChannels(CHANNEL_COUNT);
    Mix_SetMusicCMD(NULL);
    Mix_HookMusicFinished(AudioSDL::MusicFinished);

    initialized = true;

    return initialized;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Treiberaufräumfunktion.
 *
 *  @author FloSoft
 */
void AudioSDL::CleanUp(void)
{
    // Sounddeskriptoren aufräumen
    for(std::vector<Sound*>::iterator it = sounds.begin(); it != sounds.end(); ++it)
        delete (*it);

    sounds.clear();

    Mix_CloseAudio();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);

    // nun sind wir nicht mehr initalisiert
    initialized = false;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Läd einen Effekt.
 *
 *  @param[in] type Typ der Daten
 *  @param[in] data Datenblock
 *  @param[in] size Größe des Datenblocks
 *
 *  @return Sounddeskriptor bei Erfolg, @p NULL bei Fehler
 *
 *  @author FloSoft
 */
Sound* AudioSDL::LoadEffect(unsigned int data_type, unsigned char* data, unsigned long size)
{
    SoundSDL_Effect* sd = new SoundSDL_Effect();

    char file[512];
    if (!tempname(file, 512))
        return(NULL);

    strncat(file, ".wav", 512);

    FILE* dat = fopen(file, "wb");
    if (!dat)
        return(NULL);

    if (fwrite(data, 1, size, dat) != size)
        return(NULL);

    fclose(dat);

    switch(data_type)
    {
        default:
            return(NULL);

        case AudioDriver::AD_WAVE:
        {
            sd->sound = Mix_LoadWAV(file);
        } break;
        /// @todo Alle Formate die SDL mit LoadWAV laden kann angeben
    }

    unlink(file);

    if(sd->sound == NULL)
    {
        fprintf(stderr, "%s\n", Mix_GetError());
        delete sd;
        return(NULL);
    }

    sd->SetNr((int)sounds.size());
    sounds.push_back(sd);

    return sd;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Läd ein Musikstück.
 *
 *  @param[in] type Typ der Daten
 *  @param[in] data Datenblock
 *  @param[in] size Größe des Datenblocks
 *
 *  @return Sounddeskriptor bei Erfolg, @p NULL bei Fehler
 *
 *  @author FloSoft
 */
Sound* AudioSDL::LoadMusic(unsigned int data_type, unsigned char* data, unsigned long size)
{
    SoundSDL_Music* sd = new SoundSDL_Music;

    char file[512];
    if (!tempname(file, 512))
        return(NULL);

    switch(data_type)
    {
        default:
            return(NULL);

        case AudioDriver::AD_MIDI:
        {
            strncat(file, ".mid", 512);
        } break;

        case AudioDriver::AD_WAVE:
        {
            strncat(file, ".wav", 512);
        } break;

        case AudioDriver::AD_OTHER:
        {
            const char* header = (const char*)data;
            if(strncmp(header, "OggS", 4) == 0)
                strncat(file, ".ogg", 512);
            else if (strncmp(header, "ID3", 3) == 0 || ((unsigned char)header[0] == 0xFF && (unsigned char)header[1] == 0xFB) )
                strncat(file, ".mp3", 512);
            else
                strncat(file, ".tmp", 512);
        } break;

        /// @todo Alle Formate die SDL mit LoadMUS laden kann angeben
    }

    FILE* dat = fopen(file, "wb");
    if (!dat)
        return(NULL);

    if (fwrite(data, 1, size, dat) != size)
        return(NULL);

    fclose(dat);

    sd->music = Mix_LoadMUS(file);

    unlink(file);

    if(sd->music == NULL)
    {
        fprintf(stderr, "%s\n", Mix_GetError());
        delete sd;
        return(NULL);
    }

    sd->SetNr((int)sounds.size());
    sounds.push_back(sd);

    return sd;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Spielt einen Sound ab.
 *
 *  @author OLiver
 */
unsigned int AudioSDL::PlayEffect(Sound* sound, const unsigned char volume, const bool loop)
{
    if(sound == NULL)
        return 0xFFFFFFFF;

    int channel = Mix_PlayChannel(-1, static_cast<SoundSDL_Effect*>(sound)->sound, (loop) ? -1 : 0);

    if(channel == -1)
    {
        //fprintf(stderr, "%s\n", Mix_GetError());
        return 0xFFFFFFFF;
    }

    unsigned play_id = GeneratePlayID();

    // Channel reservieren
    channels[channel] = play_id;

    Mix_Volume(channel, (int(master_effects_volume)*volume / 255) / 2);

    return play_id;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Spielt die Musik ab.
 *
 *  @author FloSoft
 */
void AudioSDL::PlayMusic(Sound* sound, const unsigned repeats)
{
    // Musik starten
    Mix_PlayMusic(static_cast<SoundSDL_Music*>(sound)->music, repeats == 0 ? -1 : int(repeats));

    // Lautstärke neu setzen
    Mix_VolumeMusic(master_music_volume / 2);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Stoppt die Musik.
 *
 *  @author FloSoft
 */
void AudioSDL::StopMusic(void)
{
    // Musik anhalten
    Mix_FadeOutMusic(1000);
}

void AudioSDL::StopEffect(const unsigned play_id)
{
    // Alle Channels nach dieser ID abfragen und den jeweiligen zum Schweigen bringen
    for(unsigned int i = 0; i < CHANNEL_COUNT; ++i)
    {
        if(channels[i] == play_id)
            Mix_HaltChannel(i);
    }
}


bool AudioSDL::IsEffectPlaying(const unsigned play_id)
{
    // Play-ID suchen
    for(unsigned int i = 0; i < CHANNEL_COUNT; ++i)
    {
        if(channels[i] == play_id)
            // und wird dieser Channel auch noch gespielt?
            return (Mix_Playing(i) == 1);
    }

    return false;
}


void AudioSDL::ChangeVolume(const unsigned play_id, const unsigned char volume)
{
    // Play-ID suchen
    for(unsigned int i = 0; i < CHANNEL_COUNT; ++i)
    {
        if(channels[i] == play_id)
            // Lautstärke verändern
            Mix_Volume(i, (int(master_effects_volume)*volume / 255) / 2);
    }
}

void AudioSDL::SetMasterEffectVolume(unsigned char volume)
{
    master_effects_volume = volume;
    //Mix_SetPanning(MIX_CHANNEL_POST, volume2, volume2);

    char mix_error[1024];
    if(snprintf(mix_error, 1024, "%s", Mix_GetError()))
        printf("%s\n", mix_error);
}

void AudioSDL::SetMasterMusicVolume(unsigned char volume)
{
    master_music_volume = volume;

    // volume von 0 - 127
    Mix_VolumeMusic(volume / 2);
}

void AudioSDL::MusicFinished()
{
    nthis->adli->Msg_MusicFinished();
}
