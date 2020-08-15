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
    MOCK_NON_CONST_METHOD(LoadEffect, 2, driver::RawSoundHandle(const std::vector<char>&, const std::string&), LoadEffectFromData);
    MOCK_NON_CONST_METHOD(LoadMusic, 1, driver::RawSoundHandle(const std::string&));
    MOCK_NON_CONST_METHOD(LoadMusic, 2, driver::RawSoundHandle(const std::vector<char>&, const std::string&), LoadMusicFromData);
    MOCK_NON_CONST_METHOD(doPlayEffect, 3, int(driver::RawSoundHandle::DriverData, uint8_t, bool));
    MOCK_NON_CONST_METHOD(PlayMusic, 2);
    MOCK_NON_CONST_METHOD(StopMusic, 0);
    MOCK_NON_CONST_METHOD(doStopEffect, 1, void(int));
    MOCK_NON_CONST_METHOD(IsEffectPlaying, 1);
    MOCK_NON_CONST_METHOD(ChangeVolume, 2);
    MOCK_NON_CONST_METHOD(SetMasterEffectVolume, 1);
    MOCK_NON_CONST_METHOD(SetMusicVolume, 1);
    MOCK_NON_CONST_METHOD(doUnloadSound, 1, void(driver::RawSoundHandle sound));

    driver::RawSoundHandle doLoad(driver::SoundType type) { return createRawSoundHandle(new MockupSoundData(type), type); }
    using driver::AudioDriver::GetEffectChannel;
};

#endif // MockupAudioDriver_h__
