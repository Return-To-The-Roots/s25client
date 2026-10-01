// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwMsgbox.h"
#include "Loader.h"
#include "WindowManager.h"
#include "controls/ctrlImage.h"
#include "controls/ctrlMultiline.h"
#include "driver/KeyEvent.h"
#include "drivers/VideoDriverWrapper.h"
#include "enum_cast.hpp"
#include "ogl/glArchivItem_Bitmap.h"
#include "gameData/const_gui_ids.h"
#include <utility>

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

MsgboxConfig makeLegacyConfig(const MsgboxButton button)
{
    MsgboxConfig config;
    switch(button)
    {
        case MsgboxButton::Ok:
            config.buttons.push_back({_("OK"), MsgboxResult::Ok, TextureColor::Green2});
            config.defaultButton = 0;
            config.cancelButton = -1;
            config.focusedButton = 0;
            break;

        case MsgboxButton::OkCancel:
            config.buttons.push_back({_("OK"), MsgboxResult::Ok, TextureColor::Green2});
            config.buttons.push_back({_("Cancel"), MsgboxResult::Cancel, TextureColor::Red1});
            config.defaultButton = 0;
            config.cancelButton = 1;
            config.focusedButton = 1;
            break;

        case MsgboxButton::YesNo:
            config.buttons.push_back({_("Yes"), MsgboxResult::Yes, TextureColor::Green2});
            config.buttons.push_back({_("No"), MsgboxResult::No, TextureColor::Red1});
            config.defaultButton = 0;
            config.cancelButton = 1;
            config.focusedButton = 1;
            break;

        case MsgboxButton::YesNoCancel:
            config.buttons.push_back({_("Yes"), MsgboxResult::Yes, TextureColor::Green2});
            config.buttons.push_back({_("No"), MsgboxResult::No, TextureColor::Red1});
            config.buttons.push_back({_("Cancel"), MsgboxResult::Cancel, TextureColor::Grey});
            config.defaultButton = 0;
            config.cancelButton = 2;
            config.focusedButton = 2;
            break;
    }
    return config;
}
} // namespace

iwMsgbox::iwMsgbox(const std::string& title, const std::string& text, Window* msgHandler, MsgboxButton button,
                   MsgboxIcon icon, unsigned msgboxid)
    : iwMsgbox(title, text, msgHandler, makeLegacyConfig(button), icon, msgboxid)
{}

iwMsgbox::iwMsgbox(const std::string& title, const std::string& text, Window* msgHandler, MsgboxButton button,
                   const ResourceId& iconFile, unsigned iconIdx, unsigned msgboxid /* = 0 */)
    : iwMsgbox(title, text, msgHandler, makeLegacyConfig(button), iconFile, iconIdx, msgboxid)
{}

iwMsgbox::iwMsgbox(const std::string& title, const std::string& text, Window* msgHandler, MsgboxConfig config,
                   unsigned msgboxid /* = 0 */)
    : IngameWindow(CGI_MSGBOX, IngameWindow::posLastOrCenter, Extent(420, 140), title, LOADER.GetImageN("resource", 41),
                   true, CloseBehavior::Custom),
      msgboxid(msgboxid), buttons_(std::move(config.buttons)), defaultButton_(config.defaultButton),
      cancelButton_(config.cancelButton), focusedButton_(config.focusedButton), msgHandler_(msgHandler)
{
    Init(text, nullptr);
}

iwMsgbox::iwMsgbox(const std::string& title, const std::string& text, Window* msgHandler, MsgboxConfig config,
                   MsgboxIcon icon, unsigned msgboxid)
    : iwMsgbox(title, text, msgHandler, std::move(config), "io", rttr::enum_cast(icon), msgboxid)
{}

iwMsgbox::iwMsgbox(const std::string& title, const std::string& text, Window* msgHandler, MsgboxConfig config,
                   const ResourceId& iconFile, unsigned iconIdx, unsigned msgboxid /* = 0 */)
    : IngameWindow(CGI_MSGBOX, IngameWindow::posLastOrCenter, Extent(420, 140), title, LOADER.GetImageN("resource", 41),
                   true, CloseBehavior::Custom),
      msgboxid(msgboxid), buttons_(std::move(config.buttons)), defaultButton_(config.defaultButton),
      cancelButton_(config.cancelButton), focusedButton_(config.focusedButton), msgHandler_(msgHandler)
{
    Init(text, LOADER.GetImageN(iconFile, iconIdx));
}

void iwMsgbox::Init(const std::string& text, glArchivItem_Bitmap* icon)
{
    if(buttons_.empty())
        buttons_.push_back({_("OK"), MsgboxResult::Ok, TextureColor::Green2});
    if(buttons_.size() > 3)
        buttons_.resize(3);
    if(defaultButton_ >= buttons_.size())
        defaultButton_ = 0;
    if(cancelButton_ >= static_cast<int>(buttons_.size()))
        cancelButton_ = -1;
    if(focusedButton_ >= static_cast<int>(buttons_.size()))
        focusedButton_ = -1;

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

    const auto numButtons = static_cast<unsigned>(buttons_.size());
    const int spacing = 6;
    const int totalBtWidth = numButtons * btSize.x + (numButtons - 1) * spacing;
    int x = GetSize().x / 2 - totalBtWidth / 2;
    for(unsigned i = 0; i < numButtons; ++i)
    {
        AddButton(ID_BT_0 + i, x, buttons_[i].text, buttons_[i].color);
        x += btSize.x + spacing;
    }
    const unsigned focusedButton = focusedButton_ >= 0 ? static_cast<unsigned>(focusedButton_) : defaultButton_;
    const Window* defBt = GetCtrl<Window>(focusedButton + ID_BT_0);
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

bool iwMsgbox::Msg_KeyDown(const KeyEvent& ke)
{
    if(ke.kt == KeyType::Return)
    {
        Msg_ButtonClick(ID_BT_0 + defaultButton_);
        return true;
    }
    if(ke.kt == KeyType::Escape && cancelButton_ >= 0)
    {
        Msg_ButtonClick(ID_BT_0 + static_cast<unsigned>(cancelButton_));
        return true;
    }
    return false;
}

void iwMsgbox::Msg_ButtonClick(const unsigned ctrl_id)
{
    if(msgHandler_)
        msgHandler_->Msg_MsgBoxResult(msgboxid, buttons_[ctrl_id - ID_BT_0].result);
    Close();
}

void iwMsgbox::AddButton(unsigned short id, int x, const std::string& text, const TextureColor tc)
{
    AddTextButton(id, DrawPoint(x, GetRightBottomBoundary().y - btSize.y * 2), btSize, tc, text, NormalFont);
}
