// $Id: GameMessages.h 9539 2014-12-14 10:15:57Z marcus $
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

#ifndef GameMessage_GameCommand_h__
#define GameMessage_GameCommand_h__


#include "GameMessage.h"
#include "GameMessageInterface.h"
#include "GameProtocol.h"
#include "GameCommand.h"
#include "GameObject.h"

/*
 * das Klassenkommentar ist alles Client-Sicht, für Server-Sicht ist alles andersrum
 *
 * Konstruktor ohne Parameter ist allgemein nur zum Empfangen (immer noop!)
 * run-Methode ist Auswertung der Daten
 *
 * Konstruktor(en) mit Parametern (wenns auch nur der "reserved"-Parameter ist)
 * ist zum Verschicken der Nachrichten gedacht!
 */

class GameMessage_GameCommand : public GameMessage
{
    public:
        /// Checksumme, die der Spieler übermittelt
        unsigned checksum;
        unsigned obj_cnt;
        unsigned obj_id_cnt;
        /// Die einzelnen GameCommands
        std::vector<gc::GameCommandPtr> gcs;

    public:

        GameMessage_GameCommand(void) : GameMessage(NMS_GAMECOMMANDS) { }
        GameMessage_GameCommand(const unsigned char player, const unsigned checksum,
                                const std::vector<gc::GameCommandPtr>& gcs)
            : GameMessage(NMS_GAMECOMMANDS, player)
        {
            PushUnsignedInt(checksum);
            PushUnsignedInt(GameObject::GetObjCount());
            PushUnsignedInt(GameObject::GetObjIDCounter());
            PushUnsignedInt(gcs.size());

            for(unsigned i = 0; i < gcs.size(); ++i)
            {
                PushUnsignedChar(gcs[i]->GetType());
                gcs[i]->Serialize(this);
            }

        }

        GameMessage_GameCommand(const unsigned char* const data, const unsigned length)
            : GameMessage(NMS_GAMECOMMANDS, data, length),
              checksum(PopUnsignedInt()),
              obj_cnt(PopUnsignedInt()),
              obj_id_cnt(PopUnsignedInt()),
              gcs(PopUnsignedInt())
        {
            for(unsigned i = 0; i < gcs.size(); ++i)
            {
                gc::Type type = gc::Type(PopUnsignedChar());
                gcs[i] = gc::GameCommand::Deserialize(type, this);
            }

        }

        void Run(MessageInterface* callback)
        {
            checksum = PopUnsignedInt();
            obj_cnt = PopUnsignedInt();
            obj_id_cnt = PopUnsignedInt();
            gcs.resize(PopUnsignedInt());
            for(unsigned i = 0; i < gcs.size(); ++i)
            {
                gc::Type type = gc::Type(PopUnsignedChar());
                gcs[i] = gc::GameCommand::Deserialize(type, this);
            }

            GetInterface(callback)->OnNMSGameCommand(*this);
        }
};

#endif // GameMessage_GameCommand_h__

