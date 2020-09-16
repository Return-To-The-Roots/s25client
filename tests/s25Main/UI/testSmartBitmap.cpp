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

#include "Loader.h"
#include "PointOutput.h"
#include "RttrForeachPt.h"
#include "uiHelper/uiHelpers.hpp"
#include <libsiedler2/ArchivItem_Bitmap_Player.h>
#include <libsiedler2/ArchivItem_Bitmap_Raw.h>
#include <libsiedler2/ArchivItem_Palette.h>
#include <libsiedler2/PixelBufferBGRA.h>
#include <rttr/test/random.hpp>
#include <boost/test/unit_test.hpp>
#include <iomanip>
#include <locale>
#include <sstream>

using namespace libsiedler2;

namespace libsiedler2 {
static std::ostream& boost_test_print_type(std::ostream& os, const ColorBGRA color)
{
    os.imbue(std::locale::classic());
    return os << std::hex << std::setw(8) << color.asValue();
}
} // namespace libsiedler2

namespace {

std::unique_ptr<ArchivItem_Bitmap_Raw> createRandBmp(unsigned percentTransparent)
{
    const auto* pal = LOADER.GetPaletteN("pal5");
    auto bmp = std::make_unique<ArchivItem_Bitmap_Raw>();
    const Extent size = rttr::test::randomPoint<Extent>(10, 50);
    bmp->init(size.x, size.y, TextureFormat::BGRA);
    const auto offset = rttr::test::randomPoint<Position>(-100, 100);
    bmp->setNx(offset.x);
    bmp->setNy(offset.y);
    using rttr::test::randomValue;
    RTTR_FOREACH_PT(Position, size)
    {
        if(randomValue(1u, 100u) > percentTransparent)
            bmp->setPixel(pt.x, pt.y, pal->get(randomValue(0, 127)));
    }
    return bmp;
}
std::unique_ptr<ArchivItem_Bitmap_Player> createRandPlayerBmp(unsigned percentTransparent)
{
    const auto* pal = LOADER.GetPaletteN("colors");
    auto bmp = std::make_unique<ArchivItem_Bitmap_Player>();
    const auto size = rttr::test::randomPoint<Extent>(10, 50);
    const auto offset = rttr::test::randomPoint<Position>(-100, 100);
    bmp->setNx(offset.x);
    bmp->setNy(offset.y);
    using rttr::test::randomValue;
    PixelBufferBGRA buffer(size.x, size.y);
    RTTR_FOREACH_PT(Position, size)
    {
        if(randomValue(1u, 100u) > percentTransparent)
        {
            unsigned idx = randomValue(1u, 100u) > 60 ?
                             128 + randomValue(0, ArchivItem_Bitmap_Player::numPlayerClrs - 1) :
                             randomValue(0, 127);
            buffer.set(pt.x, pt.y, pal->get(idx));
        }
    }
    bmp->create(buffer, pal);
    return bmp;
}
} // namespace

BOOST_AUTO_TEST_CASE(PrintColor)
{
    std::stringstream s;
    boost_test_print_type(s, ColorBGRA(0x42, 0x13, 0x37, 0x99));
    BOOST_TEST(s.str() == "99371342");
}

BOOST_FIXTURE_TEST_SUITE(SmartBitmapTestSuite, uiHelper::Fixture)

BOOST_AUTO_TEST_CASE(CreateRandBmp_Works)
{
    const auto bmp = createRandBmp(50);
    BOOST_TEST(bmp->getWidth() > 0);
    BOOST_TEST(bmp->getHeight() > 0);
    const auto bmpPl = createRandPlayerBmp(50);
    BOOST_TEST(bmpPl->getWidth() > 0);
    BOOST_TEST(bmpPl->getHeight() > 0);
}

BOOST_AUTO_TEST_CASE(RegularBitmap)
{
    // Fully opaque bitmap so origin and size match
    auto bmpSrc = createRandBmp(0);
    const Extent size(bmpSrc->getWidth(), bmpSrc->getHeight());
    const auto offset = rttr::test::randomPoint<Extent>(0, 100);
    PixelBufferBGRA buffer(size.x * 2 + offset.x, size.y * 2 + offset.y);
    glSmartBitmap smartBmp;
    smartBmp.add(bmpSrc.get());
    BOOST_TEST(smartBmp.GetSize() == size);
    BOOST_TEST(smartBmp.getRequiredTexSize() == size);
    BOOST_TEST(smartBmp.GetOrigin() == Position(bmpSrc->getNx(), bmpSrc->getNy()));
    smartBmp.drawTo(buffer, offset);
    Position iSize(size);
    RTTR_FOREACH_PT(Position, Extent(buffer.getWidth(), buffer.getHeight()))
    {
        const Position bmpPos = pt - offset;
        if(bmpPos.x < 0 || bmpPos.y < 0 || bmpPos.x >= iSize.x || bmpPos.y >= iSize.y)
            BOOST_TEST(buffer.get(pt.x, pt.y) == ColorBGRA());
        else
            BOOST_TEST(buffer.get(pt.x, pt.y) == bmpSrc->getPixel(bmpPos.x, bmpPos.y));
    }
}

