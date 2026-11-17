// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DriverWrapper.h"
#include "driver/EffectPlayId.h"
#include "driver/IAudioDriverCallback.h"
#include "drivers/SoundHandle.h"
#include "s25util/Singleton.h"
#include <memory>

class SoundHandle;

namespace libsiedler2 {
class ArchivItem_Sound;
}
namespace driver {
class IAudioDriver;
struct RawSoundHandle;
} // namespace driver

///////////////////////////////////////////////////////////////////////////////
// DriverWrapper
class AudioDriverWrapper :
    public Singleton<AudioDriverWrapper, SingletonPolicies::WithLongevity>,
    public IAudioDriverCallback
{
public:
    static constexpr unsigned Longevity = 30;

    AudioDriverWrapper();
    ~AudioDriverWrapper();

    /// Loads the driver
    bool loadDriver(std::unique_ptr<driver::IAudioDriver> audioDriver);
    bool loadDriver(std::string& preference);
    /// Unloads the driver resetting all open handles
    void unloadDriver();

    /// Lädt einen Sound.
    SoundHandle loadEffect(const std::string& filepath);
    SoundHandle loadEffect(const libsiedler2::ArchivItem_Sound& soundArchiv, const std::string& extension);
    SoundHandle loadMusic(const std::string& filepath);
    SoundHandle loadMusic(const libsiedler2::ArchivItem_Sound& soundArchiv, const std::string& extension);

    /// Spielt einen Sound
    EffectPlayId playEffect(const SoundHandle& sound, uint8_t volume, bool loop);
    /// Stops a sound and resets the id to "Invalid"
    void stopEffect(EffectPlayId& playId);

    /// Spielt Midi ab
    void playMusic(const SoundHandle& sound, int repeats);

    /// Stoppt die Musik.
    void stopMusic();

    /// Wird ein Sound (noch) abgespielt?
    bool isEffectPlaying(EffectPlayId play_id);

    /// Verändert die Lautstärke von einem abgespielten Sound (falls er noch abgespielt wird)
    void changeVolume(EffectPlayId play_id, uint8_t volume);

    void setMasterEffectVolume(uint8_t volume);
    void setMusicVolume(uint8_t volume);

    std::string getName() const;

private:
    using Handle = std::unique_ptr<driver::IAudioDriver, void (*)(driver::IAudioDriver*)>;
    bool init();
    void msgMusicFinished() override;
    SoundHandle createSoundHandle(const driver::RawSoundHandle& rawHandle);

    drivers::DriverWrapper driver_wrapper;
    Handle audiodriver_;
};

#define AUDIODRIVER AudioDriverWrapper::inst()
