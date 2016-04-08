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
#include "iwPostWindow.h"
#include "world/GameWorldView.h"
#include "controls/ctrlText.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlImage.h"
#include "controls/ctrlMultiline.h"
#include "driver/src/KeyEvent.h"
#include "Loader.h"
#include "PostMsg.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Font.h"
#include "gameData/const_gui_ids.h"
#include "macros.h"
#include "GameClient.h"
#include <iostream>
// Include last!
#include "DebugNew.h" // IWYU pragma: keep

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


    gotoButton = AddImageButton(14, 181, 246, 30, 26, TC_GREY, LOADER.GetImageN("io", 107)); // Goto, nur sichtbar wenn Nachricht mit Koordinaten da
    gotoButton->SetVisible(false);
    deleteButton = AddImageButton(15, 211, 246, 30, 26, TC_GREY, LOADER.GetImageN("io", 106)); // Mülleimer, nur sichtbar, wenn Nachricht da
    deleteButton->SetVisible(false);



    postMsgInfos = AddText(18, 127, 228, "", MakeColor(255, 188, 100, 88), glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, SmallFont);
    postMsgInfos->SetVisible(false);

    postImage = AddImage(13, 127, 155, LOADER.GetImageN("io", 225));

    // Multiline-Teil mit drei leeren Zeilen erzeugen
    ctrlMultiline* text = AddMultiline(12, 126, 141, 200, 50, TC_INVISIBLE, NormalFont, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM | glArchivItem_Font::DF_NO_OUTLINE);
    text->EnableBox(false);
    text->AddString("", COLOR_WINDOWBROWN, false);
    text->AddString("", COLOR_WINDOWBROWN, false);
    text->AddString("", COLOR_WINDOWBROWN, false);

    SetMessageText(_("No letters!"));

    acceptButton = AddImageButton(16, 87, 185, 30, 26, TC_GREEN1, LOADER.GetImageN("io", 32)); // Button mit Haken, zum Annehmen von Verträgen
    acceptButton->SetVisible(false);
    declineButton = AddImageButton(17, 137, 185, 30, 26, TC_RED1, LOADER.GetImageN("io", 40)); // Button mit Kreuz, zum Ablehnen von Verträgen
    declineButton->SetVisible(false);

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
            PostMsgWithLocation* pml = dynamic_cast<PostMsgWithLocation*>(pm);
            if (pml)
            {
                gwv.MoveToMapPt(pml->GetPos());
            }
        }
        break;

        // Löschen
        case 15:
        {
            if(!GAMECLIENT.GetPostMessages().empty())
                DeletePostMessage(GetPostMsg(currentMessage));
        }
        break;

        // Haken-Button ("Ja")
        case 16:
        {
            PostMsg* pm = GetPostMsg(currentMessage);
            DiplomacyPostQuestion* dpm = dynamic_cast<DiplomacyPostQuestion*>(pm);
            if (dpm)
            {
                // Vertrag akzeptieren? Ja
                if(dpm->dp_type == DiplomacyPostQuestion::ACCEPT)
                    GAMECLIENT.AcceptPact(true, dpm->id, dpm->pt, dpm->player);
                // Vertrag beenden? Ja
                else if(dpm->dp_type == DiplomacyPostQuestion::CANCEL)
                    GAMECLIENT.CancelPact(dpm->pt, dpm->player);
                DeletePostMessage(pm);
            }
        } break;

        // Kreuz-Button ("Nein")
        case 17:
        {
            PostMsg* pm = GetPostMsg(currentMessage);
            DiplomacyPostQuestion* dpm = dynamic_cast<DiplomacyPostQuestion*>(pm);
            if (dpm)
            {
                // Vertrag annehmen? Nein
                // TODO: Sinnvoll ne art reject schicken, damit der andere mitbekommmt dass man nich will
                //if(dpm->dp_type == DiplomacyPostQuestion::ACCEPT)
                //  GAMECLIENT.CancelPact(dpm->pt,dpm->player);
                DeletePostMessage(pm);
            }
        }
        break;
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
 *
 *  @author Divan
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

    unsigned size = GAMECLIENT.GetPostMessages().size();

    // Keine Nachrichten, alles ausblenden, bis auf zentrierten Text
    if (size == 0)
    {
        SetMessageText(_("No letters!"));
        GetCtrl<ctrlMultiline>(12)->Move(xTextCenter, yTextCenter);

        postImage->SetVisible(false);
        deleteButton->SetVisible(false);
        gotoButton->SetVisible(false);
        acceptButton->SetVisible(false);
        declineButton->SetVisible(false);
        postMsgInfos->SetVisible(false);
        return;
    }

    // Falls currentMessage außerhalb der aktuellen Nachrichtenmenge liegt: korrigieren
    if (currentMessage >= size)
        currentMessage = size - 1;

    PostMsg* pm = GetPostMsg(currentMessage);

    // Nachrichten vorhanden, dann geht auf jeden Fall Löschbutton einblenden
    deleteButton->SetVisible(true);

    // Und ne Info-Zeile haben wir auch;
    std::stringstream ss;
    ss << _("Message") << " " << currentMessage + 1 << " " << _("of") << " " << size << " - GF: " << pm->GetSendFrame();
    postMsgInfos->SetText(ss.str());
    postMsgInfos->SetVisible(true);

    // Rest abhängig vom Nachrichten-Typ
    switch (pm->GetType())
    {
        case PMT_IMAGE_WITH_LOCATION:
        {
            ImagePostMsgWithLocation* ipm = dynamic_cast<ImagePostMsgWithLocation*>(pm);
            RTTR_Assert(ipm);

            SetMessageText(pm->GetText());
            GetCtrl<ctrlMultiline>(12)->Move(xTextTopCenter, yTextTopCenter);

            glArchivItem_Bitmap* img = ipm->GetImage_();
            postImage->SetImage(img);
            postImage->Move(xImgBottomCenter + img->getNx() - img->getWidth() / 2,
                            yImgBottomCenter + img->getNy() - img->getHeight());

            postImage->SetVisible(true);
            gotoButton->SetVisible(true);
            acceptButton->SetVisible(false);
            declineButton->SetVisible(false);
        } break;
        case PMT_WITH_LOCATION:
        {
            RTTR_Assert(dynamic_cast<PostMsgWithLocation*>(pm));

            SetMessageText(pm->GetText());
            GetCtrl<ctrlMultiline>(12)->Move(xTextCenter, yTextCenter);

            postImage->SetVisible(false);
            gotoButton->SetVisible(true);
            acceptButton->SetVisible(false);
            declineButton->SetVisible(false);
        } break;
        case PMT_DIPLOMACYQUESTION:
        {
            RTTR_Assert(dynamic_cast<DiplomacyPostQuestion*>(pm));

            SetMessageText(pm->GetText());
            GetCtrl<ctrlMultiline>(12)->Move(xTextTopCenter, yTextTopCenter);

            // Zwei Buttons einblenden...
            acceptButton->SetVisible(true);
            declineButton->SetVisible(true);
            gotoButton->SetVisible(false);
            postImage->SetVisible(false);
        } break;
        case PMT_NORMAL:
        case PMT_DIPLOMACYINFO:
        {
            SetMessageText(pm->GetText());
            GetCtrl<ctrlMultiline>(12)->Move(xTextCenter, yTextCenter);

            postImage->SetVisible(false);
            gotoButton->SetVisible(false);
            acceptButton->SetVisible(false);
            declineButton->SetVisible(false);
        } break;
        case PMT_SHIP:
        {
            ShipPostMsg* ipm = dynamic_cast<ShipPostMsg*>(pm);
            RTTR_Assert(ipm);

            SetMessageText(pm->GetText());
            GetCtrl<ctrlMultiline>(12)->Move(xTextTopCenter, yTextTopCenter);

            glArchivItem_Bitmap* img = ipm->GetImage_();
            postImage->SetImage(img);
            postImage->Move(xImgBottomCenter + img->getNx() - img->getWidth() / 2,
                            yImgBottomCenter + img->getNy() - img->getHeight());

            postImage->SetVisible(true);
            gotoButton->SetVisible(true);
            acceptButton->SetVisible(false);
            declineButton->SetVisible(false);
        } break;
    }
}

void iwPostWindow::SetMessageText(const std::string& message)
{
    ctrlMultiline* text = GetCtrl<ctrlMultiline>(12);

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

