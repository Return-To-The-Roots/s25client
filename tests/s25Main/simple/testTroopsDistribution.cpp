// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ai/aijh/runtime/TroopsDistribution.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(TroopsDistributionTests)

BOOST_AUTO_TEST_CASE(FairUsesRoundRobinTieBreak)
{
    const std::vector<unsigned> capacities = {3u, 3u, 3u};
    const std::vector<double> scores = {1.0, 1.0, 1.0};

    const std::vector<unsigned> limits = AIJH::ComputeTroopsDistributionLimits(capacities, scores, 4u, 1u);

    BOOST_TEST(limits.size() == 3u);
    BOOST_TEST(limits[0] == 1u);
    BOOST_TEST(limits[1] == 2u);
    BOOST_TEST(limits[2] == 1u);
}

BOOST_AUTO_TEST_CASE(WeightedScoresPreferMoreImportantBuilding)
{
    const std::vector<unsigned> capacities = {3u, 3u, 3u};
    const std::vector<double> scores = {2.0, 1.0, 0.0};

    const std::vector<unsigned> limits = AIJH::ComputeTroopsDistributionLimits(capacities, scores, 4u, 0u);

    BOOST_TEST(limits.size() == 3u);
    BOOST_TEST(limits[0] == 2u);
    BOOST_TEST(limits[1] == 1u);
    BOOST_TEST(limits[2] == 1u);
}

BOOST_AUTO_TEST_CASE(FrontierMultiplierCanPreferMidBuilding)
{
    const std::vector<unsigned> capacities = {3u, 3u};
    const std::vector<double> scores = {0.5, 2.0};

    const std::vector<unsigned> limits = AIJH::ComputeTroopsDistributionLimits(capacities, scores, 5u, 0u);

    BOOST_TEST(limits.size() == 2u);
    BOOST_TEST(limits[0] == 2u);
    BOOST_TEST(limits[1] == 3u);
}

BOOST_AUTO_TEST_SUITE_END()
