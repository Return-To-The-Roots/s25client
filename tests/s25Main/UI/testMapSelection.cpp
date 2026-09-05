// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Loader.h"
#include "RttrForeachPt.h"
#include "controls/ctrlMapSelection.h"
#include "driver/MouseCoords.h"
#include "helpers/OptionalIO.h"
#include "uiHelper/uiHelpers.hpp"
#include "rttr/test/TmpFolder.hpp"
#include <libsiedler2/Archiv.h>
#include <libsiedler2/ArchivItem_Bitmap_Raw.h>
#include <libsiedler2/PixelBufferBGRA.h>
#include <libsiedler2/libsiedler2.h>
#include <rttr/test/random.hpp>
#include <turtle/mock.hpp>

using namespace rttr::test;

namespace {
MOCK_BASE_CLASS(TestWindow, Window)
{public: TestWindow(): Window(nullptr, randomValue<unsigned>(), DrawPoint(0, 0)){}};

std::unique_ptr<libsiedler2::Archiv> createBitmapFromBuffer(libsiedler2::PixelBufferBGRA const& buffer,
                                                            const libsiedler2::ArchivItem_Palette* palette = nullptr)
{
    using namespace libsiedler2;
    auto bmpRaw = std::make_unique<ArchivItem_Bitmap_Raw>();
    bmpRaw->create(buffer, palette);
    auto bmp = std::make_unique<libsiedler2::Archiv>();
    bmp->push(std::move(bmpRaw));
    return bmp;
}

std::unique_ptr<libsiedler2::Archiv> createBitmapWithOneColor(Extent const& size, uint32_t const fillColor,
                                                              const libsiedler2::ArchivItem_Palette* palette = nullptr)
{
    using namespace libsiedler2;
    return createBitmapFromBuffer(PixelBufferBGRA(size.x, size.y, ColorBGRA(fillColor)), palette);
}

bool storeBitmap(boost::filesystem::path const& storePath, std::unique_ptr<libsiedler2::Archiv> const& bitmap)
{
    return libsiedler2::Write(storePath, *bitmap);
}

libsiedler2::PixelBufferBGRA createMapTestData(Extent const& size)
{
    /* Map Layout:
        TB
        BB
    */
    using namespace libsiedler2;
    PixelBufferBGRA buffer(size.x, size.y);
    RTTR_FOREACH_PT(Position, size)
    {
        if(pt.x >= static_cast<int>(size.x) / 2 || pt.y >= static_cast<int>(size.y) / 2)
            buffer.set(pt.x, pt.y, ColorBGRA(0xff0000ff));
    }
    return buffer;
}

libsiedler2::PixelBufferBGRA createMissionMapMaskTestData(Extent const& size)
{
    /* Map Layout:
        TR
        GB
    */
    using namespace libsiedler2;
    PixelBufferBGRA buffer(size.x, size.y);
    RTTR_FOREACH_PT(Position, size)
    {
        if(pt.x >= static_cast<int>(size.x) / 2 && pt.y < static_cast<int>(size.y) / 2)
            buffer.set(pt.x, pt.y, ColorBGRA(0xffff0000));
        if(pt.x < static_cast<int>(size.x) / 2 && pt.y >= static_cast<int>(size.y) / 2)
            buffer.set(pt.x, pt.y, ColorBGRA(0xff00ff00));
        if(pt.x >= static_cast<int>(size.x) / 2 && pt.y >= static_cast<int>(size.y) / 2)
            buffer.set(pt.x, pt.y, ColorBGRA(0xff0000ff));
    }
    return buffer;
}

SelectionMapInputData createInputForSelectionMap(rttr::test::TmpFolder const& tmp, Extent backgroundSize = {100, 100},
                                                 Extent mapSize = {100, 100},
                                                 Position mapOffsetInBackground = Position(0, 0),
                                                 Extent overlaySize = {5, 5})
{
    uint32_t const disabledColor = 0x70000000;
    uint32_t const red = 0xffff0000;
    uint32_t const green = 0xff00ff00;
    uint32_t const blue = 0xff0000ff;

    auto backgroundPath = tmp / "background.bmp";
    auto mapPath = tmp / "map.bmp";
    auto missionmapmaskPath = tmp / "missionmapmask.bmp";
    auto markerPath = tmp / "marker.bmp";
    auto conqueredPath = tmp / "conquered.bmp";

    storeBitmap(backgroundPath, createBitmapWithOneColor(backgroundSize, red));
    storeBitmap(mapPath, createBitmapFromBuffer(createMapTestData(mapSize)));
    storeBitmap(missionmapmaskPath, createBitmapFromBuffer(createMissionMapMaskTestData(mapSize)));
    storeBitmap(markerPath, createBitmapWithOneColor(overlaySize, green));
    storeBitmap(conqueredPath, createBitmapWithOneColor(overlaySize, red));

    SelectionMapInputData selectionMapInputData;
    selectionMapInputData.background = {backgroundPath.string(), 0};
    selectionMapInputData.map = {mapPath.string(), 0};
    selectionMapInputData.missionMapMask = {missionmapmaskPath.string(), 0};
    selectionMapInputData.marker = {markerPath.string(), 0};
    selectionMapInputData.conquered = {conqueredPath.string(), 0};
    selectionMapInputData.mapOffsetInBackground = mapOffsetInBackground;
    selectionMapInputData.disabledColor = disabledColor;
    selectionMapInputData.missionSelectionInfos = {{red, {Position((mapSize.x * 3) / 4, mapSize.y / 4)}},
                                                   {green, {Position(mapSize.x / 4, (mapSize.y * 3) / 4)}},
                                                   {blue, {Position((mapSize.x * 3) / 4, (mapSize.y * 3) / 4)}}};
    return selectionMapInputData;
}

void testMapSelection(Position const& windowPos, Extent const& windowExtent, Extent const& mapBackgroundSize,
                      Extent const& mapSize, Position const& mapOffsetInBackground)
{
    TestWindow wnd;
    Position offsetOfBackground = (windowExtent - mapBackgroundSize) / 2;

    const Position firstMissionMidPoint((mapSize.x * 3) / 4, mapSize.y / 4);
    const Position secondMissionMidPoint(mapSize.x / 4, (mapSize.y * 3) / 4);
    const Position thirdMissionMidPoint((mapSize.x * 3) / 4, (mapSize.y * 3) / 4);
    const Position freeQuadrantMidPoint(mapSize.x / 4, mapSize.y / 4);

    rttr::test::TmpFolder tmp;
    ctrlMapSelection ms(&wnd, 0, windowPos, windowExtent,
                        createInputForSelectionMap(tmp, mapBackgroundSize, mapSize, mapOffsetInBackground));

    // Activate first mission
    ms.setMissionsStatus({{true, false}, {false, false}, {false, false}});
    // Initial the selection is not set
    BOOST_TEST(!ms.getSelection());
    // Click on non mission area => selection will not change
    ms.Msg_LeftUp(MouseCoords(ms.GetPos() + offsetOfBackground + freeQuadrantMidPoint));
    BOOST_TEST(!ms.getSelection());
    // Click on mission area of first mission => selection will change because mission is playable
    ms.Msg_LeftUp(MouseCoords(ms.GetPos() + offsetOfBackground + firstMissionMidPoint));
    BOOST_TEST(ms.getSelection() == 0u);
    // Click on mission area of second mission => selection will not change because mission is not playable
    ms.Msg_LeftUp(MouseCoords(ms.GetPos() + offsetOfBackground + secondMissionMidPoint));
    BOOST_TEST(ms.getSelection() == 0u);
    // Click on mission area of third mission => selection will not change because mission is not playable
    ms.Msg_LeftUp(MouseCoords(ms.GetPos() + offsetOfBackground + thirdMissionMidPoint));
    BOOST_TEST(ms.getSelection() == 0u);

    // Activate first and second mission
    ms.setMissionsStatus({{true, false}, {true, false}, {false, false}});
    // Selection stays the same as before
    BOOST_TEST(ms.getSelection() == 0u);
    // Click on mission area of second mission => selection will change because mission is playable
    ms.Msg_LeftUp(MouseCoords(ms.GetPos() + offsetOfBackground + secondMissionMidPoint));
    BOOST_TEST(ms.getSelection() == 1u);
    // Click on mission area of third mission => selection will not change because mission is not playable
    ms.Msg_LeftUp(MouseCoords(ms.GetPos() + offsetOfBackground + thirdMissionMidPoint));
    BOOST_TEST(ms.getSelection() == 1u);
    // Click on non mission area => selection will not change
    ms.Msg_LeftUp(MouseCoords(ms.GetPos() + offsetOfBackground + freeQuadrantMidPoint));
    BOOST_TEST(ms.getSelection() == 1u);
    // Click on mission area of first mission => selection will change because mission is playable
    ms.Msg_LeftUp(MouseCoords(ms.GetPos() + offsetOfBackground + firstMissionMidPoint));
    BOOST_TEST(ms.getSelection() == 0u);

    // Test selection
    // Activate first and second mission
    ms.setMissionsStatus({{true, false}, {true, false}, {false, false}});
    // Setting selection to third mission not possible because mission is not playable => stay at previous value
    ms.setSelection(2);
    BOOST_TEST(ms.getSelection() == 0u);
    // Setting selection to second mission is possible because mission is playable
    ms.setSelection(1);
    BOOST_TEST(ms.getSelection() == 1u);
    // Setting selection to invalid mission resets selection to not set
    ms.setSelection(3);
    BOOST_TEST(!ms.getSelection());
    // Setting selection to third mission not possible because mission is not playable => stay at previous value
    ms.setSelection(2);
    BOOST_TEST(!ms.getSelection());
    // Setting selection to first mission is possible because mission is playable
    ms.setSelection(0);
    BOOST_TEST(ms.getSelection() == 0u);

    // Activate all missions
    ms.setMissionsStatus({{true, false}, {true, false}, {true, false}});

    // Preview mode active selection cannot change
    ms.setSelection(0);
    ms.setPreview(true);
    BOOST_TEST(ms.getSelection() == 0u);
    ms.Msg_LeftUp(MouseCoords(ms.GetPos() + offsetOfBackground + secondMissionMidPoint));
    BOOST_TEST(ms.getSelection() == 0u);
    ms.Msg_LeftUp(MouseCoords(ms.GetPos() + offsetOfBackground + thirdMissionMidPoint));
    BOOST_TEST(ms.getSelection() == 0u);
    ms.Msg_LeftUp(MouseCoords(ms.GetPos() + offsetOfBackground + freeQuadrantMidPoint));
    BOOST_TEST(ms.getSelection() == 0u);
    ms.Msg_LeftUp(MouseCoords(ms.GetPos() + offsetOfBackground + firstMissionMidPoint));
    BOOST_TEST(ms.getSelection() == 0u);
    ms.setPreview(false);

    // Click all four sides and corners and midpoint of each mission to ensure mapping of areas is correct
    std::vector<Position> midPoints = {firstMissionMidPoint, secondMissionMidPoint, thirdMissionMidPoint};
    for(size_t mid = 0; mid < midPoints.size(); mid++)
    {
        for(int i = -1; i <= 1; i++)
        {
            for(int j = -1; j <= 1; j++)
            {
                Position borderPoint(midPoints[mid].x + i * (mapSize.x / 4), midPoints[mid].y + j * (mapSize.y / 4));
                if(i == 1)
                    borderPoint.x--;
                if(j == 1)
                    borderPoint.y--;

                const unsigned deselectIndex = (mid + 1) < midPoints.size() ? (mid + 1) : 0;
                ms.Msg_LeftUp(MouseCoords(ms.GetPos() + offsetOfBackground + midPoints[deselectIndex]));
                BOOST_TEST(ms.getSelection() == deselectIndex);
                ms.Msg_LeftUp(MouseCoords(ms.GetPos() + offsetOfBackground + borderPoint));
                BOOST_TEST(ms.getSelection() == mid);
            }
        }
    }
}
} // namespace

