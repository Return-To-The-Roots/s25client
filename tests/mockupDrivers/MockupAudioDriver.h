// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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
    MOCK_NON_CONST_METHOD(msgMusicFinished, 0);
};

MOCK_BASE_CLASS(MockupAudioDriver, driver::AudioDriver)
{
    MockupAudioDriver(IAudioDriverCallback * callback) : AudioDriver(callback) {}
    ~MockupAudioDriver() override { cleanUp(); }
    const char* getName() const override { return "MockupAudio"; }
    bool initialize() override
    {
        setNumChannels(DEFAULT_NUM_CHANNELS);
        initialized = true;
        return true;
    }
    MOCK_NON_CONST_METHOD(loadEffect, 1, driver::RawSoundHandle(const std::string&));
    MOCK_NON_CONST_METHOD(loadEffect, 2, driver::RawSoundHandle(const std::vector<char>&, const std::string&),
                          LoadEffectFromData);
    MOCK_NON_CONST_METHOD(loadMusic, 1, driver::RawSoundHandle(const std::string&));
    MOCK_NON_CONST_METHOD(loadMusic, 2, driver::RawSoundHandle(const std::vector<char>&, const std::string&),
                          LoadMusicFromData);
    MOCK_NON_CONST_METHOD(doPlayEffect, 3, int(driver::RawSoundHandle::DriverData, uint8_t, bool));
    MOCK_NON_CONST_METHOD(playMusic, 2);
    MOCK_NON_CONST_METHOD(stopMusic, 0);
    MOCK_NON_CONST_METHOD(doStopEffect, 1, void(int));
    MOCK_NON_CONST_METHOD(isEffectPlaying, 1);
    MOCK_NON_CONST_METHOD(changeVolume, 2);
    MOCK_NON_CONST_METHOD(setMasterEffectVolume, 1);
    MOCK_NON_CONST_METHOD(setMusicVolume, 1);
    MOCK_NON_CONST_METHOD(doUnloadSound, 1, void(driver::RawSoundHandle sound));

    driver::RawSoundHandle doLoad(driver::SoundType type)
    {
        return createRawSoundHandle(new MockupSoundData(type), type);
    }
    using driver::AudioDriver::getEffectChannel;
};
