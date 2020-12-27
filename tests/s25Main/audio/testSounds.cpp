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

#include "Settings.h"
#include "drivers/AudioDriverWrapper.h"
#include "mockupDrivers/MockupAudioDriver.h"
#include "ogl/MusicItem.h"
#include "ogl/SoundEffectItem.h"
#include "ogl/glAllocator.h"
#include "test/testConfig.h"
#include "libsiedler2/Archiv.h"
#include "libsiedler2/ArchivItem.h"
#include "libsiedler2/libsiedler2.h"
#include "s25util/warningSuppression.h"
#include <boost/test/unit_test.hpp>

namespace bfs = boost::filesystem;

BOOST_TEST_DONT_PRINT_LOG_VALUE(EffectPlayId)
// Doesn't fully work until Boost 1.69
// BOOST_TEST_DONT_PRINT_LOG_VALUE(SoundHandle)
BOOST_TEST_DONT_PRINT_LOG_VALUE(driver::SoundType)

BOOST_AUTO_TEST_SUITE(SoundTests)

using driver::SoundType;

struct LoadMockupAudio
{
    std::unique_ptr<MockAudioDriverCallback> audioCallbackMock;
    MockupAudioDriver* audioDriverMock;
    LoadMockupAudio() : audioCallbackMock(std::make_unique<MockAudioDriverCallback>())
    {
        libsiedler2::setAllocator(new GlAllocator);
        SETTINGS.sound.effectsEnabled = true;
        SETTINGS.sound.musicEnabled = true;
        auto driver = std::make_unique<MockupAudioDriver>(audioCallbackMock.get());
        audioDriverMock = driver.get();
        AUDIODRIVER.LoadDriver(std::move(driver));
        BOOST_REQUIRE_NE(AUDIODRIVER.GetName(), "");
    }
    ~LoadMockupAudio() { AUDIODRIVER.UnloadDriver(); }
    /// Return a callable to be used with MOCK_EXPECT(...).calls(...) that loads a sound of the given type
    auto makeDoLoad(SoundType type)
    {
        return [=](auto&&...) { return audioDriverMock->doLoad(type); };
    };
    auto makeUnloadHandle(SoundType type)
    {
        // Check that the type matches and delete the driver data
        return [=](const driver::RawSoundHandle& handle) {
            BOOST_TEST(type == handle.getType());
            delete static_cast<MockupSoundData*>(handle.getDriverData());
        };
    };
};

