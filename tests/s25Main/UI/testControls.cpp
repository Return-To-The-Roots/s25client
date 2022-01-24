// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PointOutput.h"
#include "RectOutput.h"
#include "controls/ctrlDeepening.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlPreviewMinimap.h"
#include "controls/ctrlTable.h"
#include "controls/ctrlText.h"
#include "controls/ctrlTextButton.h"
#include "controls/ctrlTextDeepening.h"
#include "driver/KeyEvent.h"
#include "driver/MouseCoords.h"
#include "helpers/mathFuncs.h"
#include "ogl/glFont.h"
#include "uiHelper/uiHelpers.hpp"
#include "libsiedler2/ArchivItem_Bitmap_Player.h"
#include "libsiedler2/ArchivItem_Font.h"
#include "libsiedler2/ArchivItem_Map.h"
#include "libsiedler2/ArchivItem_Map_Header.h"
#include "libsiedler2/ArchivItem_Raw.h"
#include "libsiedler2/PixelBufferBGRA.h"
#include "rttr/test/random.hpp"
#include <boost/nowide/detail/utf.hpp>
#include <boost/test/unit_test.hpp>
#include <Loader.h>
#include <array>
#include <numeric>

// LCOV_EXCL_START
static std::ostream& boost_test_print_type(std::ostream& stream, TableSortDir dir)
{
    return stream << static_cast<int>(dir);
}
// LCOV_EXCL_STOP

static std::unique_ptr<glFont> createMockFont(const std::vector<char32_t>& chars)
{
    libsiedler2::ArchivItem_Font s2Font;
    s2Font.setDx(5);
    s2Font.setDy(9);
    libsiedler2::ArchivItem_Bitmap_Player glyph;
    libsiedler2::PixelBufferBGRA buffer(5, 9);
    libsiedler2::ArchivItem_Palette pal;
    glyph.create(buffer, &pal);
    const auto maxEl = *std::max_element(chars.begin(), chars.end());
    s2Font.alloc(maxEl + 1u);
    for(const auto c : chars)
        s2Font.setC(c, glyph);
    return std::make_unique<glFont>(s2Font);
}

BOOST_AUTO_TEST_SUITE(Controls)

static void resizeMap(libsiedler2::ArchivItem_Map& glMap, const Extent& size)
{
    libsiedler2::ArchivItem_Map map;
    auto header = std::make_unique<libsiedler2::ArchivItem_Map_Header>();
    header->setWidth(size.x);
    header->setHeight(size.y);
    header->setNumPlayers(2);
    glMap.init(std::move(header));
}

BOOST_FIXTURE_TEST_CASE(PreviewMinimap, uiHelper::Fixture)
{
    DrawPoint pos(5, 12);
    Extent size(20, 10);
    ctrlPreviewMinimap mm(nullptr, 1, pos, size, nullptr);
    BOOST_TEST_REQUIRE(mm.GetCurMapSize() == Extent::all(0));
    BOOST_TEST_REQUIRE(mm.GetBoundaryRect().getSize() == Extent::all(4)); // Padding
    // Remove padding
    mm.SetPadding(Extent::all(0));
    BOOST_TEST_REQUIRE(mm.GetBoundaryRect().getSize() == Extent::all(0));
    libsiedler2::ArchivItem_Map map;
    resizeMap(map, size);
    mm.SetMap(&map);
    BOOST_TEST_REQUIRE(mm.GetBoundaryRect().getSize().x <= size.x); //-V807
    BOOST_TEST_REQUIRE(mm.GetBoundaryRect().getSize().y <= size.y);
    const Extent origBoundary = mm.GetBoundaryRect().getSize();
    // Width smaller -> Don't go over width
    resizeMap(map, Extent(size.x / 2, size.y));
    mm.SetMap(&map);
    BOOST_TEST_REQUIRE(mm.GetBoundaryRect().getSize().x <= size.x);
    BOOST_TEST_REQUIRE(mm.GetBoundaryRect().getSize().y == size.y);
    // Height smaller -> Don't go over height
    resizeMap(map, Extent(size.x, size.y / 2));
    mm.SetMap(&map);
    BOOST_TEST_REQUIRE(mm.GetBoundaryRect().getSize().x == size.x);
    BOOST_TEST_REQUIRE(mm.GetBoundaryRect().getSize().y <= size.y);
    // Both smaller -> Stretch to original height
    resizeMap(map, size / 2u);
    mm.SetMap(&map);
    BOOST_TEST_REQUIRE(mm.GetBoundaryRect().getSize() == origBoundary);
    // Width bigger -> Narrow map
    resizeMap(map, Extent(size.x * 2, size.y));
    mm.SetMap(&map);
    BOOST_TEST_REQUIRE(mm.GetBoundaryRect().getSize().x == size.x);
    BOOST_TEST_REQUIRE(mm.GetBoundaryRect().getSize().y <= size.y);
    // Height bigger -> Narrow map in x
    resizeMap(map, Extent(size.x, size.y * 2));
    mm.SetMap(&map);
    BOOST_TEST_REQUIRE(mm.GetBoundaryRect().getSize().x <= size.x);
    BOOST_TEST_REQUIRE(mm.GetBoundaryRect().getSize().y == size.y);
    // Both bigger -> Stretch to original height
    resizeMap(map, size * 2u);
    mm.SetMap(&map);
    BOOST_TEST_REQUIRE(mm.GetBoundaryRect().getSize() == origBoundary);
}

