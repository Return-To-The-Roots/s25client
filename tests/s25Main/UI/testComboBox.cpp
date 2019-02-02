// Copyright (c) 2016 - 2019 Settlers Freaks (sf-team at siedler25.org)
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
#include "driver/MouseCoords.h"
#include "uiHelper/uiHelpers.hpp"
#include <boost/test/unit_test.hpp>
#include <controls/ctrlComboBox.h>
#include <rttr/test/random.hpp>
#include <turtle/mock.hpp>

//-V:MOCK_METHOD:813
//-V:MOCK_EXPECT:807

#define TEST(...) BOOST_TEST(__VA_ARGS__)
#define REQUIRE(...) BOOST_TEST_REQUIRE(__VA_ARGS__)
using namespace rttr::test;

namespace {
/* clang-format off */
    MOCK_BASE_CLASS(TestWindow, Window)
    {
    public:
        TestWindow() : Window(nullptr, randomValue<unsigned>(), DrawPoint(0,0)) {}
        MOCK_METHOD(Msg_ComboSelectItem, 2)
        MOCK_METHOD(Msg_ListSelectItem, 2)
    };
/* clang-format on */
} // namespace

BOOST_FIXTURE_TEST_SUITE(ComboBox, uiHelper::Fixture)

BOOST_AUTO_TEST_CASE(ItemHandling)
{
    TestWindow wnd;
    auto cb = std::make_unique<ctrlComboBox>(&wnd, randomValue<unsigned>(), DrawPoint(0, 0), Extent(40, 20), TextureColor::TC_GREEN1,
                                             NormalFont, 20, false);
    REQUIRE(cb->GetNumItems() == 0);
    REQUIRE(cb->GetSelection() == -1);
    REQUIRE(cb->GetText(0) == "");

    cb->AddString("text");
    REQUIRE(cb->GetNumItems() == 1);
    REQUIRE(cb->GetSelection() == -1);
    REQUIRE(cb->GetText(0) == "text");

    cb->SetSelection(0);
    REQUIRE(cb->GetSelection() == 0);
    REQUIRE(cb->GetText(0) == "text");
    // Out of bounds selection
    cb->SetSelection(1);
    REQUIRE(cb->GetSelection() == 0);
    REQUIRE(cb->GetText(0) == "text");

    std::vector<std::string> lines{1, "text"};
    for(int i = 0; i < 10; i++)
    {
        lines.emplace_back(randString());
        cb->AddString(lines.back());
        REQUIRE(cb->GetNumItems() == lines.size());
        REQUIRE(cb->GetSelection() == 0);
    }
    for(unsigned i = 0; i < lines.size(); i++)
    {
        REQUIRE(cb->GetText(i) == lines[i]);
        auto sel = randomValue<uint16_t>(0, lines.size() - 1);
        cb->SetSelection(sel);
        REQUIRE(cb->GetSelection() == sel);
    }

    cb->DeleteAllItems();
    REQUIRE(cb->GetNumItems() == 0);
    REQUIRE(cb->GetSelection() == -1);
    REQUIRE(cb->GetText(0) == "");
}

BOOST_AUTO_TEST_CASE(ControlWithScrollWheel, *boost::unit_test::label("seed=1547929045333605162"))
{
    TestWindow wnd;
    auto cb = std::make_unique<ctrlComboBox>(&wnd, randomValue<unsigned>(), randomPoint<DrawPoint>(0, 100), randomPoint<Extent>(20, 200),
                                             TextureColor::TC_GREEN1, NormalFont, randomValue(20, 200), false);

    for(int i = 0; i < 3; i++)
    {
        cb->AddString(randString());
    }
    REQUIRE(cb->GetSelection() == -1);
    mock::sequence s;
    // Scroll down 3 times (each selected)
    MouseCoords mc{cb->GetPos() + Position(randomValue(0u, cb->GetSize().x), randomValue(0u, cb->GetSize().y))};
    for(int i = 0; i < 3; i++)
    {
        MOCK_EXPECT(wnd.Msg_ComboSelectItem).once().with(cb->GetID(), i).in(s);
        cb->Msg_WheelDown(mc);
        REQUIRE(cb->GetSelection() == i);
    }
    // 4th time does nothing
    cb->Msg_WheelDown(mc);
    REQUIRE(cb->GetSelection() == 2);
    // Same but up
    for(int i = 1; i >= 0; i--)
    {
        MOCK_EXPECT(wnd.Msg_ComboSelectItem).once().with(cb->GetID(), i).in(s);
        cb->Msg_WheelUp(mc);
        REQUIRE(cb->GetSelection() == i);
    }
    // next time does nothing
    cb->Msg_WheelUp(mc);
    REQUIRE(cb->GetSelection() == 0);
}

