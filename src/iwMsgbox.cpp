// $Id: iwMsgbox.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#include "main.h"
#include "iwMsgbox.h"

#include "VideoDriverWrapper.h"
#include "Loader.h"
#include "controls.h"
#include "glArchivItem_Font.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p iwMsgbox.
 *
 *  @author OLiver
 */
iwMsgbox::iwMsgbox(const std::string& title, const std::string& text, Window* parent, MsgboxButton button, unsigned short icon, unsigned int msgboxid)
    : IngameWindow(CGI_MSGBOX, 0xFFFF, 0xFFFF, 420, 140, title, LOADER.GetImageN("resource", 41), true), parent(parent), button(button), msgboxid(msgboxid), text(text)
{
    AddImage(0, 42, 42, LOADER.GetImageN("io", icon));
    //ctrlMultiline *multiline = AddMultiline(1, 77, 34, 400, 90, TC_GREEN2, NormalFont);
    //multiline->AddText(text, COLOR_YELLOW);
    /*AddText(1, 80, 30, text, COLOR_YELLOW, 0, NormalFont);*/

    // Umbrechungsinformationen vom Text holen
    glArchivItem_Font::WrapInfo wi;
    NormalFont->GetWrapInfo(text, 330, 330, wi);
    // Einzelne Zeilen-Strings erzeugen
    strings = new std::string[wi.positions.size()];
    wi.CreateSingleStrings(text, strings);
    lines_count = wi.positions.size();

    // Buttons erstellen
    switch(button)
    {
        case MSB_OK:
        {
            AddButton(0, width / 2 - 45, _("OK"), TC_GREEN2);
            VideoDriverWrapper::inst().SetMousePos(GetX() + width / 2, GetY() + 110);
        } break;

        case MSB_OKCANCEL:
        {
            AddButton(0, width / 2 - 3 - 90, _("OK"), TC_GREEN2);
            AddButton(1, width / 2 + 3, _("Cancel"), TC_RED1);
            VideoDriverWrapper::inst().SetMousePos(GetX() + width / 2 + 3 + 45, GetY() + 110);
        } break;

        case MSB_YESNO:
        {
            AddButton(0, width / 2 - 3 - 90, _("Yes"), TC_GREEN2);
            AddButton(1, width / 2 + 3, _("No"), TC_RED1);
            VideoDriverWrapper::inst().SetMousePos(GetX() + width / 2 + 3 + 45, GetY() + 110);
        } break;

        case MSB_YESNOCANCEL:
        {
            AddButton(0, width / 2 - 45 - 6 - 90, _("Yes"), TC_GREEN2);
            AddButton(1, width / 2 - 45, _("No"), TC_RED1);
            AddButton(2, width / 2 + 45 + 6, _("Cancel"), TC_GREY);
            VideoDriverWrapper::inst().SetMousePos(GetX() + width / 2 + 6 + 90, GetY() + 110);
        } break;
    }
}

iwMsgbox::~iwMsgbox()
{
    delete [] strings;
}


const MsgboxResult RET_IDS[4][3] =
{
    {MSR_OK,  MSR_NOTHING, MSR_NOTHING},
    {MSR_OK,  MSR_CANCEL,  MSR_NOTHING},
    {MSR_YES, MSR_NO,      MSR_NOTHING},
    {MSR_YES, MSR_NO,      MSR_CANCEL}
};

void iwMsgbox::Msg_ButtonClick(const unsigned int ctrl_id)
{
    if(parent)
        parent->Msg_MsgBoxResult(msgboxid, RET_IDS[button][ctrl_id - 2]);
    Close();
}

void iwMsgbox::Msg_PaintAfter()
{
    // Text zeichnen
    for(unsigned i = 0; i < lines_count; ++i)
        NormalFont->Draw(GetX() + 80, GetY() + 30 + NormalFont->getHeight()*i, strings[i], glArchivItem_Font::DF_LEFT, 0xFFFFFF00);
}

void iwMsgbox::AddButton(unsigned short id, int x, const std::string& text, const TextureColor tc)
{
    Window::AddTextButton(2 + id, x, 100, 90, 20, tc, text, NormalFont);
}
