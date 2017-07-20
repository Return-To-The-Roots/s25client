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
#include "iwMissionStatement.h"
#include "controls/ctrlMultiline.h"
#include "Loader.h"
#include "ogl/glArchivItem_Font.h"
#include "GameClient.h"
#include "GameServer.h"
#include "gameData/const_gui_ids.h"
#include <boost/foreach.hpp>
#include <vector>

iwMissionStatement::iwMissionStatement(const std::string& title, const std::string& content, bool pauseGame, HelpImage image)
    : IngameWindow(CGI_MISSION_STATEMENT, IngameWindow::posLastOrCenter, Extent(640, 480), title, LOADER.GetImageN("io", 5), true, false), pauseGame_(pauseGame)
{
    glArchivItem_Bitmap* img = (image == IM_NONE) ? NULL : LOADER.GetImageN("io", image);
    const Extent imgSize(img ? img->GetSize() : Extent::all(0));
    const unsigned minImgSpaceTop = img ? 60u : 0u;
    const unsigned imgSpaceRight = img ? 20u : 0u;
    const unsigned imgSpaceLeft = img ? 10u : 0u;
    const unsigned imgSpace = imgSpaceRight + imgSpaceLeft;
    const unsigned textSpace = 8u;
    const unsigned buttonSpace = 12u;
    const Extent buttonSize(100, 22);

    const unsigned short maxTextWidth = GetIwSize().x - imgSize.x - textSpace - imgSpace;
    ctrlMultiline* text = AddMultiline(0, contentOffset + DrawPoint::all(textSpace), Extent(maxTextWidth, GetIwSize().y), TC_GREEN2, NormalFont);
    text->ShowBackground(false);
    text->AddString(content, COLOR_YELLOW, false);
    text->Resize(text->GetContentSize());

    // set window width to our determined max width
    Extent newIwSize = text->GetSize() + Extent::all(textSpace) + Extent(imgSize.x + imgSpace, buttonSize.y + 2);
    newIwSize.y = std::max(newIwSize.y, imgSize.y + minImgSpaceTop) + buttonSpace;
    SetIwSize(newIwSize);
    
    AddTextButton(1, DrawPoint((GetSize().x - buttonSize.x) / 2, GetRightBottomBoundary().y - buttonSpace - buttonSize.y), buttonSize, TC_GREY, _("Continue"), NormalFont);
    if(img)
    {
        DrawPoint imgPos = GetRightBottomBoundary() + img->GetOrigin() - DrawPoint(imgSize) - DrawPoint(imgSpaceRight, buttonSpace);
        AddImage(2, imgPos, img);
    }
}

void iwMissionStatement::Msg_ButtonClick(const unsigned int  /*ctrl_id*/)
{
    // TODO: Make something better, this is quite hacky (Client and server dependency)
    if(pauseGame_)
        GAMESERVER.SetPaused(false);
    Close();
}

void iwMissionStatement::SetActive(bool activate)
{
    IngameWindow::SetActive(activate);
    // TODO: Make something better, this is quite hacky (Client and server dependency)
    if(IsActive() && pauseGame_)
        GAMESERVER.SetPaused(true);
}