BOOST_AUTO_TEST_CASE(MultiRegularBitmap)
{
    std::vector<std::unique_ptr<ArchivItem_Bitmap_Raw>> bmps;
    glSmartBitmap smartBmp;
    for(int i = 0; i < 3; i++)
    {
        bmps.emplace_back(createRandBmp(70));
        smartBmp.add(bmps.back().get());
    }
    Position commonOrigin(bmps[0]->getNx(), bmps[0]->getNy());
    for(const auto& bmp : bmps)
    {
        commonOrigin.x = std::max<int>(commonOrigin.x, bmp->getNx());
        commonOrigin.y = std::max<int>(commonOrigin.y, bmp->getNy());
    }
    Extent size = smartBmp.GetSize();
    for(const auto& bmp : bmps)
    {
        size.x = std::max<unsigned>(size.x, bmp->getWidth() + commonOrigin.x - bmp->getNx());
        size.y = std::max<unsigned>(size.y, bmp->getHeight() + commonOrigin.y - bmp->getNy());
    }
    BOOST_TEST(smartBmp.GetSize() == smartBmp.getRequiredTexSize());
    // Note: We don't test size and origin as they may be adjusted for optimization purposes
    // which is considered an internal detail
    commonOrigin = smartBmp.GetOrigin();

    const auto offset = rttr::test::randomPoint<Extent>(0, 100);
    // Draw to a large buffer (detect pixels at wrong positions)
    PixelBufferBGRA buffer(size.x * 2 + offset.x, size.y * 2 + offset.y);
    smartBmp.drawTo(buffer, offset);
    RTTR_FOREACH_PT(Position, Extent(buffer.getWidth(), buffer.getHeight()))
    {
        const Position curPos = pt - offset;
        // Bitmaps are drawn in order added, so final pixel value is determined by all bitmaps at that position
        ColorBGRA expectedColor;
        for(const auto& bmp : bmps)
        {
            // Drawing buffer at `pos` - commonOrigin should be equal to drawing bmp at `pos` - origin
            const Position curOriginOffset = Position(bmp->getNx(), bmp->getNy()) - commonOrigin;
            const Position bmpPos = curPos + curOriginOffset;
            if(bmpPos.x >= 0 && bmpPos.y >= 0 && bmpPos.x < bmp->getWidth() && bmpPos.y < bmp->getHeight())
            {
                const ColorBGRA color = bmp->getPixel(bmpPos.x, bmpPos.y);
                if(color.getAlpha() != 0)
                    expectedColor = color;
            }
        }
        if(buffer.get(pt.x, pt.y) != expectedColor)
        {
            BOOST_TEST_INFO("Position" << pt);
            BOOST_TEST(buffer.get(pt.x, pt.y) != expectedColor);
        }
    }
}

