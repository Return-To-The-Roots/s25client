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

#include "defines.h" // IWYU pragma: keep
#include "dskTest.h"

#include "WindowManager.h"
#include "Loader.h"
#include "animation/MoveAnimation.h"
#include "desktops/dskMainMenu.h"
#include "controls/ctrlButton.h"
#include "ogl/glArchivItem_Font.h"
#include "libutil/src/colors.h"
#include <boost/foreach.hpp>

namespace{
    enum{
        ID_txtTitle = dskMenuBase::ID_FIRST_FREE,
        ID_grpBtStart,
        ID_grpBtEnd = ID_grpBtStart + 5 * 4,
        ID_btDisable,
        ID_btAniBg, ID_btAni,
        ID_btAnimate, ID_btAnimateRepeat, ID_btAnimateOscillate,
        ID_btBack
    };
}

dskTest::dskTest()
{
    AddText(ID_txtTitle, 300, 20, _("Internal test screen for developers"), COLOR_ORANGE, glArchivItem_Font::DF_CENTER, LargeFont);
    boost::array<TextureColor, 4> textures = { { TC_GREEN1, TC_GREEN2, TC_RED1, TC_GREY } };
    boost::array<std::string, 4> labels = { { "Green1", "Green2", "Red1", "Grey" } };
    unsigned yPos = 50;
    unsigned curId = ID_grpBtStart;
    for(unsigned i = 0; i < textures.size(); i++){
        AddText(curId, 10, yPos + 3, labels.at(i), COLOR_YELLOW, glArchivItem_Font::DF_LEFT, NormalFont);
        ctrlTextButton* bt;
        bt = AddTextButton(curId + 1, 120, yPos, 95, 22, textures[i], "Nothing", NormalFont);
        bt->SetIlluminated(false);
        bt->SetBorder(false);
        bt = AddTextButton(curId + 2, 220, yPos, 95, 22, textures[i], "Illuminated", NormalFont);
        bt->SetIlluminated(true);
        bt->SetBorder(false);
        bt = AddTextButton(curId + 3, 320, yPos, 95, 22, textures[i], "Border", NormalFont);
        bt->SetIlluminated(false);
        bt->SetBorder(true);
        bt = AddTextButton(curId + 4, 420, yPos, 95, 22, textures[i], "Both", NormalFont);
        bt->SetIlluminated(true);
        bt->SetBorder(true);
        curId += 5;
        yPos += 30;
    }
    RTTR_Assert(curId == ID_grpBtEnd);

    ctrlTextButton* bt = AddTextButton(ID_btAniBg, 10, yPos, 700, 40, TC_GREEN2, "", NormalFont);
    bt->SetBorder(false);
    bt->SetEnabled(false);
    bt = AddTextButton(ID_btAni, bt->GetPos().x, bt->GetPos().y + 5, 30, 30, TC_RED1, "", NormalFont);
    bt->SetBorder(false);
    bt->SetEnabled(false);
    bt->SetIlluminated(true);

    AddTextButton(ID_btDisable, 10, 550, 200, 22, TC_GREEN1, "Enable/Disable buttons", NormalFont);
    AddTextButton(ID_btAnimate, 215, 550, 100, 22, TC_GREEN1, "Animate", NormalFont);
    AddTextButton(ID_btAnimateRepeat, 320, 550, 130, 22, TC_GREEN1, "Animate-Repeat", NormalFont);
    AddTextButton(ID_btAnimateOscillate, 455, 550, 130, 22, TC_GREEN1, "Animate-Oscillate", NormalFont);
    AddTextButton(ID_btBack, 630, 550, 150, 22, TC_RED1, _("Back"), NormalFont);
}

void dskTest::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_btBack:
            WINDOWMANAGER.Switch(new dskMainMenu);
            break;
        case ID_btDisable:
            for(unsigned i = ID_grpBtStart; i < ID_grpBtEnd; i++)
            {
                ctrlTextButton* bt = GetCtrl<ctrlTextButton>(i);
                if(bt)
                    bt->SetEnabled(!bt->GetEnabled());
            }
            break;
        case ID_btAnimate:
        case ID_btAnimateOscillate:
        case ID_btAnimateRepeat:
        {
            ctrlTextButton* btAniBg = GetCtrl<ctrlTextButton>(ID_btAniBg);
            ctrlTextButton* btAni = GetCtrl<ctrlTextButton>(ID_btAni);
            std::vector<Animation*> anims = GetAnimationManager().getElementAnimations(btAni->GetID());
            // Stop all
            BOOST_FOREACH(Animation* anim, anims)
            {
                GetAnimationManager().removeAnimation(GetAnimationManager().getAnimationId(anim));
            }
            DrawPoint startPos(btAniBg->GetPos());
            startPos.y += 5;
            btAni->Move(startPos);
            DrawPoint endPos(startPos);
            endPos.x += btAniBg->GetWidth() - btAni->GetWidth();
            Animation::RepeatType repeat = Animation::RPT_None;
            if(ctrl_id == ID_btAnimateOscillate)
                repeat = Animation::RPT_Oscillate;
            else if(ctrl_id == ID_btAnimateRepeat)
                repeat = Animation::RPT_Repeat;
            GetAnimationManager().addAnimation(new MoveAnimation(btAni, endPos, 4000, repeat));
        }
    }
}