BOOST_AUTO_TEST_CASE(DefaultConstructedSoundHandleIsInvalid)
{
    SoundHandle handle;
    BOOST_TEST_REQUIRE(!handle);
    BOOST_REQUIRE_THROW(handle.getRawHandle(), std::runtime_error);
    BOOST_REQUIRE_THROW(handle.getType(), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(SoundHandleGetUnloadedWhenLastGoesOutOfScope, LoadMockupAudio)
{
    {
        BOOST_TEST_REQUIRE(MockupSoundData::numAlive == 0);
        MOCK_EXPECT(audioDriverMock->LoadEffect).once().with("Foo.wav").calls(makeDoLoad(SoundType::Effect));
        SoundHandle handle = AUDIODRIVER.LoadEffect("Foo.wav");
        BOOST_TEST_REQUIRE(MockupSoundData::numAlive == 1);
        BOOST_TEST_REQUIRE(bool(handle)); // bool() is  workaround until Boost 1.69
        BOOST_TEST_REQUIRE(handle.getType() == SoundType::Effect);
        MOCK_EXPECT(audioDriverMock->LoadMusic).once().with("Foo2.wav").calls(makeDoLoad(SoundType::Music));
        SoundHandle handle2 = AUDIODRIVER.LoadMusic("Foo2.wav");
        BOOST_TEST_REQUIRE(MockupSoundData::numAlive == 2);
        BOOST_TEST_REQUIRE(bool(handle2)); // bool() is  workaround until Boost 1.69
        BOOST_TEST_REQUIRE(handle2.getType() == SoundType::Music);

        {
            // Copy handle
            const SoundHandle handleCopy = handle;
            BOOST_TEST_REQUIRE(MockupSoundData::numAlive == 2);
            RTTR_UNUSED(handleCopy);
            // Copy goes out of scope
        }
        BOOST_TEST_REQUIRE(bool(handle)); // bool() is  workaround until Boost 1.69
        BOOST_TEST_REQUIRE(MockupSoundData::numAlive == 2);

        // Handles go out of scope -> Close them
        MOCK_EXPECT(audioDriverMock->doUnloadSound)
          .once()
          .with(handle.getRawHandle())
          .calls(makeUnloadHandle(handle.getType()));
        MOCK_EXPECT(audioDriverMock->doUnloadSound)
          .once()
          .with(handle2.getRawHandle())
          .calls(makeUnloadHandle(handle2.getType()));
    }
    BOOST_TEST_REQUIRE(MockupSoundData::numAlive == 0);
    // Release driver. Nothing will be done as sounds already unloaded
    AUDIODRIVER.UnloadDriver();
    BOOST_TEST_REQUIRE(MockupSoundData::numAlive == 0);
}

BOOST_FIXTURE_TEST_CASE(SoundHandlesCanBeUnloadedByDriver, LoadMockupAudio)
{
    {
        MOCK_EXPECT(audioDriverMock->LoadEffect).once().with("Foo.wav").calls(makeDoLoad(SoundType::Effect));
        SoundHandle handle = AUDIODRIVER.LoadEffect("Foo.wav");
        BOOST_TEST_REQUIRE(MockupSoundData::numAlive == 1);
        BOOST_TEST_REQUIRE(bool(handle)); // bool() is  workaround until Boost 1.69
        BOOST_TEST_REQUIRE(handle.getType() == SoundType::Effect);

        // Release driver
        MOCK_EXPECT(audioDriverMock->doUnloadSound)
          .once()
          .with(handle.getRawHandle())
          .calls(makeUnloadHandle(handle.getType()));
        AUDIODRIVER.UnloadDriver();
        BOOST_TEST_REQUIRE(!handle);
        // Driver data was released
        BOOST_TEST_REQUIRE(MockupSoundData::numAlive == 0);
    }
    // Handle now destroyed but driver not called
    BOOST_TEST_REQUIRE(MockupSoundData::numAlive == 0);
}

BOOST_FIXTURE_TEST_CASE(PlayFromFile, LoadMockupAudio)
{
    {
        libsiedler2::Archiv snd;
        BOOST_TEST_REQUIRE(libsiedler2::Load(rttr::test::libsiedler2TestFilesDir / "testMono.wav", snd) == 0);
        auto* effect = dynamic_cast<SoundEffectItem*>(snd[0]);
        BOOST_TEST_REQUIRE(effect);

        // First call to Play loads the sound
        mock::sequence s;
        MOCK_EXPECT(audioDriverMock->LoadEffectFromData)
          .in(s)
          .once()
          .with(mock::any, ".wav")
          .calls(makeDoLoad(SoundType::Effect));
        const int channel = 0;
        MOCK_EXPECT(audioDriverMock->doPlayEffect).in(s).once().with(mock::affirm, 50, false).returns(channel);
        EffectPlayId id = effect->Play(50, false); //-V522
        BOOST_TEST_REQUIRE(MockupSoundData::numAlive == 1);
        BOOST_TEST_REQUIRE(effect->getLoadedType() == SoundType::Effect);
        BOOST_TEST_REQUIRE(id != EffectPlayId::Invalid);
        BOOST_TEST_REQUIRE(audioDriverMock->GetEffectChannel(id) == channel);
        MOCK_EXPECT(audioDriverMock->IsEffectPlaying).in(s).once().with(id).returns(true);
        BOOST_TEST_REQUIRE(AUDIODRIVER.IsEffectPlaying(id));

        // Stop playing
        MOCK_EXPECT(audioDriverMock->doStopEffect).in(s).once().with(channel);
        MOCK_EXPECT(audioDriverMock->IsEffectPlaying).in(s).once().with(id).returns(false);
        AUDIODRIVER.StopEffect(id);
        BOOST_TEST_REQUIRE(audioDriverMock->GetEffectChannel(id) == -1);
        BOOST_TEST_REQUIRE(!AUDIODRIVER.IsEffectPlaying(id));

        // Unload when effect goes out of scope
        MOCK_EXPECT(audioDriverMock->doUnloadSound).in(s).once().calls(makeUnloadHandle(SoundType::Effect));
    }
    // Unloaded when it goes out of scope
    BOOST_TEST_REQUIRE(MockupSoundData::numAlive == 0);

    // Same for different music types
    for(const bfs::path musicFile : {"test.ogg", "testMidi.mid", "testXMidi.xmi"})
    {
        mock::sequence s;
        // All midi types are treated as .midi
        const auto* const extension = (musicFile.extension() == ".ogg") ? ".ogg" : ".midi";
        libsiedler2::Archiv musicArchiv;
        BOOST_TEST_REQUIRE(libsiedler2::Load(rttr::test::libsiedler2TestFilesDir / musicFile, musicArchiv) == 0);
        auto* music = dynamic_cast<MusicItem*>(musicArchiv[0]);
        BOOST_TEST_REQUIRE(music);

        // First call to Play loads the sound
        BOOST_TEST_REQUIRE(MockupSoundData::numAlive == 0);
        MOCK_EXPECT(audioDriverMock->LoadMusicFromData)
          .in(s)
          .once()
          .with(mock::any, extension)
          .calls(makeDoLoad(SoundType::Music));
        MOCK_EXPECT(audioDriverMock->PlayMusic).in(s).once().with(mock::any, 0);
        music->Play();
        BOOST_TEST_REQUIRE(MockupSoundData::numAlive == 1);
        BOOST_TEST_REQUIRE(music->getLoadedType() == SoundType::Music);

        MOCK_EXPECT(audioDriverMock->StopMusic).in(s).once();
        AUDIODRIVER.StopMusic();
        MOCK_EXPECT(audioDriverMock->doUnloadSound).in(s).once().calls(makeUnloadHandle(SoundType::Music));
    }
    BOOST_TEST_REQUIRE(MockupSoundData::numAlive == 0);
}

BOOST_AUTO_TEST_SUITE_END()
