// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwMsgbox.h"
#include "Loader.h"
#include "WindowManager.h"
#include "controls/ctrlImage.h"
#include "controls/ctrlMultiline.h"
#include "drivers/VideoDriverWrapper.h"
#include "enum_cast.hpp"
#include "helpers/EnumArray.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "gameData/const_gui_ids.h"

namespace {
enum IDS
{
    ID_ICON,
    ID_TEXT,
    ID_BT_0
};
const Extent btSize(90, 20);
const unsigned short paddingX = 15; /// Padding in X/to image
const unsigned short minTextWidth = 150;
const unsigned short maxTextHeight = 200;
} // namespace

iwMsgbox::iwMsgbox(const std::string& title, const std::string& text, Window* msgHandler, MsgboxButton button,
                   MsgboxIcon icon, unsigned msgboxid)
    : iwMsgbox(title, text, msgHandler, button, "io", rttr::enum_cast(icon), msgboxid)
{}

iwMsgbox::iwMsgbox(const std::string& title, const std::string& text, Window* msgHandler, MsgboxButton button,
                   const ResourceId& iconFile, unsigned iconIdx, unsigned msgboxid /* = 0 */)
    : IngameWindow(CGI_MSGBOX, IngameWindow::posLastOrCenter, Extent(420, 140), title, LOADER.getImageN("resource", 41),
                   true, CloseBehavior::Custom),
      button(button), msgboxid(msgboxid), msgHandler_(msgHandler)
{
    Init(text, iconFile, iconIdx);
}

void iwMsgbox::Init(const std::string& text, const ResourceId& iconFile, unsigned iconIdx)
{
    glArchivItem_Bitmap* icon = LOADER.getImageN(iconFile, iconIdx);
    if(icon)
        AddImage(ID_ICON, contentOffset + DrawPoint(30, 20), icon);
    int textX = icon ? icon->getWidth() - icon->getNx() + getCtrl<Window>(ID_ICON)->getPos().x : contentOffset.x;
    textX += paddingX;
    Extent txtSize = Extent(std::max<int>(minTextWidth, getRightBottomBoundary().x - textX - paddingX), maxTextHeight);
    ctrlMultiline* multiline =
      AddMultiline(ID_TEXT, DrawPoint(textX, contentOffset.y + 5), txtSize, TextureColor::Green2, NormalFont);
    multiline->ShowBackground(false);
    multiline->AddString(text, COLOR_YELLOW);
    multiline->Resize(multiline->getContentSize());
    // 10 padding, button/button padding
    Extent newIwSize(multiline->getPos() + multiline->getSize() + Extent(paddingX, 10 + btSize.y * 2));
    // Increase window size if required
    setIwSize(elMax(getIwSize(), newIwSize));

    unsigned defaultBt = 0;
    // Buttons erstellen
    switch(button)
    {
        case MsgboxButton::Ok:
            AddButton(ID_BT_0, getSize().x / 2 - 45, _("OK"), TextureColor::Green2);
            defaultBt = 0;
            break;

        case MsgboxButton::OkCancel:
            AddButton(ID_BT_0, getSize().x / 2 - 3 - 90, _("OK"), TextureColor::Green2);
            AddButton(ID_BT_0 + 1, getSize().x / 2 + 3, _("Cancel"), TextureColor::Red1);
            defaultBt = 1;
            break;

        case MsgboxButton::YesNo:
            AddButton(ID_BT_0, getSize().x / 2 - 3 - 90, _("Yes"), TextureColor::Green2);
            AddButton(ID_BT_0 + 1, getSize().x / 2 + 3, _("No"), TextureColor::Red1);
            defaultBt = 1;
            break;

        case MsgboxButton::YesNoCancel:
            AddButton(ID_BT_0, getSize().x / 2 - 45 - 6 - 90, _("Yes"), TextureColor::Green2);
            AddButton(ID_BT_0 + 1, getSize().x / 2 - 45, _("No"), TextureColor::Red1);
            AddButton(ID_BT_0 + 2, getSize().x / 2 + 45 + 6, _("Cancel"), TextureColor::Grey);
            defaultBt = 2;
            break;
    }
    const Window* defBt = getCtrl<Window>(defaultBt + ID_BT_0);
    if(defBt)
        VIDEODRIVER.setMousePos(defBt->getDrawPos() + DrawPoint(defBt->getSize()) / 2);
    WINDOWMANAGER.setCursor();
}

