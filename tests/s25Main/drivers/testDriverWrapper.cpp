// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
            BOOST_TEST(!driver.getName().empty());
            BOOST_TEST(bfs::exists(driver.getFile()));
        }
    }
}

BOOST_AUTO_TEST_CASE(AllAudioDriversAreLoadable)
{
    const auto drivers = drivers::DriverWrapper::LoadDriverList(drivers::DriverType::Audio);
    for(const auto& driver : drivers)
    {
        std::string preference = driver.getName();
        BOOST_TEST_CHECKPOINT("Loading " << preference);
        BOOST_TEST_REQUIRE(AUDIODRIVER.LoadDriver(preference));
        BOOST_TEST(preference == driver.getName());
        BOOST_TEST(AUDIODRIVER.getName() == driver.getName());
        AUDIODRIVER.UnloadDriver();
    }
}

BOOST_AUTO_TEST_CASE(AllVideoDriversAreLoadable)
{
    const auto drivers = drivers::DriverWrapper::LoadDriverList(drivers::DriverType::Video);
    for(const auto& driver : drivers)
    {
        std::string preference = driver.getName();
        BOOST_TEST_CHECKPOINT("Loading " << preference);
        BOOST_TEST_REQUIRE(VIDEODRIVER.LoadDriver(preference));
        BOOST_TEST(preference == driver.getName());
        BOOST_TEST(VIDEODRIVER.getName() == driver.getName());
        BOOST_TEST(VIDEODRIVER.getDriver());
        VIDEODRIVER.UnloadDriver();
    }
}
