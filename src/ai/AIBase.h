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
#ifndef AIBASE_H_INCLUDED
#define AIBASE_H_INCLUDED

#pragma once

#include "AIEventManager.h"
#include "AIInterface.h"
#include "GameCommand.h"

class GameWorldBase;
class GameClientPlayer;
class GlobalGameSettings;
class GameClientPlayerList;

/// Base class for all AI player. The AIBase class provides an interface which may be implemented by different AI
/// players. 
class AIBase
{
    protected:

		/// Unique player ID an AI player needs, e.g. to inspect the map. 
        const unsigned char playerid;

		/// Reference to the game world to get information for strategic planning
        const GameWorldBase& gwb;

		/// Reference to its own GameClientPlayer to get additional information. e.g. about economy
        const GameClientPlayer& player;

		/// Reference to a list of all player, e.g. to check for possible alliances
        const GameClientPlayerList& players;

		/// Queue of game comments to process
		std::vector<gc::GameCommandPtr> gcs;

		/// AI difficulty level
        const AI::Level level;

		/// Abstract interface to forward commands (???)
		AIInterface* aii;

    public:

		/// Creates a new instance of AIBase initializes the members with the specified 
		/// information (game world, client player, player list, etc.).
        AIBase(const unsigned char playerid, const GameWorldBase& gwb, const GameClientPlayer& player,
               const GameClientPlayerList& players, const GlobalGameSettings& ggs, const AI::Level level)
            : playerid(playerid), gwb(gwb), player(player), players(players), level(level), aii(new AIInterface(gwb, player, players, gcs, playerid)), ggs(ggs) {}

		/// Destroys the current instance of AIBase and releases its memory. 
        virtual ~AIBase() {}

		/// Executes one AI move based on the current state of the game world. 
		/// This method shell be called once per frame to adapt AI actions regularily.
		virtual void RunGF(const unsigned gf, bool gfisnwf) = 0;

		/// Reference to the global game settings. Those settings may be influencing the decision 
		/// making process of the AI player (e.g. special behaviour if FOW is activated).
		const GlobalGameSettings& ggs;

		/// Provides access to the GameCommands which are enqueued to be executed
		const std::vector<gc::GameCommandPtr>& GetGameCommands() const { return gcs; }
        
		/// Marks all GameCommands in the queue as executed
		void FetchGameCommands() { gcs.clear(); }

		/// Sends an AI event (???)
        virtual void SendAIEvent(AIEvent::Base* ev) { delete ev; }
};

#endif //!AIBASE_H_INCLUDED
