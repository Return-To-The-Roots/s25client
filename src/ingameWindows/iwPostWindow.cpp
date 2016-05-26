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
        ID_FIRST_FREE = 2,
        ID_SHOW_ALL,
        ID_SHOW_GEN,
        ID_SHOW_MIL,
        ID_SHOW_GEO,
        ID_SHOW_ECO,
        ID_SHOW_DIPLO,
        ID_GO_START,
        ID_GO_BACK,
        ID_GO_FWD,
        ID_GO_END,
        ID_GOTO,
        ID_DELETE,
        ID_IMG,
        ID_TEXT,
        ID_ACCEPT,
        ID_DENY,
        ID_INFO
    };
}

iwPostWindow::iwPostWindow(GameWorldView& gwv, PostBox& postBox):
    IngameWindow(CGI_POSTOFFICE, 0xFFFF, 0xFFFF, 254, 295, _("Post office"), LOADER.GetImageN("resource", 41)),
    gwv(gwv), postBox(postBox), showAll(true), curCategory(PostCategory::General), curMsg(NULL)
{
    AddImageButton(ID_SHOW_ALL,    18, 25, 35, 35, TC_GREY, LOADER.GetImageN("io", 190)); // Viewer: 191 - Papier
    AddImageButton(ID_SHOW_MIL,    56, 25, 35, 35, TC_GREY, LOADER.GetImageN("io", 30));  // Viewer:  31 - Soldat
    AddImageButton(ID_SHOW_GEO,    91, 25, 35, 35, TC_GREY, LOADER.GetImageN("io", 20));  // Viewer:  21 - Geologe
    AddImageButton(ID_SHOW_ECO,   126, 25, 35, 35, TC_GREY, LOADER.GetImageN("io", 28));  // Viewer:  29 - Wage
    AddImageButton(ID_SHOW_DIPLO, 161, 25, 35, 35, TC_GREY, LOADER.GetImageN("io", 189)); // Viewer: 190 - Neue Nachricht
    AddImageButton(ID_SHOW_GEN,   199, 25, 35, 35, TC_GREY, LOADER.GetImageN("io", 79));  // Viewer:  80 - Notiz
    AddImage(0, 126, 151, LOADER.GetImageN("io", 228));
    AddImageButton(1, 18, 242, 30, 35, TC_GREY, LOADER.GetImageN("io", 225));  // Viewer: 226 - Hilfe
    AddImageButton(ID_GO_START, 51, 246, 30, 26, TC_GREY, LOADER.GetImageN("io", 102));// Viewer: 103 - Schnell zurück
    AddImageButton(ID_GO_BACK, 81, 246, 30, 26, TC_GREY, LOADER.GetImageN("io", 103)); // Viewer: 104 - Zurück
    AddImageButton(ID_GO_FWD, 111, 246, 30, 26, TC_GREY, LOADER.GetImageN("io", 104)); // Viewer: 105 - Vor
    AddImageButton(ID_GO_END, 141, 246, 30, 26, TC_GREY, LOADER.GetImageN("io", 105)); // Viewer: 106 - Schnell vor

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
    text->AddString("", COLOR_WINDOWBROWN, false);

    // Button with OK and deny sign (tick and cross) for contracts
    AddImageButton(ID_ACCEPT, 87, 185, 30, 26, TC_GREEN1, LOADER.GetImageN("io", 32))->SetVisible(false);
    AddImageButton(ID_DENY, 137, 185, 30, 26, TC_RED1, LOADER.GetImageN("io", 40))->SetVisible(false);

    FilterMessages();
    curMsgId = curMsgIdxs.size();
    DisplayPostMessage();
}

