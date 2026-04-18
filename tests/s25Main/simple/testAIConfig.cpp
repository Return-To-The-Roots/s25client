// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ai/aijh/config/AIConfig.h"

#include <boost/test/unit_test.hpp>
#include <fstream>

BOOST_AUTO_TEST_SUITE(AIConfigTests)

BOOST_AUTO_TEST_CASE(TroopsDistributionStrategy_DefaultsToFair)
{
    AIConfig config;
    BOOST_TEST(static_cast<int>(config.troopsDistribution.strategy)
               == static_cast<int>(TroopsDistributionStrategy::Fair));
}

BOOST_AUTO_TEST_CASE(TroopsDistributionStrategy_CanBeParsed)
{
    const std::string configPath = "/tmp/rttr-ai-troops-distribution-strategy.yaml";
    {
        std::ofstream out(configPath);
        BOOST_TEST_REQUIRE(out.good());
        out << "troopsDistribution:\n";
        out << "  strategy: protected-building-value\n";
        out << "  frontierMultipliers:\n";
        out << "    Near: 1.1\n";
        out << "    Mid: 0.9\n";
    }

    AIConfig config;
    applyWeightsCfg(configPath, config);

    BOOST_TEST(static_cast<int>(config.troopsDistribution.strategy)
               == static_cast<int>(TroopsDistributionStrategy::ProtectedBuildingValue));
    BOOST_TEST(config.troopsDistribution.frontierMultipliers[FrontierDistance::Near] == 1.1);
    BOOST_TEST(config.troopsDistribution.frontierMultipliers[FrontierDistance::Mid] == 0.9);
    BOOST_TEST(config.troopsDistribution.frontierMultipliers[FrontierDistance::Far] == 1.0);
    BOOST_TEST(config.troopsDistribution.frontierMultipliers[FrontierDistance::Harbor] == 1.0);
}

BOOST_AUTO_TEST_SUITE_END()
