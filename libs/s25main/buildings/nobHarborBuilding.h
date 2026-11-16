// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/EnumArray.h"
#include "nobBaseWarehouse.h"
#include "gameTypes/MapTypes.h"
#include "gameData/MilitaryConsts.h"
#include <list>

class noShip;
class SerializedGameData;
class Ware;
class noBaseBuilding;
class noFigure;
class noRoadNode;
class nobMilitary;
class nofAttacker;
class nofDefender;
class GameEvent;

class nobHarborBuilding : public nobBaseWarehouse
{
    struct ExpeditionInfo
    {
        ExpeditionInfo() : boards(0), stones(0), active(false), builder(false) {}
        ExpeditionInfo(SerializedGameData& sgd);
        void Serialize(SerializedGameData& sgd) const;

        /// Anzahl an Brettern und Steinen, die bereits angesammelt wurden
        unsigned boards, stones;
        /// Expedition in Vorbereitung?
        bool active;
        /// Bauarbeiter schon da?
        bool builder;
    } expedition;

    struct ExplorationExpeditionInfo
    {
        ExplorationExpeditionInfo() : active(false), scouts(0) {}
        ExplorationExpeditionInfo(SerializedGameData& sgd);
        void Serialize(SerializedGameData& sgd) const;

        /// Expedition in Vorbereitung?
        bool active;
        /// Anzahl der Erkunder, die schon da sind
        unsigned scouts;
    } exploration_expedition;

    /// Bestell-Ware-Event
    const GameEvent* orderware_ev;
    /// Die Meeres-IDs aller angrenzenden Meere (jeweils für die 6 drumherumliegenden Küstenpunkte)
    helpers::EnumArray<SeaId, Direction> seaIds;
    /// Liste von Waren, die weggeschifft werden sollen
    std::list<std::unique_ptr<Ware>> wares_for_ships;
    /// Liste von Menschen, die weggeschifft werden sollen
    struct FigureForShip
    {
        std::unique_ptr<noFigure> fig;
        MapPoint dest;
    };
    std::list<FigureForShip> figures_for_ships;

    /// Liste von angreifenden Soldaten, die verschifft werden sollen
    struct SoldierForShip
    {
        std::unique_ptr<nofAttacker> attacker;
        MapPoint dest;
    };
    std::list<SoldierForShip> soldiers_for_ships;

private:
    /// Bestellt die zusätzlichen erforderlichen Waren für eine Expedition
    void orderExpeditionWares();
    /// Prüft, ob eine Expedition von den Waren her vollständig ist und ruft ggf. das Schiff
    void checkExpeditionReady();
    /// Prüft, ob eine Expedition von den Spähern her vollständig ist und ruft ggf. das Schiff
    void checkExplorationExpeditionReady();
    /// Gibt zurück, ob Expedition vollständig ist
    bool isExpeditionReady() const;
    /// Gibt zurück, ob Erkundungs-Expedition vollständig ist
    bool isExplorationExpeditionReady() const;
    /// Abgeleitete kann eine gerade erzeugte Ware ggf. sofort verwenden
    /// (muss in dem Fall true zurückgeben)
    bool useWareAtOnce(std::unique_ptr<Ware>& ware, noBaseBuilding& goal) override;
    /// Dasselbe für Menschen
    bool useFigureAtOnce(std::unique_ptr<noFigure>& fig, noRoadNode& goal) override;
    /// Bestellte Figur, die sich noch inder Warteschlange befindet, kommt nicht mehr und will rausgehauen werden
    void cancelFigure(noFigure* figure) override;
    /// Bestellt ein Schiff zum Hafen, sofern dies nötig ist
    void orderShip();

    /// Stellt Verteidiger zur Verfügung
    std::unique_ptr<nofDefender> provideDefender(nofAttacker& attacker) override;

    friend class SerializedGameData;
    friend class BuildingFactory;
    nobHarborBuilding(MapPoint pos, unsigned char player, Nation nation);
    nobHarborBuilding(SerializedGameData& sgd, unsigned obj_id);

protected:
    void DestroyBuilding() override;

public:
    unsigned getMilitaryRadius() const override { return HARBOR_RADIUS; }

    /// Serialisierung
    void Serialize(SerializedGameData& sgd) const override;
    GO_Type getGOT() const final { return GO_Type::NobHarborbuilding; }
    void draw(DrawPoint drawPt) override;
    void handleEvent(unsigned id) override;

