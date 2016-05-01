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
#include "iwPostWindow.h"
#include "world/GameWorldView.h"
#include "controls/ctrlText.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlImage.h"
#include "controls/ctrlMultiline.h"
#include "postSystem/PostMsg.h"
#include "postSystem/DiplomacyPostQuestion.h"
#include "driver/src/KeyEvent.h"
#include "Loader.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Font.h"
#include "gameData/const_gui_ids.h"
#include "macros.h"
#include "GameClient.h"
#include <iostream>

// Only internally visible
namespace{
    // Enum is auto-numbering once we set a start value
    enum ButtonIds{
        ID_GOTO = 14,
        ID_DELETE,
        ID_IMG,
        ID_TEXT,
        ID_ACCEPT,
        ID_DENY,
        ID_INFO
    };
}

iwPostWindow::iwPostWindow(GameWorldView& gwv, PostBox& postBox)
    : IngameWindow(CGI_POSTOFFICE, 0xFFFF, 0xFFFF, 254, 295, _("Post office"), LOADER.GetImageN("resource", 41)), gwv(gwv), postBox(postBox)
{
    AddImageButton( 0, 18, 25, 35, 35, TC_GREY, LOADER.GetImageN("io", 190));   // Viewer: 191 - Papier
    AddImageButton( 1, 56, 25, 35, 35, TC_GREY, LOADER.GetImageN("io", 30));    // Viewer:  31 - Soldat
    AddImageButton( 2, 91, 25, 35, 35, TC_GREY, LOADER.GetImageN("io", 20));    // Viewer:  21 - Geologe
    AddImageButton( 3, 126, 25, 35, 35, TC_GREY, LOADER.GetImageN("io", 28));   // Viewer:  29 - Wage
    AddImageButton( 4, 161, 25, 35, 35, TC_GREY, LOADER.GetImageN("io", 189));  // Viewer: 190 - Neue Nachricht
    AddImageButton( 5, 199, 25, 35, 35, TC_GREY, LOADER.GetImageN("io", 79));   // Viewer:  80 - Notiz
    AddImage(  6, 126, 151, LOADER.GetImageN("io", 228));
    AddImageButton( 7, 18, 242, 30, 35, TC_GREY, LOADER.GetImageN("io", 225));  // Viewer: 226 - Hilfe
    AddImageButton( 8, 51, 246, 30, 26, TC_GREY, LOADER.GetImageN("io", 102));  // Viewer: 103 - Schnell zurück
    AddImageButton( 9, 81, 246, 30, 26, TC_GREY, LOADER.GetImageN("io", 103));  // Viewer: 104 - Zurück
    AddImageButton(10, 111, 246, 30, 26, TC_GREY, LOADER.GetImageN("io", 104)); // Viewer: 105 - Vor
    AddImageButton(11, 141, 246, 30, 26, TC_GREY, LOADER.GetImageN("io", 105)); // Viewer: 106 - Schnell vor

    // Goto, nur sichtbar wenn Nachricht mit Koordinaten da
    AddImageButton(ID_GOTO, 181, 246, 30, 26, TC_GREY, LOADER.GetImageN("io", 107))->SetVisible(false);
    // Mülleimer, nur sichtbar, wenn Nachricht da
    AddImageButton(ID_DELETE, 211, 246, 30, 26, TC_GREY, LOADER.GetImageN("io", 106))->SetVisible(false);

    AddText(ID_INFO, 127, 228, "", MakeColor(255, 188, 100, 88), glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, SmallFont)->SetVisible(false);

    AddImage(ID_IMG, 127, 155, LOADER.GetImageN("io", 225));

    // Multiline-Teil mit drei leeren Zeilen erzeugen
    ctrlMultiline* text = AddMultiline(ID_TEXT, 126, 141, 200, 50, TC_INVISIBLE, NormalFont, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM | glArchivItem_Font::DF_NO_OUTLINE);
    text->EnableBox(false);
    text->AddString("", COLOR_WINDOWBROWN, false);
    text->AddString("", COLOR_WINDOWBROWN, false);
    text->AddString("", COLOR_WINDOWBROWN, false);

    // Button with OK and deny sign (tick and cross) for contracts
    AddImageButton(ID_ACCEPT, 87, 185, 30, 26, TC_GREEN1, LOADER.GetImageN("io", 32))->SetVisible(false);
    AddImageButton(ID_DENY, 137, 185, 30, 26, TC_RED1, LOADER.GetImageN("io", 40))->SetVisible(false);

    lastMsgCt = postBox.GetNumMsgs();
    curMsgIdx = postBox.GetNumMsgs();
    DisplayPostMessage();
}

