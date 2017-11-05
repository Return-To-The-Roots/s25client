// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "rttrDefines.h" // IWYU pragma: keep
#include "GameMessage_GameCommand.h"
#include "GameMessageInterface.h"
#include "GameObject.h"
#include "GameProtocol.h"
#include "libutil/Serializer.h"

AsyncChecksum::AsyncChecksum() : randChecksum(0), objCt(0), objIdCt(0) {}

AsyncChecksum::AsyncChecksum(unsigned randChecksum)
    : randChecksum(randChecksum), objCt(GameObject::GetObjCount()), objIdCt(GameObject::GetObjIDCounter())
{}

AsyncChecksum::AsyncChecksum(unsigned randChecksum, unsigned objCt, unsigned objIdCt)
    : randChecksum(randChecksum), objCt(objCt), objIdCt(objIdCt)
{}

//////////////////////////////////////////////////////////////////////////

GameMessage_GameCommand::GameMessage_GameCommand() : GameMessage(NMS_GAMECOMMANDS) {}

GameMessage_GameCommand::GameMessage_GameCommand(const unsigned char player, const AsyncChecksum& checksum,
                                                 const std::vector<gc::GameCommandPtr>& gcs)
    : GameMessage(NMS_GAMECOMMANDS, player), gcs(checksum, gcs)
{}

void GameMessage_GameCommand::Serialize(Serializer& ser) const
{
    GameMessage::Serialize(ser);
    gcs.Serialize(ser);
}

void GameMessage_GameCommand::Deserialize(Serializer& ser)
{
    GameMessage::Deserialize(ser);
    gcs.Deserialize(ser);
}

bool GameMessage_GameCommand::Run(MessageInterface* callback)
{
    return GetInterface(callback)->OnGameMessage(*this);
}
