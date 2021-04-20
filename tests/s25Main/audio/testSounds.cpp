// Copyright (C) 2016 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "LoadMockupAudio.h"
#include "drivers/AudioDriverWrapper.h"
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

// LCOV_EXCL_START
BOOST_TEST_DONT_PRINT_LOG_VALUE(EffectPlayId)
BOOST_TEST_DONT_PRINT_LOG_VALUE(SoundHandle)
BOOST_TEST_DONT_PRINT_LOG_VALUE(driver::SoundType)
// LCOV_EXCL_STOP

BOOST_AUTO_TEST_SUITE(SoundTests)

using driver::SoundType;

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
        BOOST_TEST_REQUIRE(handle);
        BOOST_TEST_REQUIRE(handle.getType() == SoundType::Effect);
        MOCK_EXPECT(audioDriverMock->LoadMusic).once().with("Foo2.wav").calls(makeDoLoad(SoundType::Music));
        SoundHandle handle2 = AUDIODRIVER.LoadMusic("Foo2.wav");
        BOOST_TEST_REQUIRE(MockupSoundData::numAlive == 2);
        BOOST_TEST_REQUIRE(handle2);
        BOOST_TEST_REQUIRE(handle2.getType() == SoundType::Music);

        {
            // Copy handle
            const SoundHandle handleCopy = handle;
            BOOST_TEST_REQUIRE(MockupSoundData::numAlive == 2);
            RTTR_UNUSED(handleCopy);
            // Copy goes out of scope
        }
        BOOST_TEST_REQUIRE(handle);
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
        BOOST_TEST_REQUIRE(handle);
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
    libsiedler2::setAllocator(new GlAllocator);
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
        const auto idCopy = id;
        AUDIODRIVER.StopEffect(id);
        BOOST_TEST(id == EffectPlayId::Invalid);
        BOOST_TEST_REQUIRE(audioDriverMock->GetEffectChannel(idCopy) == -1);
        BOOST_TEST_REQUIRE(!AUDIODRIVER.IsEffectPlaying(idCopy));

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
