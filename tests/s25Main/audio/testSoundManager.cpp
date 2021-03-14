// Copyright (c) 2021 - 2021 Settlers Freaks (sf-team at siedler25.org)
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

#include "LoadMockupAudio.h"
#include "Loader.h"
#include "SoundManager.h"
#include "drivers/AudioDriverWrapper.h"
#include "helpers/Range.h"
#include "ogl/SoundEffectItem.h"
#include "nodeObjs/noBase.h"
#include "libsiedler2/Archiv.h"
#include "libsiedler2/ArchivItem.h"
#include "libsiedler2/enumTypes.h"
#include "rttr/test/MockClock.hpp"
#include "s25util/warningSuppression.h"
#include <rttr/test/TmpFolder.hpp>
#include <boost/filesystem.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/test/unit_test.hpp>

// LCOV_EXCL_START
BOOST_TEST_DONT_PRINT_LOG_VALUE(driver::SoundType)
// LCOV_EXCL_STOP

namespace {
class DummyEffect final : public libsiedler2::ArchivItem, public SoundEffectItem
{
public:
    DummyEffect() : libsiedler2::ArchivItem(libsiedler2::BobType::Sound) {}
    RTTR_CLONEABLE(DummyEffect) // LCOV_EXCL_LINE
protected:
    SoundHandle Load() override { return AUDIODRIVER.LoadEffect("foo.wav"); }
};

class DummyNO final : public noBase
{
public:
    DummyNO() : noBase(NodalObjectType::Animal) {}
    // LCOV_EXCL_START
    void Draw(DrawPoint) override {}
    void Destroy() override {}
    GO_Type GetGOT() const override { return GO_Type::Animal; }
    // LCOV_EXCL_STOP
};

struct LoadMockupAudioWithSounds : LoadMockupAudio
{
    LoadMockupAudioWithSounds()
    {
        rttr::test::TmpFolder tmpFolder;
        const auto soundPath = tmpFolder / boost::filesystem::path("sound");
        boost::filesystem::create_directory(soundPath);
        {
            // Just create any file so the loader has something to load
            boost::nowide::ofstream f(soundPath / "0.txt");
            f << "Test";
        }
        BOOST_TEST_REQUIRE(LOADER.Load(soundPath));
        libsiedler2::Archiv& sounds = LOADER.GetArchive("sound");
        sounds.alloc(115);
        for(int i = 50; i < 115; ++i)
            sounds.set(i, std::make_unique<DummyEffect>());
        MOCK_EXPECT(audioDriverMock->LoadEffect).calls(makeDoLoad(driver::SoundType::Effect));
        MOCK_EXPECT(audioDriverMock->doUnloadSound).calls(makeUnloadHandle(driver::SoundType::Effect));
    }
};
} // namespace

BOOST_AUTO_TEST_SUITE(SoundTests)

BOOST_FIXTURE_TEST_CASE(LimitNumberOfSameSounds, LoadMockupAudioWithSounds)
{
    {
        SoundManager manager;
        DummyNO obj1, obj2, obj3;
        int channel = 0;
        MOCK_EXPECT(audioDriverMock->doPlayEffect).exactly(SoundManager::maxPlayCtPerSound).returns(std::ref(channel));
        for(unsigned i : helpers::range(SoundManager::maxPlayCtPerSound))
        {
            manager.playNOSound(50, obj1, 10u + i);
            ++channel;
        }
        mock::verify();
        // Do not play the same sound again, even when from different objects
        for(const DummyNO& obj : {obj1, obj2, obj3})
            manager.playNOSound(50, obj, 20u);

        // Ignore sound from same object and same ID
        MOCK_EXPECT(audioDriverMock->doPlayEffect).once().returns(channel++);
        manager.playNOSound(51, obj1, 10u);
        manager.playNOSound(51, obj1, 10u);
        // But not from other object
        MOCK_EXPECT(audioDriverMock->doPlayEffect).once().returns(channel++);
        manager.playNOSound(51, obj2, 10u);
        mock::verify();
        // On destruction of the manager all sounds are stopped
        MOCK_EXPECT(audioDriverMock->doStopEffect).exactly(SoundManager::maxPlayCtPerSound + 2u);
    }
    mock::verify();
}

BOOST_FIXTURE_TEST_CASE(StopWorkingRemovesAllSoundsOfObj, LoadMockupAudioWithSounds)
{
    SoundManager manager;
    DummyNO obj1, obj2, obj3;
    MOCK_EXPECT(audioDriverMock->doPlayEffect).exactly(3).calls([channel = 0](auto&&...) mutable { return ++channel; });
    manager.playNOSound(50, obj1, 10u);
    manager.playNOSound(51, obj1, 10u);
    manager.playNOSound(50, obj2, 10u);
    MOCK_EXPECT(audioDriverMock->doStopEffect).exactly(2);
    manager.stopSounds(obj1);
    mock::verify();

    // Last sound remove even when soundeffects are disabled
    SETTINGS.sound.effectsEnabled = false;
    MOCK_EXPECT(audioDriverMock->doStopEffect).once();
    manager.stopSounds(obj2);
}

