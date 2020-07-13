// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "iwPostWindow.h"
#include "Loader.h"
#include "WindowManager.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlImage.h"
#include "controls/ctrlMultiline.h"
#include "controls/ctrlText.h"
#include "driver/KeyEvent.h"
#include "ingameWindows/iwMissionStatement.h"
#include "iwHelp.h"
#include "network/GameClient.h"
#include "ogl/FontStyle.h"
#include "postSystem/DiplomacyPostQuestion.h"
#include "postSystem/PostBox.h"
#include "postSystem/PostMsg.h"
#include "world/GameWorldView.h"
#include "gameData/const_gui_ids.h"
#include <iostream>
#include <sstream>

// Only internally visible
namespace {
// Enum is auto-numbering once we set a start value
enum ButtonIds
{
    ID_FIRST_FREE = 1,
    ID_SHOW_ALL,
    ID_SHOW_GOAL,
    ID_SHOW_MIL,
    ID_SHOW_GEO,
    ID_SHOW_ECO,
    ID_SHOW_GEN,
    ID_HELP,
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
} // namespace

iwPostWindow::iwPostWindow(GameWorldView& gwv, PostBox& postBox)
    : IngameWindow(CGI_POSTOFFICE, IngameWindow::posLastOrCenter, Extent(254, 295), _("Post office"), LOADER.GetImageN("resource", 41)),
      gwv(gwv), postBox(postBox), showAll(true), curCategory(PostCategory::General), curMsg(nullptr), lastHasMissionGoal(true)
{
    AddImageButton(ID_SHOW_ALL, DrawPoint(18, 25), Extent(35, 35), TC_GREY, LOADER.GetImageN("io", 190));  // Viewer: 191 - Papier
    AddImageButton(ID_SHOW_MIL, DrawPoint(56, 25), Extent(35, 35), TC_GREY, LOADER.GetImageN("io", 30));   // Viewer:  31 - Soldat
    AddImageButton(ID_SHOW_GEO, DrawPoint(91, 25), Extent(35, 35), TC_GREY, LOADER.GetImageN("io", 20));   // Viewer:  21 - Geologe
    AddImageButton(ID_SHOW_ECO, DrawPoint(126, 25), Extent(35, 35), TC_GREY, LOADER.GetImageN("io", 28));  // Viewer:  29 - Wage
    AddImageButton(ID_SHOW_GEN, DrawPoint(161, 25), Extent(35, 35), TC_GREY, LOADER.GetImageN("io", 189)); // Viewer: 190 - Neue Nachricht
    AddImageButton(ID_SHOW_GOAL, DrawPoint(199, 25), Extent(35, 35), TC_GREY, LOADER.GetImageN("io", 79)); // Viewer:  80 - Notiz
    AddImage(0, DrawPoint(126, 151), LOADER.GetImageN("io", 228));
    AddImageButton(ID_HELP, DrawPoint(18, 242), Extent(30, 35), TC_GREY, LOADER.GetImageN("io", 225));     // Viewer: 226 - Hilfe
    AddImageButton(ID_GO_START, DrawPoint(51, 246), Extent(30, 26), TC_GREY, LOADER.GetImageN("io", 102)); // Viewer: 103 - Schnell zurück
    AddImageButton(ID_GO_BACK, DrawPoint(81, 246), Extent(30, 26), TC_GREY, LOADER.GetImageN("io", 103));  // Viewer: 104 - Zurück
    AddImageButton(ID_GO_FWD, DrawPoint(111, 246), Extent(30, 26), TC_GREY, LOADER.GetImageN("io", 104));  // Viewer: 105 - Vor
    AddImageButton(ID_GO_END, DrawPoint(141, 246), Extent(30, 26), TC_GREY, LOADER.GetImageN("io", 105));  // Viewer: 106 - Schnell vor

    // Goto, nur sichtbar wenn Nachricht mit Koordinaten da
    AddImageButton(ID_GOTO, DrawPoint(181, 246), Extent(30, 26), TC_GREY, LOADER.GetImageN("io", 107))->SetVisible(false);
    // Mülleimer, nur sichtbar, wenn Nachricht da
    AddImageButton(ID_DELETE, DrawPoint(211, 246), Extent(30, 26), TC_GREY, LOADER.GetImageN("io", 106))->SetVisible(false);

    AddText(ID_INFO, DrawPoint(127, 228), "", MakeColor(255, 188, 100, 88), FontStyle::CENTER | FontStyle::BOTTOM, SmallFont)
      ->SetVisible(false);

    AddImage(ID_IMG, DrawPoint(127, 155), LOADER.GetImageN("io", 225));

    // Multiline-Teil mit drei leeren Zeilen erzeugen
    ctrlMultiline* text = AddMultiline(ID_TEXT, DrawPoint(126, 141), Extent(200, 0), TC_INVISIBLE, NormalFont,
                                       FontStyle::CENTER | FontStyle::BOTTOM | FontStyle::NO_OUTLINE);
    text->SetNumVisibleLines(4);
    text->ShowBackground(false);

    // Button with OK and deny sign (tick and cross) for contracts
    AddImageButton(ID_ACCEPT, DrawPoint(87, 185), Extent(30, 26), TC_GREEN1, LOADER.GetImageN("io", 32))->SetVisible(false);
    AddImageButton(ID_DENY, DrawPoint(137, 185), Extent(30, 26), TC_RED1, LOADER.GetImageN("io", 40))->SetVisible(false);

    FilterMessages();
    curMsgId = curMsgIdxs.size();
    DisplayPostMessage();
}

void iwPostWindow::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_HELP:
            WINDOWMANAGER.ReplaceWindow(
              std::make_unique<iwHelp>(_("All important messages are collected in this window and "
                                         "sorted into groups. If this window is not open, the dove "
                                         "symbol at the bottom of the screen indicates the arrival of a new message.")));
            break;
        case ID_SHOW_ALL:
            showAll = true;
            FilterMessages();
            curMsgId = curMsgIdxs.size();
            DisplayPostMessage();
            break;
        case ID_SHOW_GOAL:
            if(!postBox.GetCurrentMissionGoal().empty())
                WINDOWMANAGER.Show(
                  std::make_unique<iwMissionStatement>(_("Diary"), postBox.GetCurrentMissionGoal(), false, iwMissionStatement::IM_AVATAR9));
            break;
        case ID_SHOW_MIL: SwitchCategory(PostCategory::Military); break;
        case ID_SHOW_GEO: SwitchCategory(PostCategory::Geologist); break;
        case ID_SHOW_ECO: SwitchCategory(PostCategory::Economy); break;
        case ID_SHOW_GEN: SwitchCategory(PostCategory::General); break;
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
            if(curMsg && curMsg->GetPos().isValid())
                gwv.MoveToMapPt(curMsg->GetPos());
        }
        break;

        case ID_DELETE: // Delete
        case ID_DENY:   // Cross (Deny)
        {
            if(!ValidateMessages() || !curMsg)
                return;
            const auto* dcurMsg = dynamic_cast<const DiplomacyPostQuestion*>(curMsg);
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
            const auto* dcurMsg = dynamic_cast<const DiplomacyPostQuestion*>(GetMsg(curMsgId));
            if(dcurMsg)
            {
                // New contract?
                if(dcurMsg->IsAccept())
                    GAMECLIENT.AcceptPact(dcurMsg->GetPactId(), dcurMsg->GetPactType(), dcurMsg->GetPlayerId());
                else
                    GAMECLIENT.CancelPact(dcurMsg->GetPactType(), dcurMsg->GetPlayerId());
                postBox.DeleteMsg(dcurMsg);
            }
        }
        break;
    }
}