BOOST_AUTO_TEST_CASE(PlayerBitmap)
{
    // Fully opaque bitmap so origin and size match
    auto bmp = createRandPlayerBmp(0);
    const auto* pal = LOADER.GetPaletteN("colors");
    const Extent size(bmp->getWidth(), bmp->getHeight());
    const auto offset = rttr::test::randomPoint<Extent>(0, 100);
    glSmartBitmap smartBmp;
    smartBmp.add(bmp.get());
    BOOST_TEST(smartBmp.GetSize() == size);
    BOOST_TEST(smartBmp.getRequiredTexSize() == Extent(size.x * 2, size.y));
    BOOST_TEST(smartBmp.GetOrigin() == Position(bmp->getNx(), bmp->getNy()));
    PixelBufferBGRA buffer(size.x * 4 + offset.x, size.y * 4 + offset.y);
    smartBmp.drawTo(buffer, offset);
    RTTR_FOREACH_PT(Position, Extent(buffer.getWidth(), buffer.getHeight()))
    {
        const Position bmpPos = pt - offset;
        ColorBGRA expectedColor;
        if(bmpPos.x >= 0 && bmpPos.y >= 0 && bmpPos.y < bmp->getHeight())
        {
            if(bmpPos.x >= static_cast<int>(smartBmp.GetSize().x))
            {
                // Player color
                const int plClrPos = bmpPos.x - smartBmp.GetSize().x;
                if(plClrPos < bmp->getWidth() && bmp->isPlayerColor(plClrPos, bmpPos.y))
                {
                    expectedColor = pal->get(bmp->getPlayerColorIdx(plClrPos, bmpPos.y) + 128);
                }
            } else if(bmpPos.x < bmp->getWidth())
                expectedColor = bmp->getPixel(bmpPos.x, bmpPos.y);
        }
        if(buffer.get(pt.x, pt.y) != expectedColor)
        {
            BOOST_TEST_INFO("Position" << pt);
            BOOST_TEST(buffer.get(pt.x, pt.y) != expectedColor);
        }
    }
}

BOOST_AUTO_TEST_CASE(MultiPlayerBitmap)
{
    std::vector<std::unique_ptr<ArchivItem_Bitmap_Player>> bmps;
    glSmartBitmap smartBmp;
    for(int i = 0; i < 3; i++)
    {
        bmps.emplace_back(createRandPlayerBmp(50));
        smartBmp.add(bmps.back().get());
    }

    Extent size = smartBmp.GetSize();
    BOOST_TEST(smartBmp.getRequiredTexSize() == Extent(size.x * 2, size.y));

    Position commonOrigin(bmps[0]->getNx(), bmps[0]->getNy());
    for(const auto& bmp : bmps)
    {
        commonOrigin.x = std::max<int>(commonOrigin.x, bmp->getNx());
        commonOrigin.y = std::max<int>(commonOrigin.y, bmp->getNy());
    }
    for(const auto& bmp : bmps)
    {
        size.x = std::max<unsigned>(size.x, bmp->getWidth() + commonOrigin.x - bmp->getNx());
        size.y = std::max<unsigned>(size.y, bmp->getHeight() + commonOrigin.y - bmp->getNy());
    }
    // Note: We don't test size and origin as they may be adjusted for optimization purposes
    // which is considered an internal detail
    commonOrigin = smartBmp.GetOrigin();

    const auto offset = rttr::test::randomPoint<Extent>(0, 100);
    // Draw to a large buffer (detect pixels at wrong positions)
    PixelBufferBGRA buffer(size.x * 4 + offset.x, size.y * 4 + offset.y);
    smartBmp.drawTo(buffer, offset);
    const auto* pal = LOADER.GetPaletteN("colors");
    RTTR_FOREACH_PT(Position, Extent(buffer.getWidth(), buffer.getHeight()))
    {
        const Position curPos = pt - offset;
        const bool isPlayerClrRegion =
          (curPos.x >= static_cast<int>(smartBmp.GetSize().x) && curPos.x < static_cast<int>(smartBmp.GetSize().x) * 2);
        // Bitmaps are drawn in order added, so final pixel value is determined by all bitmaps at that position
        ColorBGRA expectedColor;
        for(const auto& bmp : bmps)
        {
            // Drawing buffer at `pos` - commonOrigin should be equal to drawing bmp at `pos` - origin
            const Position curOriginOffset = Position(bmp->getNx(), bmp->getNy()) - commonOrigin;
            const Position bmpPos = curPos + curOriginOffset - Extent(isPlayerClrRegion ? smartBmp.GetSize().x : 0, 0);
            if(bmpPos.x >= 0 && bmpPos.y >= 0 && bmpPos.x < bmp->getWidth() && bmpPos.y < bmp->getHeight())
            {
                const ColorBGRA color = bmp->getPixel(bmpPos.x, bmpPos.y);
                if(color.getAlpha() != 0)
                {
                    // Color overwrite (make transparent) player clr
                    expectedColor = isPlayerClrRegion ? ColorBGRA() : color;
                }
                if(bmp->isPlayerColor(bmpPos.x, bmpPos.y))
                    expectedColor = pal->get(bmp->getPlayerColorIdx(bmpPos.x, bmpPos.y) + 128);
            }
        }
        if(buffer.get(pt.x, pt.y) != expectedColor)
        {
            BOOST_TEST_INFO("Position" << pt);
            BOOST_TEST(buffer.get(pt.x, pt.y) != expectedColor);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
