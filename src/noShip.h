// $Id: noShip.h 9357 2014-04-25 15:35:25Z FloSoft $
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

#ifndef NO_SHIP_H_
#define NO_SHIP_H_

#include "noMovable.h"
#include "MapConsts.h"
#include <list>

class nobHarborBuilding;

#define SHIP_CAPACITY 40


class noFigure;
class Ware;
class nofAttacker;

/// Klasse für die Schiffe
class noShip : public noMovable
{
        /// Spieler des Schiffes
        unsigned char player;

        /// Was macht das Schiff gerade?
        enum State
        {
            STATE_IDLE = 0, /// Schiff hat nix zu tun und hängt irgendwo an der Küste rum
            STATE_GOTOHARBOR,
            STATE_EXPEDITION_LOADING,
            STATE_EXPEDITION_UNLOADING,
            STATE_EXPEDITION_WAITING,
            STATE_EXPEDITION_DRIVING,
            STATE_EXPLORATIONEXPEDITION_LOADING,
            STATE_EXPLORATIONEXPEDITION_UNLOADING,
            STATE_EXPLORATIONEXPEDITION_WAITING,
            STATE_EXPLORATIONEXPEDITION_DRIVING,
            STATE_TRANSPORT_LOADING, // Schiff wird mit Waren/Figuren erst noch beladen, bleibt also für kurze Zeit am Hafen
            STATE_TRANSPORT_DRIVING, /// Schiff transportiert Waren/Figuren von einen Ort zum anderen
            STATE_TRANSPORT_UNLOADING, /// Entlädt Schiff am Zielhafen, kurze Zeit ankern, bevor Waren im Hafengebäude ankommen..
            STATE_SEAATTACK_LOADING,
            STATE_SEAATTACK_UNLOADING,
            STATE_SEAATTACK_DRIVINGTODESTINATION, /// Fährt mit den Soldaten zum Zielhafenpunkt
            STATE_SEAATTACK_WAITING, /// wartet an der Küste, während die Soldaten was schönes machen
            STATE_SEAATTACK_RETURN /// fährt mit den Soldaten wieder zurück zum Heimathafen

        } state;

        /// Das Meer, auf dem dieses Schiff fährt
        unsigned short sea_id;
        /// Zielpunkt des Schiffes
        unsigned goal_harbor_id;
        /// Anlegepunkt am Zielhafen, d.h. die Richtung relativ zum Zielpunkt
        unsigned char goal_dir;
        /// Namen des Schiffs
        std::string name;
        /// Schiffsroute und Position
        unsigned pos;
        std::vector<unsigned char> route;
        /// Ladung des Schiffes
        std::list<noFigure*> figures;
        std::list<Ware*> wares;
        /// Gibt an, ob das Schiff verlassen auf dem Meer auf einen Anlegeplatz wartet,
        /// um sein Zeug auszuladen
        bool lost;
        /// Bei Schiffen im STATE_SEAATTACK_WAITING:
        /// Anzahl der Soldaten, die noch kommen müssten
        /// For ships in STATE_TRANSPORT_x a 1 indicates that the ship is carrying returning soldiers from a sea attack
        unsigned remaining_sea_attackers;
        /// Heimathafen der Schiffs-Angreifer
        unsigned home_harbor;
        /// Anzahl an Strecke, die das Schiff schon seit Expeditionsstart zurückgelegt hat
        unsigned covered_distance;

    private:

        /// entscheidet, was nach einem gefahrenen Abschnitt weiter zu tun ist
        void Driven();
        /// Fängt an zu fahren
        void StartDriving(const unsigned char dir);

        void HandleState_GoToHarbor();
        void HandleState_ExpeditionDriving();
        void HandleState_ExplorationExpeditionDriving();
        void HandleState_TransportDriving();
        void HandleState_SeaAttackDriving();
        void HandleState_SeaAttackReturn();

        enum Result
        {
            DRIVING = 0,
            GOAL_REACHED,
            NO_ROUTE_FOUND,
            HARBOR_DOESNT_EXIST
        };


        /// Fährt weiter zu einem Hafen
        Result DriveToHarbour();
        /// Fährt weiter zu Hafenbauplatz
        Result DriveToHarbourPlace();

        /// Zeichnet das Schiff stehend mit oder ohne Waren
        void DrawFixed(const int x, const int y, const bool draw_wares);
        /// Zeichnet normales Fahren auf dem Meer ohne irgendwelche Güter
        void DrawDriving(int& x, int& y);
        /// Zeichnet normales Fahren auf dem Meer mit Gütern
        void DrawDrivingWithWares(int& x, int& y);

        /// Startet die eigentliche Transportaktion, nachdem das Schiff beladen wurde
        void StartTransport();

        /// Startet Schiffs-Angreiff
        void StartSeaAttack();

        /// Fängt an mit idlen und setzt nötigen Sachen auf NULL
        void StartIdling();

        /// Fängt an zu einem Hafen zu fahren (berechnet Route usw.)
        void StartDrivingToHarborPlace();