BOOST_FIXTURE_TEST_CASE(OceanSounds, LoadMockupAudioWithSounds)
{
    {
        SoundManager manager;
        // Don't play sounds for less than 10% water:
        for(auto waterPercent : helpers::range(10))
            manager.playOceanBrawling(waterPercent);

        // Play on 10% water
        int volume = -1;
        MOCK_EXPECT(audioDriverMock->IsEffectPlaying).once().with(EffectPlayId::Invalid).returns(false);
        MOCK_EXPECT(audioDriverMock->doPlayEffect).once().with(mock::any, mock::retrieve(volume), true).returns(0);
        manager.playOceanBrawling(10);
        mock::verify();
        BOOST_TEST(volume > 0);

        // If already playing don't change it or play again, just adjust volume
        MOCK_EXPECT(audioDriverMock->IsEffectPlaying).once().returns(true);
        MOCK_EXPECT(audioDriverMock->ChangeVolume).once().with(mock::any, volume);
        manager.playOceanBrawling(10);
        mock::verify();

        // More water -> Sound gets louder
        MOCK_EXPECT(audioDriverMock->IsEffectPlaying).once().returns(true);
        int oldVolume = volume;
        MOCK_EXPECT(audioDriverMock->ChangeVolume).once().with(mock::any, mock::retrieve(volume));
        manager.playOceanBrawling(20);
        mock::verify();
        BOOST_TEST(volume > oldVolume);

        // Max volume
        MOCK_EXPECT(audioDriverMock->IsEffectPlaying).once().returns(true);
        oldVolume = volume;
        MOCK_EXPECT(audioDriverMock->ChangeVolume).once().with(mock::any, mock::retrieve(volume));
        manager.playOceanBrawling(100);
        mock::verify();
        BOOST_TEST(volume > oldVolume);
        BOOST_TEST(volume <= 255);

        // Less water --> Stop effect
        MOCK_EXPECT(audioDriverMock->doStopEffect).once();
        manager.playOceanBrawling(9);
        mock::verify();

        // Play again after old effect was stopped
        MOCK_EXPECT(audioDriverMock->IsEffectPlaying).once().returns(false);
        MOCK_EXPECT(audioDriverMock->doPlayEffect).once().returns(1);
        manager.playOceanBrawling(10);
        mock::verify();

        // Trigger a known bug in SDL_mixer where IsEffectPlaying might wrongly return false but effect is still playing
        MOCK_EXPECT(audioDriverMock->IsEffectPlaying).once().returns(false);
        MOCK_EXPECT(audioDriverMock->doStopEffect).once(); // It should make sure the effect is actually stopped
        MOCK_EXPECT(audioDriverMock->doPlayEffect).once().returns(1);
        manager.playOceanBrawling(10);
        mock::verify();

        // Stop the ocean sound on destroy
        MOCK_EXPECT(audioDriverMock->doStopEffect).once();
    }
}

BOOST_FIXTURE_TEST_CASE(BirdSounds, LoadMockupAudioWithSounds)
{
    rttr::test::MockClockFixture mockClock;
    using namespace std::chrono_literals;
    {
        SoundManager manager;
        // Don't play bird sounds if there are no trees
        manager.playBirdSounds(0);

        // Now play (increase time by a lot to trigger replay)
        mockClock.currentTime += 10s;
        MOCK_EXPECT(audioDriverMock->doPlayEffect).once().returns(0);
        manager.playBirdSounds(1);

        // for a lot of trees, the bird sound should play at least once per second
        for(const auto i : helpers::range(100))
        {
            RTTR_UNUSED(i);
            mockClock.currentTime += 1s;
            MOCK_EXPECT(audioDriverMock->doPlayEffect).once().returns(0);
            manager.playBirdSounds(10000);
        }

        // Stop the bird sound on destroy
        MOCK_EXPECT(audioDriverMock->doStopEffect).once();
    }
}

BOOST_FIXTURE_TEST_CASE(AnimalSounds, LoadMockupAudioWithSounds)
{
    {
        SoundManager manager;
        // Check that the same sound is played as long as allowed
        int channel = 0;
        MOCK_EXPECT(audioDriverMock->doPlayEffect).exactly(SoundManager::maxPlayCtPerSound).returns(std::ref(channel));
        for(unsigned i : helpers::range(SoundManager::maxPlayCtPerSound))
        {
            // All previously added sounds are checked
            if(i)
                MOCK_EXPECT(audioDriverMock->IsEffectPlaying).exactly(i).returns(true);
            manager.playAnimalSound(50);
            ++channel;
        }
        // Do not play the same sound again
        MOCK_EXPECT(audioDriverMock->IsEffectPlaying).exactly(SoundManager::maxPlayCtPerSound).returns(true);
        manager.playAnimalSound(50);
        mock::verify();

        // Can play another sound
        MOCK_EXPECT(audioDriverMock->doPlayEffect).once().returns(channel++);
        manager.playAnimalSound(51);
        mock::verify();

        // If one sound has ended, we can play another one
        MOCK_EXPECT(audioDriverMock->doPlayEffect).once().returns(channel++);
        MOCK_EXPECT(audioDriverMock->IsEffectPlaying).once().returns(false);
        MOCK_EXPECT(audioDriverMock->IsEffectPlaying).exactly(SoundManager::maxPlayCtPerSound - 1u).returns(true);
        manager.playAnimalSound(50);
        mock::verify();

        // Sounds are stopped once the manager gets destroyed
        MOCK_EXPECT(audioDriverMock->doStopEffect).exactly(SoundManager::maxPlayCtPerSound + 1);
    }
}

BOOST_AUTO_TEST_SUITE_END()