void iwPostWindow::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 8:
            // To oldest
            curMsgIdx = 0;
            DisplayPostMessage();
            break;
        case 9:
            // Back
            curMsgIdx = (curMsgIdx > 0) ? curMsgIdx - 1 : 0;
            DisplayPostMessage();
            break;
        case 10:
            // Forward
            ++curMsgIdx;
            DisplayPostMessage();
            break;
        case 11:
            // To newest
            curMsgIdx = postBox.GetNumMsgs();
            DisplayPostMessage();
            break;
        case 14:
        {
            // Goto
            PostMsg* curMsg = postBox.GetMsg(curMsgIdx);
            if (curMsg && curMsg->GetPos().isValid())
                gwv.MoveToMapPt(curMsg->GetPos());
        }
        break;

        
        case 15: // Delete
        case 17: // Cross (Deny)
        {
            PostMsg* curMsg = postBox.GetMsg(curMsgIdx);
            DiplomacyPostQuestion* dcurMsg = dynamic_cast<DiplomacyPostQuestion*>(curMsg);
            if(dcurMsg)
            {
                // If it is a question about a new contract, tell the other player we denied it
                if(dcurMsg->IsAccept())
                  GAMECLIENT.CancelPact(dcurMsg->GetPactType(), dcurMsg->GetPlayerId());
            }
            postBox.DeleteMsg(curMsg);
        }
        break;

        // Haken-Button ("Ja")
        case 16:
        {
            DiplomacyPostQuestion* dcurMsg = dynamic_cast<DiplomacyPostQuestion*>(postBox.GetMsg(curMsgIdx));
            if (dcurMsg)
            {
                // New contract?
                if(dcurMsg->IsAccept())
                    GAMECLIENT.AcceptPact(true, dcurMsg->GetPactId(), dcurMsg->GetPactType(), dcurMsg->GetPlayerId());
                else
                    GAMECLIENT.CancelPact(dcurMsg->GetPactType(), dcurMsg->GetPlayerId());
                postBox.DeleteMsg(dcurMsg);
            }
        } break;

    }

}

void iwPostWindow::Msg_PaintBefore()
{
    if(curMsg && curMsg != postBox.GetMsg(curMsgIdx))
    {
        // Message was either deleted or others were added and message was shifted
        // So first search it, if not found we will display the next (newer) message
        for(unsigned i = curMsgIdx; curMsgIdx > 0; curMsgIdx--)
        {
            if(postBox.GetMsg(i - 1))
            {
                curMsgIdx = i - 1;
                break;
            }
        }
        
        DisplayPostMessage();
    } else if(lastMsgCt != postBox.GetNumMsgs())
    {
        // At least update message count in status bar
        DisplayPostMessage();
        lastMsgCt = postBox.GetNumMsgs();
    }
}

/**
 *  React on keypress
 */
bool iwPostWindow::Msg_KeyDown(const KeyEvent& ke)
{
    switch(ke.kt)
    {
        default:
            break;
        case KT_DELETE: // Delete current message
            Msg_ButtonClick(15);
            return true;
    }

    switch(ke.c)
    {
        case '+': // Next message
            Msg_ButtonClick(10);
            return true;
        case '-': // Previous message
            Msg_ButtonClick(9);
            return true;
        case 'g': // Go to site of event
            Msg_ButtonClick(14);
            return true;
    }

    return false;
}

