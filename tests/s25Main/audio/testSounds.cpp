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
#include "driver/SoundHandle.h"
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

BOOST_AUTO_TEST_SUITE(SoundTests)

struct LoadMockupAudio
{
    LoadMockupAudio()
    {
        libsiedler2::setAllocator(new GlAllocator);
        SETTINGS.sound.effekte = true;
        SETTINGS.sound.musik = true;
        AUDIODRIVER.LoadDriver(new MockupAudioDriver);
        BOOST_REQUIRE_NE(AUDIODRIVER.GetName(), "");
    }
};

BOOST_FIXTURE_TEST_CASE(SoundHandles, LoadMockupAudio)
{
    SoundHandle handle;
    BOOST_REQUIRE(!handle.isValid());
    BOOST_REQUIRE_EQUAL(handle.getType(), SD_UNKNOWN);
    {
        BOOST_REQUIRE_EQUAL(MockupSoundDesc::numAlive, 0);
        SoundHandle localHandle = AUDIODRIVER.LoadEffect("Foo.wav");
        BOOST_REQUIRE_EQUAL(MockupSoundDesc::numAlive, 1);
        BOOST_REQUIRE(localHandle.isValid());
        BOOST_REQUIRE_EQUAL(localHandle.getType(), SD_EFFECT);
        SoundHandle localHandle2 = AUDIODRIVER.LoadMusic("Foo.wav");
        BOOST_REQUIRE_EQUAL(MockupSoundDesc::numAlive, 2);
        BOOST_REQUIRE(localHandle2.isValid());
        BOOST_REQUIRE_EQUAL(localHandle2.getType(), SD_MUSIC);
        // Handles go out of scope -> Close them
    }
    BOOST_REQUIRE_EQUAL(MockupSoundDesc::numAlive, 0);
    handle = AUDIODRIVER.LoadEffect("Foo.wav");
    BOOST_REQUIRE_EQUAL(MockupSoundDesc::numAlive, 1);
    BOOST_REQUIRE(handle.isValid());
    BOOST_REQUIRE_EQUAL(handle.getType(), SD_EFFECT);
    {
        // Copy handle
        const SoundHandle localHandle = handle;
        BOOST_REQUIRE_EQUAL(MockupSoundDesc::numAlive, 1);
        RTTR_UNUSED(localHandle);
        // Copy goes out of scope
    }
    BOOST_REQUIRE(handle.isValid());
    BOOST_REQUIRE_EQUAL(MockupSoundDesc::numAlive, 1);
    // Release driver
    AUDIODRIVER.UnloadDriver();
    BOOST_REQUIRE(!handle.isValid());
    BOOST_REQUIRE_EQUAL(handle.getType(), SD_UNKNOWN);
    // Still alive though invalid
    BOOST_REQUIRE_EQUAL(MockupSoundDesc::numAlive, 1);
}

BOOST_FIXTURE_TEST_CASE(PlayFromFile, LoadMockupAudio)
{
    libsiedler2::Archiv snd;
    std::array<std::string, 3> musicFiles = {{"/test.ogg", "/testMidi.mid", "/testXMidi.xmi"}};
    BOOST_REQUIRE_EQUAL(libsiedler2::Load(RTTR_LIBSIEDLER2_TEST_FILES_DIR "/testMono.wav", snd), 0);
    auto* effect = dynamic_cast<SoundEffectItem*>(snd[0]);
    BOOST_REQUIRE(effect);
    EffectPlayId id = effect->Play(50, false); //-V522
    BOOST_REQUIRE_EQUAL(MockupSoundDesc::numAlive, 1);
    BOOST_REQUIRE_EQUAL(effect->getLoadedType(), SD_EFFECT);
    BOOST_REQUIRE(id >= 0);
    BOOST_REQUIRE(AUDIODRIVER.IsEffectPlaying(id));
    AUDIODRIVER.StopEffect(id);
    BOOST_REQUIRE(!AUDIODRIVER.IsEffectPlaying(id));
    for(const auto& musicFile : musicFiles)
    {
        libsiedler2::Archiv musicArchiv;
        BOOST_REQUIRE_EQUAL(libsiedler2::Load(RTTR_LIBSIEDLER2_TEST_FILES_DIR + musicFile, musicArchiv), 0);
        auto* music = dynamic_cast<MusicItem*>(musicArchiv[0]);
        BOOST_REQUIRE(music);
        BOOST_REQUIRE_EQUAL(MockupSoundDesc::numAlive, 1);
        music->Play(0); //-V522
        BOOST_REQUIRE_EQUAL(MockupSoundDesc::numAlive, 2);
        BOOST_REQUIRE_EQUAL(music->getLoadedType(), SD_MUSIC);
        AUDIODRIVER.StopMusic();
    }
}

BOOST_AUTO_TEST_SUITE_END()
