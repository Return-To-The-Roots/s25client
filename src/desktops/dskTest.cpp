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

#include "rttrDefines.h" // IWYU pragma: keep
#include "dskTest.h"

#include "Loader.h"
#include "WindowManager.h"
#include "animation/BlinkButtonAnim.h"
#include "animation/MoveAnimation.h"
#include "controls/ctrlButton.h"
#include "desktops/dskMainMenu.h"
#include "ogl/glArchivItem_Font.h"
#include "libutil/colors.h"
#include <boost/foreach.hpp>

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
    ID_btBack
};
}

dskTest::dskTest()
{
    AddText(ID_txtTitle, DrawPoint(300, 20), _("Internal test screen for developers"), COLOR_ORANGE, glArchivItem_Font::DF_CENTER,
            LargeFont);
    boost::array<TextureColor, 4> textures = {{TC_GREEN1, TC_GREEN2, TC_RED1, TC_GREY}};
    boost::array<std::string, 4> labels = {{"Green1", "Green2", "Red1", "Grey"}};
    unsigned yPos = 50;
    unsigned curId = ID_grpBtStart;
    for(unsigned i = 0; i < textures.size(); i++)
    {
        AddText(curId, DrawPoint(10, yPos + 3), labels.at(i), COLOR_YELLOW, glArchivItem_Font::DF_LEFT, NormalFont);
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

    AddTextButton(ID_btDisable, DrawPoint(10, 550), Extent(200, 22), TC_GREEN1, "Enable/Disable buttons", NormalFont);
    AddTextButton(ID_btAnimate, DrawPoint(215, 550), Extent(100, 22), TC_GREEN1, "Animate", NormalFont);
    AddTextButton(ID_btAnimateRepeat, DrawPoint(320, 550), Extent(130, 22), TC_GREEN1, "Animate-Repeat", NormalFont);
    AddTextButton(ID_btAnimateOscillate, DrawPoint(455, 550), Extent(130, 22), TC_GREEN1, "Animate-Oscillate", NormalFont);
    AddTextButton(ID_btBack, DrawPoint(630, 550), Extent(150, 22), TC_RED1, _("Back"), NormalFont);
}

void dskTest::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_btBack: WINDOWMANAGER.Switch(new dskMainMenu); break;
        case ID_btDisable:
            for(unsigned i = ID_grpBtStart; i < ID_grpBtEnd; i++)
            {
                ctrlButton* bt = GetCtrl<ctrlButton>(i);
                if(bt)
                    bt->SetEnabled(!bt->GetEnabled());
            }
            break;
        case ID_btAnimate:
        case ID_btAnimateOscillate:
        case ID_btAnimateRepeat:
        {
            ctrlButton* btAniBg = GetCtrl<ctrlButton>(ID_btAniBg);
            ctrlButton* btAni = GetCtrl<ctrlButton>(ID_btAni);
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
        }
    }
}
