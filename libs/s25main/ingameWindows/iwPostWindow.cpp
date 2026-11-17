// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
    : IngameWindow(CGI_POSTOFFICE, IngameWindow::posLastOrCenter, Extent(254, 295), _("Post office"),
                   LOADER.getImageN("resource", 41)),
      gwv(gwv), postBox(postBox), showAll(true), curCategory(PostCategory::General), curMsg(nullptr),
      lastHasMissionGoal(true)
{
    addImageButton(ID_SHOW_ALL, DrawPoint(18, 25), Extent(35, 35), TextureColor::Grey,
                   LOADER.getImageN("io", 190)); // Viewer: 191 - Papier
    addImageButton(ID_SHOW_MIL, DrawPoint(56, 25), Extent(35, 35), TextureColor::Grey,
                   LOADER.getImageN("io", 30)); // Viewer:  31 - Soldat
    addImageButton(ID_SHOW_GEO, DrawPoint(91, 25), Extent(35, 35), TextureColor::Grey,
                   LOADER.getImageN("io", 20)); // Viewer:  21 - Geologe
    addImageButton(ID_SHOW_ECO, DrawPoint(126, 25), Extent(35, 35), TextureColor::Grey,
                   LOADER.getImageN("io", 28)); // Viewer:  29 - Wage
    addImageButton(ID_SHOW_GEN, DrawPoint(161, 25), Extent(35, 35), TextureColor::Grey,
                   LOADER.getImageN("io", 189)); // Viewer: 190 - Neue Nachricht
    addImageButton(ID_SHOW_GOAL, DrawPoint(199, 25), Extent(35, 35), TextureColor::Grey,
                   LOADER.getImageN("io", 79)); // Viewer:  80 - Notiz
    addImage(0, DrawPoint(126, 151), LOADER.getImageN("io", 228));
    addImageButton(ID_HELP, DrawPoint(18, 242), Extent(30, 35), TextureColor::Grey,
                   LOADER.getImageN("io", 225)); // Viewer: 226 - Hilfe
    addImageButton(ID_GO_START, DrawPoint(51, 246), Extent(30, 26), TextureColor::Grey,
                   LOADER.getImageN("io", 102)); // Viewer: 103 - Schnell zurück
    addImageButton(ID_GO_BACK, DrawPoint(81, 246), Extent(30, 26), TextureColor::Grey,
                   LOADER.getImageN("io", 103)); // Viewer: 104 - Zurück
    addImageButton(ID_GO_FWD, DrawPoint(111, 246), Extent(30, 26), TextureColor::Grey,
                   LOADER.getImageN("io", 104)); // Viewer: 105 - Vor
    addImageButton(ID_GO_END, DrawPoint(141, 246), Extent(30, 26), TextureColor::Grey,
                   LOADER.getImageN("io", 105)); // Viewer: 106 - Schnell vor

    // Goto, nur sichtbar wenn Nachricht mit Koordinaten da
    addImageButton(ID_GOTO, DrawPoint(181, 246), Extent(30, 26), TextureColor::Grey, LOADER.getImageN("io", 107))
      ->setVisible(false);
    // Mülleimer, nur sichtbar, wenn Nachricht da
    addImageButton(ID_DELETE, DrawPoint(211, 246), Extent(30, 26), TextureColor::Grey, LOADER.getImageN("io", 106))
      ->setVisible(false);

    addText(ID_INFO, DrawPoint(127, 228), "", MakeColor(255, 188, 100, 88), FontStyle::CENTER | FontStyle::BOTTOM,
            SmallFont)
      ->setVisible(false);

    addImage(ID_IMG, DrawPoint(127, 155), LOADER.getImageN("io", 225));

    // Multiline-Teil mit drei leeren Zeilen erzeugen
    ctrlMultiline* text = addMultiline(ID_TEXT, DrawPoint(126, 141), Extent(200, 0), TextureColor::Invisible,
                                       NormalFont, FontStyle::CENTER | FontStyle::BOTTOM | FontStyle::NO_OUTLINE);
    text->setNumVisibleLines(4);
    text->showBackground(false);

    // Button with OK and deny sign (tick and cross) for contracts
    addImageButton(ID_ACCEPT, DrawPoint(87, 185), Extent(30, 26), TextureColor::Green1, LOADER.getImageN("io", 32))
      ->setVisible(false);
    addImageButton(ID_DENY, DrawPoint(137, 185), Extent(30, 26), TextureColor::Red1, LOADER.getImageN("io", 40))
      ->setVisible(false);

    filterMessages();
    curMsgId = curMsgIdxs.size();
    displayPostMessage();
}

