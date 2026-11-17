// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "dskTest.h"
#include "CollisionDetection.h"
#include "Loader.h"
#include "WindowManager.h"
#include "animation/BlinkButtonAnim.h"
#include "animation/MoveAnimation.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlText.h"
#include "desktops/dskMainMenu.h"
#include "desktops/dskTextureTest.h"
#include "dskBenchmark.h"
#include "files.h"
#include "ogl/FontStyle.h"
#include "s25util/colors.h"

namespace {
enum
{
    ID_txtTitle = dskMenuBase::ID_FIRST_FREE,
    ID_grpBtStart,
    ID_grpBtEnd = ID_grpBtStart + 5 * 4,
    ID_btDisable,
    ID_btAniBg,
    ID_btAni,
    ID_btAnimate,
    ID_btAnimateRepeat,
    ID_btAnimateOscillate,
    ID_btTextureTest,
    ID_edtTest,
    ID_txtTest,
    ID_cbTxtSize,
    ID_btHideCtrls,
    ID_btShowBenchmark
};
}

dskTest::dskTest() : curBGIdx(LOAD_SCREENS.size())
{
    addText(ID_txtTitle, DrawPoint(300, 20), _("Internal test screen for developers"), COLOR_ORANGE, FontStyle::CENTER,
            LargeFont);
    std::array<TextureColor, 4> textures = {
      {TextureColor::Green1, TextureColor::Green2, TextureColor::Red1, TextureColor::Grey}};
    std::array<std::string, 4> labels = {{"Green1", "Green2", "Red1", "Grey"}};
    unsigned yPos = 50;
    unsigned curId = ID_grpBtStart;
    for(unsigned i = 0; i < textures.size(); i++)
    {
        addText(curId, DrawPoint(10, yPos + 3), labels.at(i), COLOR_YELLOW, FontStyle::LEFT, NormalFont);
        ctrlButton* bt;
        bt = addTextButton(curId + 1, DrawPoint(120, yPos), Extent(95, 22), textures[i], "Nothing", NormalFont);
        bt->setIlluminated(false);
        bt->setBorder(false);
        bt = addTextButton(curId + 2, DrawPoint(220, yPos), Extent(95, 22), textures[i], "Illuminated", NormalFont);
        bt->setIlluminated(true);
        bt->setBorder(false);
        bt = addTextButton(curId + 3, DrawPoint(320, yPos), Extent(95, 22), textures[i], "Border", NormalFont);
        bt->setIlluminated(false);
        bt->setBorder(true);
        bt = addTextButton(curId + 4, DrawPoint(420, yPos), Extent(95, 22), textures[i], "Both", NormalFont);
        bt->setIlluminated(true);
        bt->setBorder(true);
        curId += 5;
        yPos += 30;
    }
    RTTR_Assert(curId == ID_grpBtEnd);

    DrawPoint btPos(10, yPos);
    ctrlButton* bt = addTextButton(ID_btAniBg, btPos, Extent(700, 40), TextureColor::Green2, "", NormalFont);
    bt->setBorder(false);
    bt->setEnabled(false);
    bt = addTextButton(ID_btAni, btPos + DrawPoint(0, 5), Extent(30, 30), TextureColor::Red1, "", NormalFont);
    bt->setBorder(false);
    bt->setEnabled(false);
    bt->setIlluminated(true);

    btPos.y += 45;
    addEdit(ID_edtTest, btPos, Extent(150, 22), TextureColor::Green2, NormalFont, 0, false, false, true);
    btPos.x += 170;
    ctrlComboBox* cb = addComboBox(ID_cbTxtSize, btPos, Extent(100, 22), TextureColor::Green2, NormalFont, 100);
    cb->addItem("Small Font");
    cb->addItem("Medium Font");
    cb->addItem("Large Font");
    cb->setSelection(0);
    btPos.x += 110;
    btPos.y += 11;
    addText(ID_txtTest, btPos, "Enter something", COLOR_YELLOW, FontStyle::VCENTER, SmallFont);

    addTextButton(ID_btDisable, DrawPoint(10, 540), Extent(150, 22), TextureColor::Green1, "Enable/Disable buttons",
                  NormalFont);
    addTextButton(ID_btAnimate, DrawPoint(165, 540), Extent(80, 22), TextureColor::Green1, "Animate", NormalFont);
    addTextButton(ID_btAnimateRepeat, DrawPoint(250, 540), Extent(110, 22), TextureColor::Green1, "Animate-Repeat",
                  NormalFont);
    addTextButton(ID_btAnimateOscillate, DrawPoint(365, 540), Extent(110, 22), TextureColor::Green1,
                  "Animate-Oscillate", NormalFont);
    addTextButton(ID_btHideCtrls, DrawPoint(480, 540), Extent(140, 22), TextureColor::Green1, "Hide all elements (H)",
                  NormalFont);
    addTextButton(ID_btTextureTest, DrawPoint(625, 540), Extent(90, 22), TextureColor::Green1, "Texture test",
                  NormalFont);
    addTextButton(ID_btShowBenchmark, DrawPoint(720, 540), Extent(80, 22), TextureColor::Green1, "Benchmark",
                  NormalFont);
}