BOOST_FIXTURE_TEST_CASE(EditShowsCorrectChars, uiHelper::Fixture)
{
    using utfDecoder = boost::nowide::detail::utf::utf_traits<char>;
    // Use a 1 Byte and a 2 Byte UTF8 "char"
    const std::array<std::string, 6> chars = {u8"a", u8"b", u8"c", u8"\u0424", u8"\u041A", u8"\u043b"};
    std::vector<char32_t> codepoints(chars.size() + 1);
    std::transform(chars.begin(), chars.end(), codepoints.begin(), [](const auto& c) {
        auto it = c.begin();
        return utfDecoder::decode_valid(it);
    });
    codepoints.back() = '?';
    const auto font = createMockFont(codepoints);
    ctrlEdit edt(nullptr, 0, DrawPoint(0, 0), Extent(90, 15), TextureColor::Green1, font.get());
    ctrlEdit edt2(nullptr, 0, DrawPoint(0, 0), Extent(90, 15), TextureColor::Green1, font.get());
    const ctrlBaseText* txt = edt.GetCtrl<ctrlBaseText>(0);
    const ctrlBaseText* txt2 = edt2.GetCtrl<ctrlBaseText>(0);
    BOOST_TEST_REQUIRE(txt);
    BOOST_TEST_REQUIRE(txt2);
    BOOST_TEST_REQUIRE(txt->GetText() == "");
    // Width available for text excludes the border left&right and 1 "Dx" for new chars
    const auto allowedWidth = edt.GetSize().x - ctrlDeepening::borderSize.x * 2;
    std::vector<std::string> curChars;
    // Add random chars and observe front chars beeing removed
    for(int i = 0; i < 30; i++)
    {
        curChars.push_back(chars[rttr::test::randomValue<size_t>(0u, chars.size() - 1)]);
        auto it = curChars.back().begin();
        const char32_t c = utfDecoder::decode_valid(it);
        std::string curText = std::accumulate(curChars.begin(), curChars.end(), std::string{});
        edt.SetText(curText);
        // Activate
        edt2.Msg_LeftDown(MouseCoords(edt2.GetPos(), true));
        edt2.Msg_PaintAfter();
        edt2.Msg_KeyDown(KeyEvent{KeyType::Char, c, false, false, false});
        // Remove chars from front until in size
        auto itFirst = curChars.begin();
        while(font->getWidth(curText) > allowedWidth)
        {
            curText = std::accumulate(++itFirst, curChars.end(), std::string{});
        }
        BOOST_TEST_REQUIRE(txt->GetText() == curText);
        BOOST_TEST_REQUIRE(txt2->GetText() == curText);
    }
    // Check navigating of cursor
    edt.Msg_LeftDown(MouseCoords(edt.GetPos(), true)); // Activate
    edt.Msg_PaintAfter();
    int curCursorPos = curChars.size(); // Current cursor should be at end
    while(!curChars.empty())
    {
        int moveOffset = rttr::test::randomValue<int>(-curCursorPos - 1,
                                                      curChars.size() - curCursorPos + 1); //+-1 to check for "overrun"
        for(; moveOffset < 0; ++moveOffset, --curCursorPos)
            edt.Msg_KeyDown(KeyEvent{KeyType::Left, 0, false, false, false});
        for(; moveOffset > 0; --moveOffset, ++curCursorPos)
            edt.Msg_KeyDown(KeyEvent{KeyType::Right, 0, false, false, false});
        curCursorPos = helpers::clamp(curCursorPos, 0, static_cast<int>(curChars.size()));
        // Erase one char (currently only good way to check where the cursor is
        edt.Msg_KeyDown(KeyEvent{KeyType::Backspace, 0, false, false, false});
        if(curCursorPos > 0)
        {
            curChars.erase(curChars.begin() + --curCursorPos);
        }
        const auto curText = std::accumulate(curChars.begin(), curChars.end(), std::string{});
        BOOST_TEST_REQUIRE(edt.GetText() == curText);
    }
    // Check "movement" of text when using cursor
    // First create a text bigger than fit
    curChars.clear();
    std::string curText;
    do
    {
        curChars.push_back(chars[rttr::test::randomValue<size_t>(0u, chars.size() - 1)]);
        curText += curChars.back();
    } while(font->getWidth(curText) <= allowedWidth);
    edt.SetText(curText);
    // Now cursor is at end and first "char" is not showing
    curCursorPos = curChars.size();
    const auto txtWithoutFirst = curText.substr(curChars.front().size());
    BOOST_TEST_REQUIRE(font->getWidth(txtWithoutFirst) <= allowedWidth); // Sanity check
    // At least 5 chars before cursor should be shown -> No change until curCursorPos <= 5
    do
    {
        BOOST_TEST_REQUIRE(txt->GetText() == txtWithoutFirst);
        edt.Msg_KeyDown(KeyEvent{KeyType::Left, 0, false, false, false});
        --curCursorPos;
    } while(curCursorPos > 5);
    while(curCursorPos-- >= 0)
    {
        BOOST_TEST_REQUIRE(txt->GetText() == curText); // Trailing chars are removed by font rendering
        edt.Msg_KeyDown(KeyEvent{KeyType::Left, 0, false, false, false});
    }
    // Moving fully right shows txt again
    curCursorPos = 0;
    while(static_cast<unsigned>(curCursorPos++) < curChars.size())
        edt.Msg_KeyDown(KeyEvent{KeyType::Right, 0, false, false, false});
    BOOST_TEST_REQUIRE(txt->GetText() == txtWithoutFirst);
}

