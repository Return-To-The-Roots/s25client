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

/**
 *  Konstruktor von @p iwTextfile.
 *
 *  @todo Das Fenster ist zu klein, um 80 Zeichen pro Zeile darstellen zu
 *        koennen. Ein horizontaler Scrollbalken fuer 640x480 waere nicht
 *        schlecht.
 */
iwMissionStatement::iwMissionStatement(const std::string& title, const std::string& content)
    : IngameWindow(CGI_MISSION_STATEMENT, IngameWindow::posLastOrCenter, 640, 480, title, LOADER.GetImageN("io", 5), true, false)
{
    glArchivItem_Bitmap* img = LOADER.GetImageN("io", 234);
    const unsigned short imgWidth = img->getWidth();
    const unsigned short minImgSpaceTop = 60u;
    const unsigned short imgSpaceRight = 20u;
    const unsigned short imgSpaceLeft = 10u;
    const unsigned short imgSpace = imgSpaceRight + imgSpaceLeft;
    const unsigned short textSpace = 8u;
    const unsigned short buttonSpace = 12u;
    const unsigned short buttonHeight = 22u;

    const unsigned short maxTextWidth = GetIwWidth() - imgWidth - textSpace - imgSpace;
    ctrlMultiline* text = AddMultiline(0, contentOffset.x + textSpace, contentOffset.y + textSpace, maxTextWidth, GetIwHeight(), TC_GREEN2, NormalFont, glArchivItem_Font::DF_LEFT | glArchivItem_Font::DF_TOP);
    text->EnableBox(false);

    glArchivItem_Font::WrapInfo wi = NormalFont->GetWrapInfo(content, text->GetWidth(), text->GetWidth());
    std::vector<std::string> lines = wi.CreateSingleStrings(content);

    unsigned short max_line_width = 0;
    BOOST_FOREACH(const std::string& line, lines)
    {
        text->AddString(line, COLOR_YELLOW, false); // add this line to the window contents
        unsigned short current_line_width = NormalFont->getWidth(line); // get the width of line in normal font
        if (current_line_width > max_line_width) // if wider than max, re-set max
            max_line_width = current_line_width;
    }

    // set window width to our determined max width
    SetIwWidth(max_line_width + textSpace + imgWidth + imgSpace);
    SetIwHeight(std::max<unsigned>(text->GetLineCount() * NormalFont->getHeight() + textSpace + buttonHeight + buttonSpace*2, img->getHeight() + minImgSpaceTop + buttonSpace));
    
    AddTextButton(1, (width_ - 100) / 2, GetIwBottomBoundary() - buttonSpace - buttonHeight, 100, buttonHeight, TC_GREY, _("Continue"), NormalFont);
    AddImage(2, GetIwRightBoundary() - imgWidth + img->getNx() - imgSpaceRight, GetIwBottomBoundary() - buttonSpace - img->getHeight() + img->getNy(), img);
}

void iwMissionStatement::Msg_ButtonClick(const unsigned int  /*ctrl_id*/)
{
    // TODO: Make something better, this is quite hacky (Client and server dependency)
    if(GAMECLIENT.IsSinglePlayer())
    {
        RTTR_Assert(GAMECLIENT.IsHost());
        GAMESERVER.SetPaused(false);
    }
    Close();
}

void iwMissionStatement::SetActive(bool activate)
{
    IngameWindow::SetActive(activate);
    // TODO: Make something better, this is quite hacky (Client and server dependency)
    if(IsActive() && GAMECLIENT.IsSinglePlayer())
    {
        RTTR_Assert(GAMECLIENT.IsHost());
        GAMESERVER.SetPaused(true);
    }
}