void dskTest::msgEditChange(const unsigned ctrl_id)
{
    if(ctrl_id == ID_edtTest)
        getCtrl<ctrlText>(ID_txtTest)->setText(getCtrl<ctrlEdit>(ID_edtTest)->getText());
}

void dskTest::msgComboSelectItem(const unsigned ctrl_id, const unsigned selection)
{
    if(ctrl_id == ID_cbTxtSize)
    {
        if(selection == 0)
            getCtrl<ctrlText>(ID_txtTest)->setFont(SmallFont);
        else if(selection == 1)
            getCtrl<ctrlText>(ID_txtTest)->setFont(NormalFont);
        else
            getCtrl<ctrlText>(ID_txtTest)->setFont(LargeFont);
    }
}

void dskTest::msgButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_btTextureTest: WINDOWMANAGER.switchTo(std::make_unique<dskTextureTest>()); break;
        case ID_btShowBenchmark: WINDOWMANAGER.switchTo(std::make_unique<dskBenchmark>()); break;
        case ID_btDisable:
            for(unsigned i = ID_grpBtStart; i < ID_grpBtEnd; i++)
            {
                auto* bt = getCtrl<ctrlButton>(i);
                if(bt)
                    bt->setEnabled(!bt->getEnabled());
            }
            break;
        case ID_btAnimate:
        case ID_btAnimateOscillate:
        case ID_btAnimateRepeat:
        {
            auto* btAniBg = getCtrl<ctrlButton>(ID_btAniBg);
            auto* btAni = getCtrl<ctrlButton>(ID_btAni);
            // Stop all
            getAnimationManager().removeElementAnimations(btAni->getID());
            getAnimationManager().finishElementAnimations(ID_btAnimate, false);
            getAnimationManager().finishElementAnimations(ID_btAnimateOscillate, false);
            getAnimationManager().finishElementAnimations(ID_btAnimateRepeat, false);
            DrawPoint startPos(btAniBg->getPos());
            startPos.y += 5;
            btAni->setPos(startPos);
            DrawPoint endPos(startPos);
            endPos.x += btAniBg->getSize().x - btAni->getSize().x;
            Animation::RepeatType repeat = Animation::RepeatType::None;
            if(ctrl_id == ID_btAnimateOscillate)
                repeat = Animation::RepeatType::Oscillate;
            else if(ctrl_id == ID_btAnimateRepeat)
                repeat = Animation::RepeatType::Repeat;
            getAnimationManager().addAnimation(new MoveAnimation(btAni, endPos, 4000, repeat));
            getAnimationManager().addAnimation(new BlinkButtonAnim(getCtrl<ctrlButton>(ctrl_id)));
            break;
        }
        case ID_btHideCtrls: toggleCtrlVisibility();
    }
}

bool dskTest::msgRightUp(const MouseCoords& mc)
{
    std::vector<ctrlButton*> bts = getCtrls<ctrlButton>();
    for(ctrlButton* bt : bts)
    {
        if(IsPointInRect(mc.pos, bt->getDrawRect()))
        {
            bt->setChecked(!bt->getCheck());
            return true;
        }
    }
    return dskMenuBase::msgRightUp(mc);
}

void dskTest::toggleCtrlVisibility()
{
    for(int i = ID_txtTitle; i <= ID_btHideCtrls; i++)
    {
        auto* ctrl = getCtrl<Window>(i);
        if(ctrl)
            ctrl->setVisible(!ctrl->isVisible());
    }
}

bool dskTest::msgKeyDown(const KeyEvent& ke)
{
    if(ke.kt == KeyType::Char && ke.c == 'h')
        toggleCtrlVisibility();
    else if(ke.kt == KeyType::Left)
    {
        curBGIdx = (curBGIdx > 0) ? curBGIdx - 1 : LOAD_SCREENS.size() - 1;
        background = LOADER.getImageN(ResourceId(LOAD_SCREENS[curBGIdx]), 0);
    } else if(ke.kt == KeyType::Right)
    {
        curBGIdx = (curBGIdx < LOAD_SCREENS.size() - 1) ? curBGIdx + 1 : 0;
        background = LOADER.getImageN(ResourceId(LOAD_SCREENS[curBGIdx]), 0);
    } else if(ke.kt == KeyType::Escape)
        WINDOWMANAGER.switchTo(std::make_unique<dskMainMenu>());
    else
        return false;
    return true;
}
