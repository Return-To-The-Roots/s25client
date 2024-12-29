// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Loader.h"
#include "PointOutput.h"
#include "RttrForeachPt.h"
#include "uiHelper/uiHelpers.hpp"
#include <libsiedler2/ArchivItem_Bitmap_Player.h>
#include <libsiedler2/ArchivItem_Bitmap_Raw.h>
#include <libsiedler2/ArchivItem_Palette.h>
#include <libsiedler2/PixelBufferBGRA.h>
#include <rttr/test/random.hpp>
#include <rttr/test/stubFunction.hpp>
#include <s25util/warningSuppression.h>
#include <glad/glad.h>
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

namespace rttrOglMock2 {
RTTR_IGNORE_DIAGNOSTIC("-Wmissing-declarations")

const Point<float>* texturePointer;
const Point<float>* vertexPointer;
std::vector<Point<float>> textureCoords;
std::vector<Point<float>> vertexCoords;
int callCount;

void APIENTRY glVertexPointer(GLint size, GLenum type, GLsizei stride, const void* pointer)
{
    ++callCount;
    RTTR_Assert(type == GL_FLOAT);
    RTTR_Assert(stride == 0);
    RTTR_Assert(size == 2);
    vertexPointer = static_cast<const Point<float>*>(pointer);
}

void APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
    ++callCount;
    RTTR_Assert(mode == GL_QUADS);
    RTTR_Assert(texturePointer != nullptr);

    textureCoords.resize(count);
    std::copy(&texturePointer[first], &texturePointer[first + count], textureCoords.begin());

    vertexCoords.resize(count);
    std::copy(&vertexPointer[first], &vertexPointer[first + count], vertexCoords.begin());
}

void APIENTRY glEnableClientState(GLenum array)
{
    RTTR_UNUSED(array);
    ++callCount;
}

void APIENTRY glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void* pointer)
{
    ++callCount;
    RTTR_Assert(type == GL_FLOAT);
    RTTR_Assert(stride == 0);
    RTTR_Assert(size == 2);
    texturePointer = static_cast<const Point<float>*>(pointer);
}

void APIENTRY glColorPointer(GLint size, GLenum type, GLsizei stride, const void* pointer)
{
    ++callCount;
    RTTR_UNUSED(size);
    RTTR_UNUSED(type);
    RTTR_UNUSED(stride);
    RTTR_UNUSED(pointer);
}

void APIENTRY glDisableClientState(GLenum array)
{
    ++callCount;
    RTTR_UNUSED(array);
}

void APIENTRY glBindTexture(GLenum target, GLuint texture)
{
    ++callCount;
    RTTR_UNUSED(target);
    RTTR_UNUSED(texture);
}

RTTR_POP_DIAGNOSTIC
} // namespace rttrOglMock2

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
            // LCOV_EXCL_START
            BOOST_TEST_INFO("Position" << pt);
            BOOST_TEST(buffer.get(pt.x, pt.y) != expectedColor);
            // LCOV_EXCL_STOP
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
            // LCOV_EXCL_START
            BOOST_TEST_INFO("Position" << pt);
            BOOST_TEST(buffer.get(pt.x, pt.y) != expectedColor);
            // LCOV_EXCL_STOP
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
            // LCOV_EXCL_START
            BOOST_TEST_INFO("Position" << pt);
            BOOST_TEST(buffer.get(pt.x, pt.y) != expectedColor);
            // LCOV_EXCL_STOP
        }
    }
}

BOOST_AUTO_TEST_CASE(DrawPercent)
{
    // TODO: check for proper ceil/floor behaviour

    auto bmpSrc = createRandBmp(0);
    const Extent size(bmpSrc->getWidth(), bmpSrc->getHeight());
    glSmartBitmap smartBmp;
    smartBmp.add(bmpSrc.get());

    RTTR_STUB_FUNCTION(glVertexPointer, rttrOglMock2::glVertexPointer);
    RTTR_STUB_FUNCTION(glDrawArrays, rttrOglMock2::glDrawArrays);
    RTTR_STUB_FUNCTION(glEnableClientState, rttrOglMock2::glEnableClientState);
    RTTR_STUB_FUNCTION(glTexCoordPointer, rttrOglMock2::glTexCoordPointer);
    RTTR_STUB_FUNCTION(glColorPointer, rttrOglMock2::glColorPointer);
    RTTR_STUB_FUNCTION(glDisableClientState, rttrOglMock2::glDisableClientState);
    RTTR_STUB_FUNCTION(glBindTexture, rttrOglMock2::glBindTexture);

    // drawPercent(0) shouldn't do a thing
    rttrOglMock2::callCount = 0;
    smartBmp.drawPercent(DrawPoint::all(0), 0);
    BOOST_TEST(rttrOglMock2::callCount == 0);

    smartBmp.drawPercent(DrawPoint::all(0), 50);
    BOOST_TEST(rttrOglMock2::textureCoords.size() == size_t(4));
    // All top texture coords have same Y component
    BOOST_TEST(rttrOglMock2::textureCoords[0].y == rttrOglMock2::textureCoords[3].y);
    // All bottom texture coords have same Y component
    BOOST_TEST(rttrOglMock2::textureCoords[1].y == rttrOglMock2::textureCoords[2].y);
    // Bottom starts at bottom
    BOOST_TEST(rttrOglMock2::textureCoords[1].y == smartBmp.texCoords[1].y);
    // Top starts at given percentage
    BOOST_TEST(rttrOglMock2::textureCoords[0].y == (smartBmp.texCoords[0].y + smartBmp.texCoords[1].y) / 2);

    smartBmp.drawPercent(DrawPoint::all(0), 100);
    BOOST_TEST(rttrOglMock2::textureCoords.size() == size_t(4));
    // All top texture coords have same Y component
    BOOST_TEST(rttrOglMock2::textureCoords[0].y == rttrOglMock2::textureCoords[3].y);
    // All bottom texture coords have same Y component
    BOOST_TEST(rttrOglMock2::textureCoords[1].y == rttrOglMock2::textureCoords[2].y);
    // Top texture Y components match top of the texture
    BOOST_TEST(rttrOglMock2::textureCoords[0].y == smartBmp.texCoords[0].y);
    // Bottom texture Y components match bottom of the texture
    BOOST_TEST(rttrOglMock2::textureCoords[1].y == smartBmp.texCoords[1].y);
}

BOOST_AUTO_TEST_SUITE_END()