    /// Eine bestellte Ware konnte doch nicht kommen
    void wareLost(Ware& ware) override;
    /// Legt eine Ware im Lagerhaus ab
    void addWare(std::unique_ptr<Ware> ware) override;
    /// Eine Figur geht ins Lagerhaus
    void addFigure(std::unique_ptr<noFigure> figure, bool increase_visual_counts) override;
    /// Berechnet Wichtigkeit einer neuen Ware für den Hafen (Waren werden für Expeditionen
    /// benötigt!)
    unsigned calcDistributionPoints(GoodType type) const;

    /// Storniert die Bestellung für eine bestimmte Ware, die mit einem Schiff transportiert werden soll
    std::unique_ptr<Ware> cancelWareForShip(Ware* ware);

    /// Startet eine Expedition
    void startExpedition();
    void stopExpedition();

    /// Startet eine Erkundungs-Expedition
    void startExplorationExpedition();
    void stopExplorationExpedition();

    /// Ist Expedition in Vorbereitung?
    bool isExpeditionActive() const { return expedition.active; }
    /// Ist Erkundungs-Expedition in Vorbereitung?
    bool isExplorationExpeditionActive() const { return exploration_expedition.active; }
    /// Schiff ist angekommen
    void shipArrived(noShip& ship);
    /// Schiff konnte nicht mehr kommen
    void shipLost(noShip* ship);

    /// Abfangen, wenn ein Mann nicht mehr kommen kann --> könnte ein Bauarbeiter sein und
    /// wenn wir einen benötigen, müssen wir einen neuen bestellen
    void removeDependentFigure(noFigure& figure) override;

    /// Gibt die Hafenplatz-ID zurück, auf der der Hafen steht
    HarborId getHarborPosID() const;

    struct ShipConnection
    {
        /// Zielhafen
        noRoadNode* dest;
        /// Kosten für die Strecke in Weglänge eines einfachen Trägers
        unsigned way_costs;
    };
    /// Gibt eine Liste mit möglichen Verbindungen zurück
    std::vector<ShipConnection> getShipConnections() const;

    /// Fügt einen Mensch hinzu, der mit dem Schiff irgendwo hin fahren will
    void addFigureForShip(std::unique_ptr<noFigure> fig, MapPoint dest);
    /// Fügt eine Ware hinzu, die mit dem Schiff verschickt werden soll
    void addWareForShip(std::unique_ptr<Ware> ware);

    /// A ware changed its route and doesn't want to use the ship anymore
    void wareDontWantToTravelByShip(Ware* ware);

    /// Gibt Anzahl der Schiffe zurück, die noch für ausstehende Aufgaben benötigt werden
    unsigned getNumNeededShips() const;
    /// Gibt die Wichtigkeit an, dass ein Schiff kommen muss (0 -> keine Bedürftigkeit)
    int getNeedForShip(unsigned ships_coming) const;

    /// Erhält die Waren von einem Schiff und nimmt diese in den Warenbestand auf
    void receiveGoodsFromShip(std::list<std::unique_ptr<noFigure>>& figures, std::list<std::unique_ptr<Ware>>& wares);

    nofAggressiveDefender* sendAggressiveDefender(nofAttacker& attacker) override;

    struct SeaAttackerBuilding
    {
        /// Comparator that compares only the building pointer
        struct CmpBuilding
        {
            const nobMilitary* const search;
            CmpBuilding(const nobMilitary* const search) : search(search) {}
            bool operator()(const SeaAttackerBuilding& other) const noexcept { return other.building == search; }
        };
        /// Das Gebäude selbst
        nobMilitary* building;
        // Dazugehöriger Hafen, wo die Angreifer dann auf das Schiff warten sollen
        nobHarborBuilding* harbor;
        /// Entfernung Hafen - anderer Hafen
        unsigned distance;

        bool operator==(const nobMilitary* const building) const noexcept { return (this->building == building); };
    };

    /// Gibt die Angreifer zurück, die dieser Hafen für einen Seeangriff zur Verfügung stellen kann
    /// defender_harbors sind dabei mögliche Zielhäfen
    std::vector<SeaAttackerBuilding> getAttackerBuildingsForSeaAttack(const std::vector<HarborId>& defender_harbors);
    /// Gibt verfügbare Angreifer zurück
    std::vector<SeaAttackerBuilding> getAttackerBuildingsForSeaIdAttack();

    /// Fügt einen Schiffs-Angreifer zum Hafen hinzu
    void addSeaAttacker(std::unique_ptr<nofAttacker> attacker);
    /// Attacker does not want to attack anymore
    void cancelSeaAttacker(nofAttacker* attacker);

    /// People waiting for a ship have to examine their route if a road was destroyed
    void examineShipRouteOfPeople();

    /// Is the harbor just being destroyed right now?
    bool isBeingDestroyedNow() const;
};