void iwPostWindow::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_SHOW_ALL:
            showAll = true;
            FilterMessages();
            curMsgId = curMsgIdxs.size();
            DisplayPostMessage();
            break;
        case ID_SHOW_GEN:
            SwitchCategory(PostCategory::General);
            break;
        case ID_SHOW_MIL:
            SwitchCategory(PostCategory::Military);
            break;
        case ID_SHOW_GEO:
            SwitchCategory(PostCategory::Geologist);
            break;
        case ID_SHOW_ECO:
            SwitchCategory(PostCategory::Economy);
            break;
        case ID_SHOW_DIPLO:
            SwitchCategory(PostCategory::Diplomacy);
            break;
        case ID_GO_START:
            // To oldest
            curMsgId = 0;
            DisplayPostMessage();
            break;
        case ID_GO_BACK:
            // Back
            curMsgId = (curMsgId > 0) ? curMsgId - 1 : 0;
            DisplayPostMessage();
            break;
        case ID_GO_FWD:
            // Forward
            ++curMsgId;
            DisplayPostMessage();
            break;
        case ID_GO_END:
            // To newest
            curMsgId = postBox.GetNumMsgs();
            DisplayPostMessage();
            break;
        case ID_GOTO:
        {
            // Goto
            if(!ValidateMessages())
                return;
            const PostMsg* curMsg = GetMsg(curMsgId);
            if (curMsg && curMsg->GetPos().isValid())
                gwv.MoveToMapPt(curMsg->GetPos());
        }
        break;

        
        case ID_DELETE: // Delete
        case ID_DENY: // Cross (Deny)
        {
            if(!ValidateMessages())
                return;
            const PostMsg* curMsg = GetMsg(curMsgId);
            const DiplomacyPostQuestion* dcurMsg = dynamic_cast<const DiplomacyPostQuestion*>(curMsg);
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
        case ID_ACCEPT:
        {
            if(!ValidateMessages())
                return;
            const DiplomacyPostQuestion* dcurMsg = dynamic_cast<const DiplomacyPostQuestion*>(GetMsg(curMsgId));
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
    ValidateMessages();
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
            Msg_ButtonClick(ID_DELETE);
            return true;
    }

    switch(ke.c)
    {
        case '+': // Next message
            Msg_ButtonClick(ID_GO_FWD);
            return true;
        case '-': // Previous message
            Msg_ButtonClick(ID_GO_BACK);
            return true;
        case 'g': // Go to site of event
            Msg_ButtonClick(ID_GOTO);
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

    unsigned size = curMsgIdxs.size();

    // Keine Nachrichten, alles ausblenden, bis auf zentrierten Text
    if (size == 0)
    {
        SetMessageText(_("No letters!"));
        GetCtrl<Window>(ID_TEXT)->Move(xTextCenter, yTextCenter);
        return;
    }

    // Falls currentMessage außerhalb der aktuellen Nachrichtenmenge liegt: korrigieren
    if (curMsgId >= size)
        curMsgId = size - 1;

    curMsg = GetMsg(curMsgId);

    // We have a message, display its status...
    std::stringstream ss;
    ss << _("Message") << " " << curMsgId + 1 << " " << _("of") << " " << size << " - GF: " << curMsg->GetSendFrame();
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
    for(unsigned i = 0; i < 4; ++i)
    {
        if (i < lines.size())
            text->SetLine(i, lines[i], COLOR_WINDOWBROWN);
        else
            text->SetLine(i, "", COLOR_WINDOWBROWN);
    }
}

void iwPostWindow::FilterMessages()
{
    curMsgIdxs.clear();
    lastMsgCt = postBox.GetNumMsgs();
    for(unsigned i=0; i<lastMsgCt; i++)
    {
        if(showAll || postBox.GetMsg(i)->GetCategory() == curCategory)
            curMsgIdxs.push_back(i);
    }
}

bool iwPostWindow::ValidateMessages()
{
    if(lastMsgCt == postBox.GetNumMsgs() && GetMsg(curMsgId) == curMsg)
        return true;
    // Messages have changed -> Update filter
    FilterMessages();
    if(!curMsg)
    {
        // No last message? Display oldest
        curMsgId = 0;
        DisplayPostMessage();
        return false;
    }else
    {
        // Message was either deleted or others were added and message was shifted
        // So first search it, if not found we will display the next (newer) message
        for(unsigned i = curMsgId; i > 0; i--)
        {
            if(postBox.GetMsg(i - 1) == curMsg)
            {
                curMsgId = i - 1;
                DisplayPostMessage();
                return true;
            }
        }
        // Display next valid one
        DisplayPostMessage();
        return false;
    }
}

const PostMsg* iwPostWindow::GetMsg(unsigned id) const
{
    if(id < curMsgIdxs.size())
        return postBox.GetMsg(curMsgIdxs[id]);
    else
        return NULL;
}

void iwPostWindow::SwitchCategory(PostCategory cat)
{
    showAll = false;
    curCategory = cat;
    FilterMessages();
    curMsgId = curMsgIdxs.size();
    DisplayPostMessage();
}