BOOST_AUTO_TEST_CASE(AdjustWidthForMaxChars_SetsCorrectSize)
{
    auto font = createMockFont({'?', 'a', 'z'});
    {
        ctrlTextDeepening txt(nullptr, 1, rttr::test::randomPoint<DrawPoint>(), rttr::test::randomPoint<Extent>(),
                              TextureColor::Green1, "foo", font.get(), COLOR_BLACK);
        const Extent sizeBefore = txt.GetSize();
        // Don't assume size, so get size for 0 chars
        txt.ResizeForMaxChars(0);
        const Extent sizeZero = txt.GetSize();
        BOOST_TEST(sizeZero.y == sizeBefore.y);
        const auto numChars = rttr::test::randomValue(1u, 20u);
        txt.ResizeForMaxChars(numChars);
        BOOST_TEST(txt.GetSize() == Extent(sizeZero.x + numChars * font->getDx(), sizeBefore.y));
    }
    {
        ctrlTextButton txt(nullptr, 1, rttr::test::randomPoint<DrawPoint>(), rttr::test::randomPoint<Extent>(),
                           TextureColor::Green1, "foo", font.get(), "tooltip");
        const Extent sizeBefore = txt.GetSize();
        // Don't assume size, so get size for 0 chars
        txt.ResizeForMaxChars(0);
        const Extent sizeZero = txt.GetSize();
        BOOST_TEST(sizeZero.y == sizeBefore.y);
        const auto numChars = rttr::test::randomValue(1u, 20u);
        txt.ResizeForMaxChars(numChars);
        BOOST_TEST(txt.GetSize() == Extent(sizeZero.x + numChars * font->getDx(), sizeBefore.y));
    }
}