iwMsgbox::~iwMsgbox() = default;

void iwMsgbox::MoveIcon(const DrawPoint& pos)
{
    auto* icon = getCtrl<ctrlImage>(ID_ICON);
    if(icon)
    {
        icon->setPos(elMax(pos, DrawPoint(0, 0)));
        const ITexture* iconImg = icon->getImage();
        DrawPoint iconPos(icon->getPos() - iconImg->getOrigin());
        DrawPoint textPos = contentOffset + DrawPoint(paddingX, 5);
        Extent textMaxSize;
        if(iconPos.x < 100)
        {
            // icon left
            textPos.x = iconPos.x + iconImg->getSize().x + paddingX;
            textMaxSize.x = std::max<int>(minTextWidth, 400 - textPos.x - paddingX);
            textMaxSize.y = maxTextHeight;
        } else if(iconPos.x > 300)
        {
            // icon right
            textMaxSize.x = iconPos.x - 2 * paddingX;
            textMaxSize.y = maxTextHeight;
        } else if(iconPos.y + iconImg->getSize().y < 50)
        {
            // icon top
            textPos.y = iconPos.y + iconImg->getSize().x + paddingX;
            textMaxSize.x = 400 - 2 * paddingX;
            textMaxSize.y = maxTextHeight;
        } else if(iconPos.y > 150)
        {
            // icon bottom
            textMaxSize.x = 400 - 2 * paddingX;
            textMaxSize.y = iconPos.y - textPos.y;
        } else
        {
            // Icon middle -> Overlay text
            textMaxSize.x = 400 - 2 * paddingX;
            textMaxSize.y = maxTextHeight;
        }
        auto* multiline = getCtrl<ctrlMultiline>(ID_TEXT);
        multiline->setPos(textPos);
        multiline->Resize(textMaxSize);
        multiline->Resize(multiline->getContentSize());

        DrawPoint newSize = iconPos + DrawPoint(iconImg->getSize());
        newSize = elMax(newSize, multiline->getPos() + DrawPoint(multiline->getSize()) + DrawPoint::all(paddingX));
        newSize += DrawPoint(0, 10 + btSize.y * 2) + DrawPoint(contentOffsetEnd);
        DrawPoint btMoveDelta(newSize - getSize());
        btMoveDelta.x /= 2;
        for(unsigned i = 0; i < 3; i++)
        {
            auto* bt = getCtrl<Window>(i + ID_BT_0);
            if(bt)
                bt->setPos(bt->getPos() + btMoveDelta);
        }
        Resize(Extent(newSize));
    }
}

constexpr helpers::EnumArray<std::array<MsgboxResult, 3>, MsgboxButton> RET_IDS = {
  {{MsgboxResult::Ok},
   {MsgboxResult::Ok, MsgboxResult::Cancel},
   {MsgboxResult::Yes, MsgboxResult::No},
   {MsgboxResult::Yes, MsgboxResult::No, MsgboxResult::Cancel}}};

void iwMsgbox::msgButtonClick(const unsigned ctrl_id)
{
    if(msgHandler_)
        msgHandler_->msgMsgBoxResult(msgboxid, RET_IDS[button][ctrl_id - ID_BT_0]);
    Close();
}

void iwMsgbox::AddButton(unsigned short id, int x, const std::string& text, const TextureColor tc)
{
    AddTextButton(id, DrawPoint(x, getRightBottomBoundary().y - btSize.y * 2), btSize, tc, text, NormalFont);
}
