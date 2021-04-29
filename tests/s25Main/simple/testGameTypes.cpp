// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ai/AIResource.h"
#include "helpers/EnumRange.h"
#include "gameTypes/GameTypesOutput.h"
#include "gameTypes/Resource.h"
#include "gameData/JobConsts.h"
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(GameTypes)

BOOST_AUTO_TEST_CASE(ResourceValues)
{
    {
        Resource res;
        BOOST_TEST(res.getType() == ResourceType::Nothing);
        BOOST_TEST(res.getAmount() == 0u);
    }
    {
        Resource res(ResourceType::Nothing, 5);
        BOOST_TEST(res.getType() == ResourceType::Nothing);
        BOOST_TEST(res.getAmount() == 0u);
    }

    // Basic value
    Resource res(ResourceType::Gold, 10);
    BOOST_TEST(res.getType() == ResourceType::Gold);
    BOOST_TEST(res.getAmount() == 10u);
    // Change type
    res.setType(ResourceType::Iron);
    BOOST_TEST(res.getType() == ResourceType::Iron);
    BOOST_TEST(res.getAmount() == 10u);
    // Amount
    res.setAmount(5);
    BOOST_TEST(res.getType() == ResourceType::Iron);
    BOOST_TEST(res.getAmount() == 5u);
    // Copy value
    Resource res2(res.getValue());
    BOOST_TEST(res.getType() == ResourceType::Iron);
    BOOST_TEST(res.getAmount() == 5u);
    // Set 0
    res2.setAmount(0);
    BOOST_TEST(res2.getType() == ResourceType::Iron);
    BOOST_TEST(res2.getAmount() == 0u);
    // Has
    BOOST_TEST(res.has(ResourceType::Iron));
    BOOST_TEST(!res.has(ResourceType::Gold));
    BOOST_TEST(!res2.has(ResourceType::Iron));
    BOOST_TEST(!res.has(ResourceType::Nothing));
    BOOST_TEST(!res2.has(ResourceType::Nothing));
    // Nothing -> 0
    BOOST_TEST_REQUIRE(res.getAmount() != 0u);
    res.setType(ResourceType::Nothing);
    BOOST_TEST(res.getType() == ResourceType::Nothing);
    BOOST_TEST(res.getAmount() == 0u);
    // And stays 0
    res.setAmount(10);
    BOOST_TEST(res.getType() == ResourceType::Nothing);
    BOOST_TEST(res.getAmount() == 0u);
    BOOST_TEST(!res.has(ResourceType::Iron));
    BOOST_TEST(!res.has(ResourceType::Nothing));
    // Overflow check
    res2.setAmount(15);
    BOOST_TEST(res2.getType() == ResourceType::Iron);
    BOOST_TEST(res2.getAmount() == 15u);
    res2.setAmount(17);
    BOOST_TEST(res2.getType() == ResourceType::Iron);
    // Unspecified
    BOOST_TEST(res2.getAmount() < 17u);
}

BOOST_AUTO_TEST_CASE(ResourceConvertToFromUInt8)
{
    for(const auto type : helpers::enumRange<ResourceType>())
    {
        for(auto amount : {1u, 5u, 15u})
        {
            const Resource res(type, amount);
            const Resource res2(static_cast<uint8_t>(res));
            BOOST_TEST(res2.getType() == type);
            if(type == ResourceType::Nothing)
                amount = 0u;
            BOOST_TEST(res2.getAmount() == amount);
        }
    }
    // Out of bounds -> Validated
    const Resource res(0xFF);
    BOOST_TEST(res.getType() == ResourceType::Nothing);
    BOOST_TEST(res.getAmount() == 0u);
}

BOOST_AUTO_TEST_CASE(NationSpecificJobBobs)
{
    // Helper is not nation specific
    BOOST_TEST(JOB_SPRITE_CONSTS[Job::Helper].getBobId(Nation::Vikings)
               == JOB_SPRITE_CONSTS[Job::Helper].getBobId(Nation::Africans));
    BOOST_TEST(JOB_SPRITE_CONSTS[Job::Helper].getBobId(Nation::Vikings)
               == JOB_SPRITE_CONSTS[Job::Helper].getBobId(Nation::Babylonians));
    // Soldiers are
    BOOST_TEST(JOB_SPRITE_CONSTS[Job::Private].getBobId(Nation::Vikings)
               != JOB_SPRITE_CONSTS[Job::Private].getBobId(Nation::Africans));
    // Non native nations come after native ones
    BOOST_TEST(JOB_SPRITE_CONSTS[Job::Private].getBobId(Nation::Vikings)
               < JOB_SPRITE_CONSTS[Job::Private].getBobId(Nation::Babylonians));
    // Same for scouts
    BOOST_TEST(JOB_SPRITE_CONSTS[Job::Scout].getBobId(Nation::Vikings)
               != JOB_SPRITE_CONSTS[Job::Scout].getBobId(Nation::Africans));
    BOOST_TEST(JOB_SPRITE_CONSTS[Job::Scout].getBobId(Nation::Vikings)
               < JOB_SPRITE_CONSTS[Job::Scout].getBobId(Nation::Babylonians));
}

BOOST_AUTO_TEST_CASE(AIResourcesMatchValues)
{
    // This simply tests that the convertToNodeResource function works, which is enough to verify the correctness of the
    // enumerator values

#define TEST_AIRESOURCE_SINGLE(s, EnumName, Enumerator)                                      \
    static_assert(convertToNodeResource(EnumName::Enumerator) == AINodeResource::Enumerator, \
                  "Mismatch for " BOOST_PP_STRINGIZE(EnumName) "::" BOOST_PP_STRINGIZE(Enumerator));

    // Generate a static assert for each enumerator
#define TEST_AIRESOURCE(EnumName, ...) \
    BOOST_PP_SEQ_FOR_EACH(TEST_AIRESOURCE_SINGLE, EnumName, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

    TEST_AIRESOURCE(AIResource, Gold, Ironore, Coal, Granite, Fish, Wood, Stones, Plantspace, Borderland)

    TEST_AIRESOURCE(AISurfaceResource, Wood, Stones, Blocked, Nothing)

    TEST_AIRESOURCE(AISubSurfaceResource, Gold, Ironore, Coal, Granite, Fish, Nothing)

    BOOST_TEST(true);
}

BOOST_AUTO_TEST_SUITE_END()
