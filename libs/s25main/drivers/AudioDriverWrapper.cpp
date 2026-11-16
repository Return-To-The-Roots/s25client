// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AudioDriverWrapper.h"
#include "MusicPlayer.h"
#include "RTTR_Assert.h"
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
void AudioDriverWrapper::PlayMusic(const SoundHandle& sound, int repeats)
{
    if(audiodriver_ && sound)
        audiodriver_->PlayMusic(sound.getRawHandle(), repeats);
}

/// Stoppt die Musik.
void AudioDriverWrapper::StopMusic()
{
    if(audiodriver_)
        audiodriver_->StopMusic();
}

/// Wird ein Sound (noch) abgespielt?
bool AudioDriverWrapper::isEffectPlaying(EffectPlayId play_id)
{
    if(audiodriver_)
        return audiodriver_->isEffectPlaying(play_id);
    else
        return false;
}

/// Verändert die Lautstärke von einem abgespielten Sound (falls er noch abgespielt wird)
void AudioDriverWrapper::ChangeVolume(EffectPlayId play_id, uint8_t volume)
{
    if(audiodriver_)
        audiodriver_->ChangeVolume(play_id, volume);
}

void AudioDriverWrapper::setMasterEffectVolume(uint8_t volume)
{
    if(audiodriver_)
        audiodriver_->setMasterEffectVolume(volume);
}

void AudioDriverWrapper::setMusicVolume(uint8_t volume)
{
    if(audiodriver_)
        audiodriver_->setMusicVolume(volume);
}

std::string AudioDriverWrapper::getName() const
{
    const char* name = (audiodriver_) ? audiodriver_->getName() : nullptr;
    return (name) ? name : "";
}

bool AudioDriverWrapper::Init()
{
    if(!audiodriver_ || !audiodriver_->Initialize())
    {
        UnloadDriver();
        return false;
    }

    LOG.write(_("Loaded audio driver \"%1%\"\n")) % getName();
    return true;
}

/// Lädt den Treiber
bool AudioDriverWrapper::LoadDriver(std::unique_ptr<driver::IAudioDriver> audioDriver)
{
    audiodriver_ = Handle(audioDriver.release(), [](driver::IAudioDriver* p) { delete p; });
    return Init();
}

bool AudioDriverWrapper::LoadDriver(std::string& preference)
{
    RTTR_Assert(!audiodriver_);
    // DLL laden
    if(!driver_wrapper.Load(drivers::DriverType::Audio, preference))
        return false;

    auto createAudioInstance = driver_wrapper.getFunction<decltype(CreateAudioInstance)>("CreateAudioInstance");
    auto freeAudioInstance = driver_wrapper.getFunction<decltype(FreeAudioInstance)>("FreeAudioInstance");
    RTTR_Assert(createAudioInstance && freeAudioInstance);

    // Instanz erzeugen
    audiodriver_ = Handle(createAudioInstance(this, VIDEODRIVER.getMapPointer()), freeAudioInstance);
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

    return createSoundHandle(audiodriver_->LoadMusic(filepath));
}

SoundHandle AudioDriverWrapper::LoadMusic(const libsiedler2::ArchivItem_Sound& soundArchiv,
                                          const std::string& extension)
{
    if(!audiodriver_)
        return SoundHandle();
    ovectorstream tmp;
    if(soundArchiv.write(tmp) != 0)
        return SoundHandle();
    return createSoundHandle(audiodriver_->LoadMusic(tmp.vector(), extension));
}

SoundHandle AudioDriverWrapper::LoadEffect(const std::string& filepath)
{
    if(!audiodriver_)
        return SoundHandle();

    return createSoundHandle(audiodriver_->LoadEffect(filepath));
}

SoundHandle AudioDriverWrapper::LoadEffect(const libsiedler2::ArchivItem_Sound& soundArchiv,
                                           const std::string& extension)
{
    if(!audiodriver_)
        return SoundHandle();
    ovectorstream tmp;
    if(soundArchiv.write(tmp) != 0)
        return SoundHandle();
    return createSoundHandle(audiodriver_->LoadEffect(tmp.vector(), extension));
}

EffectPlayId AudioDriverWrapper::PlayEffect(const SoundHandle& sound, uint8_t volume, const bool loop)
{
    if(!audiodriver_ || !sound)
        return EffectPlayId::Invalid;

    return audiodriver_->PlayEffect(sound.getRawHandle(), volume, loop);
}

void AudioDriverWrapper::StopEffect(EffectPlayId& playId)
{
    if(audiodriver_)
        audiodriver_->StopEffect(playId);
    playId = EffectPlayId::Invalid;
}

void AudioDriverWrapper::msgMusicFinished()
{
    // MusicManager Bescheid sagen
    MUSICPLAYER.MusicFinished();
}

/// Wraps the raw handle inside an RAII wrapper
SoundHandle AudioDriverWrapper::createSoundHandle(const driver::RawSoundHandle& rawHandle)
{
    auto releaseSound = [this](driver::RawSoundHandle* handlePtr) {
        if(handlePtr && handlePtr->getDriverData())
            audiodriver_->unloadSound(*handlePtr);
        delete handlePtr;
    };
    auto registerForUnload = [this](driver::RawSoundHandle* handlePtr) { audiodriver_->registerForUnload(handlePtr); };
    return SoundHandle(rawHandle, releaseSound, registerForUnload);
}
