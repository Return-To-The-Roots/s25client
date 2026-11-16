// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CollisionDetection.h"
#include "GameWithLuaAccess.h"
#include "Loader.h"
#include "WindowManager.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlImage.h"
#include "controls/ctrlMultiline.h"
#include "ingameWindows/iwMissionStatement.h"
#include "ingameWindows/iwMsgbox.h"
#include "ogl/glFont.h"
#include "uiHelper/uiHelpers.hpp"

struct LuaGuiFixture : LuaTestsFixture, uiHelper::Fixture
{};

BOOST_FIXTURE_TEST_SUITE(LuaGUITestSuite, LuaGuiFixture)

BOOST_AUTO_TEST_CASE(MissionStatement)
{
    // Set player
    MOCK_EXPECT(localGameState.getPlayerId).returns(1);

    BOOST_TEST_REQUIRE(!WINDOWMANAGER.getTopMostWindow());

    // Show mission to non-existing player
    executeLua("rttr:MissionStatement(99, 'Title', 'Text')");
    BOOST_TEST_REQUIRE(!WINDOWMANAGER.getTopMostWindow());

    // Show mission to other player
    executeLua("rttr:MissionStatement(0, 'Title', 'Text')");
    BOOST_TEST_REQUIRE(!WINDOWMANAGER.getTopMostWindow());

    // Show it to us
    executeLua("rttr:MissionStatement(1, 'Title', 'Text')");
    const auto* wnd = dynamic_cast<iwMissionStatement*>(WINDOWMANAGER.getTopMostWindow());
    BOOST_TEST_REQUIRE(wnd);
    BOOST_TEST(wnd->IsActive());
    BOOST_TEST(wnd->getTitle() == _("Title"));

    // double windows stack, newest modal goes on top
    executeLua("rttr:MissionStatement(1, 'Title2', 'Text')");
    const auto* wnd2 = dynamic_cast<iwMissionStatement*>(WINDOWMANAGER.getTopMostWindow());
    BOOST_TEST_REQUIRE(wnd2);
    BOOST_TEST(wnd2 != wnd);
    BOOST_TEST(wnd2->getTitle() == "Title2");
    // Close 2nd (topmost) wnd
    WINDOWMANAGER.CloseNow(const_cast<iwMissionStatement*>(wnd2));
    // 1st shows again
    wnd2 = dynamic_cast<iwMissionStatement*>(WINDOWMANAGER.getTopMostWindow());
    BOOST_TEST_REQUIRE(wnd2);
    BOOST_TEST(wnd2 == wnd);
    BOOST_TEST(wnd2->getTitle() == _("Title"));
    // Close wnd
    WINDOWMANAGER.CloseNow(const_cast<iwMissionStatement*>(wnd));
    BOOST_TEST_REQUIRE(!WINDOWMANAGER.getTopMostWindow());

    // No image
    executeLua("rttr:MissionStatement(1, 'Title', 'Text', IM_NONE)");
    wnd = dynamic_cast<iwMissionStatement*>(WINDOWMANAGER.getTopMostWindow());
    BOOST_TEST_REQUIRE(wnd);
    BOOST_TEST(wnd->IsActive());
    BOOST_TEST(wnd->getTitle() == _("Title"));
    BOOST_TEST(wnd->getCtrls<ctrlImage>().empty());
    WINDOWMANAGER.CloseNow(const_cast<iwMissionStatement*>(wnd));
    // Non-default image
    executeLua("rttr:MissionStatement(1, 'Title', 'Text', IM_AVATAR10)");
    wnd = dynamic_cast<iwMissionStatement*>(WINDOWMANAGER.getTopMostWindow());
    BOOST_TEST_REQUIRE(wnd);
    BOOST_TEST(wnd->IsActive());
    BOOST_TEST(wnd->getTitle() == _("Title"));
    BOOST_TEST(!wnd->getCtrls<ctrlImage>().empty());
    WINDOWMANAGER.CloseNow(const_cast<iwMissionStatement*>(wnd));
    // Invalid image
    executeLua("rttr:MissionStatement(1, 'Title', 'Text', 999999)");
    wnd = dynamic_cast<iwMissionStatement*>(WINDOWMANAGER.getTopMostWindow());
    BOOST_TEST_REQUIRE(wnd);
    BOOST_TEST(wnd->IsActive());
    BOOST_TEST(wnd->getTitle() == _("Title"));
    BOOST_TEST(wnd->getCtrls<ctrlImage>().empty());
    WINDOWMANAGER.CloseNow(const_cast<iwMissionStatement*>(wnd));
}

