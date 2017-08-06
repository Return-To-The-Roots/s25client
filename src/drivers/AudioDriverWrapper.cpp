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

#include "defines.h" // IWYU pragma: keep
#include "AudioDriverWrapper.h"
#include "MusicPlayer.h"
#include "Settings.h"
#include "VideoDriverWrapper.h"
#include "driver/src/AudioInterface.h"
#include "libsiedler2/src/ArchivItem_Sound.h"
#include "libutil/src/tmpFile.h"
#include <ostream>

AudioDriverWrapper::AudioDriverWrapper() : audiodriver(0)
{
}

AudioDriverWrapper::~AudioDriverWrapper()
{
    PDRIVER_FREEAUDIOINSTANCE FreeAudioInstance = pto2ptf<PDRIVER_FREEAUDIOINSTANCE>(driver_wrapper.GetDLLFunction("FreeAudioInstance"));
    if(FreeAudioInstance)
        FreeAudioInstance(audiodriver);
}

/// Spielt Midi ab
void AudioDriverWrapper::PlayMusic(Sound* sound, const unsigned repeats)
{
    if(audiodriver)
        audiodriver->PlayMusic(sound, repeats);
}

/// Stoppt die Musik.
void AudioDriverWrapper::StopMusic()
{
    if(audiodriver)
        audiodriver->StopMusic();
}

/// Wird ein Sound (noch) abgespielt?
bool AudioDriverWrapper::IsEffectPlaying(const unsigned play_id)
{
    if(audiodriver)
        return audiodriver->IsEffectPlaying(play_id);
    else
        return false;
}

/// Verändert die Lautstärke von einem abgespielten Sound (falls er noch abgespielt wird)
void AudioDriverWrapper::ChangeVolume(const unsigned play_id, const unsigned char volume)
{
    if(audiodriver)
        audiodriver->ChangeVolume(play_id, volume);
}

void AudioDriverWrapper::SetMasterEffectVolume(unsigned char volume)
{
    if(audiodriver)
        audiodriver->SetMasterEffectVolume(volume);
}

void AudioDriverWrapper::SetMasterMusicVolume(unsigned char volume)
{
    if(audiodriver)
        audiodriver->SetMasterMusicVolume(volume);
}

std::string AudioDriverWrapper::GetName() const
{
    if(audiodriver)
        return audiodriver->GetName();
    else
        return "";
}

/// Lädt den Treiber
bool AudioDriverWrapper::LoadDriver()
{
    // DLL laden
    if(!driver_wrapper.Load(DriverWrapper::DT_AUDIO, SETTINGS.driver.audio))
        return false;

    PDRIVER_CREATEAUDIOINSTANCE CreateAudioInstance =
      pto2ptf<PDRIVER_CREATEAUDIOINSTANCE>(driver_wrapper.GetDLLFunction("CreateAudioInstance"));

    // Instanz erzeugen
    audiodriver = CreateAudioInstance(this, VIDEODRIVER.GetMapPointer());
    if(!audiodriver)
        return false;

    if(!audiodriver->Initialize())
    {
        delete audiodriver;
        audiodriver = NULL;
        return false;
    }

    return true;
}

/**
 *  Lädt einen Sound.
 *
 *  @return Sounddeskriptor bei Erfolg, @p NULL bei Fehler
 */
Sound* AudioDriverWrapper::LoadMusic(const std::string& filepath)
{
    if(!audiodriver)
        return NULL;

    return audiodriver->LoadMusic(filepath);
}

Sound* AudioDriverWrapper::LoadMusic(const libsiedler2::baseArchivItem_Sound& soundArchiv, const std::string& extension)
{
    std::ofstream fs;
    std::string filePath = createTempFile(fs, extension);
    if(!fs)
        return NULL;
    if(soundArchiv.write(fs) != 0)
        return NULL;
    fs.close();
    Sound* sound = LoadMusic(filePath);
    unlinkFile(filePath);
    return sound;
}

Sound* AudioDriverWrapper::LoadEffect(const std::string& filepath)
{
    if(!audiodriver)
        return NULL;

    return audiodriver->LoadEffect(filepath);
}

Sound* AudioDriverWrapper::LoadEffect(const libsiedler2::baseArchivItem_Sound& soundArchiv, const std::string& extension)
{
    std::ofstream fs;
    std::string filePath = createTempFile(fs, extension);
    if(!fs)
        return NULL;
    if(soundArchiv.write(fs) != 0)
        return NULL;
    fs.close();
    Sound* sound = LoadEffect(filePath);
    unlinkFile(filePath);
    return sound;
}

unsigned AudioDriverWrapper::PlayEffect(Sound* sound, const unsigned char volume, const bool loop)
{
    if(!audiodriver)
        return 0;

    return audiodriver->PlayEffect(sound, volume, loop);
}

void AudioDriverWrapper::StopEffect(const unsigned play_id)
{
    if(!audiodriver)
        return;

    return audiodriver->StopEffect(play_id);
}

void AudioDriverWrapper::Msg_MusicFinished()
{
    // MusicManager Bescheid sagen
    MUSICPLAYER.MusicFinished();
}
