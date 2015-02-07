// $Id: Ware.h 9599 2015-02-07 11:08:22Z marcus $
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

#ifndef WARE_H_
#define WARE_H_

#include "GameConsts.h"
#include "GameObject.h"
#include "MapConsts.h"
#include "noRoadNode.h"

class noBaseBuilding;
class GameWorld;
class nobHarborBuilding;


// Die Klasse Ware kennzeichnet eine Ware, die von einem Träger transportiert wird bzw gerade an einer Flagge liegt
class Ware : public GameObject
{
        /// Die Richtung von der Fahne auf dem Weg, auf dem die Ware transportiert werden will als nächstes
        unsigned char next_dir;
        /// In welchem Status die Ware sich gerade befindet
        enum State
        {
            STATE_WAITINWAREHOUSE = 0, // Ware wartet im Lagerhaus noch auf Auslagerun
            STATE_WAITATFLAG, // Ware liegt an einer Fahne und wartet auf den Träger, der kommt
            STATE_CARRIED, // Ware wird von einem Träger getragen
            STATE_WAITFORSHIP, // Ware wartet im Hafengebäude auf das Schiff, das sie abholt
            STATE_ONSHIP // Ware befindet sich auf einem Schiff
        } state;
        /// Auf welcher Flagge, in welchem Gebäude die Ware gerade ist (bei STATE_CARRIED ist es die Flagge, zu der die Ware getragen wird!)
        noRoadNode* location;

    public:

        /// Was für eine Ware
        const GoodType type;
        /// Wo die Ware mal hin soll
        noBaseBuilding* goal;
        /// Nächster Hafenpunkt, der ggf. angesteuert werden soll
        Point<MapCoord> next_harbor;

    public:

        Ware(const GoodType type, noBaseBuilding* goal, noRoadNode* location);
        Ware(SerializedGameData* sgd, const unsigned obj_id);

        ~Ware();

        void Destroy(void);

        /// Serialisierungsfunktionen
    protected:  void Serialize_Ware(SerializedGameData* sgd) const;
    public:     void Serialize(SerializedGameData* sgd) const { Serialize_Ware(sgd); }

        GO_Type GetGOT() const { return GOT_WARE; }

        /// siehe oben
        inline unsigned char GetNextDir() const { return next_dir; }
        /// Gibt nächsten Hafen zurück, falls vorhanden
        Point<MapCoord> GetNextHarbor() const { return  next_harbor; }
        /// Berechnet den Weg neu zu ihrem Ziel
        void RecalcRoute();
		/// set new next dir
		void SetNextDir(unsigned char newnextdir) {next_dir=newnextdir;}
        /// Wird aufgerufen, wenn es das Ziel der Ware nicht mehr gibt und sie wieder "nach Hause" getragen werden muss
        void GoalDestroyed();
        /// Verändert den Status der Ware
        void LieAtFlag(noRoadNode* flag) { state = STATE_WAITATFLAG; location = flag; }
        void Carry(noRoadNode* next_flag) { state = STATE_CARRIED; location = next_flag;  }
        /// Gibt dem Ziel der Ware bekannt, dass diese nicht mehr kommen kann
        void NotifyGoalAboutLostWare();
        /// Wenn die Ware vernichtet werden muss
        void WareLost(const unsigned char player);
        /// Gibt Status der Ware zurück
        bool LieAtFlag() const { return (state == STATE_WAITATFLAG); }
        bool LieInWarehouse() const { return (state == STATE_WAITINWAREHOUSE); }
        bool LieInHarborBuilding() const { return (state == STATE_WAITFORSHIP); }
        /// Sagt dem Träger Bescheid, dass sie in die aktuelle (next_dir) Richtung nicht mehr getragen werden will
        void RemoveWareJobForCurrentDir(const unsigned char last_next_dir);
        /// Überprüft, ob es noch ein Weg zum Ziel gibt für Waren, die noch im Lagerhaus liegen
        bool FindRouteFromWarehouse();
        /// Sagt der Ware, dass sie sich ein Lagerhaus nochmal suchen soll (für LostWares gedacht, die kein Lagerhaus mehr gefunden haben)
        void FindRouteToWarehouse();
		///a building is looking for a ware - check if this lost ware can be send to the building and then do it
		unsigned CheckNewGoalForLostWare(noBaseBuilding* newgoal);
		void SetNewGoalForLostWare(noBaseBuilding* newgoal);
        /// Gibt Ort der Ware zurück
        noRoadNode* GetLocation() { return location; }
        /// Ist die Ware eine LostWare (Ware, die kein Ziel mehr hat und irgendwo sinnlos rumliegt)?
        bool IsLostWare() const { return ((goal ? false : true) && state != STATE_ONSHIP); }
        /// Informiert Ware, dass eine Schiffsreise beginnt
        void StartShipJourney();
        /// Informiert Ware, dass Schiffsreise beendet ist und die Ware nun in einem Hafengebäude liegt
        /// Gibt true zurück, wenn die Ware rausgetragen will oder false, wenn sie kein Ziel mehr hat und ins Hafengebäude
        /// eingelagert werden will
        bool ShipJorneyEnded(nobHarborBuilding* hb);
        /// Beginnt damit auf ein Schiff im Hafen zu warten
        void WaitForShip(nobHarborBuilding* hb);

