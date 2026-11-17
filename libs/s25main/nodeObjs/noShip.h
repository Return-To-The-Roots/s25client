// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/PtrSpan.h"
#include "noMovable.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/MapTypes.h"
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
    SeaId seaId_;
    /// Zielpunkt des Schiffes
    HarborId goalHarbor;
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
    HarborId homeHarbor;
    /// Anzahl an Strecke, die das Schiff schon seit Expeditionsstart zurückgelegt hat
    unsigned covered_distance;

private:
    /// entscheidet, was nach einem gefahrenen Abschnitt weiter zu tun ist
    void driven();
    /// Fängt an zu fahren
    void startDriving(Direction dir);

    void handleStateGoToHarbor();
    void handleStateExpeditionDriving();
    void handleStateExplorationExpeditionDriving();
    void handleStateTransportDriving();
    void handleStateSeaAttackDriving();
    void handleStateSeaAttackReturn();

    enum class Result
    {
        Driving,
        GoalReached,
        NoRouteFound,
        HarborDoesntExist
    };

    /// Fährt weiter zu einem Hafen
    Result driveToHarbour();
    /// Fährt weiter zu Hafenbauplatz
    Result driveToHarbourPlace();

    /// Zeichnet das Schiff stehend mit oder ohne Waren
    void drawFixed(DrawPoint drawPt, bool drawWares);
    /// Zeichnet normales Fahren auf dem Meer ohne irgendwelche Güter
    void drawDriving(DrawPoint& drawPt);
    /// Zeichnet normales Fahren auf dem Meer mit Gütern
    void drawDrivingWithWares(DrawPoint& drawPt);

    /// Startet die eigentliche Transportaktion, nachdem das Schiff beladen wurde
    void startTransport();

    /// Startet Schiffs-Angreiff
    void startSeaAttack();

    /// Fängt an mit idlen und setzt nötigen Sachen auf nullptr
    void startIdling();

    /// Fängt an zu einem Hafen zu fahren (berechnet Route usw.)
    void startDrivingToHarborPlace();

    /// Looks for a harbour to unload the goods (e.g if old one was destroyed)
    void findUnloadGoal(State newState);
    /// Aborts a sea attack (in case harbor was not found anymore)
    void abortSeaAttack();

public:
    noShip(MapPoint pos, unsigned char player);
    noShip(SerializedGameData& sgd, unsigned obj_id);

    ~noShip() override;

    void serialize(SerializedGameData& sgd) const override;
    void destroy() override;

    GO_Type getGOT() const final { return GO_Type::Ship; }

    // An x,y zeichnen
    void draw(DrawPoint drawPt) override;
    // Benachrichtigen, wenn neuer gf erreicht wurde
    void handleEvent(unsigned id) override;

    /// Gibt den Besitzer zurück
    unsigned char getPlayerId() const { return ownerId_; }
    /// Gibt die ID des Meeres zurück, auf dem es sich befindet
    SeaId getSeaID() const { return seaId_; }
    /// Gibt den Schiffsnamen zurück
    const std::string& getName() const { return name; }
    /// Hat das Schiff gerade nichts zu tun
    bool isIdling() const { return (state == State::Idle); }
    bool isLost() const { return lost; }
    /// Führt das Schiff gerade eine Expedition durch und wartet auf weitere Befehle?
    bool isWaitingForExpeditionInstructions() const { return (state == State::ExpeditionWaiting); }
    /// Ist das Schiff gerade irgendwie am Expeditionieren und hat entsprechenden Kram an Bord?
    bool isOnExpedition() const
    {
        return (state == State::ExpeditionLoading || state == State::ExpeditionWaiting
                || state == State::ExpeditionDriving);
    }
    /// Ist das Schiff gerade irgendwie am Explorations-Expeditionieren und hat entsprechenden Kram an Bord?
    bool isOnExplorationExpedition() const
    {
        return (state == State::ExplorationexpeditionLoading || state == State::ExplorationexpeditionUnloading
                || state == State::ExplorationexpeditionWaiting || state == State::ExplorationexpeditionDriving);
    }
    bool isOnAttackMission() const
    {
        return (state == State::SeaattackLoading || state == State::SeaattackUnloading
                || state == State::SeaattackDrivingToDestination || state == State::SeaattackWaiting
                || state == State::SeaattackReturnDriving);
    }
    bool isLoading() const;
    bool isUnloading() const;
    /// Gibt Liste der Waren an Bord zurück
    auto getWares() const { return helpers::nonNullPtrSpan(wares); }
    /// Gibt Liste der Menschen an Bord zurück
    auto getFigures() const { return helpers::nonNullPtrSpan(figures); }
    bool isOnBoard(const noFigure& figure) const;
    /// Gibt Sichtradius dieses Schiffes zurück
    unsigned getVisualRange() const;

    /// Return the harbor ID where the ship currently is. Only valid when waiting for expedition instructions
    HarborId getCurrentHarbor() const;
    /// Return the harbor the ship is currently targeting (0 if ship is idling)
    HarborId getTargetHarbor() const;
    /// Return the source harbor from where the ship left the current mission (0 if ship is idling)
    HarborId getHomeHarbor() const;

    /// Fährt zum Hafen, um dort eine Mission (Expedition) zu erledigen
    void goToHarbor(const nobHarborBuilding& hb, const std::vector<Direction>& route);
    /// Startet eine Expedition
    void startExpedition(HarborId homeHarborId);
    /// Startet eine Erkundungs-Expedition
    void startExplorationExpedition(HarborId homeHarborId);
    /// Weist das Schiff an, in einer bestimmten Richtung die Expedition fortzusetzen
    void continueExpedition(ShipDirection dir);
    /// Weist das Schiff an, eine Expedition abzubrechen (nur wenn es steht) und zum
    /// Hafen zurückzukehren
    void CancelExpedition();
    /// Weist das Schiff an, seine Erkundungs-Expedition fortzusetzen
    void continueExplorationExpedition();
    /// Gibt zurück, ob das Schiff jetzt in der Lage wäre, eine Kolonie zu gründen
    bool isAbleToFoundColony() const;
    /// Weist das Schiff an, an der aktuellen Position einen Hafen zu gründen
    void foundColony();
    /// Return true, if the ship is driving to this harbor as its final destination (free for new orders)
    bool isGoingToHarbor(const nobHarborBuilding& hb) const;

    /// Belädt das Schiff mit Waren und Figuren, um eine Transportfahrt zu starten
    void prepareTransport(HarborId homeHarborId, MapPoint goal, std::list<std::unique_ptr<noFigure>> figures,
                          std::list<std::unique_ptr<Ware>> wares);

    /// Belädt das Schiff mit Schiffs-Angreifern
    void prepareSeaAttack(HarborId homeHarborId, MapPoint goal, std::vector<std::unique_ptr<nofAttacker>> attackers);
    /// Sagt Bescheid, dass ein Schiffsangreifer nicht mehr mit nach Hause fahren will
    void seaAttackerWishesNoReturn();
    /// Schiffs-Angreifer sind nach dem Angriff wieder zurückgekehrt
    void addReturnedAttacker(std::unique_ptr<nofAttacker> attacker);

    /// Sagt dem Schiff, das ein bestimmter Hafen zerstört wurde
    void harborDestroyed(nobHarborBuilding* hb);
    /// Sagt dem Schiff, dass ein neuer Hafen erbaut wurde
    void newHarborBuilt(nobHarborBuilding* hb);
};