// Zeigt Nachricht an, passt Steuerelemente an
void iwPostWindow::DisplayPostMessage()
{
    const unsigned xImgBottomCenter = 127;
    const unsigned yImgBottomCenter = 210;

    // todo: koordinaten abschmecken
    const unsigned xTextTopCenter = 127;
    const unsigned yTextTopCenter = 110;

    const unsigned xTextCenter = 126;
    const unsigned yTextCenter = 151;

    // Hide everything
    GetCtrl<Window>(ID_IMG)->SetVisible(false);
    GetCtrl<Window>(ID_DELETE)->SetVisible(false);
    GetCtrl<Window>(ID_GOTO)->SetVisible(false);
    GetCtrl<Window>(ID_ACCEPT)->SetVisible(false);
    GetCtrl<Window>(ID_DENY)->SetVisible(false);
    GetCtrl<Window>(ID_INFO)->SetVisible(false);

    unsigned size = postBox.GetNumMsgs();

    // Keine Nachrichten, alles ausblenden, bis auf zentrierten Text
    if (size == 0)
    {
        SetMessageText(_("No letters!"));
        GetCtrl<Window>(ID_TEXT)->Move(xTextCenter, yTextCenter);
        return;
    }

    // Falls currentMessage außerhalb der aktuellen Nachrichtenmenge liegt: korrigieren
    if (curMsgIdx >= size)
        curMsgIdx = size - 1;

    curMsg = postBox.GetMsg(curMsgIdx);

    // We have a message, display its status...
    std::stringstream ss;
    ss << _("Message") << " " << curMsgIdx + 1 << " " << _("of") << " " << size << " - GF: " << curMsg->GetSendFrame();
    GetCtrl<ctrlText>(ID_INFO)->SetText(ss.str());
    GetCtrl<ctrlText>(ID_INFO)->SetVisible(true);
    // ...and delete button
    GetCtrl<Window>(ID_DELETE)->SetVisible(true);

    SetMessageText(curMsg->GetText());
    glArchivItem_Bitmap* const img = curMsg->GetImage_();
    if(img)
    {
        // We have an image, show it centered
        GetCtrl<ctrlImage>(ID_IMG)->SetImage(img);
        GetCtrl<Window>(ID_IMG)->Move(xImgBottomCenter + img->getNx() - img->getWidth() / 2,
            yImgBottomCenter + img->getNy() - img->getHeight());

        GetCtrl<Window>(ID_IMG)->SetVisible(true);
    }
    if(dynamic_cast<const DiplomacyPostQuestion*>(curMsg))
    {
        GetCtrl<Window>(ID_ACCEPT)->SetVisible(true);
        GetCtrl<Window>(ID_DENY)->SetVisible(true);
    }
    // Place text at top or center depending on whether we have an img or the acceptButton
    if(img || GetCtrl<Window>(ID_ACCEPT)->GetVisible())
        GetCtrl<Window>(ID_TEXT)->Move(xTextTopCenter, yTextTopCenter);
    else
        GetCtrl<Window>(ID_TEXT)->Move(xTextCenter, yTextCenter);
    // If message contains valid position, allow going to it
    if(curMsg->GetPos().isValid())
        GetCtrl<Window>(ID_GOTO)->SetVisible(true);
}

void iwPostWindow::SetMessageText(const std::string& message)
{
    ctrlMultiline* text = GetCtrl<ctrlMultiline>(ID_TEXT);

    glArchivItem_Font::WrapInfo wi = NormalFont->GetWrapInfo(message, 190, 190);
    std::vector<std::string> lines = wi.CreateSingleStrings(message);
    for(unsigned i = 0; i < 3; ++i)
    {
        if (i < lines.size())
            text->SetLine(i, lines[i], COLOR_WINDOWBROWN);
        else
            text->SetLine(i, "", COLOR_WINDOWBROWN);
    }
}
