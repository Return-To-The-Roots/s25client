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
    unloadDriver();
}

/// Spielt Midi ab
void AudioDriverWrapper::playMusic(const SoundHandle& sound, int repeats)
{
    if(audiodriver_ && sound)
        audiodriver_->playMusic(sound.getRawHandle(), repeats);
}

/// Stoppt die Musik.
void AudioDriverWrapper::stopMusic()
{
    if(audiodriver_)
        audiodriver_->stopMusic();
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
void AudioDriverWrapper::changeVolume(EffectPlayId play_id, uint8_t volume)
{
    if(audiodriver_)
        audiodriver_->changeVolume(play_id, volume);
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

bool AudioDriverWrapper::init()
{
    if(!audiodriver_ || !audiodriver_->initialize())
    {
        unloadDriver();
        return false;
    }

    LOG.write(_("Loaded audio driver \"%1%\"\n")) % getName();
    return true;
}

/// Lädt den Treiber
bool AudioDriverWrapper::loadDriver(std::unique_ptr<driver::IAudioDriver> audioDriver)
{
    audiodriver_ = Handle(audioDriver.release(), [](driver::IAudioDriver* p) { delete p; });
    return init();
}

bool AudioDriverWrapper::loadDriver(std::string& preference)
{
    RTTR_Assert(!audiodriver_);
    // DLL laden
    if(!driver_wrapper.load(drivers::DriverType::Audio, preference))
        return false;

    auto createAudioInstance = driver_wrapper.getFunction<decltype(CreateAudioInstance)>("CreateAudioInstance");
    auto freeAudioInstance = driver_wrapper.getFunction<decltype(FreeAudioInstance)>("FreeAudioInstance");
    RTTR_Assert(createAudioInstance && freeAudioInstance);

    // Instanz erzeugen
    audiodriver_ = Handle(createAudioInstance(this, VIDEODRIVER.getMapPointer()), freeAudioInstance);
    return init();
}

void AudioDriverWrapper::unloadDriver()
{
    audiodriver_.reset();
    driver_wrapper.unload();
}

/**
 *  Lädt einen Sound.
 *
 *  @return Sounddeskriptor bei Erfolg, @p nullptr bei Fehler
 */
SoundHandle AudioDriverWrapper::loadMusic(const std::string& filepath)
{
    if(!audiodriver_)
        return SoundHandle();

    return createSoundHandle(audiodriver_->loadMusic(filepath));
}

SoundHandle AudioDriverWrapper::loadMusic(const libsiedler2::ArchivItem_Sound& soundArchiv,
                                          const std::string& extension)
{
    if(!audiodriver_)
        return SoundHandle();
    ovectorstream tmp;
    if(soundArchiv.write(tmp) != 0)
        return SoundHandle();
    return createSoundHandle(audiodriver_->loadMusic(tmp.vector(), extension));
}

SoundHandle AudioDriverWrapper::loadEffect(const std::string& filepath)
{
    if(!audiodriver_)
        return SoundHandle();

    return createSoundHandle(audiodriver_->loadEffect(filepath));
}

SoundHandle AudioDriverWrapper::loadEffect(const libsiedler2::ArchivItem_Sound& soundArchiv,
                                           const std::string& extension)
{
    if(!audiodriver_)
        return SoundHandle();
    ovectorstream tmp;
    if(soundArchiv.write(tmp) != 0)
        return SoundHandle();
    return createSoundHandle(audiodriver_->loadEffect(tmp.vector(), extension));
}

EffectPlayId AudioDriverWrapper::playEffect(const SoundHandle& sound, uint8_t volume, const bool loop)
{
    if(!audiodriver_ || !sound)
        return EffectPlayId::Invalid;

    return audiodriver_->playEffect(sound.getRawHandle(), volume, loop);
}

void AudioDriverWrapper::stopEffect(EffectPlayId& playId)
{
    if(audiodriver_)
        audiodriver_->stopEffect(playId);
    playId = EffectPlayId::Invalid;
}

void AudioDriverWrapper::msgMusicFinished()
{
    // MusicManager Bescheid sagen
    MUSICPLAYER.musicFinished();
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
