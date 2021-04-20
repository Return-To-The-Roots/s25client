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
    bool LoadDriver(std::unique_ptr<driver::IAudioDriver> audioDriver);
    bool LoadDriver(std::string& preference);
    /// Unloads the driver resetting all open handles
    void UnloadDriver();

    /// Lädt einen Sound.
    SoundHandle LoadEffect(const std::string& filepath);
    SoundHandle LoadEffect(const libsiedler2::ArchivItem_Sound& soundArchiv, const std::string& extension);
    SoundHandle LoadMusic(const std::string& filepath);
    SoundHandle LoadMusic(const libsiedler2::ArchivItem_Sound& soundArchiv, const std::string& extension);

    /// Spielt einen Sound
    EffectPlayId PlayEffect(const SoundHandle& sound, uint8_t volume, bool loop);
    /// Stops a sound and resets the id to "Invalid"
    void StopEffect(EffectPlayId& playId);

    /// Spielt Midi ab
    void PlayMusic(const SoundHandle& sound, int repeats);

    /// Stoppt die Musik.
    void StopMusic();

    /// Wird ein Sound (noch) abgespielt?
    bool IsEffectPlaying(EffectPlayId play_id);

    /// Verändert die Lautstärke von einem abgespielten Sound (falls er noch abgespielt wird)
    void ChangeVolume(EffectPlayId play_id, uint8_t volume);

    void SetMasterEffectVolume(uint8_t volume);
    void SetMusicVolume(uint8_t volume);

    std::string GetName() const;

private:
    using Handle = std::unique_ptr<driver::IAudioDriver, void (*)(driver::IAudioDriver*)>;
    bool Init();
    void Msg_MusicFinished() override;
    SoundHandle createSoundHandle(const driver::RawSoundHandle& rawHandle);

    drivers::DriverWrapper driver_wrapper;
    Handle audiodriver_;
};

#define AUDIODRIVER AudioDriverWrapper::inst()
