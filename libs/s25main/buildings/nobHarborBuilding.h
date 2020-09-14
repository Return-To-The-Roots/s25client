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

#pragma once

#include "helpers/EnumArray.h"
#include "nobBaseWarehouse.h"
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
    helpers::EnumArray<unsigned short, Direction> seaIds;
    /// Liste von Waren, die weggeschifft werden sollen
    std::list<Ware*> wares_for_ships;
    /// Liste von Menschen, die weggeschifft werden sollen
    struct FigureForShip
    {
        noFigure* fig;
        MapPoint dest;
    };
    std::list<FigureForShip> figures_for_ships;

    /// Liste von angreifenden Soldaten, die verschifft werden sollen
    struct SoldierForShip
    {
        nofAttacker* attacker;
        MapPoint dest;
    };
    std::list<SoldierForShip> soldiers_for_ships;

private:
    /// Bestellt die zusätzlichen erforderlichen Waren für eine Expedition
    void OrderExpeditionWares();
    /// Prüft, ob eine Expedition von den Waren her vollständig ist und ruft ggf. das Schiff
    void CheckExpeditionReady();
    /// Prüft, ob eine Expedition von den Spähern her vollständig ist und ruft ggf. das Schiff
    void CheckExplorationExpeditionReady();
    /// Gibt zurück, ob Expedition vollständig ist
    bool IsExpeditionReady() const;
    /// Gibt zurück, ob Erkundungs-Expedition vollständig ist
    bool IsExplorationExpeditionReady() const;
    /// Abgeleitete kann eine gerade erzeugte Ware ggf. sofort verwenden
    /// (muss in dem Fall true zurückgeben)
    bool UseWareAtOnce(Ware* ware, noBaseBuilding& goal) override;
    /// Dasselbe für Menschen
    bool UseFigureAtOnce(noFigure* fig, noRoadNode& goal) override;
    /// Bestellte Figur, die sich noch inder Warteschlange befindet, kommt nicht mehr und will rausgehauen werden
    void CancelFigure(noFigure* figure) override;
    /// Bestellt ein Schiff zum Hafen, sofern dies nötig ist
    void OrderShip();

    /// Stellt Verteidiger zur Verfügung
    nofDefender* ProvideDefender(nofAttacker* attacker) override;

    friend class SerializedGameData;
    friend class BuildingFactory;
    nobHarborBuilding(MapPoint pos, unsigned char player, Nation nation);
    nobHarborBuilding(SerializedGameData& sgd, unsigned obj_id);

protected:
    void DestroyBuilding() override;

public:
    unsigned GetMilitaryRadius() const override { return HARBOR_RADIUS; }

    /// Serialisierung
    void Serialize(SerializedGameData& sgd) const override;
    GO_Type GetGOT() const override { return GOT_NOB_HARBORBUILDING; }
    void Draw(DrawPoint drawPt) override;
    void HandleEvent(unsigned id) override;

    /// Eine bestellte Ware konnte doch nicht kommen
    void WareLost(Ware* ware) override;
    /// Legt eine Ware im Lagerhaus ab
    void AddWare(Ware*& ware) override;
    /// Eine Figur geht ins Lagerhaus
    void AddFigure(noFigure* figure, bool increase_visual_counts) override;
    /// Berechnet Wichtigkeit einer neuen Ware für den Hafen (Waren werden für Expeditionen
    /// benötigt!)
    unsigned CalcDistributionPoints(GoodType type) const;

    /// Storniert die Bestellung für eine bestimmte Ware, die mit einem Schiff transportiert werden soll
    void CancelWareForShip(Ware* ware);

    /// Startet eine Expedition
    void StartExpedition();
    void StopExpedition();

    /// Startet eine Erkundungs-Expedition
    void StartExplorationExpedition();
    void StopExplorationExpedition();

    /// Ist Expedition in Vorbereitung?
    bool IsExpeditionActive() const { return expedition.active; }
    /// Ist Erkundungs-Expedition in Vorbereitung?
    bool IsExplorationExpeditionActive() const { return exploration_expedition.active; }
    /// Schiff ist angekommen
    void ShipArrived(noShip* ship);
    /// Schiff konnte nicht mehr kommen
    void ShipLost(noShip* ship);

    /// Abfangen, wenn ein Mann nicht mehr kommen kann --> könnte ein Bauarbeiter sein und
    /// wenn wir einen benötigen, müssen wir einen neuen bestellen
    void RemoveDependentFigure(noFigure* figure) override;

    /// Gibt die Hafenplatz-ID zurück, auf der der Hafen steht
    unsigned GetHarborPosID() const;

    struct ShipConnection
    {
        /// Zielhafen
        noRoadNode* dest;
        /// Kosten für die Strecke in Weglänge eines einfachen Trägers
        unsigned way_costs;
    };
    /// Gibt eine Liste mit möglichen Verbindungen zurück
    std::vector<ShipConnection> GetShipConnections() const;

    /// Fügt einen Mensch hinzu, der mit dem Schiff irgendwo hin fahren will
    void AddFigureForShip(noFigure* fig, MapPoint dest);
    /// Fügt eine Ware hinzu, die mit dem Schiff verschickt werden soll
    void AddWareForShip(Ware*& ware);

    /// A ware changed its route and doesn't want to use the ship anymore
    void WareDontWantToTravelByShip(Ware* ware);

    /// Gibt Anzahl der Schiffe zurück, die noch für ausstehende Aufgaben benötigt werden
    unsigned GetNumNeededShips() const;
    /// Gibt die Wichtigkeit an, dass ein Schiff kommen muss (0 -> keine Bedürftigkeit)
    int GetNeedForShip(unsigned ships_coming) const;

    /// Erhält die Waren von einem Schiff und nimmt diese in den Warenbestand auf
    void ReceiveGoodsFromShip(std::list<noFigure*>& figures, std::list<Ware*>& wares);

    nofAggressiveDefender* SendAggressiveDefender(nofAttacker* attacker) override;

    struct SeaAttackerBuilding
    {
        /// Comparator that compares only the building pointer
        struct CmpBuilding
        {
            const nobMilitary* const search;
            CmpBuilding(const nobMilitary* const search) : search(search) {}
            bool operator()(const SeaAttackerBuilding& other) { return other.building == search; }
        };
        /// Das Gebäude selbst
        nobMilitary* building;
        // Dazugehöriger Hafen, wo die Angreifer dann auf das Schiff warten sollen
        nobHarborBuilding* harbor;
        /// Entfernung Hafen - anderer Hafen
        unsigned distance;

        bool operator==(const nobMilitary* const building) const { return (this->building == building); };
    };

    /// Gibt die Angreifer zurück, die dieser Hafen für einen Seeangriff zur Verfügung stellen kann
    /// defender_harbors sind dabei mögliche Zielhäfen
    std::vector<SeaAttackerBuilding> GetAttackerBuildingsForSeaAttack(const std::vector<unsigned>& defender_harbors);
    /// Gibt verfügbare Angreifer zurück
    std::vector<SeaAttackerBuilding> GetAttackerBuildingsForSeaIdAttack();

    /// Fügt einen Schiffs-Angreifer zum Hafen hinzu
    void AddSeaAttacker(nofAttacker* attacker);
    /// Attacker does not want to attack anymore
    void CancelSeaAttacker(nofAttacker* attacker);

    /// People waiting for a ship have to examine their route if a road was destroyed
    void ExamineShipRouteOfPeople();

    /// Is the harbor just being destroyed right now?
    bool IsBeingDestroyedNow() const;
};
