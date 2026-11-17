// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Loader.h"
#include "controls/ctrlComboBox.h"
#include "driver/MouseCoords.h"
#include "uiHelper/uiHelpers.hpp"
#include <rttr/test/random.hpp>
#include <turtle/mock.hpp>
#include <boost/test/unit_test.hpp>
#include <helpers/optional_io.h>

//-V:MOCK_METHOD:813
//-V:MOCK_EXPECT:807

#define TEST(...) BOOST_TEST(__VA_ARGS__)
#define REQUIRE(...) BOOST_TEST_REQUIRE(__VA_ARGS__)
using namespace rttr::test;

namespace {
MOCK_BASE_CLASS(TestWindow, Window)
{
public:
    TestWindow() : Window(nullptr, randomValue<unsigned>(), DrawPoint(0, 0)) {}
    MOCK_METHOD(msgComboSelectItem, 2)
    MOCK_METHOD(msgListSelectItem, 2)
};
} // namespace

BOOST_FIXTURE_TEST_SUITE(ComboBox, uiHelper::Fixture)

BOOST_AUTO_TEST_CASE(ItemHandling)
{
    TestWindow wnd;
    auto cb = std::make_unique<ctrlComboBox>(&wnd, randomValue<unsigned>(), DrawPoint(0, 0), Extent(40, 20),
                                             TextureColor::Green1, NormalFont, 20, false);
    REQUIRE(cb->getNumItems() == 0);
    REQUIRE(!cb->getSelection());

    cb->addItem("text");
    REQUIRE(cb->getNumItems() == 1);
    REQUIRE(!cb->getSelection());
    REQUIRE(cb->getText(0) == "text");

    cb->setSelection(0);
    REQUIRE(cb->getSelection() == 0u);
    REQUIRE(cb->getText(0) == "text");
    // Out of bounds selection
    cb->setSelection(1);
    REQUIRE(cb->getSelection() == 0u);
    REQUIRE(cb->getText(0) == "text");

    std::vector<std::string> lines{1, "text"};
    for(int i = 0; i < 10; i++)
    {
        lines.emplace_back(randString());
        cb->addItem(lines.back());
        REQUIRE(cb->getNumItems() == lines.size());
        REQUIRE(cb->getSelection() == 0u);
    }
    for(unsigned i = 0; i < lines.size(); i++)
    {
        REQUIRE(cb->getText(i) == lines[i]);
        auto sel = randomValue<unsigned>(0, lines.size() - 1);
        cb->setSelection(sel);
        REQUIRE(cb->getSelection() == sel);
    }

    cb->deleteAllItems();
    REQUIRE(cb->getNumItems() == 0);
    REQUIRE(!cb->getSelection());
}

BOOST_AUTO_TEST_CASE(ControlWithScrollWheel)
{
    TestWindow wnd;
    auto cb = std::make_unique<ctrlComboBox>(&wnd, randomValue<unsigned>(), randomPoint<DrawPoint>(0, 100),
                                             randomPoint<Extent>(20, 200), TextureColor::Green1, NormalFont,
                                             randomValue(20, 200), false);

    for(int i = 0; i < 3; i++)
    {
        cb->addItem(randString());
    }
    REQUIRE(!cb->getSelection());
    mock::sequence s;
    // Scroll down 3 times (each selected)
    MouseCoords mc{cb->getPos()
                   + Position(randomValue(0u, cb->getSize().x - 1u), randomValue(0u, cb->getSize().y - 1u))};
    for(unsigned i = 0; i < 3u; i++)
    {
        MOCK_EXPECT(wnd.msgComboSelectItem).once().with(cb->getID(), static_cast<unsigned>(i)).in(s);
        cb->msgWheelDown(mc);
        REQUIRE(cb->getSelection() == i);
    }
    // 4th time does nothing
    cb->msgWheelDown(mc);
    REQUIRE(cb->getSelection() == 2u);
    // Same but up
    for(int i = 1; i >= 0; i--)
    {
        MOCK_EXPECT(wnd.msgComboSelectItem).once().with(cb->getID(), static_cast<unsigned>(i)).in(s);
        cb->msgWheelUp(mc);
        REQUIRE(cb->getSelection() == static_cast<unsigned>(i));
    }
    // next time does nothing
    cb->msgWheelUp(mc);
    REQUIRE(cb->getSelection() == 0u);
}

