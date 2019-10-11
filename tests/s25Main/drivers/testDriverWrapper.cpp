// Copyright (c) 2016 - 2018 Settlers Freaks (sf-team at siedler25.org)
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

#include "drivers/AudioDriverWrapper.h"
#include "drivers/VideoDriverWrapper.h"
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(GetDriverList)
{
    for(auto type : {drivers::DriverType::Video, drivers::DriverType::Audio})
    {
        const auto drivers = drivers::DriverWrapper::LoadDriverList(type);
        BOOST_TEST(!drivers.empty());
        for(const auto& driver : drivers)
        {
            BOOST_TEST(!driver.GetName().empty());
            BOOST_TEST(bfs::exists(driver.GetFile()));
        }
    }
}

BOOST_AUTO_TEST_CASE(AllAudioDriversAreLoadable)
{
    const auto drivers = drivers::DriverWrapper::LoadDriverList(drivers::DriverType::Audio);
    for(const auto& driver : drivers)
    {
        std::string preference = driver.GetName();
        BOOST_TEST_CHECKPOINT("Loading " << preference);
        BOOST_TEST_REQUIRE(AUDIODRIVER.LoadDriver(preference));
        BOOST_TEST(preference == driver.GetName());
        BOOST_TEST(AUDIODRIVER.GetName() == driver.GetName());
        AUDIODRIVER.UnloadDriver();
    }
}

BOOST_AUTO_TEST_CASE(AllVideoDriversAreLoadable)
{
    const auto drivers = drivers::DriverWrapper::LoadDriverList(drivers::DriverType::Video);
    for(const auto& driver : drivers)
    {
        std::string preference = driver.GetName();
        BOOST_TEST_CHECKPOINT("Loading " << preference);
        BOOST_TEST_REQUIRE(VIDEODRIVER.LoadDriver(preference));
        BOOST_TEST(preference == driver.GetName());
        BOOST_TEST(VIDEODRIVER.GetName() == driver.GetName());
        BOOST_TEST(VIDEODRIVER.GetDriver());
        VIDEODRIVER.UnloadDriver();
    }
}