BOOST_AUTO_TEST_CASE(ListRemove)
{
    TestWindow wnd;
    auto list = std::make_unique<ctrlList>(&wnd, randomValue<unsigned>(), randomPoint<DrawPoint>(0, 100), randomPoint<Extent>(20, 200),
                                           TextureColor::TC_GREEN1, NormalFont);
    std::vector<std::string> lines;
    for(int i = 0; i < 10; i++)
    {
        lines.emplace_back(randString());
        list->AddString(lines.back());
        REQUIRE(list->GetNumLines() == lines.size());
        REQUIRE(list->GetSelection() == -1);
    }
    mock::sequence s;
    MOCK_EXPECT(wnd.Msg_ListSelectItem).once().with(list->GetID(), 4).in(s);
    list->SetSelection(4);
    REQUIRE(list->GetSelection() == 4);
    REQUIRE(list->GetSelItemText() == lines[4]);
    // To big
    list->Remove(10);
    REQUIRE(list->GetNumLines() == lines.size());
    REQUIRE(list->GetSelItemText() == lines[4]);
    // Remove after selected. Selection remains
    list->Remove(5);
    REQUIRE(list->GetNumLines() == lines.size() - 1);
    REQUIRE(list->GetSelItemText() == lines[4]);
    lines.erase(lines.begin() + 5);
    // Remove all before. Selection remains
    for(int i = 1; i <= 4; i++)
    {
        list->Remove(randomValue(0, 4 - i));
        REQUIRE(list->GetNumLines() == lines.size() - i);
        REQUIRE(list->GetSelItemText() == lines[4]);
    }
    lines.erase(lines.begin(), lines.begin() + 4);
    for(unsigned i = 0; i < lines.size(); i++)
        REQUIRE(list->GetItemText(i) == lines[i]);
    // Remove selection -> Next selected
    REQUIRE(list->GetNumLines() == 5);
    REQUIRE(list->GetSelection() == 0);
    MOCK_EXPECT(wnd.Msg_ListSelectItem).once().with(list->GetID(), 0).in(s);
    list->Remove(0);
    REQUIRE(list->GetNumLines() == 4);
    REQUIRE(list->GetSelection() == 0);
    REQUIRE(list->GetSelItemText() == lines[1]);
    lines.erase(lines.begin());
    // Also for middle pos
    MOCK_EXPECT(wnd.Msg_ListSelectItem).once().with(list->GetID(), 2).in(s);
    MOCK_EXPECT(wnd.Msg_ListSelectItem).once().with(list->GetID(), 2).in(s);
    list->SetSelection(2);
    list->Remove(2);
    REQUIRE(list->GetNumLines() == 3);
    REQUIRE(list->GetSelection() == 2);
    REQUIRE(list->GetSelItemText() == lines[3]);
    lines.erase(lines.begin() + 2);
    // For last pos previous is selected
    list->SetSelection(list->GetNumLines() - 1);
    MOCK_EXPECT(wnd.Msg_ListSelectItem).once().with(list->GetID(), 1).in(s);
    list->Remove(list->GetNumLines() - 1);
    REQUIRE(list->GetNumLines() == 2);
    REQUIRE(list->GetSelection() == 1);
    REQUIRE(list->GetSelItemText() == lines[lines.size() - 2]);
    // Removing all sets selection to -1
    list->Remove(0);
    list->Remove(0);
    REQUIRE(list->GetNumLines() == 0);
    REQUIRE(list->GetSelection() == -1);
    REQUIRE(list->GetSelItemText() == "");
}

BOOST_AUTO_TEST_SUITE_END()
