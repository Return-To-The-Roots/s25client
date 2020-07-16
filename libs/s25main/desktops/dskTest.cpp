// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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
    AddText(ID_txtTitle, DrawPoint(300, 20), _("Internal test screen for developers"), COLOR_ORANGE, FontStyle::CENTER, LargeFont);
    std::array<TextureColor, 4> textures = {{TC_GREEN1, TC_GREEN2, TC_RED1, TC_GREY}};
    std::array<std::string, 4> labels = {{"Green1", "Green2", "Red1", "Grey"}};
    unsigned yPos = 50;
    unsigned curId = ID_grpBtStart;
    for(unsigned i = 0; i < textures.size(); i++)
    {
        AddText(curId, DrawPoint(10, yPos + 3), labels.at(i), COLOR_YELLOW, FontStyle::LEFT, NormalFont);
        ctrlButton* bt;
        bt = AddTextButton(curId + 1, DrawPoint(120, yPos), Extent(95, 22), textures[i], "Nothing", NormalFont);
        bt->SetIlluminated(false);
        bt->SetBorder(false);
        bt = AddTextButton(curId + 2, DrawPoint(220, yPos), Extent(95, 22), textures[i], "Illuminated", NormalFont);
        bt->SetIlluminated(true);
        bt->SetBorder(false);
        bt = AddTextButton(curId + 3, DrawPoint(320, yPos), Extent(95, 22), textures[i], "Border", NormalFont);
        bt->SetIlluminated(false);
        bt->SetBorder(true);
        bt = AddTextButton(curId + 4, DrawPoint(420, yPos), Extent(95, 22), textures[i], "Both", NormalFont);
        bt->SetIlluminated(true);
        bt->SetBorder(true);
        curId += 5;
        yPos += 30;
    }
    RTTR_Assert(curId == ID_grpBtEnd);

    DrawPoint btPos(10, yPos);
    ctrlButton* bt = AddTextButton(ID_btAniBg, btPos, Extent(700, 40), TC_GREEN2, "", NormalFont);
    bt->SetBorder(false);
    bt->SetEnabled(false);
    bt = AddTextButton(ID_btAni, btPos + DrawPoint(0, 5), Extent(30, 30), TC_RED1, "", NormalFont);
    bt->SetBorder(false);
    bt->SetEnabled(false);
    bt->SetIlluminated(true);

    btPos.y += 45;
    AddEdit(ID_edtTest, btPos, Extent(150, 22), TC_GREEN2, NormalFont, 0, false, false, true);
    btPos.x += 170;
    ctrlComboBox* cb = AddComboBox(ID_cbTxtSize, btPos, Extent(100, 22), TC_GREEN2, NormalFont, 100);
    cb->AddString("Small Font");
    cb->AddString("Medium Font");
    cb->AddString("Large Font");
    cb->SetSelection(0);
    btPos.x += 110;
    btPos.y += 11;
    AddText(ID_txtTest, btPos, "Enter something", COLOR_YELLOW, FontStyle::VCENTER, SmallFont);

    AddTextButton(ID_btDisable, DrawPoint(10, 540), Extent(150, 22), TC_GREEN1, "Enable/Disable buttons", NormalFont);
    AddTextButton(ID_btAnimate, DrawPoint(165, 540), Extent(80, 22), TC_GREEN1, "Animate", NormalFont);
    AddTextButton(ID_btAnimateRepeat, DrawPoint(250, 540), Extent(110, 22), TC_GREEN1, "Animate-Repeat", NormalFont);
    AddTextButton(ID_btAnimateOscillate, DrawPoint(365, 540), Extent(110, 22), TC_GREEN1, "Animate-Oscillate", NormalFont);
    AddTextButton(ID_btHideCtrls, DrawPoint(480, 540), Extent(140, 22), TC_GREEN1, "Hide all elements (H)", NormalFont);
    AddTextButton(ID_btTextureTest, DrawPoint(625, 540), Extent(90, 22), TC_GREEN1, "Texture test", NormalFont);
    AddTextButton(ID_btShowBenchmark, DrawPoint(720, 540), Extent(80, 22), TC_GREEN1, "Benchmark", NormalFont);
}

void dskTest::Msg_EditChange(const unsigned ctrl_id)
{
    if(ctrl_id == ID_edtTest)
        GetCtrl<ctrlText>(ID_txtTest)->SetText(GetCtrl<ctrlEdit>(ID_edtTest)->GetText());
}

