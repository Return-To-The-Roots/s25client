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
    : IngameWindow(CGI_MSGBOX, IngameWindow::posLastOrCenter, Extent(420, 140), title, LOADER.GetImageN("resource", 41),
                   true, CloseBehavior::Custom),
      button(button), msgboxid(msgboxid), msgHandler_(msgHandler)
{
    Init(text, iconFile, iconIdx);
}

void iwMsgbox::Init(const std::string& text, const ResourceId& iconFile, unsigned iconIdx)
{
    glArchivItem_Bitmap* icon = LOADER.GetImageN(iconFile, iconIdx);
    if(icon)
        AddImage(ID_ICON, contentOffset + DrawPoint(30, 20), icon);
    int textX = icon ? icon->getWidth() - icon->getNx() + GetCtrl<Window>(ID_ICON)->GetPos().x : contentOffset.x;
    textX += paddingX;
    Extent txtSize = Extent(std::max<int>(minTextWidth, GetRightBottomBoundary().x - textX - paddingX), maxTextHeight);
    ctrlMultiline* multiline =
      AddMultiline(ID_TEXT, DrawPoint(textX, contentOffset.y + 5), txtSize, TextureColor::Green2, NormalFont);
    multiline->ShowBackground(false);
    multiline->AddString(text, COLOR_YELLOW);
    multiline->Resize(multiline->GetContentSize());
    // 10 padding, button/button padding
    Extent newIwSize(multiline->GetPos() + multiline->GetSize() + Extent(paddingX, 10 + btSize.y * 2));
    // Increase window size if required
    SetIwSize(elMax(GetIwSize(), newIwSize));

    unsigned defaultBt = 0;
    // Buttons erstellen
    switch(button)
    {
        case MsgboxButton::Ok:
            AddButton(ID_BT_0, GetSize().x / 2 - 45, _("OK"), TextureColor::Green2);
            defaultBt = 0;
            break;

        case MsgboxButton::OkCancel:
            AddButton(ID_BT_0, GetSize().x / 2 - 3 - 90, _("OK"), TextureColor::Green2);
            AddButton(ID_BT_0 + 1, GetSize().x / 2 + 3, _("Cancel"), TextureColor::Red1);
            defaultBt = 1;
            break;

        case MsgboxButton::YesNo:
            AddButton(ID_BT_0, GetSize().x / 2 - 3 - 90, _("Yes"), TextureColor::Green2);
            AddButton(ID_BT_0 + 1, GetSize().x / 2 + 3, _("No"), TextureColor::Red1);
            defaultBt = 1;
            break;

        case MsgboxButton::YesNoCancel:
            AddButton(ID_BT_0, GetSize().x / 2 - 45 - 6 - 90, _("Yes"), TextureColor::Green2);
            AddButton(ID_BT_0 + 1, GetSize().x / 2 - 45, _("No"), TextureColor::Red1);
            AddButton(ID_BT_0 + 2, GetSize().x / 2 + 45 + 6, _("Cancel"), TextureColor::Grey);
            defaultBt = 2;
            break;
    }
    const Window* defBt = GetCtrl<Window>(defaultBt + ID_BT_0);
    if(defBt)
        VIDEODRIVER.SetMousePos(defBt->GetDrawPos() + DrawPoint(defBt->GetSize()) / 2);
    WINDOWMANAGER.SetCursor();
}

iwMsgbox::~iwMsgbox() = default;

void iwMsgbox::MoveIcon(const DrawPoint& pos)
{
    auto* icon = GetCtrl<ctrlImage>(ID_ICON);
    if(icon)
    {
        icon->SetPos(elMax(pos, DrawPoint(0, 0)));
        const ITexture* iconImg = icon->GetImage();
        DrawPoint iconPos(icon->GetPos() - iconImg->GetOrigin());
        DrawPoint textPos = contentOffset + DrawPoint(paddingX, 5);
        Extent textMaxSize;
        if(iconPos.x < 100)
        {
            // icon left
            textPos.x = iconPos.x + iconImg->GetSize().x + paddingX;
            textMaxSize.x = std::max<int>(minTextWidth, 400 - textPos.x - paddingX);
            textMaxSize.y = maxTextHeight;
        } else if(iconPos.x > 300)
        {
            // icon right
            textMaxSize.x = iconPos.x - 2 * paddingX;
            textMaxSize.y = maxTextHeight;
        } else if(iconPos.y + iconImg->GetSize().y < 50)
        {
            // icon top
            textPos.y = iconPos.y + iconImg->GetSize().x + paddingX;
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
        auto* multiline = GetCtrl<ctrlMultiline>(ID_TEXT);
        multiline->SetPos(textPos);
        multiline->Resize(textMaxSize);
        multiline->Resize(multiline->GetContentSize());

        DrawPoint newSize = iconPos + DrawPoint(iconImg->GetSize());
        newSize = elMax(newSize, multiline->GetPos() + DrawPoint(multiline->GetSize()) + DrawPoint::all(paddingX));
        newSize += DrawPoint(0, 10 + btSize.y * 2) + DrawPoint(contentOffsetEnd);
        DrawPoint btMoveDelta(newSize - GetSize());
        btMoveDelta.x /= 2;
        for(unsigned i = 0; i < 3; i++)
        {
            auto* bt = GetCtrl<Window>(i + ID_BT_0);
            if(bt)
                bt->SetPos(bt->GetPos() + btMoveDelta);
        }
        Resize(Extent(newSize));
    }
}

constexpr helpers::EnumArray<std::array<MsgboxResult, 3>, MsgboxButton> RET_IDS = {
  {{MsgboxResult::Ok},
   {MsgboxResult::Ok, MsgboxResult::Cancel},
   {MsgboxResult::Yes, MsgboxResult::No},
   {MsgboxResult::Yes, MsgboxResult::No, MsgboxResult::Cancel}}};

void iwMsgbox::Msg_ButtonClick(const unsigned ctrl_id)
{
    if(msgHandler_)
        msgHandler_->Msg_MsgBoxResult(msgboxid, RET_IDS[button][ctrl_id - ID_BT_0]);
    Close();
}

void iwMsgbox::AddButton(unsigned short id, int x, const std::string& text, const TextureColor tc)
{
    AddTextButton(id, DrawPoint(x, GetRightBottomBoundary().y - btSize.y * 2), btSize, tc, text, NormalFont);
}
