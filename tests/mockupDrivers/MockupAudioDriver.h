// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#pragma once

#ifndef MockupAudioDriver_h__
#define MockupAudioDriver_h__

#include "driver/AudioDriver.h"
#include "driver/IAudioDriverCallback.h"

struct MockupSoundDesc : public SoundDesc
{
    static int numAlive;

    MockupSoundDesc(SoundType type)
        : SoundDesc(type)
    {
        numAlive++; 
    }
    
    ~MockupSoundDesc() override
    {
        numAlive--; 
    }
};

struct MockupAudioDriver final : public AudioDriver, IAudioDriverCallback
{
protected:
    void DoUnloadSound(SoundDesc& sound) override
    { 
        static_cast<MockupSoundDesc&>(sound).setInvalid();
    }

public:
    MockupAudioDriver()
        : AudioDriver(this) 
    {}
    
    ~MockupAudioDriver() override
    {
        CleanUp(); 
    }
    
    const char* GetName() const override
    {
        return "MockupAudio"; 
    }
    
    bool Initialize() override
    {
        SetNumChannels(MAX_NUM_CHANNELS);
        return initialized = true;
    }

    SoundHandle LoadEffect(const std::string&) override
    { 
        return CreateSoundHandle(new MockupSoundDesc(SD_EFFECT)); 
    }

    SoundHandle LoadEffect(const std::vector<char>&, const std::string&) override
    {
        return CreateSoundHandle(new MockupSoundDesc(SD_EFFECT));
    }

    SoundHandle LoadMusic(const std::string&) override
    {
        return CreateSoundHandle(new MockupSoundDesc(SD_MUSIC)); 
    }

    SoundHandle LoadMusic(const std::vector<char>&, const std::string&) override
    {
        return CreateSoundHandle(new MockupSoundDesc(SD_MUSIC));
    }

    EffectPlayId PlayEffect(const SoundHandle& sound, uint8_t /*volume*/, bool /*loop*/) override
    {
        if(!sound.isValid())
            return -1;
        static int channel = 0;
        if(static_cast<unsigned>(++channel) >= MAX_NUM_CHANNELS)
            channel = 0;
        return AddPlayedEffect(channel);
    }

    void PlayMusic(const SoundHandle&, unsigned /*repeats*/) override {}
    void StopMusic() override {}
    void StopEffect(EffectPlayId play_id) override { RemoveEffect(play_id); }
    bool IsEffectPlaying(EffectPlayId play_id) override { return GetEffectChannel(play_id) >= 0; }
    void ChangeVolume(EffectPlayId, uint8_t /*volume*/) override {}
    void SetMasterEffectVolume(uint8_t /*volume*/) override {}
    void SetMusicVolume(uint8_t /*volume*/) override {}
    void Msg_MusicFinished() override {}
};

#endif // MockupAudioDriver_h__
