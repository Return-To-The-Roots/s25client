// $Id: CatapultStone.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef CATAPULT_STONE_H_
#define CATAPULT_STONE_H_

#include "GameObject.h"
#include "EventManager.h"

class nobMilitary;
class GameWorldView;
class GameWorldViewer;


/// Klasse für einen fliegenden Katapultstein
class CatapultStone : public GameObject
{
    private:

        /// (Map-)Koordinaten des Hauses, in dem er einschlagen soll
        const unsigned short dest_building_x, dest_building_y;
        /// Aufschlagspunkt des Steines (Map-Koordinaten!)
        const unsigned short dest_map_x, dest_map_y;
        /// Koordinaten der Startposition des Steins
        const int start_x, start_y;
        /// Koordinaten der Zielposition des Steins
        const int dest_x, dest_y;
        /// Explodiert der Stein schon? (false = fliegt)
        bool explode;
        /// Flieg-/Explodier-Event
        EventManager::Event* event;

    public:

        CatapultStone(const unsigned short dest_building_x, const unsigned short dest_building_y, const unsigned short dest_map_x, const unsigned short dest_map_y,
                      const int start_x, const int start_y, const int dest_x, const int dest_y, const unsigned fly_duration);

        CatapultStone(SerializedGameData* sgd, const unsigned obj_id);

        ~CatapultStone() {}

        /// Zerstören
        void Destroy();

        /// Serialisierungsfunktionen
    protected:  void Serialize_CatapultStone(SerializedGameData* sgd) const;
    public:     void Serialize(SerializedGameData* sgd) const { Serialize_CatapultStone(sgd); }

        // Zeichnet den fliegenden Stein
        void Draw(const GameWorldView& gwv, const int xoffset, const int yoffset);

        /// Event-Handler
        void HandleEvent(const unsigned int id);

        GO_Type GetGOT() const { return GOT_CATAPULTSTONE; }
};

#endif // !CATAPULT_STONE_H_
