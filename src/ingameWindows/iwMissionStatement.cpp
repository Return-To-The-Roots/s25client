// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
    : IngameWindow(CGI_MISSION_STATEMENT, IngameWindow::posLastOrCenter, 640, 480, title, LOADER.GetImageN("io", 5), true, false), pauseGame_(pauseGame)
{
    glArchivItem_Bitmap* img = (image == IM_NONE) ? NULL : LOADER.GetImageN("io", image);
    const unsigned short imgWidth = img ? img->getWidth() : 0u;
    const unsigned short imgHeight = img ? img->getHeight() : 0u;
    const unsigned short minImgSpaceTop = img ? 60u : 0u;
    const unsigned short imgSpaceRight = img ? 20u : 0u;
    const unsigned short imgSpaceLeft = img ? 10u : 0u;
    const unsigned short imgSpace = imgSpaceRight + imgSpaceLeft;
    const unsigned short textSpace = 8u;
    const unsigned short buttonSpace = 12u;
    const unsigned short buttonHeight = 22u;

    const unsigned short maxTextWidth = GetIwWidth() - imgWidth - textSpace - imgSpace;
    ctrlMultiline* text = AddMultiline(0, contentOffset.x + textSpace, contentOffset.y + textSpace, maxTextWidth, GetIwHeight(), TC_GREEN2, NormalFont);
    text->ShowBackground(false);
    text->AddString(content, COLOR_YELLOW, false);
    text->Resize(text->GetContentWidth(), text->GetContentHeight());

    // set window width to our determined max width
    SetIwWidth(text->GetWidth() + textSpace + imgWidth + imgSpace);
    SetIwHeight(std::max<unsigned>(text->GetHeight() + textSpace + buttonHeight + buttonSpace*2, imgHeight + minImgSpaceTop + buttonSpace));
    
    AddTextButton(1, (width_ - 100) / 2, GetIwBottomBoundary() - buttonSpace - buttonHeight, 100, buttonHeight, TC_GREY, _("Continue"), NormalFont);
    if(img)
        AddImage(2, GetIwRightBoundary() - imgWidth + img->getNx() - imgSpaceRight, GetIwBottomBoundary() - buttonSpace - img->getHeight() + img->getNy(), img);
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
