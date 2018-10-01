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
#ifndef SDL_H_INCLUDED
#define SDL_H_INCLUDED

#include <AudioDriver.h>
class IAudioDriverCallback;
class SoundHandle;

/// Klasse für den SDL Audiotreiber.
class AudioSDL : public AudioDriver
{
private:
    /// Lautstärke der Effekte.
    uint8_t master_effects_volume;
    /// Lautstärke der Musik.
    uint8_t master_music_volume;

public:
    AudioSDL(IAudioDriverCallback* driverCallback);
    ~AudioSDL() override;

    /// Return the name of the driver
    const char* GetName() const override;
    bool Initialize() override;
    /// Closes all open handles of the driver (stops and unloads music and effects)
    void CleanUp() override;

    SoundHandle LoadEffect(const std::string& filepath) override;
    SoundHandle LoadMusic(const std::string& filepath) override;

    /// Plays an effect at the given volume. If loop is true, effect is looped indefinitely
    EffectPlayId PlayEffect(const SoundHandle& sound, uint8_t volume, bool loop) override;
    /// Plays the given music. Only 1 music will be played. If Repeats is 0 it will loop indefinitely,
    /// otherwise it loops the many times. TODO: What about not looping it (e.g. playing only once?)
    void PlayMusic(const SoundHandle& sound, unsigned repeats) override;
    /// Stop the music
    void StopMusic() override;
    /// Stop the effect with the given id (if it is still playing)
    void StopEffect(EffectPlayId play_id) override;
    /// Is the effect still being played
    bool IsEffectPlaying(EffectPlayId play_id) override;
    /// Changes volume [0..256) of a played sound (if it is still playing) relative to the master effect volume
    void ChangeVolume(EffectPlayId play_id, uint8_t volume) override;
    /// Set the master effect volume [0..256) at which all effects will be played. Changing the volume of an effect will be relative to this
    void SetMasterEffectVolume(uint8_t volume) override;
    /// Sets the music volume [0..256)
    void SetMusicVolume(uint8_t volume) override;

protected:
    void DoUnloadSound(SoundDesc& sound) override;

private:
    /// Calculates the volume at which the effect should be played using the master volume and the passed volume
    uint8_t CalcEffectVolume(uint8_t volume) const;
    /// Callback für Audiotreiber
    static void MusicFinished();
};

#endif // !SDL_H_INCLUDED
