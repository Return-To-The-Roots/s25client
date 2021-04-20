// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/PtrSpan.h"
#include "noMovable.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/ShipDirection.h"
#include <list>
#include <memory>
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
    enum class State : uint8_t
    {
        Idle, /// Schiff hat nix zu tun und hängt irgendwo an der Küste rum
        Gotoharbor,
        ExpeditionLoading,
        ExpeditionUnloading,
        ExpeditionWaiting,
        ExpeditionDriving,
        ExplorationexpeditionLoading,
        ExplorationexpeditionUnloading,
        ExplorationexpeditionWaiting,
        ExplorationexpeditionDriving,
        TransportLoading,   // Schiff wird mit Waren/Figuren erst noch beladen, bleibt also für kurze Zeit am Hafen
        TransportDriving,   /// Schiff transportiert Waren/Figuren von einen Ort zum anderen
        TransportUnloading, /// Entlädt Schiff am Zielhafen, kurze Zeit ankern, bevor Waren im Hafengebäude
                            /// ankommen..
        SeaattackLoading,
        SeaattackUnloading,
        SeaattackDrivingToDestination, /// Fährt mit den Soldaten zum Zielhafenpunkt
        SeaattackWaiting,              /// wartet an der Küste, während die Soldaten was schönes machen
        SeaattackReturnDriving         /// fährt mit den Soldaten wieder zurück zum Heimathafen
    } state;
    friend constexpr auto maxEnumValue(State) { return State::SeaattackReturnDriving; }

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
    std::list<std::unique_ptr<noFigure>> figures;
    std::list<std::unique_ptr<Ware>> wares;
    /// Gibt an, ob das Schiff verlassen auf dem Meer auf einen Anlegeplatz wartet,
    /// um sein Zeug auszuladen
    bool lost;
    /// Bei Schiffen im SeaattackWaiting:
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
    void StartDriving(Direction dir);

    void HandleState_GoToHarbor();
    void HandleState_ExpeditionDriving();
    void HandleState_ExplorationExpeditionDriving();
    void HandleState_TransportDriving();
    void HandleState_SeaAttackDriving();
    void HandleState_SeaAttackReturn();

    enum class Result
    {
        Driving,
        GoalReached,
        NoRouteFound,
        HarborDoesntExist
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
    noShip(MapPoint pos, unsigned char player);
    noShip(SerializedGameData& sgd, unsigned obj_id);

    ~noShip() override;

    void Serialize(SerializedGameData& sgd) const override;
    void Destroy() override;

    GO_Type GetGOT() const final { return GO_Type::Ship; }

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
    bool IsIdling() const { return (state == State::Idle); }
    bool IsLost() const { return lost; }
    /// Führt das Schiff gerade eine Expedition durch und wartet auf weitere Befehle?
    bool IsWaitingForExpeditionInstructions() const { return (state == State::ExpeditionWaiting); }
    /// Ist das Schiff gerade irgendwie am Expeditionieren und hat entsprechenden Kram an Bord?
    bool IsOnExpedition() const
    {
        return (state == State::ExpeditionLoading || state == State::ExpeditionWaiting
                || state == State::ExpeditionDriving);
    }
    /// Ist das Schiff gerade irgendwie am Explorations-Expeditionieren und hat entsprechenden Kram an Bord?
    bool IsOnExplorationExpedition() const
    {
        return (state == State::ExplorationexpeditionLoading || state == State::ExplorationexpeditionUnloading
                || state == State::ExplorationexpeditionWaiting || state == State::ExplorationexpeditionDriving);
    }
    bool IsOnAttackMission() const
    {
        return (state == State::SeaattackLoading || state == State::SeaattackUnloading
                || state == State::SeaattackDrivingToDestination || state == State::SeaattackWaiting
                || state == State::SeaattackReturnDriving);
    }
    bool IsLoading() const;
    bool IsUnloading() const;
    /// Gibt Liste der Waren an Bord zurück
    auto GetWares() const { return helpers::nonNullPtrSpan(wares); }
    /// Gibt Liste der Menschen an Bord zurück
    auto GetFigures() const { return helpers::nonNullPtrSpan(figures); }
    bool IsOnBoard(const noFigure& figure) const;
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
    void StartExpedition(unsigned homeHarborId);
    /// Startet eine Erkundungs-Expedition
    void StartExplorationExpedition(unsigned homeHarborId);
    /// Weist das Schiff an, in einer bestimmten Richtung die Expedition fortzusetzen
    void ContinueExpedition(ShipDirection dir);
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
    void PrepareTransport(unsigned homeHarborId, MapPoint goal, std::list<std::unique_ptr<noFigure>> figures,
                          std::list<std::unique_ptr<Ware>> wares);

    /// Belädt das Schiff mit Schiffs-Angreifern
    void PrepareSeaAttack(unsigned homeHarborId, MapPoint goal, std::vector<std::unique_ptr<nofAttacker>> attackers);
    /// Sagt Bescheid, dass ein Schiffsangreifer nicht mehr mit nach Hause fahren will
    void SeaAttackerWishesNoReturn();
    /// Schiffs-Angreifer sind nach dem Angriff wieder zurückgekehrt
    void AddReturnedAttacker(std::unique_ptr<nofAttacker> attacker);

    /// Sagt dem Schiff, das ein bestimmter Hafen zerstört wurde
    void HarborDestroyed(nobHarborBuilding* hb);
    /// Sagt dem Schiff, dass ein neuer Hafen erbaut wurde
    void NewHarborBuilt(nobHarborBuilding* hb);
};