BOOST_AUTO_TEST_CASE(TextControlWorks)
{
    auto font = createMockFont({'?', 'a', 'z'});
    const auto parentPos = rttr::test::randomPoint<DrawPoint>(-1000, 1000);
    Window parent(nullptr, 0, parentPos);
    const auto pos = rttr::test::randomPoint<DrawPoint>(-1000, 1000);
    const auto* testText = "a?z?a?z?a?z?a?z?a?z?";
    ctrlText text(&parent, 0, pos, testText, COLOR_YELLOW, FontStyle{}, font.get());

    // Test positioning
    BOOST_TEST(text.GetPos() == pos);
    const auto origBoundaryRect = text.GetBoundaryRect();
    BOOST_TEST(origBoundaryRect.getOrigin() == pos + parentPos);
    const auto fullTextWidth = font->getWidth(testText);
    BOOST_TEST(origBoundaryRect.getSize() == Extent(fullTextWidth, font->getHeight()));

    // Test alignment
    {
        const Position origSize(origBoundaryRect.getSize()); // Convert to signed type
        ctrlText textRight(&parent, 0, pos, testText, COLOR_YELLOW, FontStyle::RIGHT, font.get());
        BOOST_TEST(textRight.GetBoundaryRect() == Rect::move(origBoundaryRect, Position(-origSize.x, 0)));
        ctrlText textCenter(&parent, 0, pos, testText, COLOR_YELLOW, FontStyle::CENTER, font.get());
        BOOST_TEST(textCenter.GetBoundaryRect() == Rect::move(origBoundaryRect, Position(-origSize.x / 2, 0)));
        ctrlText textBottom(&parent, 0, pos, testText, COLOR_YELLOW, FontStyle::BOTTOM, font.get());
        BOOST_TEST(textBottom.GetBoundaryRect() == Rect::move(origBoundaryRect, Position(0, -origSize.y)));
        ctrlText textVCenter(&parent, 0, pos, testText, COLOR_YELLOW, FontStyle::VCENTER, font.get());
        BOOST_TEST(textVCenter.GetBoundaryRect() == Rect::move(origBoundaryRect, Position(0, -origSize.y / 2)));
        ctrlText textFullCenter(&parent, 0, pos, testText, COLOR_YELLOW, FontStyle::CENTER | FontStyle::VCENTER,
                                font.get());
        BOOST_TEST(textFullCenter.GetBoundaryRect()
                   == Rect::move(origBoundaryRect, Position(-origSize.x / 2, -origSize.y / 2)));
        ctrlText textBottomRight(&parent, 0, pos, testText, COLOR_YELLOW, FontStyle::RIGHT | FontStyle::BOTTOM,
                                 font.get());
        BOOST_TEST(textBottomRight.GetBoundaryRect()
                   == Rect::move(origBoundaryRect, Position(-origSize.x, -origSize.y)));
    }

    // Test maxWidth
    {
        text.setMaxWidth(fullTextWidth); // Limit width without actually limiting it
        BOOST_TEST(text.GetBoundaryRect() == origBoundaryRect);
        // Limit width truncating the text
        const auto maxWidth = rttr::test::randomValue(font->getDx(), fullTextWidth - 1u);
        text.setMaxWidth(maxWidth);
        const auto newBoundaryRect = text.GetBoundaryRect();
        BOOST_TEST(newBoundaryRect.getOrigin() == origBoundaryRect.getOrigin());
        // TODO: Test that with a maxWidth of x-1 the width matches that of the last few chars replaced by dots
        BOOST_TEST(newBoundaryRect.getSize().x <= maxWidth);
        BOOST_TEST(newBoundaryRect.getSize().y == origBoundaryRect.getSize().y);
    }
}