BOOST_AUTO_TEST_CASE(ListRemove)
{
    TestWindow wnd;
    auto list = std::make_unique<ctrlList>(&wnd, randomValue<unsigned>(), randomPoint<DrawPoint>(0, 100),
                                           randomPoint<Extent>(20, 200), TextureColor::Green1, NormalFont);
    std::vector<std::string> lines;
    for(int i = 0; i < 10; i++)
    {
        lines.emplace_back(randString());
        list->addItem(lines.back());
        REQUIRE(list->getNumLines() == lines.size());
        REQUIRE(!list->getSelection());
    }
    mock::sequence s;
    MOCK_EXPECT(wnd.msgListSelectItem).once().with(list->getID(), 4).in(s);
    list->setSelection(4);
    REQUIRE(list->getSelection() == 4u);
    REQUIRE(list->getSelItemText() == lines[4]);
    // To big
    list->remove(10);
    REQUIRE(list->getNumLines() == lines.size());
    REQUIRE(list->getSelItemText() == lines[4]);
    // Remove after selected. Selection remains
    list->remove(5);
    REQUIRE(list->getNumLines() == lines.size() - 1);
    REQUIRE(list->getSelItemText() == lines[4]);
    lines.erase(lines.begin() + 5);
    // Remove all before. Selection remains
    for(int i = 1; i <= 4; i++)
    {
        list->remove(randomValue(0, 4 - i));
        REQUIRE(list->getNumLines() == lines.size() - i);
        REQUIRE(list->getSelItemText() == lines[4]);
    }
    lines.erase(lines.begin(), lines.begin() + 4);
    for(unsigned i = 0; i < lines.size(); i++)
        REQUIRE(list->getItemText(i) == lines[i]);
    // Remove selection -> Next selected
    REQUIRE(list->getNumLines() == 5);
    REQUIRE(list->getSelection() == 0u);
    MOCK_EXPECT(wnd.msgListSelectItem).once().with(list->getID(), 0).in(s);
    list->remove(0);
    REQUIRE(list->getNumLines() == 4);
    REQUIRE(list->getSelection() == 0u);
    REQUIRE(list->getSelItemText() == lines[1]);
    lines.erase(lines.begin());
    // Also for middle pos
    MOCK_EXPECT(wnd.msgListSelectItem).once().with(list->getID(), 2).in(s);
    MOCK_EXPECT(wnd.msgListSelectItem).once().with(list->getID(), 2).in(s);
    list->setSelection(2);
    list->remove(2);
    REQUIRE(list->getNumLines() == 3);
    REQUIRE(list->getSelection() == 2u);
    REQUIRE(list->getSelItemText() == lines[3]);
    lines.erase(lines.begin() + 2);
    // For last pos previous is selected
    list->setSelection(list->getNumLines() - 1);
    MOCK_EXPECT(wnd.msgListSelectItem).once().with(list->getID(), 1).in(s);
    list->remove(list->getNumLines() - 1);
    REQUIRE(list->getNumLines() == 2);
    REQUIRE(list->getSelection() == 1u);
    REQUIRE(list->getSelItemText() == lines[lines.size() - 2]);
    // Removing all sets selection to -1
    list->remove(0);
    list->remove(0);
    REQUIRE(list->getNumLines() == 0);
    REQUIRE(!list->getSelection());
    REQUIRE(list->getSelItemText() == "");
}

BOOST_AUTO_TEST_SUITE_END()
