// Copyright (c) 2016 - 2020 Settlers Freaks (sf-team at siedler25.org)
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