void dskTest::Msg_ComboSelectItem(const unsigned ctrl_id, const unsigned selection)
{
    if(ctrl_id == ID_cbTxtSize)
    {
        if(selection == 0)
            GetCtrl<ctrlText>(ID_txtTest)->SetFont(SmallFont);
        else if(selection == 1)
            GetCtrl<ctrlText>(ID_txtTest)->SetFont(NormalFont);
        else
            GetCtrl<ctrlText>(ID_txtTest)->SetFont(LargeFont);
    }
}

void dskTest::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_btTextureTest: WINDOWMANAGER.Switch(std::make_unique<dskTextureTest>()); break;
        case ID_btShowBenchmark: WINDOWMANAGER.Switch(std::make_unique<dskBenchmark>()); break;
        case ID_btDisable:
            for(unsigned i = ID_grpBtStart; i < ID_grpBtEnd; i++)
            {
                auto* bt = GetCtrl<ctrlButton>(i);
                if(bt)
                    bt->SetEnabled(!bt->GetEnabled());
            }
            break;
        case ID_btAnimate:
        case ID_btAnimateOscillate:
        case ID_btAnimateRepeat:
        {
            auto* btAniBg = GetCtrl<ctrlButton>(ID_btAniBg);
            auto* btAni = GetCtrl<ctrlButton>(ID_btAni);
            // Stop all
            GetAnimationManager().removeElementAnimations(btAni->GetID());
            GetAnimationManager().finishElementAnimations(ID_btAnimate, false);
            GetAnimationManager().finishElementAnimations(ID_btAnimateOscillate, false);
            GetAnimationManager().finishElementAnimations(ID_btAnimateRepeat, false);
            DrawPoint startPos(btAniBg->GetPos());
            startPos.y += 5;
            btAni->SetPos(startPos);
            DrawPoint endPos(startPos);
            endPos.x += btAniBg->GetSize().x - btAni->GetSize().x;
            Animation::RepeatType repeat = Animation::RPT_None;
            if(ctrl_id == ID_btAnimateOscillate)
                repeat = Animation::RPT_Oscillate;
            else if(ctrl_id == ID_btAnimateRepeat)
                repeat = Animation::RPT_Repeat;
            GetAnimationManager().addAnimation(new MoveAnimation(btAni, endPos, 4000, repeat));
            GetAnimationManager().addAnimation(new BlinkButtonAnim(GetCtrl<ctrlButton>(ctrl_id)));
            break;
        }
        case ID_btHideCtrls: ToggleCtrlVisibility();
    }
}

bool dskTest::Msg_RightUp(const MouseCoords& mc)
{
    std::vector<ctrlButton*> bts = GetCtrls<ctrlButton>();
    for(ctrlButton* bt : bts)
    {
        if(IsPointInRect(mc.GetPos(), bt->GetDrawRect()))
        {
            bt->SetChecked(!bt->GetCheck());
            return true;
        }
    }
    return dskMenuBase::Msg_RightUp(mc);
}

void dskTest::ToggleCtrlVisibility()
{
    for(int i = ID_txtTitle; i <= ID_btHideCtrls; i++)
    {
        auto* ctrl = GetCtrl<Window>(i);
        if(ctrl)
            ctrl->SetVisible(!ctrl->IsVisible());
    }
}

bool dskTest::Msg_KeyDown(const KeyEvent& ke)
{
    if(ke.kt == KT_CHAR && ke.c == 'h')
        ToggleCtrlVisibility();
    else if(ke.kt == KT_LEFT)
    {
        curBGIdx = (curBGIdx > 0) ? curBGIdx - 1 : LOAD_SCREENS.size() - 1;
        background = LOADER.GetImageN(LOAD_SCREENS[curBGIdx], 0);
    } else if(ke.kt == KT_RIGHT)
    {
        curBGIdx = (curBGIdx < LOAD_SCREENS.size() - 1) ? curBGIdx + 1 : 0;
        background = LOADER.GetImageN(LOAD_SCREENS[curBGIdx], 0);
    } else if(ke.kt == KT_ESCAPE)
        WINDOWMANAGER.Switch(std::make_unique<dskMainMenu>());
    else
        return false;
    return true;
}