void iwPostWindow::msgButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_HELP:
            WINDOWMANAGER.replaceWindow(std::make_unique<iwHelp>(
              _("All important messages are collected in this window and "
                "sorted into groups. If this window is not open, the dove "
                "symbol at the bottom of the screen indicates the arrival of a new message.")));
            break;
        case ID_SHOW_ALL:
            showAll = true;
            filterMessages();
            curMsgId = curMsgIdxs.size();
            displayPostMessage();
            break;
        case ID_SHOW_GOAL:
            if(!postBox.getCurrentMissionGoal().empty())
                WINDOWMANAGER.show(std::make_unique<iwMissionStatement>(_("Diary"), postBox.getCurrentMissionGoal(),
                                                                        false, iwMissionStatement::IM_AVATAR9));
            break;
        case ID_SHOW_MIL: switchCategory(PostCategory::Military); break;
        case ID_SHOW_GEO: switchCategory(PostCategory::Geologist); break;
        case ID_SHOW_ECO: switchCategory(PostCategory::Economy); break;
        case ID_SHOW_GEN: switchCategory(PostCategory::General); break;
        case ID_GO_START:
            // To oldest
            curMsgId = 0;
            displayPostMessage();
            break;
        case ID_GO_BACK:
            // Back
            curMsgId = (curMsgId > 0) ? curMsgId - 1 : 0;
            displayPostMessage();
            break;
        case ID_GO_FWD:
            // Forward
            ++curMsgId;
            displayPostMessage();
            break;
        case ID_GO_END:
            // To newest
            curMsgId = postBox.getNumMsgs();
            displayPostMessage();
            break;
        case ID_GOTO:
        {
            // Goto
            if(!validateMessages())
                return;
            if(curMsg && curMsg->getPos().isValid())
                gwv.moveToMapPt(curMsg->getPos());
        }
        break;

        case ID_DELETE: // Delete
        {
            if(!validateMessages() || !curMsg)
                return;
            postBox.deleteMsg(curMsg);
            break;
        }
        case ID_DENY: // Cross (Deny)
        {
            if(!validateMessages() || !curMsg)
                return;
            const auto* dcurMsg = dynamic_cast<const DiplomacyPostQuestion*>(curMsg);
            if(dcurMsg)
            {
                // If it is a question about a new contract, tell the other player we denied it
                if(dcurMsg->isAccept())
                    if(GAMECLIENT.CancelPact(dcurMsg->getPactType(), dcurMsg->getPlayerId()))
                        postBox.deleteMsg(curMsg);
            }
        }
        break;

        // Haken-Button ("Ja")
        case ID_ACCEPT:
        {
            if(!validateMessages())
                return;
            const auto* dcurMsg = dynamic_cast<const DiplomacyPostQuestion*>(getMsg(curMsgId));
            if(dcurMsg)
            {
                bool success = false;
                // New contract?
                if(dcurMsg->isAccept())
                    success =
                      GAMECLIENT.AcceptPact(dcurMsg->getPactId(), dcurMsg->getPactType(), dcurMsg->getPlayerId());
                else
                    success = GAMECLIENT.CancelPact(dcurMsg->getPactType(), dcurMsg->getPlayerId());
                if(success)
                    postBox.deleteMsg(dcurMsg);
            }
        }
        break;
    }
}

void iwPostWindow::msgPaintBefore()
{
    IngameWindow::msgPaintBefore();
    validateMessages();
    if(lastHasMissionGoal != !postBox.getCurrentMissionGoal().empty())
    {
        lastHasMissionGoal = !postBox.getCurrentMissionGoal().empty();
        getCtrl<Window>(ID_SHOW_GOAL)->setVisible(lastHasMissionGoal);
    }
}

/**
 *  React on keypress
 */
bool iwPostWindow::msgKeyDown(const KeyEvent& ke)
{
    switch(ke.kt)
    {
        default: break;
        case KeyType::Delete: // Delete current message
#ifdef __APPLE__
        case KeyType::Backspace: // Macs usually have no delete key on small keyboards, so backspace is more convenient
#endif
            msgButtonClick(ID_DELETE);
            return true;
    }

    switch(ke.c)
    {
        case '+': // Next message
            msgButtonClick(ID_GO_FWD);
            return true;
        case '-': // Previous message
            msgButtonClick(ID_GO_BACK);
            return true;
        case 'g': // Go to site of event
            msgButtonClick(ID_GOTO);
            return true;
    }

    return false;
}

