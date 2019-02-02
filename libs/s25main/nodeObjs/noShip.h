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

#ifndef NO_SHIP_H_
#define NO_SHIP_H_

#include "noMovable.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/ShipDirection.h"
#include <list>
#include <vector>

#define SHIP_CAPACITY 40

class noFigure;
class nobHarborBuilding;
class Ware;
class nofAttacker;
class SerializedGameData;

/// Klasse für die Schiffe
class noShip : public noMovable
{
    /// Spieler des Schiffes
    unsigned char ownerId_;

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
        STATE_TRANSPORT_LOADING,   // Schiff wird mit Waren/Figuren erst noch beladen, bleibt also für kurze Zeit am Hafen
        STATE_TRANSPORT_DRIVING,   /// Schiff transportiert Waren/Figuren von einen Ort zum anderen
        STATE_TRANSPORT_UNLOADING, /// Entlädt Schiff am Zielhafen, kurze Zeit ankern, bevor Waren im Hafengebäude ankommen..
        STATE_SEAATTACK_LOADING,
        STATE_SEAATTACK_UNLOADING,
        STATE_SEAATTACK_DRIVINGTODESTINATION, /// Fährt mit den Soldaten zum Zielhafenpunkt
        STATE_SEAATTACK_WAITING,              /// wartet an der Küste, während die Soldaten was schönes machen
        STATE_SEAATTACK_RETURN_DRIVING        /// fährt mit den Soldaten wieder zurück zum Heimathafen
    } state;

    /// Das Meer, auf dem dieses Schiff fährt
    unsigned short seaId_;
    /// Zielpunkt des Schiffes
    unsigned goal_harborId;
    /// Anlegepunkt am Zielhafen, d.h. die Richtung relativ zum Zielpunkt
    unsigned char goal_dir;
    /// Namen des Schiffs
    std::string name;
    /// Schiffsroute und Position
    unsigned curRouteIdx;
    std::vector<Direction> route_;
    /// Ladung des Schiffes
    std::list<noFigure*> figures;
    std::list<Ware*> wares;
    /// Gibt an, ob das Schiff verlassen auf dem Meer auf einen Anlegeplatz wartet,
    /// um sein Zeug auszuladen
    bool lost;
    /// Bei Schiffen im STATE_SEAATTACK_WAITING:
    /// Anzahl der Soldaten, die noch kommen müssten
    unsigned remaining_sea_attackers;
    /// Heimathafen der Schiffs-Angreifer
    unsigned home_harbor;
    /// Anzahl an Strecke, die das Schiff schon seit Expeditionsstart zurückgelegt hat
    unsigned covered_distance;

private:
    /// entscheidet, was nach einem gefahrenen Abschnitt weiter zu tun ist
    void Driven();
    /// Fängt an zu fahren
    void StartDriving(const Direction dir);

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
    void DrawFixed(DrawPoint drawPt, bool draw_wares);
    /// Zeichnet normales Fahren auf dem Meer ohne irgendwelche Güter
    void DrawDriving(DrawPoint& drawPt);
    /// Zeichnet normales Fahren auf dem Meer mit Gütern
    void DrawDrivingWithWares(DrawPoint& drawPt);

    /// Startet die eigentliche Transportaktion, nachdem das Schiff beladen wurde
    void StartTransport();

    /// Startet Schiffs-Angreiff
    void StartSeaAttack();

    /// Fängt an mit idlen und setzt nötigen Sachen auf nullptr
    void StartIdling();

    /// Fängt an zu einem Hafen zu fahren (berechnet Route usw.)
    void StartDrivingToHarborPlace();

    /// Looks for a harbour to unload the goods (e.g if old one was destroyed)
    void FindUnloadGoal(State newState);
    /// Aborts a sea attack (in case harbor was not found anymore)
    void AbortSeaAttack();

