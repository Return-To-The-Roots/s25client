// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwMissionStatement.h"
#include "Loader.h"
#include "controls/ctrlMultiline.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "gameData/const_gui_ids.h"

iwMissionStatement::iwMissionStatement(const std::string& title, const std::string& content, bool pauseGame,
                                       HelpImage image)
    : IngameWindow(CGI_MISSION_STATEMENT, IngameWindow::posLastOrCenter, Extent(640, 480), title,
                   LOADER.GetImageN("io", 5), true, false),
      pauseGame_(pauseGame)
{
    glArchivItem_Bitmap* img = (image == IM_NONE) ? nullptr : LOADER.GetImageN("io", image);
    const Extent imgSize(img ? img->GetSize() : Extent::all(0));
    const unsigned minImgSpaceTop = img ? 60u : 0u;
    const unsigned imgSpaceRight = img ? 20u : 0u;
    const unsigned imgSpaceLeft = img ? 10u : 0u;
    const unsigned imgSpace = imgSpaceRight + imgSpaceLeft;
    const unsigned textSpace = 8u;
    const unsigned buttonSpace = 12u;
    const Extent buttonSize(100, 22);

    const unsigned short maxTextWidth = GetIwSize().x - imgSize.x - textSpace - imgSpace;
    ctrlMultiline* text = AddMultiline(0, contentOffset + DrawPoint::all(textSpace),
                                       Extent(maxTextWidth, GetIwSize().y), TextureColor::Green2, NormalFont);
    text->ShowBackground(false);
    text->AddString(content, COLOR_YELLOW, false);
    text->Resize(text->GetContentSize());

    // set window width to our determined max width
    Extent newIwSize = text->GetSize() + Extent::all(textSpace) + Extent(imgSize.x + imgSpace, buttonSize.y + 2);
    newIwSize = elMax(newIwSize, buttonSize);
    newIwSize.y = std::max(newIwSize.y, imgSize.y + minImgSpaceTop) + buttonSpace;
    SetIwSize(newIwSize);

    AddTextButton(1,
                  DrawPoint((GetSize().x - buttonSize.x) / 2, GetRightBottomBoundary().y - buttonSpace - buttonSize.y),
                  buttonSize, TextureColor::Grey, _("Continue"), NormalFont);
    if(img)
    {
        DrawPoint imgPos =
          GetRightBottomBoundary() + img->GetOrigin() - DrawPoint(imgSize) - DrawPoint(imgSpaceRight, buttonSpace);
        AddImage(2, imgPos, img);
    }
}

void iwMissionStatement::Msg_ButtonClick(const unsigned /*ctrl_id*/)
{
    if(pauseGame_ && GAMECLIENT.GetState() != ClientState::Stopped)
        GAMECLIENT.SetPause(false);
    Close();
}

void iwMissionStatement::SetActive(bool activate)
{
    IngameWindow::SetActive(activate);
    if(IsActive() && pauseGame_ && GAMECLIENT.GetState() != ClientState::Stopped)
        GAMECLIENT.SetPause(true);
}
