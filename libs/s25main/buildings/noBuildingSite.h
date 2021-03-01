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

#include "noBaseBuilding.h"
#include "gameTypes/GoodTypes.h"
#include <cstdint>
#include <list>

class nofBuilder;
class nofPlaner;
class FOWObject;
class SerializedGameData;
class Ware;
class noFigure;
class noRoadNode;

/// repräsentiert eine Baustelle
class noBuildingSite : public noBaseBuilding
{
    enum class BuildingSiteState : uint8_t
    {
        Planing,
        Building
    };
    friend constexpr auto maxEnumValue(BuildingSiteState) { return noBuildingSite::BuildingSiteState::Building; }
    friend class nofBuilder;

    /// Typ/Status der Baustelle
    BuildingSiteState state;
    /// Planierer
    nofPlaner* planer;
    /// Bauarbeiter, der an dieser Baustelle arbeitet
    nofBuilder* builder;
    /// Bretter und Steine, die hier liegen
    unsigned char boards, stones;
    /// Bretter und Steine, die schon verbaut wurden
    unsigned char used_boards, used_stones;
    /// Gibt den Baufortschritt an, wie hoch das Gebäude schon gebaut ist, gemessen in 8 Stufen für jede verbaute Ware
    unsigned char build_progress;
    /// Bestellte Bretter und Steine, d.h. Steine/Bretter, die noch "bestellt" wurden, aber noch nicht da sind
    std::list<Ware*> ordered_boards, ordered_stones;

public:
    unsigned char getUsedBoards() const { return used_boards; }
    unsigned char getUsedStones() const { return used_stones; }
    unsigned char getBoards() const { return boards; }
    unsigned char getStones() const { return stones; }

    noBuildingSite(BuildingType type, MapPoint pos, unsigned char player);
    /// Konstruktor für Hafenbaustellen vom Schiff aus
    noBuildingSite(MapPoint pos, unsigned char player);
    noBuildingSite(SerializedGameData& sgd, unsigned obj_id);

    ~noBuildingSite() override;

    void Destroy() override;
    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::Buildingsite; }
    unsigned GetMilitaryRadius() const override;

    void Draw(DrawPoint drawPt) override;

    /// Erzeugt von ihnen selbst ein FOW Objekt als visuelle "Erinnerung" für den Fog of War
    std::unique_ptr<FOWObject> CreateFOWObject() const override;

    void AddWare(Ware*& ware) override;
    void GotWorker(Job job, noFigure* worker) override;

    /// Fordert Baumaterial an
    void OrderConstructionMaterial();
    /// Wird aufgerufen, wenn der Bauarbeiter kündigt
    void Abrogate();
    /// Eine bestellte Ware konnte doch nicht kommen
    void WareLost(Ware* ware) override;
    /// Gibt den Bau-Fortschritt zurück
    unsigned char GetBuildProgress(bool percent = true) const;

    unsigned CalcDistributionPoints(noRoadNode* start, GoodType goodtype);

    /// Wird aufgerufen, wenn eine neue Ware zum dem Gebäude geliefert wird (nicht wenn sie bestellt wurde vom Gebäude!)
    void TakeWare(Ware* ware) override;
    /// Gibt zurück, ob die Baustelle fertiggestellt ist
    bool IsBuildingComplete();

    /// Aufgerufen, wenn Planierung beendet wurde
    void PlaningFinished();
    /// Gibt zurück, ob eine bestimmte Baustellen eine Baustelle ist, die vom Schiff aus errichtet wurde
    bool IsHarborBuildingSiteFromSea() const;
};
