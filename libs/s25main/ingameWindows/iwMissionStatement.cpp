// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
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
                   LOADER.getImageN("io", 5), true, CloseBehavior::Custom),
      pauseGame_(pauseGame && !GAMECLIENT.isReplayModeOn())
{
    glArchivItem_Bitmap* img = (image == IM_NONE) ? nullptr : LOADER.getImageN("io", image);
    const Extent imgSize(img ? img->getSize() : Extent::all(0));
    const unsigned minImgSpaceTop = img ? 60u : 0u;
    const unsigned imgSpaceRight = img ? 20u : 0u;
    const unsigned imgSpaceLeft = img ? 10u : 0u;
    const unsigned imgSpace = imgSpaceRight + imgSpaceLeft;
    const unsigned textSpace = 8u;
    const unsigned buttonSpace = 12u;
    const Extent buttonSize(100, 22);

    const unsigned short maxTextWidth = getIwSize().x - imgSize.x - textSpace - imgSpace;
    ctrlMultiline* text = AddMultiline(0, contentOffset + DrawPoint::all(textSpace),
                                       Extent(maxTextWidth, getIwSize().y), TextureColor::Green2, NormalFont);
    text->ShowBackground(false);
    text->AddString(content, COLOR_YELLOW, false);
    text->Resize(text->getContentSize());

    // set window width to our determined max width
    Extent newIwSize = text->getSize() + Extent::all(textSpace) + Extent(imgSize.x + imgSpace, buttonSize.y + 2);
    newIwSize = elMax(newIwSize, buttonSize);
    newIwSize.y = std::max(newIwSize.y, imgSize.y + minImgSpaceTop) + buttonSpace;
    setIwSize(newIwSize);

    AddTextButton(1,
                  DrawPoint((getSize().x - buttonSize.x) / 2, getRightBottomBoundary().y - buttonSpace - buttonSize.y),
                  buttonSize, TextureColor::Grey, _("Continue"), NormalFont);
    if(img)
    {
        DrawPoint imgPos =
          getRightBottomBoundary() + img->getOrigin() - DrawPoint(imgSize) - DrawPoint(imgSpaceRight, buttonSpace);
        AddImage(2, imgPos, img);
    }
}

void iwMissionStatement::msgButtonClick(const unsigned /*ctrl_id*/)
{
    if(pauseGame_ && GAMECLIENT.getState() != ClientState::Stopped)
        GAMECLIENT.setPause(false);
    Close();
}

void iwMissionStatement::setActive(bool activate)
{
    IngameWindow::setActive(activate);
    if(IsActive() && pauseGame_ && GAMECLIENT.getState() != ClientState::Stopped)
        GAMECLIENT.setPause(true);
}
