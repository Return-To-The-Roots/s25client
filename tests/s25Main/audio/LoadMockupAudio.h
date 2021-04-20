// Copyright (c) 2016 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Settings.h"
#include "drivers/AudioDriverWrapper.h"
#include "mockupDrivers/MockupAudioDriver.h"
#include "s25util/Log.h"
#include "s25util/NullWriter.h"
#include <memory>

struct LoadMockupAudio
{
    std::unique_ptr<MockAudioDriverCallback> audioCallbackMock;
    MockupAudioDriver* audioDriverMock;
    LoadMockupAudio() : audioCallbackMock(std::make_unique<MockAudioDriverCallback>())
    {
        LOG.setWriter(new NullWriter(), LogTarget::All);
        SETTINGS.sound.effectsEnabled = true;
        SETTINGS.sound.musicEnabled = true;
        auto driver = std::make_unique<MockupAudioDriver>(audioCallbackMock.get());
        audioDriverMock = driver.get();
        AUDIODRIVER.LoadDriver(std::move(driver));
        BOOST_TEST_REQUIRE(AUDIODRIVER.GetName() != "");
    }
    ~LoadMockupAudio() { AUDIODRIVER.UnloadDriver(); }
    /// Return a callable to be used with MOCK_EXPECT(...).calls(...) that loads a sound of the given type
    auto makeDoLoad(driver::SoundType type)
    {
        return [=](auto&&...) { return audioDriverMock->doLoad(type); };
    };
    auto makeUnloadHandle(driver::SoundType type)
    {
        // Check that the type matches and delete the driver data
        return [=](const driver::RawSoundHandle& handle) {
            BOOST_TEST(type == handle.getType());
            delete static_cast<MockupSoundData*>(handle.getDriverData());
        };
    };
};