BOOST_AUTO_TEST_CASE(MessageBoxTest)
{
    // Default Info
    executeLua("rttr:MsgBox('Title', string.rep('Text\\n', 15))");
    const auto* wnd = dynamic_cast<iwMsgbox*>(WINDOWMANAGER.getTopMostWindow());
    BOOST_TEST_REQUIRE(wnd);
    BOOST_TEST(wnd->IsActive());
    BOOST_TEST(wnd->getTitle() == _("Title"));
    BOOST_TEST(!wnd->getCtrls<ctrlImage>().empty());
    BOOST_TEST(wnd->getCtrls<ctrlImage>().front()->getImage()
               == LOADER.getImageN("io", rttr::enum_cast(MsgboxIcon::ExclamationGreen)));
    BOOST_TEST(!wnd->getCtrls<ctrlButton>().empty());
    // 15 lines of text, button must be below
    BOOST_TEST(wnd->getCtrls<ctrlButton>().front()->getPos().y > 15 * NormalFont->getHeight());
    // And window higher than button
    BOOST_TEST(static_cast<int>(wnd->getIwSize().y) > wnd->getCtrls<ctrlButton>().front()->getPos().y);
    WINDOWMANAGER.CloseNow(const_cast<iwMsgbox*>(wnd));
    // Error
    executeLua("rttr:MsgBox('Title', 'Text', true)");
    wnd = dynamic_cast<iwMsgbox*>(WINDOWMANAGER.getTopMostWindow());
    BOOST_TEST_REQUIRE(wnd);
    BOOST_TEST(wnd->IsActive());
    BOOST_TEST(wnd->getTitle() == _("Title"));
    BOOST_TEST(!wnd->getCtrls<ctrlImage>().empty());
    BOOST_TEST(wnd->getCtrls<ctrlImage>().front()->getImage()
               == LOADER.getImageN("io", rttr::enum_cast(MsgboxIcon::ExclamationRed)));
    WINDOWMANAGER.CloseNow(const_cast<iwMsgbox*>(wnd));
    // MsgBoxEx
    executeLua("rttr:MsgBoxEx('Title', 'Text', 'io', 100)");
    wnd = dynamic_cast<iwMsgbox*>(WINDOWMANAGER.getTopMostWindow());
    BOOST_TEST_REQUIRE(wnd);
    BOOST_TEST(wnd->IsActive());
    BOOST_TEST(wnd->getTitle() == _("Title"));
    BOOST_TEST(!wnd->getCtrls<ctrlImage>().empty());
    BOOST_TEST(wnd->getCtrls<ctrlImage>().front()->getImage() == LOADER.getImageN("io", 100));
    WINDOWMANAGER.CloseNow(const_cast<iwMsgbox*>(wnd));
    // MsgBoxEx with position
    executeLua("rttr:MsgBoxEx('Title', 'Text', 'io', 101, 500, 200)");
    wnd = dynamic_cast<iwMsgbox*>(WINDOWMANAGER.getTopMostWindow());
    BOOST_TEST_REQUIRE(wnd);
    BOOST_TEST(wnd->IsActive());
    BOOST_TEST(wnd->getTitle() == _("Title"));
    BOOST_TEST(!wnd->getCtrls<ctrlImage>().empty());
    const ctrlImage* img = wnd->getCtrls<ctrlImage>().front();
    const ITexture* actImg = img->getImage();
    const Position imgPos = img->getPos();
    BOOST_TEST(actImg == LOADER.getImageN("io", 101));
    BOOST_TEST(imgPos.x == 500);
    BOOST_TEST(imgPos.y == 200);
    // Window must be bigger than image pos+size
    Position imgEndPos(imgPos + actImg->getSize() - actImg->getOrigin());
    BOOST_TEST(static_cast<int>(wnd->getSize().x) > imgEndPos.x); //-V807
    BOOST_TEST(static_cast<int>(wnd->getSize().y) > imgEndPos.y);
    const ctrlButton* bt = wnd->getCtrls<ctrlButton>().front();
    // button must be below start of image
    BOOST_TEST(bt->getPos().y > imgPos.y - actImg->getOrigin().y); //-V807
    // and centered
    BOOST_TEST(bt->getPos().x <= static_cast<int>(wnd->getSize().x / 2));
    BOOST_TEST(bt->getPos().x > static_cast<int>(wnd->getSize().x / 2 - bt->getSize().x));

    // Left, right, top, bottom
    const std::vector<DrawPoint> imgPts{DrawPoint(30, 30), DrawPoint(300, 30), DrawPoint(150, 30), DrawPoint(150, 300)};
    for(const auto& imgPt : imgPts)
    {
        const_cast<iwMsgbox*>(wnd)->MoveIcon(imgPt);
        Rect imgRect(img->getPos() - actImg->getOrigin(), actImg->getSize());
        // Image must be in wnd
        BOOST_TEST(static_cast<int>(wnd->getSize().x) > imgRect.right);
        BOOST_TEST(static_cast<int>(wnd->getSize().y) > imgRect.bottom);
        // Not over button or text
        BOOST_TEST(!DoRectsIntersect(imgRect, bt->getDrawRect()));
        const ctrlMultiline* text = wnd->getCtrls<ctrlMultiline>().front();
        BOOST_TEST(!DoRectsIntersect(imgRect, text->getDrawRect()));
    }
}

BOOST_AUTO_TEST_SUITE_END()
