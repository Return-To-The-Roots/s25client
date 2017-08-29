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

AudioDriverWrapper::AudioDriverWrapper() : audiodriver_(NULL), loadedFromDll(false)
{
}

AudioDriverWrapper::~AudioDriverWrapper()
{
    UnloadDriver();
}

/// Spielt Midi ab
void AudioDriverWrapper::PlayMusic(const SoundHandle& sound, unsigned repeats)
{
    if(audiodriver_)
        audiodriver_->PlayMusic(sound, repeats);
}

/// Stoppt die Musik.
void AudioDriverWrapper::StopMusic()
{
    if(audiodriver_)
        audiodriver_->StopMusic();
}

/// Wird ein Sound (noch) abgespielt?
bool AudioDriverWrapper::IsEffectPlaying(EffectPlayId play_id)
{
    if(audiodriver_)
        return audiodriver_->IsEffectPlaying(play_id);
    else
        return false;
}

/// Verändert die Lautstärke von einem abgespielten Sound (falls er noch abgespielt wird)
void AudioDriverWrapper::ChangeVolume(EffectPlayId play_id, uint8_t volume)
{
    if(audiodriver_)
        audiodriver_->ChangeVolume(play_id, volume);
}

void AudioDriverWrapper::SetMasterEffectVolume(uint8_t volume)
{
    if(audiodriver_)
        audiodriver_->SetMasterEffectVolume(volume);
}

void AudioDriverWrapper::SetMusicVolume(uint8_t volume)
{
    if(audiodriver_)
        audiodriver_->SetMusicVolume(volume);
}

std::string AudioDriverWrapper::GetName() const
{
    if(audiodriver_)
        return audiodriver_->GetName();
    else
        return "";
}

/// Lädt den Treiber
bool AudioDriverWrapper::LoadDriver(IAudioDriver* audioDriver)
{
    loadedFromDll = audioDriver == NULL;
    if(audioDriver)
        audiodriver_ = audioDriver;
    else
    {
        // DLL laden
        if(!driver_wrapper.Load(DriverWrapper::DT_AUDIO, SETTINGS.driver.audio))
            return false;

        PDRIVER_CREATEAUDIOINSTANCE CreateAudioInstance =
          pto2ptf<PDRIVER_CREATEAUDIOINSTANCE>(driver_wrapper.GetDLLFunction("CreateAudioInstance"));

        // Instanz erzeugen
        audiodriver_ = CreateAudioInstance(this, VIDEODRIVER.GetMapPointer());
        if(!audiodriver_)
        {
            UnloadDriver();
            return false;
        }
    }

    if(!audiodriver_->Initialize())
    {
        UnloadDriver();
        return false;
    }

    return true;
}

void AudioDriverWrapper::UnloadDriver()
{
    if(loadedFromDll)
    {
        PDRIVER_FREEAUDIOINSTANCE FreeAudioInstance =
          pto2ptf<PDRIVER_FREEAUDIOINSTANCE>(driver_wrapper.GetDLLFunction("FreeAudioInstance"));
        if(FreeAudioInstance)
            FreeAudioInstance(audiodriver_);
        driver_wrapper.Unload();
    } else
        delete audiodriver_;
    audiodriver_ = NULL;
}

/**
 *  Lädt einen Sound.
 *
 *  @return Sounddeskriptor bei Erfolg, @p NULL bei Fehler
 */
SoundHandle AudioDriverWrapper::LoadMusic(const std::string& filepath)
{
    if(!audiodriver_)
        return SoundHandle();

    return audiodriver_->LoadMusic(filepath);
}

SoundHandle AudioDriverWrapper::LoadMusic(const libsiedler2::ArchivItem_Sound& soundArchiv, const std::string& extension)
{
    std::ofstream fs;
    std::string filePath = createTempFile(fs, extension);
    if(!fs)
        return SoundHandle();
    if(soundArchiv.write(fs) != 0)
        return SoundHandle();
    fs.close();
    SoundHandle sound = LoadMusic(filePath);
    unlinkFile(filePath);
    return sound;
}

SoundHandle AudioDriverWrapper::LoadEffect(const std::string& filepath)
{
    if(!audiodriver_)
        return SoundHandle();

    return audiodriver_->LoadEffect(filepath);
}

SoundHandle AudioDriverWrapper::LoadEffect(const libsiedler2::ArchivItem_Sound& soundArchiv, const std::string& extension)
{
    std::ofstream fs;
    std::string filePath = createTempFile(fs, extension);
    if(!fs)
        return SoundHandle();
    if(soundArchiv.write(fs) != 0)
        return SoundHandle();
    fs.close();
    SoundHandle sound = LoadEffect(filePath);
    unlinkFile(filePath);
    return sound;
}

EffectPlayId AudioDriverWrapper::PlayEffect(const SoundHandle& sound, uint8_t volume, const bool loop)
{
    if(!audiodriver_)
        return 0;

    return audiodriver_->PlayEffect(sound, volume, loop);
}

void AudioDriverWrapper::StopEffect(const unsigned play_id)
{
    if(!audiodriver_)
        return;

    return audiodriver_->StopEffect(play_id);
}

void AudioDriverWrapper::Msg_MusicFinished()
{
    // MusicManager Bescheid sagen
    MUSICPLAYER.MusicFinished();
}