public:
    noShip(const MapPoint pt, unsigned char player);
    noShip(SerializedGameData& sgd, unsigned obj_id);

    ~noShip() override;

    void Serialize(SerializedGameData& sgd) const override;
    void Destroy() override;

    GO_Type GetGOT() const override { return GOT_SHIP; }

    // An x,y zeichnen
    void Draw(DrawPoint drawPt) override;
    // Benachrichtigen, wenn neuer gf erreicht wurde
    void HandleEvent(unsigned id) override;

    /// Gibt den Besitzer zurück
    unsigned char GetPlayerId() const { return ownerId_; }
    /// Gibt die ID des Meeres zurück, auf dem es sich befindet
    unsigned short GetSeaID() const { return seaId_; }
    /// Gibt den Schiffsnamen zurück
    const std::string& GetName() const { return name; }
    /// Hat das Schiff gerade nichts zu tun
    bool IsIdling() const { return (state == STATE_IDLE); }
    bool IsLost() const { return lost; }
    /// Führt das Schiff gerade eine Expedition durch und wartet auf weitere Befehle?
    bool IsWaitingForExpeditionInstructions() const { return (state == STATE_EXPEDITION_WAITING); }
    /// Ist das Schiff gerade irgendwie am Expeditionieren und hat entsprechenden Kram an Bord?
    bool IsOnExpedition() const
    {
        return (state == STATE_EXPEDITION_LOADING || state == STATE_EXPEDITION_WAITING || state == STATE_EXPEDITION_DRIVING);
    }
    /// Ist das Schiff gerade irgendwie am Explorations-Expeditionieren und hat entsprechenden Kram an Bord?
    bool IsOnExplorationExpedition() const
    {
        return (state == STATE_EXPLORATIONEXPEDITION_LOADING || state == STATE_EXPLORATIONEXPEDITION_UNLOADING
                || state == STATE_EXPLORATIONEXPEDITION_WAITING || state == STATE_EXPLORATIONEXPEDITION_DRIVING);
    }
    bool IsOnAttackMission() const
    {
        return (state == STATE_SEAATTACK_LOADING || state == STATE_SEAATTACK_UNLOADING || state == STATE_SEAATTACK_DRIVINGTODESTINATION
                || state == STATE_SEAATTACK_WAITING || state == STATE_SEAATTACK_RETURN_DRIVING);
    }
    bool IsLoading() const;
    bool IsUnloading() const;
    /// Gibt Liste der Waren an Bord zurück
    const std::list<Ware*>& GetWares() const { return wares; }
    /// Gibt Liste der Menschen an Bord zurück
    const std::list<noFigure*>& GetFigures() const { return figures; }
    /// Gibt Sichtradius dieses Schiffes zurück
    unsigned GetVisualRange() const;

    /// Return the harbor ID where the ship currently is. Only valid when waiting for expedition instructions
    unsigned GetCurrentHarbor() const;
    /// Return the harbor the ship is currently targeting (0 if ship is idling)
    unsigned GetTargetHarbor() const;
    /// Return the source harbor from where the ship left the current mission (0 if ship is idling)
    unsigned GetHomeHarbor() const;

    /// Fährt zum Hafen, um dort eine Mission (Expedition) zu erledigen
    void GoToHarbor(const nobHarborBuilding& hb, const std::vector<Direction>& route);
    /// Startet eine Expedition
    void StartStopExpedition(unsigned homeHarborId);
    /// Startet eine Erkundungs-Expedition
    void StartStopExplorationExpedition(unsigned homeHarborId);
    /// Weist das Schiff an, in einer bestimmten Richtung die Expedition fortzusetzen
    void ContinueExpedition(const ShipDirection dir);
    /// Weist das Schiff an, eine Expedition abzubrechen (nur wenn es steht) und zum
    /// Hafen zurückzukehren
    void CancelExpedition();
    /// Weist das Schiff an, seine Erkundungs-Expedition fortzusetzen
    void ContinueExplorationExpedition();
    /// Gibt zurück, ob das Schiff jetzt in der Lage wäre, eine Kolonie zu gründen
    bool IsAbleToFoundColony() const;
    /// Weist das Schiff an, an der aktuellen Position einen Hafen zu gründen
    void FoundColony();
    /// Return true, if the ship is driving to this harbor as its final destination (free for new orders)
    bool IsGoingToHarbor(const nobHarborBuilding& hb) const;

    /// Belädt das Schiff mit Waren und Figuren, um eine Transportfahrt zu starten
    void PrepareTransport(unsigned homeHarborId, MapPoint goal, const std::list<noFigure*>& figures, const std::list<Ware*>& wares);

    /// Belädt das Schiff mit Schiffs-Angreifern
    void PrepareSeaAttack(unsigned homeHarborId, MapPoint goal, const std::list<noFigure*>& figures);
    /// Sagt Bescheid, dass ein Schiffsangreifer nicht mehr mit nach Hause fahren will
    void SeaAttackerWishesNoReturn();
    /// Schiffs-Angreifer sind nach dem Angriff wieder zurückgekehrt
    void AddReturnedAttacker(nofAttacker* attacker);

    /// Sagt dem Schiff, das ein bestimmter Hafen zerstört wurde
    void HarborDestroyed(nobHarborBuilding* hb);
    /// Sagt dem Schiff, dass ein neuer Hafen erbaut wurde
    void NewHarborBuilt(nobHarborBuilding* hb);
};

#endif
