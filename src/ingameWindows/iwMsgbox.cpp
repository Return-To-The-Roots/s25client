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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h" // IWYU pragma: keep
#include "iwMsgbox.h"

#include "drivers/VideoDriverWrapper.h"
#include "Loader.h"
#include "ogl/glArchivItem_Font.h"
#include "controls/ctrlImage.h"
#include "gameData/const_gui_ids.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

iwMsgbox::iwMsgbox(const std::string& title, const std::string& text, Window* msgHandler, MsgboxButton button, MsgboxIcon icon, unsigned msgboxid)
    : IngameWindow(CGI_MSGBOX, 0xFFFF, 0xFFFF, 420, 140, title, LOADER.GetImageN("resource", 41), true, true), button(button), msgboxid(msgboxid), msgHandler_(msgHandler)
{
    Init(text, "io", icon);
}

iwMsgbox::iwMsgbox(const std::string& title, const std::string& text, Window* msgHandler, MsgboxButton button, const std::string& iconFile, unsigned iconIdx, unsigned msgboxid /* = 0 */)
    : IngameWindow(CGI_MSGBOX, 0xFFFF, 0xFFFF, 420, 140, title, LOADER.GetImageN("resource", 41), true, true), button(button), msgboxid(msgboxid), msgHandler_(msgHandler)
{
    Init(text, iconFile, iconIdx);
}

void iwMsgbox::Init(const std::string& text, const std::string& iconFile, unsigned iconIdx)
{
    glArchivItem_Bitmap* icon = LOADER.GetImageN(iconFile, iconIdx);
    if(icon)
        AddImage(0, 42, 42, icon);
    //ctrlMultiline *multiline = AddMultiline(1, 77, 34, 400, 90, TC_GREEN2, NormalFont);
    //multiline->AddText(text, COLOR_YELLOW);
    /*AddText(1, 80, 30, text, COLOR_YELLOW, 0, NormalFont);*/

    // Umbrechungsinformationen vom Text holen
    glArchivItem_Font::WrapInfo wi = NormalFont->GetWrapInfo(text, 330, 330);
    // Einzelne Zeilen-Strings erzeugen
    strings = wi.CreateSingleStrings(text);

    // Buttons erstellen
    switch(button)
    {
    case MSB_OK:
    {
        AddButton(0, width_ / 2 - 45, _("OK"), TC_GREEN2);
        VIDEODRIVER.SetMousePos(GetX() + width_ / 2, GetY() + 110);
    } break;

    case MSB_OKCANCEL:
    {
        AddButton(0, width_ / 2 - 3 - 90, _("OK"), TC_GREEN2);
        AddButton(1, width_ / 2 + 3, _("Cancel"), TC_RED1);
        VIDEODRIVER.SetMousePos(GetX() + width_ / 2 + 3 + 45, GetY() + 110);
    } break;

    case MSB_YESNO:
    {
        AddButton(0, width_ / 2 - 3 - 90, _("Yes"), TC_GREEN2);
        AddButton(1, width_ / 2 + 3, _("No"), TC_RED1);
        VIDEODRIVER.SetMousePos(GetX() + width_ / 2 + 3 + 45, GetY() + 110);
    } break;

    case MSB_YESNOCANCEL:
    {
        AddButton(0, width_ / 2 - 45 - 6 - 90, _("Yes"), TC_GREEN2);
        AddButton(1, width_ / 2 - 45, _("No"), TC_RED1);
        AddButton(2, width_ / 2 + 45 + 6, _("Cancel"), TC_GREY);
        VIDEODRIVER.SetMousePos(GetX() + width_ / 2 + 6 + 90, GetY() + 110);
    } break;
    }
}

iwMsgbox::~iwMsgbox()
{}


void iwMsgbox::MoveIcon(int x, int y)
{
    ctrlImage* icon = GetCtrl<ctrlImage>(0);
    if(icon)
        icon->Move(x, y);
}

const MsgboxResult RET_IDS[MSB_YESNOCANCEL + 1][3] =
{
    {MSR_OK,  MSR_NOTHING, MSR_NOTHING},
    {MSR_OK,  MSR_CANCEL,  MSR_NOTHING},
    {MSR_YES, MSR_NO,      MSR_NOTHING},
    {MSR_YES, MSR_NO,      MSR_CANCEL}
};

void iwMsgbox::Msg_ButtonClick(const unsigned int ctrl_id)
{
    if(msgHandler_)
        msgHandler_->Msg_MsgBoxResult(msgboxid, RET_IDS[button][ctrl_id - 2]);
    Close();
}

void iwMsgbox::Msg_PaintAfter()
{
    // Text zeichnen
    for(unsigned i = 0; i < strings.size(); ++i)
        NormalFont->Draw(GetX() + 80, GetY() + 30 + NormalFont->getHeight()*i, strings[i], glArchivItem_Font::DF_LEFT, COLOR_YELLOW);
}

void iwMsgbox::AddButton(unsigned short id, int x, const std::string& text, const TextureColor tc)
{
    Window::AddTextButton(2 + id, x, 100, 90, 20, tc, text, NormalFont);
}
