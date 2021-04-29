// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CollisionDetection.h"
#include "ogl/glSmartBitmap.h"
#include "ogl/glTexturePacker.h"
#include "uiHelper/uiHelpers.hpp"
#include "libsiedler2/ArchivItem_Bitmap_Raw.h"
#include "libsiedler2/PixelBufferBGRA.h"
#include <boost/test/unit_test.hpp>
#include <Rect.h>
#include <array>

BOOST_FIXTURE_TEST_SUITE(TexturePacker, uiHelper::Fixture)

BOOST_AUTO_TEST_CASE(SizeAndPosCorrect)
{
    std::array<libsiedler2::ArchivItem_Bitmap_Raw, 3> bmps;
    std::array<glSmartBitmap, 3> smartBmps;
    glTexturePacker packer;
    for(unsigned i = 0; i < bmps.size(); ++i)
    {
        libsiedler2::PixelBufferBGRA buffer(5 + i, 11 + i * 3, libsiedler2::ColorBGRA(0xFFFFFFFF));
        bmps[i].create(buffer);
        smartBmps[i].add(&bmps[i]);
        packer.add(smartBmps[i]);
    }
    BOOST_TEST_REQUIRE(packer.pack());

    // All textures must be the same
    const auto texture = smartBmps[0].getTexture();
    for(const auto& bmp : smartBmps)
    {
        BOOST_TEST(bmp.isGenerated());
        BOOST_TEST(bmp.getTexture() == texture);
    }

    // Sizes must be correct
    BOOST_TEST_REQUIRE(packer.getTextures().size() == 1u);
    const Point<float> size(packer.getTextures()[0].getSize());
    for(const auto& bmp : smartBmps)
    {
        const auto curTexSize = bmp.texCoords[2] - bmp.texCoords[0];
        BOOST_TEST(curTexSize.x == bmp.getRequiredTexSize().x / size.x);
        BOOST_TEST(curTexSize.y == bmp.getRequiredTexSize().y / size.y);
    }

    // Non-overlapping
    for(const auto& bmp : smartBmps)
    {
        Rect rect1(Position(bmp.texCoords[0] * size), Extent((bmp.texCoords[2] - bmp.texCoords[0]) * size));
        for(const auto& bmp2 : smartBmps)
        {
            if(&bmp == &bmp2)
                continue;

            Rect rect2(Position(bmp2.texCoords[0] * size), Extent((bmp2.texCoords[2] - bmp2.texCoords[0]) * size));
            BOOST_TEST(!DoRectsIntersect(rect1, rect2));
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
