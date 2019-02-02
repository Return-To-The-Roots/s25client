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
#include "iwMsgbox.h"
#include "GameManager.h"
#include "Loader.h"
#include "controls/ctrlImage.h"
#include "controls/ctrlMultiline.h"
#include "drivers/VideoDriverWrapper.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "gameData/const_gui_ids.h"
#include "libsiedler2/ArchivItem_BitmapBase.h"

namespace {
enum IDS
{
    ID_ICON = 0,
    ID_TEXT,
    ID_BT_0
};
const Extent btSize(90, 20);
const unsigned short paddingX = 15; /// Padding in X/to image
const unsigned short minTextWidth = 150;
const unsigned short maxTextHeight = 200;
} // namespace

iwMsgbox::iwMsgbox(const std::string& title, const std::string& text, Window* msgHandler, MsgboxButton button, MsgboxIcon icon,
                   unsigned msgboxid)
    : IngameWindow(CGI_MSGBOX, IngameWindow::posLastOrCenter, Extent(420, 140), title, LOADER.GetImageN("resource", 41), true, false),
      button(button), msgboxid(msgboxid), msgHandler_(msgHandler)
{
    Init(text, "io", icon);
}

iwMsgbox::iwMsgbox(const std::string& title, const std::string& text, Window* msgHandler, MsgboxButton button, const std::string& iconFile,
                   unsigned iconIdx, unsigned msgboxid /* = 0 */)
    : IngameWindow(CGI_MSGBOX, IngameWindow::posLastOrCenter, Extent(420, 140), title, LOADER.GetImageN("resource", 41), true, false),
      button(button), msgboxid(msgboxid), msgHandler_(msgHandler)
{
    Init(text, iconFile, iconIdx);
}

void iwMsgbox::Init(const std::string& text, const std::string& iconFile, unsigned iconIdx)
{
    glArchivItem_Bitmap* icon = LOADER.GetImageN(iconFile, iconIdx);
    if(icon)
        AddImage(ID_ICON, contentOffset + DrawPoint(30, 20), icon);
    int textX = icon ? icon->getWidth() - icon->getNx() + GetCtrl<Window>(ID_ICON)->GetPos().x : contentOffset.x;
    textX += paddingX;
    Extent txtSize = Extent(std::max<int>(minTextWidth, GetRightBottomBoundary().x - textX - paddingX), maxTextHeight);
    ctrlMultiline* multiline = AddMultiline(ID_TEXT, DrawPoint(textX, contentOffset.y + 5), txtSize, TC_GREEN2, NormalFont);
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
        case MSB_OK:
            AddButton(0, GetSize().x / 2 - 45, _("OK"), TC_GREEN2);
            defaultBt = 0;
            break;

        case MSB_OKCANCEL:
            AddButton(0, GetSize().x / 2 - 3 - 90, _("OK"), TC_GREEN2);
            AddButton(1, GetSize().x / 2 + 3, _("Cancel"), TC_RED1);
            defaultBt = 1;
            break;

        case MSB_YESNO:
            AddButton(0, GetSize().x / 2 - 3 - 90, _("Yes"), TC_GREEN2);
            AddButton(1, GetSize().x / 2 + 3, _("No"), TC_RED1);
            defaultBt = 1;
            break;

        case MSB_YESNOCANCEL:
            AddButton(0, GetSize().x / 2 - 45 - 6 - 90, _("Yes"), TC_GREEN2);
            AddButton(1, GetSize().x / 2 - 45, _("No"), TC_RED1);
            AddButton(2, GetSize().x / 2 + 45 + 6, _("Cancel"), TC_GREY);
            defaultBt = 2;
            break;
    }
    const Window* defBt = GetCtrl<Window>(defaultBt + ID_BT_0);
    if(defBt)
        VIDEODRIVER.SetMousePos(defBt->GetDrawPos() + DrawPoint(defBt->GetSize()) / 2);
    GAMEMANAGER.SetCursor();
}

iwMsgbox::~iwMsgbox() {}

void iwMsgbox::MoveIcon(const DrawPoint& pos)
{
    ctrlImage* icon = GetCtrl<ctrlImage>(ID_ICON);
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
        ctrlMultiline* multiline = GetCtrl<ctrlMultiline>(ID_TEXT);
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
            Window* bt = GetCtrl<Window>(i + ID_BT_0);
            if(bt)
                bt->SetPos(bt->GetPos() + btMoveDelta);
        }
        Resize(Extent(newSize));
    }
}

const MsgboxResult RET_IDS[MSB_YESNOCANCEL + 1][3] = {
  {MSR_OK, MSR_NOTHING, MSR_NOTHING}, {MSR_OK, MSR_CANCEL, MSR_NOTHING}, {MSR_YES, MSR_NO, MSR_NOTHING}, {MSR_YES, MSR_NO, MSR_CANCEL}};

void iwMsgbox::Msg_ButtonClick(const unsigned ctrl_id)
{
    if(msgHandler_)
        msgHandler_->Msg_MsgBoxResult(msgboxid, RET_IDS[button][ctrl_id - ID_BT_0]);
    Close();
}

void iwMsgbox::AddButton(unsigned short id, int x, const std::string& text, const TextureColor tc)
{
    AddTextButton(ID_BT_0 + id, DrawPoint(x, GetRightBottomBoundary().y - btSize.y * 2), btSize, tc, text, NormalFont);
}
