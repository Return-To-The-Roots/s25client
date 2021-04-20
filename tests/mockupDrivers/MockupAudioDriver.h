// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "driver/AudioDriver.h"
#include "driver/IAudioDriverCallback.h"
#include <turtle/mock.hpp>

struct MockupSoundData
{
    static int numAlive;
    driver::SoundType type;
    MockupSoundData(driver::SoundType type) : type(type) { numAlive++; }
    ~MockupSoundData() { numAlive--; }
};

MOCK_BASE_CLASS(MockAudioDriverCallback, IAudioDriverCallback)
{
    MOCK_NON_CONST_METHOD(Msg_MusicFinished, 0);
};

MOCK_BASE_CLASS(MockupAudioDriver, driver::AudioDriver)
{
    MockupAudioDriver(IAudioDriverCallback * callback) : AudioDriver(callback) {}
    ~MockupAudioDriver() override { CleanUp(); }
    const char* GetName() const override { return "MockupAudio"; }
    bool Initialize() override
    {
        SetNumChannels(DEFAULT_NUM_CHANNELS);
        initialized = true;
        return true;
    }
    MOCK_NON_CONST_METHOD(LoadEffect, 1, driver::RawSoundHandle(const std::string&));
    MOCK_NON_CONST_METHOD(LoadEffect, 2, driver::RawSoundHandle(const std::vector<char>&, const std::string&),
                          LoadEffectFromData);
    MOCK_NON_CONST_METHOD(LoadMusic, 1, driver::RawSoundHandle(const std::string&));
    MOCK_NON_CONST_METHOD(LoadMusic, 2, driver::RawSoundHandle(const std::vector<char>&, const std::string&),
                          LoadMusicFromData);
    MOCK_NON_CONST_METHOD(doPlayEffect, 3, int(driver::RawSoundHandle::DriverData, uint8_t, bool));
    MOCK_NON_CONST_METHOD(PlayMusic, 2);
    MOCK_NON_CONST_METHOD(StopMusic, 0);
    MOCK_NON_CONST_METHOD(doStopEffect, 1, void(int));
    MOCK_NON_CONST_METHOD(IsEffectPlaying, 1);
    MOCK_NON_CONST_METHOD(ChangeVolume, 2);
    MOCK_NON_CONST_METHOD(SetMasterEffectVolume, 1);
    MOCK_NON_CONST_METHOD(SetMusicVolume, 1);
    MOCK_NON_CONST_METHOD(doUnloadSound, 1, void(driver::RawSoundHandle sound));

    driver::RawSoundHandle doLoad(driver::SoundType type)
    {
        return createRawSoundHandle(new MockupSoundData(type), type);
    }
    using driver::AudioDriver::GetEffectChannel;
};
