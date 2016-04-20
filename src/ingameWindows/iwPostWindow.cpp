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

iwPostWindow::iwPostWindow(GameWorldView& gwv)
    : IngameWindow(CGI_POSTOFFICE, 0xFFFF, 0xFFFF, 254, 295, _("Post office"), LOADER.GetImageN("resource", 41)), gwv(gwv)
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

    currentMessage = 0;
    DisplayPostMessage();

    lastSize = GAMECLIENT.GetPostMessages().size();
}

void iwPostWindow::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
            // Schnell zurück
        case 8: currentMessage = 0;
            DisplayPostMessage();
            break;
            // Zurück
        case 9: currentMessage = (currentMessage > 0) ? currentMessage - 1 : 0;
            DisplayPostMessage();
            break;
            // Vor
        case 10: currentMessage = (currentMessage < GAMECLIENT.GetPostMessages().size()-1) ? currentMessage + 1 : GAMECLIENT.GetPostMessages().size()-1;
            DisplayPostMessage();
            break;
            // Schnell vor
        case 11: currentMessage = GAMECLIENT.GetPostMessages().size() - 1;
            DisplayPostMessage();
            break;

            // Goto
        case 14:
        {
            PostMsg* pm = GetPostMsg(currentMessage);
            if (pm && pm->GetPos().isValid())
                gwv.MoveToMapPt(pm->GetPos());
        }
        break;

        
        case 15: // Delete
        case 17: // Cross (Deny)
        {
            PostMsg* pm = GetPostMsg(currentMessage);
            DiplomacyPostQuestion* dpm = dynamic_cast<DiplomacyPostQuestion*>(pm);
            if(dpm)
            {
                // If it is a question about a new contract, tell the other player we denied it
                if(dpm->IsAccept())
                  GAMECLIENT.CancelPact(dpm->GetPactType(), dpm->GetPlayerId());
            }
            DeletePostMessage(pm);
        }
        break;

        // Haken-Button ("Ja")
        case 16:
        {
            DiplomacyPostQuestion* dpm = dynamic_cast<DiplomacyPostQuestion*>(GetPostMsg(currentMessage));
            if (dpm)
            {
                // New contract?
                if(dpm->IsAccept())
                    GAMECLIENT.AcceptPact(true, dpm->GetPactId(), dpm->GetPactType(), dpm->GetPlayerId());
                else
                    GAMECLIENT.CancelPact(dpm->GetPactType(), dpm->GetPlayerId());
                DeletePostMessage(dpm);
            }
        } break;

    }

}

void iwPostWindow::Msg_PaintBefore()
{
    // Immer wenn sich die Anzahl der Nachrichten geändert hat neu prüfen was so angezeigt werden muss
    unsigned currentSize = GAMECLIENT.GetPostMessages().size();
    if (currentSize != lastSize)
    {
        // Neue Nachrichten dazugekommen, während das Fenster offen ist:
        // Ansicht der vorherigen Nachricht beibehalten, außer es gab vorher gar keine Nachricht

        if (lastSize < currentSize && lastSize != 0
                // Wenn die erste Nachricht ausgewählt wurde, nehmen bleiben wir bei der ersten (=aktuellsten)
                && currentMessage > 0)
        {
            currentMessage += currentSize - lastSize;

            // Falls das zufällig grad die 20 Nachrichtengrenze überschritten hat: Auf letzte Nachricht springen
            if (currentMessage >= MAX_POST_MESSAGES)
                currentMessage = MAX_POST_MESSAGES - 1;
        }
        lastSize = GAMECLIENT.GetPostMessages().size();

        // Anzeigeeinstellungen setzen
        DisplayPostMessage();
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
        {
            Msg_ButtonClick(15);
        } return true;
    }

    switch(ke.c)
    {
        case '+': // Next message
        {
            Msg_ButtonClick(10);
        } return true;
        case '-': // Previous message
        {
            Msg_ButtonClick(9);
        } return true;
        case 'g': // Go to site of event
        {
            if (!GAMECLIENT.GetPostMessages().empty())
            {
                Msg_ButtonClick(14);
            }
        } return true;
    }

    return false;
}

// Holt Nachricht Nummer pos
PostMsg* iwPostWindow::GetPostMsg(unsigned pos) const
{
    PostMsg* pm = 0;
    unsigned counter = 0;
    for(std::list<PostMsg*>::const_iterator it = GAMECLIENT.GetPostMessages().begin(); it != GAMECLIENT.GetPostMessages().end(); ++it)
    {
        if (counter == pos)
        {
            pm = *it;
            break;
        }
        counter++;
    }
    RTTR_Assert(pm);
    return pm;
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

    unsigned size = GAMECLIENT.GetPostMessages().size();

    // Keine Nachrichten, alles ausblenden, bis auf zentrierten Text
    if (size == 0)
    {
        SetMessageText(_("No letters!"));
        GetCtrl<Window>(ID_TEXT)->Move(xTextCenter, yTextCenter);
        return;
    }

    // Falls currentMessage außerhalb der aktuellen Nachrichtenmenge liegt: korrigieren
    if (currentMessage >= size)
        currentMessage = size - 1;

    PostMsg* pm = GetPostMsg(currentMessage);

    // We have a message, display its status...
    std::stringstream ss;
    ss << _("Message") << " " << currentMessage + 1 << " " << _("of") << " " << size << " - GF: " << pm->GetSendFrame();
    GetCtrl<ctrlText>(ID_INFO)->SetText(ss.str());
    GetCtrl<ctrlText>(ID_INFO)->SetVisible(true);
    // ...and delete button
    GetCtrl<Window>(ID_DELETE)->SetVisible(true);

    SetMessageText(pm->GetText());
    glArchivItem_Bitmap* const img = pm->GetImage_();
    if(img)
    {
        // We have an image, show it centered
        GetCtrl<ctrlImage>(ID_IMG)->SetImage(img);
        GetCtrl<Window>(ID_IMG)->Move(xImgBottomCenter + img->getNx() - img->getWidth() / 2,
            yImgBottomCenter + img->getNy() - img->getHeight());

        GetCtrl<Window>(ID_IMG)->SetVisible(true);
    }
    if(dynamic_cast<DiplomacyPostQuestion*>(pm))
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
    if(pm->GetPos().isValid())
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

void iwPostWindow::DeletePostMessage(PostMsg* pm)
{
    GAMECLIENT.DeletePostMessage(pm);
    currentMessage = (currentMessage > 0) ? currentMessage - 1 : 0;
}

