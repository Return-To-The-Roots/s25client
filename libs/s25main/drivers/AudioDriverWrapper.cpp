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

#include "rttrDefines.h" // IWYU pragma: keep
#include "AudioDriverWrapper.h"
#include "MusicPlayer.h"
#include "VideoDriverWrapper.h"
#include "driver/AudioInterface.h"
#include "mygettext/mygettext.h"
#include "libsiedler2/ArchivItem_Sound.h"
#include "s25util/Log.h"
#include <boost/interprocess/streams/vectorstream.hpp>
#include <ostream>
#include <vector>

using ovectorstream = boost::interprocess::basic_ovectorstream<std::vector<char>>;

AudioDriverWrapper::AudioDriverWrapper() : audiodriver_(nullptr, nullptr) {}

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
    const char* name = (audiodriver_) ? audiodriver_->GetName() : nullptr;
    return (name) ? name : "";
}

bool AudioDriverWrapper::Init()
{
    if(!audiodriver_ || !audiodriver_->Initialize())
    {
        UnloadDriver();
        return false;
    }

    LOG.write(_("Loaded audio driver \"%1%\"\n")) % GetName();
    return true;
}

/// Lädt den Treiber
bool AudioDriverWrapper::LoadDriver(IAudioDriver* audioDriver)
{
    audiodriver_ = Handle(audioDriver, [](IAudioDriver* p) { delete p; });
    return Init();
}

bool AudioDriverWrapper::LoadDriver(std::string& preference)
{
    RTTR_Assert(!audiodriver_);
    // DLL laden
    if(!driver_wrapper.Load(drivers::DriverType::Audio, preference))
        return false;

    auto createAudioInstance = driver_wrapper.GetFunction<decltype(CreateAudioInstance)>("CreateAudioInstance");
    auto freeAudioInstance = driver_wrapper.GetFunction<decltype(FreeAudioInstance)>("FreeAudioInstance");
    RTTR_Assert(createAudioInstance && freeAudioInstance);

    // Instanz erzeugen
    audiodriver_ = Handle(createAudioInstance(this, VIDEODRIVER.GetMapPointer()), freeAudioInstance);
    return Init();
}

void AudioDriverWrapper::UnloadDriver()
{
    audiodriver_.reset();
    driver_wrapper.Unload();
}

/**
 *  Lädt einen Sound.
 *
 *  @return Sounddeskriptor bei Erfolg, @p nullptr bei Fehler
 */
SoundHandle AudioDriverWrapper::LoadMusic(const std::string& filepath)
{
    if(!audiodriver_)
        return SoundHandle();

    return audiodriver_->LoadMusic(filepath);
}

SoundHandle AudioDriverWrapper::LoadMusic(const libsiedler2::ArchivItem_Sound& soundArchiv, const std::string& extension)
{
    if(!audiodriver_)
        return SoundHandle();
    ovectorstream tmp;
    if(soundArchiv.write(tmp) != 0)
        return SoundHandle();
    return audiodriver_->LoadMusic(tmp.vector(), extension);
}

SoundHandle AudioDriverWrapper::LoadEffect(const std::string& filepath)
{
    if(!audiodriver_)
        return SoundHandle();

    return audiodriver_->LoadEffect(filepath);
}

SoundHandle AudioDriverWrapper::LoadEffect(const libsiedler2::ArchivItem_Sound& soundArchiv, const std::string& extension)
{
    if(!audiodriver_)
        return SoundHandle();
    ovectorstream tmp;
    if(soundArchiv.write(tmp) != 0)
        return SoundHandle();
    return audiodriver_->LoadEffect(tmp.vector(), extension);
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
