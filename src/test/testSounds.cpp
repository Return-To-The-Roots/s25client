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

#include "defines.h" // IWYU pragma: keep
#include "MockupAudioDriver.h"
#include "driver/src/SoundHandle.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(SoundTests)

BOOST_AUTO_TEST_CASE(SoundHandles)
{
    SoundHandle handle;
    BOOST_REQUIRE(!handle.isValid());
    BOOST_REQUIRE_EQUAL(handle.getType(), SD_UNKNOWN);
    {
        MockupAudioDriver driver;
        {
            BOOST_REQUIRE_EQUAL(MockupSoundDesc::numAlive, 0);
            SoundHandle localHandle = driver.LoadEffect("Foo.wav");
            BOOST_REQUIRE_EQUAL(MockupSoundDesc::numAlive, 1);
            BOOST_REQUIRE(localHandle.isValid());
            BOOST_REQUIRE_EQUAL(localHandle.getType(), SD_EFFECT);
            SoundHandle localHandle2 = driver.LoadMusic("Foo.wav");
            BOOST_REQUIRE_EQUAL(MockupSoundDesc::numAlive, 2);
            BOOST_REQUIRE(localHandle2.isValid());
            BOOST_REQUIRE_EQUAL(localHandle2.getType(), SD_MUSIC);
            // Handles go out of scope -> Close them
        }
        BOOST_REQUIRE_EQUAL(MockupSoundDesc::numAlive, 0);
        handle = driver.LoadEffect("Foo.wav");
        BOOST_REQUIRE_EQUAL(MockupSoundDesc::numAlive, 1);
        BOOST_REQUIRE(handle.isValid());
        BOOST_REQUIRE_EQUAL(handle.getType(), SD_EFFECT);
        {
            // Copy handle
            SoundHandle localHandle = handle;
            BOOST_REQUIRE_EQUAL(MockupSoundDesc::numAlive, 1);
            // Copy goes out of scope
        }
        BOOST_REQUIRE(handle.isValid());
        BOOST_REQUIRE_EQUAL(MockupSoundDesc::numAlive, 1);
        // Driver goes out of scope
    }
    BOOST_REQUIRE(!handle.isValid());
    BOOST_REQUIRE_EQUAL(handle.getType(), SD_UNKNOWN);
}

BOOST_AUTO_TEST_SUITE_END()