BOOST_FIXTURE_TEST_SUITE(MapSelection, uiHelper::Fixture)

BOOST_AUTO_TEST_CASE(MapNoOffsetAndSameSize)
{
    Position const windowPos(0, 0);
    Extent const windowSize(100, 100);
    Extent const mapBackgroundSize = {100, 100};
    Extent const mapSize = {100, 100};
    Position const mapOffsetInBackground = Position(0, 0);
    testMapSelection(windowPos, windowSize, mapBackgroundSize, mapSize, mapOffsetInBackground);
}

BOOST_AUTO_TEST_CASE(MapNoOffsetAndDifferentSizeInEachDirection)
{
    Position const windowPos(0, 0);
    Extent const windowSize(200, 100);
    Extent const mapBackgroundSize = {200, 100};
    Extent const mapSize = {200, 100};
    Position const mapOffsetInBackground = Position(0, 0);
    testMapSelection(windowPos, windowSize, mapBackgroundSize, mapSize, mapOffsetInBackground);
}

BOOST_AUTO_TEST_CASE(MapNoOffsetAndDifferentSizeInEachDirectionPlusWindowOffset)
{
    Position const windowPos(10, 10);
    Extent const windowSize(200, 100);
    Extent const mapBackgroundSize = {200, 100};
    Extent const mapSize = {200, 100};
    Position const mapOffsetInBackground = Position(0, 0);
    testMapSelection(windowPos, windowSize, mapBackgroundSize, mapSize, mapOffsetInBackground);
}

BOOST_AUTO_TEST_SUITE_END()