static std::vector<std::string> getRow(const ctrlTable& table, unsigned row)
{
    std::vector<std::string> values(table.GetNumColumns());
    for(unsigned i = 0; i < table.GetNumColumns(); i++)
        values[i] = table.GetItemText(row, i);
    return values;
}

BOOST_AUTO_TEST_CASE(TableSorting)
{
    auto font = createMockFont({'?', 'a', 'z'});
    ctrlTable table(nullptr, 0, DrawPoint::all(0), Extent(400, 300), TextureColor::Green1, font.get(),
                    ctrlTable::Columns{{"String", 1, TableSortType::String},
                                       {"MapSize", 2, TableSortType::MapSize},
                                       {"Number", 3, TableSortType::Number},
                                       {"Date", 5, TableSortType::Date}});
    const std::vector<std::string> r1 = {"acc", "10x20", "1", "10.11.2000 - 10:12"};
    const std::vector<std::string> r2 = {"abc", "10x20", "2", "10.11.2000 - 10:14"};
    const std::vector<std::string> r3 = {"acd", "8x20", "5", "10.11.2000 - 11:12"};
    const std::vector<std::string> r4 = {"ccc", "10x30", "10", "09.11.2000 - 10:12"};
    const std::vector<std::string> r5 = {"zcc", "30x10", "13", "10.12.2000 - 10:12"};
    const std::vector<std::string> r6 = {"tts", "10x22", "42", "10.11.2001 - 10:12"};
    table.AddRow(r1);
    table.AddRow(r2);
    table.AddRow(r3);
    table.AddRow(r4);
    table.AddRow(r5);
    table.AddRow(r6);

    auto testRowsEqual = [&table](std::array<const std::vector<std::string>*, 6> rows) {
        for(unsigned i = 0; i < rows.size(); i++)
        {
            BOOST_TEST_CONTEXT("row " << i)
            {
                BOOST_TEST(getRow(table, i) == *rows[i], boost::test_tools::per_element());
            }
        }
    };

    table.SortRows(0, TableSortDir::Descending);
    BOOST_TEST(table.GetSortColumn() == 0);
    BOOST_TEST(table.GetSortDirection() == TableSortDir::Descending);
    BOOST_TEST_CONTEXT("String column") testRowsEqual({&r5, &r6, &r4, &r3, &r1, &r2});

    table.SortRows(1, TableSortDir::Ascending);
    BOOST_TEST(table.GetSortColumn() == 1);
    BOOST_TEST(table.GetSortDirection() == TableSortDir::Ascending);
    BOOST_TEST_CONTEXT("MapSize column") testRowsEqual({&r3, &r1, &r2, &r6, &r4, &r5});

    table.SortRows(2, TableSortDir::Descending);
    BOOST_TEST(table.GetSortColumn() == 2);
    BOOST_TEST(table.GetSortDirection() == TableSortDir::Descending);
    BOOST_TEST_CONTEXT("Number column") testRowsEqual({&r6, &r5, &r4, &r3, &r2, &r1});

    table.SortRows(3, TableSortDir::Ascending);
    BOOST_TEST(table.GetSortColumn() == 3);
    BOOST_TEST(table.GetSortDirection() == TableSortDir::Ascending);
    BOOST_TEST_CONTEXT("Date column") testRowsEqual({&r4, &r1, &r2, &r3, &r5, &r6});
    table.SortRows(3, TableSortDir::Descending);
    BOOST_TEST(table.GetSortColumn() == 3);
    BOOST_TEST(table.GetSortDirection() == TableSortDir::Descending);
    BOOST_TEST_CONTEXT("Date column") testRowsEqual({&r6, &r5, &r3, &r2, &r1, &r4});
}

BOOST_AUTO_TEST_SUITE_END()
