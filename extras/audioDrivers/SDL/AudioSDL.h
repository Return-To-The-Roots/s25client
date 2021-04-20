// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <driver/AudioDriver.h>
class IAudioDriverCallback;
class SoundHandle;

/// Klasse für den SDL Audiotreiber.
class AudioSDL final : public driver::AudioDriver
{
private:
    /// Lautstärke der Effekte.
    uint8_t master_effects_volume;
    /// Lautstärke der Musik.
    uint8_t master_music_volume;

public:
    AudioSDL(IAudioDriverCallback* adli);
    ~AudioSDL() override;

    /// Return the name of the driver
    const char* GetName() const override;
    bool Initialize() override;
    /// Closes all open handles of the driver (stops and unloads music and effects)
    void CleanUp() override;

    driver::RawSoundHandle LoadEffect(const std::string& filepath) override;
    driver::RawSoundHandle LoadEffect(const std::vector<char>& data, const std::string& ext) override;
    driver::RawSoundHandle LoadMusic(const std::string& filepath) override;
    driver::RawSoundHandle LoadMusic(const std::vector<char>& data, const std::string& ext) override;

    /// Plays an effect at the given volume. If loop is true, effect is looped indefinitely
    int doPlayEffect(driver::RawSoundHandle::DriverData driverData, uint8_t volume, bool loop) override;
    /// Plays the given music. Only 1 music will be played.
    /// If repeats is less than 0 it will loop indefinitely, otherwise it will be repeated that many times.
    void PlayMusic(const driver::RawSoundHandle& sound, int repeats) override;
    /// Stop the music
    void StopMusic() override;
    /// Stop the effect on this channel
    void doStopEffect(int channel) override;
    /// Is the effect still being played
    bool IsEffectPlaying(EffectPlayId play_id) override;
    /// Changes volume [0..256) of a played sound (if it is still playing) relative to the master effect volume
    void ChangeVolume(EffectPlayId play_id, uint8_t volume) override;
    /// Set the master effect volume [0..256) at which all effects will be played. Changing the volume of an effect will
    /// be relative to this
    void SetMasterEffectVolume(uint8_t volume) override;
    /// Sets the music volume [0..256)
    void SetMusicVolume(uint8_t volume) override;

protected:
    void doUnloadSound(driver::RawSoundHandle sound) override;

private:
    /// Calculates the volume at which the effect should be played using the master volume and the passed volume
    uint8_t CalcEffectVolume(uint8_t volume) const;
    /// Callback für Audiotreiber
    static void MusicFinished();
};