    public:

        /// Konstruktor
        noShip(const unsigned short x, const unsigned short y, const unsigned char player);
        noShip(SerializedGameData* sgd, const unsigned obj_id);

        ~noShip() {}

        void Serialize(SerializedGameData* sgd) const;
        void Destroy();

        GO_Type GetGOT() const { return GOT_SHIP; }

        // An x,y zeichnen
        void Draw(int x, int y);
        // Benachrichtigen, wenn neuer gf erreicht wurde
        void HandleEvent(const unsigned int id);

        /// Gibt den Besitzer zurück
        unsigned char GetPlayer() const { return player; }
        /// Hat das Schiff gerade nichts zu tun
        bool IsIdling() const { return (state == STATE_IDLE); }
        /// Gibt die ID des Meeres zurück, auf dem es sich befindet
        unsigned short GetSeaID() const { return sea_id; }
        /// Gibt den Schiffsnamen zurück
        const std::string& GetName() const { return name; }
        /// Führt das Schiff gerade eine Expedition durch und wartet auf weitere Befehle?
        bool IsWaitingForExpeditionInstructions() const
        { return (state == STATE_EXPEDITION_WAITING); }
        /// Ist das Schiff gerade irgendwie am Expeditionieren und hat entsprechenden Kram an Bord?
        bool IsOnExpedition() const
        {
            return (state == STATE_EXPEDITION_LOADING || state == STATE_EXPEDITION_WAITING
                    || state == STATE_EXPEDITION_DRIVING);
        }
        /// Ist das Schiff gerade irgendwie am Explorations-Expeditionieren und hat entsprechenden Kram an Bord?
        bool IsOnExplorationExpedition() const
        {
            return (state == STATE_EXPLORATIONEXPEDITION_LOADING || state == STATE_EXPLORATIONEXPEDITION_UNLOADING
                    || state == STATE_EXPLORATIONEXPEDITION_WAITING || state == STATE_EXPLORATIONEXPEDITION_DRIVING);
        }
        /// Gibt Liste der Waren an Bord zurück
        const std::list<Ware*> &GetWares() const { return wares; }
        /// Gibt Liste der Menschen an Bord zurück
        const std::list<noFigure*> &GetFigures() const { return figures; }
        /// Gibt Sichtradius dieses Schiffes zurück
        unsigned GetVisualRange() const;

        /// Beim Warten bei der Expedition: Gibt die Hafenpunkt-ID zurück, wo es sich gerade befindet
        unsigned GetCurrentHarbor() const;

        /// Sagt dem Schiff, an welchem Hafenpunkt es gerade ankert, wenn es das selber noch nicht weiß
        void AssignHarborId(const unsigned harbor_id)
        { this->goal_harbor_id = harbor_id; }

        /// Fährt zum Hafen, um dort eine Mission (Expedition) zu erledigen
        void GoToHarbor(nobHarborBuilding* hb, const std::vector<unsigned char>& route);
        /// Startet eine Expedition
        void StartExpedition();
        /// Startet eine Erkundungs-Expedition
        void StartExplorationExpedition();
        /// Weist das Schiff an, in einer bestimmten Richtung die Expedition fortzusetzen
        void ContinueExpedition(const unsigned char dir);
        /// Weist das Schiff an, eine Expedition abzubrechen (nur wenn es steht) und zum
        /// Hafen zurückzukehren
        void CancelExpedition();
        /// Weist das Schiff an, seine Erkundungs-Expedition fortzusetzen
        void ContinueExplorationExpedition();
        /// Gibt zurück, ob das Schiff jetzt in der Lage wäre, eine Kolonie zu gründen
        bool IsAbleToFoundColony() const;
        /// Weist das Schiff an, an der aktuellen Position einen Hafen zu gründen
        void FoundColony();
        /// Gibt zurück, ob das Schiff einen bestimmten Hafen ansteuert
        bool IsGoingToHarbor(nobHarborBuilding* hb) const;

        /// Belädt das Schiff mit Waren und Figuren, um eine Transportfahrt zu starten
        void PrepareTransport(Point<MapCoord> goal, const std::list<noFigure*>& figures, const std::list<Ware*>& wares);

        /// Belädt das Schiff mit Schiffs-Angreifern
        void PrepareSeaAttack(Point<MapCoord> goal, const std::list<noFigure*>& figures);
        /// Sagt Bescheid, dass ein Schiffsangreifer nicht mehr mit nach Hause fahren will
        void SeaAttackerWishesNoReturn();
        /// Schiffs-Angreifer sind nach dem Angriff wieder zurückgekehrt
        void AddAttacker(nofAttacker* attacker);

        /// Sagt dem Schiff, das ein bestimmter Hafen zerstört wurde
        void HarborDestroyed(nobHarborBuilding* hb);
        /// Sagt dem Schiff, dass ein neuer Hafen erbaut wurde
        void NewHarborBuilt(nobHarborBuilding* hb);





};


#endif