// Zeigt Nachricht an, passt Steuerelemente an
void iwPostWindow::displayPostMessage()
{
    const DrawPoint imgBottomCenter(127, 210); //-V821
    // todo: koordinaten abschmecken
    const DrawPoint textTopCenter(127, 110); //-V821
    const DrawPoint textCenter(126, 151);

    // Hide everything
    getCtrl<Window>(ID_IMG)->setVisible(false);
    getCtrl<Window>(ID_DELETE)->setVisible(false);
    getCtrl<Window>(ID_GOTO)->setVisible(false);
    getCtrl<Window>(ID_ACCEPT)->setVisible(false);
    getCtrl<Window>(ID_DENY)->setVisible(false);
    getCtrl<Window>(ID_INFO)->setVisible(false);

    unsigned size = curMsgIdxs.size();

    // Keine Nachrichten, alles ausblenden, bis auf zentrierten Text
    if(size == 0)
    {
        setMessageText(_("No letters!"));
        getCtrl<Window>(ID_TEXT)->setPos(textCenter);
        curMsg = nullptr;
        return;
    }

    // Falls currentMessage außerhalb der aktuellen Nachrichtenmenge liegt: korrigieren
    if(curMsgId >= size)
        curMsgId = size - 1;

    curMsg = getMsg(curMsgId);

    // We have a message, display its status...
    std::stringstream ss;
    ss << _("Message") << " " << curMsgId + 1 << "/" << size << " - " << _("Time") << ": "
       << GAMECLIENT.formatGFTime(curMsg->getSendFrame());
    getCtrl<ctrlText>(ID_INFO)->setText(ss.str());
    getCtrl<ctrlText>(ID_INFO)->setVisible(true);
    // ...and delete button
    getCtrl<Window>(ID_DELETE)->setVisible(true);

    setMessageText(curMsg->getText());
    ITexture* const img = curMsg->getImage();
    if(img)
    {
        // We have an image, show it centered
        getCtrl<ctrlImage>(ID_IMG)->setImage(img);
        getCtrl<Window>(ID_IMG)->setPos(imgBottomCenter + img->getOrigin()
                                        - DrawPoint(img->getSize().x / 2, img->getSize().y));

        getCtrl<Window>(ID_IMG)->setVisible(true);
    }
    if(dynamic_cast<const DiplomacyPostQuestion*>(curMsg))
    {
        getCtrl<Window>(ID_ACCEPT)->setVisible(true);
        getCtrl<Window>(ID_DENY)->setVisible(true);
    }
    // Place text at top or center depending on whether we have an img or the acceptButton
    if(img || getCtrl<Window>(ID_ACCEPT)->isVisible())
        getCtrl<Window>(ID_TEXT)->setPos(textTopCenter);
    else
        getCtrl<Window>(ID_TEXT)->setPos(textCenter);
    // If message contains valid position, allow going to it
    if(curMsg->getPos().isValid())
        getCtrl<Window>(ID_GOTO)->setVisible(true);
}

void iwPostWindow::setMessageText(const std::string& message)
{
    auto* text = getCtrl<ctrlMultiline>(ID_TEXT);
    text->clear();
    text->addString(message, COLOR_WINDOWBROWN);
}

void iwPostWindow::filterMessages()
{
    curMsgIdxs.clear();
    lastMsgCt = postBox.getNumMsgs();
    for(unsigned i = 0; i < lastMsgCt; i++)
    {
        if(showAll)
            curMsgIdxs.push_back(i);
        else
        {
            const PostCategory msgCat = postBox.getMsg(i)->getCategory();
            // We sort diplomacy messages to general messages as we don't have another button
            if(msgCat == curCategory || (curCategory == PostCategory::General && msgCat == PostCategory::Diplomacy))
                curMsgIdxs.push_back(i);
        }
    }
}

bool iwPostWindow::validateMessages()
{
    if(lastMsgCt == postBox.getNumMsgs() && getMsg(curMsgId) == curMsg)
        return true;
    // Messages have changed -> Update filter
    filterMessages();
    if(!curMsg)
    {
        // No last message? Display oldest
        curMsgId = 0;
        displayPostMessage();
        return false;
    } else
    {
        // Message was either deleted or others were added and message was shifted
        // So first search it, if not found we will display the next (newer) message
        for(unsigned i = curMsgId; i > 0; i--)
        {
            if(postBox.getMsg(i - 1) == curMsg)
            {
                curMsgId = i - 1;
                displayPostMessage();
                return true;
            }
        }
        // Display next valid one
        displayPostMessage();
        return false;
    }
}

const PostMsg* iwPostWindow::getMsg(unsigned id) const
{
    if(id < curMsgIdxs.size())
        return postBox.getMsg(curMsgIdxs[id]);
    else
        return nullptr;
}

void iwPostWindow::switchCategory(PostCategory cat)
{
    showAll = false;
    curCategory = cat;
    filterMessages();
    curMsgId = curMsgIdxs.size();
    displayPostMessage();
}
