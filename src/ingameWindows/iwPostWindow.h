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

#ifndef WP_POSTOFFICE_H_
#define WP_POSTOFFICE_H_

#include "IngameWindow.h"

class GameWorldView;
class PostMsg;
class ctrlImage;
class ctrlImageButton;
class ctrlText;
struct KeyEvent;

class iwPostWindow : public IngameWindow
{
    public:
        iwPostWindow(GameWorldView& gwv);
        void Msg_PaintBefore() override;
        void Msg_ButtonClick(const unsigned int ctrl_id) override;
        bool Msg_KeyDown(const KeyEvent& ke) override;

    private:
        GameWorldView& gwv;
        ctrlImage* postImage;
        ctrlText* postMsgInfos;

        ctrlImageButton* gotoButton;
        ctrlImageButton* deleteButton;

        ctrlImageButton* acceptButton;
        ctrlImageButton* declineButton;

        unsigned currentMessage;

        /// Liefert Pointer auf die pos-te Nachricht zurück
        PostMsg* GetPostMsg(unsigned pos) const;

        /// Passt Steuerelemente an, setzt Einstellung für diverse Controls passend für die aktuelle PostMessage
        void DisplayPostMessage();

        /// Nachricht löschen
        void DeletePostMessage(PostMsg* pm);

        /// Setzt den Text mehrzeilig in das Postfenster
        void SetMessageText(const std::string& message);

        unsigned lastSize;
};

#endif
