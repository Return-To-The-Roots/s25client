// Copyright (c) 2016 Settlers Freaks (sf-team at siedler25.org)
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

#pragma once

#ifndef GameServerInterface_h__
#define GameServerInterface_h__

class GameMessage;
class GlobalGameSettings;
struct JoinPlayerInfo;

/// Abstract interface for the GameServer so it can be emulated for testing
class GameServerInterface
{
public:
    virtual ~GameServerInterface(){}
    virtual bool IsRunning() const = 0;
    virtual unsigned GetMaxPlayerCount() const = 0;
    virtual JoinPlayerInfo& GetJoinPlayer(unsigned playerIdx) = 0;
    virtual void KickPlayer(unsigned playerIdx) = 0;
    virtual void CheckAndSetColor(unsigned playerIdx, unsigned newColor) = 0;
    virtual void AnnounceStatusChange() = 0;
    virtual const GlobalGameSettings& GetGGS() const = 0;
    virtual void ChangeGlobalGameSettings(const GlobalGameSettings& ggs) = 0;
    virtual void SendToAll(const GameMessage& msg) = 0;
};

#endif // GameServerInterface_h__