        std::string ToString() const {std::stringstream s; s << "Ware(" << obj_id << "): type=" << GoodType2String(type) << ", location=" << location->GetX() << "," << location->GetY(); return s.str();}

        static std::string GoodType2String(GoodType value)
        {
            switch (value)
            {
                case GD_AXE: return("GD_AXE");
                case GD_BEER: return("GD_BEER");
                case GD_BOARDS: return("GD_BOARDS");
                case GD_BOAT: return("GD_BOAT");
                case GD_BOW: return("GD_BOW");
                case GD_BREAD: return("GD_BREAD");
                case GD_CLEAVER: return("GD_CLEAVER");
                case GD_COAL: return("GD_COAL");
                case GD_COINS: return("GD_COINS");
                case GD_CRUCIBLE: return("GD_CRUCIBLE");
                case GD_FISH: return("GD_FISH");
                case GD_FLOUR: return("GD_FLOUR");
                case GD_GOLD: return("GD_GOLD");
                case GD_GRAIN: return("GD_GRAIN");
                case GD_HAM: return("GD_HAM");
                case GD_HAMMER: return("GD_HAMMER");
                case GD_IRON: return("GD_IRON");
                case GD_IRONORE: return("GD_IRONORE");
                case GD_MEAT: return("GD_MEAT");
                case GD_NOTHING: return("GD_NOTHING");
                case GD_PICKAXE: return("GD_PICKAXE");
                case GD_RODANDLINE: return("GD_RODANDLINE");
                case GD_ROLLINGPIN: return("GD_ROLLINGPIN");
                case GD_SAW: return("GD_SAW");
                case GD_SCYTHE: return("GD_SCYTHE");
                case GD_SHIELDAFRICANS: return("GD_SHIELDAFRICANS");
                case GD_SHIELDJAPANESE: return("GD_SHIELDJAPANESE");
                case GD_SHIELDROMANS: return("GD_SHIELDROMANS");
                case GD_SHIELDVIKINGS: return("GD_SHIELDVIKINGS");
                case GD_SHOVEL: return("GD_SHOVEL");
                case GD_STONES: return("GD_STONES");
                case GD_SWORD: return("GD_SWORD");
                case GD_TONGS: return("GD_TONGS");
                case GD_WATER: return("GD_WATER");
                case GD_WATEREMPTY: return("GD_WATEREMPTY");
                case GD_WOOD: return("GD_WOOD");
            }

            return("unknown");
        }
};




#endif