void iwPostWindow::Msg_PaintBefore()
{
    IngameWindow::Msg_PaintBefore();
    ValidateMessages();
    if(lastHasMissionGoal != !postBox.GetCurrentMissionGoal().empty())
    {
        lastHasMissionGoal = !postBox.GetCurrentMissionGoal().empty();
        GetCtrl<Window>(ID_SHOW_GOAL)->SetVisible(lastHasMissionGoal);
    }
}

/**
 *  React on keypress
 */
bool iwPostWindow::Msg_KeyDown(const KeyEvent& ke)
{
    switch(ke.kt)
    {
        default: break;
        case KT_DELETE: // Delete current message
#ifdef __APPLE__
        case KT_BACKSPACE: // Macs usually have no delete key on small keyboards, so backspace is more convenient
#endif
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
    const DrawPoint imgBottomCenter(127, 210); //-V821
    // todo: koordinaten abschmecken
    const DrawPoint textTopCenter(127, 110); //-V821
    const DrawPoint textCenter(126, 151);

    // Hide everything
    GetCtrl<Window>(ID_IMG)->SetVisible(false);
    GetCtrl<Window>(ID_DELETE)->SetVisible(false);
    GetCtrl<Window>(ID_GOTO)->SetVisible(false);
    GetCtrl<Window>(ID_ACCEPT)->SetVisible(false);
    GetCtrl<Window>(ID_DENY)->SetVisible(false);
    GetCtrl<Window>(ID_INFO)->SetVisible(false);

    unsigned size = curMsgIdxs.size();

    // Keine Nachrichten, alles ausblenden, bis auf zentrierten Text
    if(size == 0)
    {
        SetMessageText(_("No letters!"));
        GetCtrl<Window>(ID_TEXT)->SetPos(textCenter);
        curMsg = nullptr;
        return;
    }

    // Falls currentMessage außerhalb der aktuellen Nachrichtenmenge liegt: korrigieren
    if(curMsgId >= size)
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
    ITexture* const img = curMsg->GetImage_();
    if(img)
    {
        // We have an image, show it centered
        GetCtrl<ctrlImage>(ID_IMG)->SetImage(img);
        GetCtrl<Window>(ID_IMG)->SetPos(imgBottomCenter + img->GetOrigin() - DrawPoint(img->GetSize().x / 2, img->GetSize().y));

        GetCtrl<Window>(ID_IMG)->SetVisible(true);
    }
    if(dynamic_cast<const DiplomacyPostQuestion*>(curMsg))
    {
        GetCtrl<Window>(ID_ACCEPT)->SetVisible(true);
        GetCtrl<Window>(ID_DENY)->SetVisible(true);
    }
    // Place text at top or center depending on whether we have an img or the acceptButton
    if(img || GetCtrl<Window>(ID_ACCEPT)->IsVisible())
        GetCtrl<Window>(ID_TEXT)->SetPos(textTopCenter);
    else
        GetCtrl<Window>(ID_TEXT)->SetPos(textCenter);
    // If message contains valid position, allow going to it
    if(curMsg->GetPos().isValid())
        GetCtrl<Window>(ID_GOTO)->SetVisible(true);
}

void iwPostWindow::SetMessageText(const std::string& message)
{
    auto* text = GetCtrl<ctrlMultiline>(ID_TEXT);
    text->Clear();
    text->AddString(message, COLOR_WINDOWBROWN);
}

void iwPostWindow::FilterMessages()
{
    curMsgIdxs.clear();
    lastMsgCt = postBox.GetNumMsgs();
    for(unsigned i = 0; i < lastMsgCt; i++)
    {
        if(showAll)
            curMsgIdxs.push_back(i);
        else
        {
            const PostCategory msgCat = postBox.GetMsg(i)->GetCategory();
            // We sort diplomacy messages to general messages as we don't have another button
            if(msgCat == curCategory || (curCategory == PostCategory::General && msgCat == PostCategory::Diplomacy))
                curMsgIdxs.push_back(i);
        }
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
    } else
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
        return nullptr;
}

void iwPostWindow::SwitchCategory(PostCategory cat)
{
    showAll = false;
    curCategory = cat;
    FilterMessages();
    curMsgId = curMsgIdxs.size();
    DisplayPostMessage();
}
