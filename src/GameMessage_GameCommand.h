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

#ifndef GameMessage_GameCommand_h__
#define GameMessage_GameCommand_h__


#include "GameMessage.h"
#include "GameMessageInterface.h"
#include "GameProtocol.h"
#include "GameCommand.h"
#include "GameObject.h"
#include "libutil/src/Serializer.h"
#include <vector>

/*
 * das Klassenkommentar ist alles Client-Sicht, für Server-Sicht ist alles andersrum
 *
 * Konstruktor ohne Parameter ist allgemein nur zum Empfangen (immer noop!)
 * run-Methode ist Auswertung der Daten
 *
 * Konstruktor(en) mit Parametern (wenns auch nur der "reserved"-Parameter ist)
 * ist zum Verschicken der Nachrichten gedacht!
 */

struct AsyncChecksum{
    unsigned randState;
    unsigned objCt;
    unsigned objIdCt;
    AsyncChecksum(): randState(0), objCt(0), objIdCt(0){}
    explicit AsyncChecksum(unsigned randState): randState(randState), objCt(GameObject::GetObjCount()), objIdCt(GameObject::GetObjIDCounter()){}
    AsyncChecksum(unsigned randState, unsigned objCt, unsigned objIdCt): randState(randState), objCt(objCt), objIdCt(objIdCt){}

    inline bool operator==(const AsyncChecksum& rhs) const;
    inline bool operator!=(const AsyncChecksum& rhs) const;
};

class GameMessage_GameCommand : public GameMessage
{
    public:
        /// Checksumme, die der Spieler übermittelt
        AsyncChecksum checksum;
        /// Die einzelnen GameCommands
        std::vector<gc::GameCommandPtr> gcs;

    public:

        GameMessage_GameCommand() : GameMessage(NMS_GAMECOMMANDS) { } //-V730
        GameMessage_GameCommand(const unsigned char player, const AsyncChecksum& checksum, const std::vector<gc::GameCommandPtr>& gcs)
            : GameMessage(NMS_GAMECOMMANDS, player), checksum(checksum), gcs(gcs){}

        void Serialize(Serializer& ser) const override
        {
            GameMessage::Serialize(ser);
            ser.PushUnsignedInt(checksum.randState);
            ser.PushUnsignedInt(checksum.objCt);
            ser.PushUnsignedInt(checksum.objIdCt);
            ser.PushUnsignedInt(gcs.size());

            for(unsigned i = 0; i < gcs.size(); ++i)
            {
                ser.PushUnsignedChar(gcs[i]->GetType());
                gcs[i]->Serialize(ser);
            }

        }

        void Deserialize(Serializer& ser) override
        {
            GameMessage::Deserialize(ser);
            checksum.randState = ser.PopUnsignedInt();
            checksum.objCt = ser.PopUnsignedInt();
            checksum.objIdCt = ser.PopUnsignedInt();
            gcs.resize(ser.PopUnsignedInt());
            for(unsigned i = 0; i < gcs.size(); ++i)
            {
                gc::Type type = gc::Type(ser.PopUnsignedChar());
                gcs[i] = gc::GameCommand::Deserialize(type, ser);
            }
        }

        void Run(MessageInterface* callback) override
        {
            GetInterface(callback)->OnGameMessage(*this);
        }
};

bool AsyncChecksum::operator==(const AsyncChecksum& rhs) const
{
    return randState == rhs.randState &&
               objCt == rhs.objCt &&
             objIdCt == rhs.objIdCt;
}

bool AsyncChecksum::operator!=(const AsyncChecksum& rhs) const
{
    return !(*this == rhs);
}

#endif // GameMessage_GameCommand_h__

