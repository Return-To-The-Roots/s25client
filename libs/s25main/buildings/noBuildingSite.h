// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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

    void destroy() override;
    void serialize(SerializedGameData& sgd) const override;

    GO_Type getGOT() const final { return GO_Type::Buildingsite; }
    unsigned getMilitaryRadius() const override;

    void draw(DrawPoint drawPt) override;

    /// Erzeugt von ihnen selbst ein FOW Objekt als visuelle "Erinnerung" für den Fog of War
    std::unique_ptr<FOWObject> createFOWObject() const override;

    void addWare(std::unique_ptr<Ware> ware) override;
    void gotWorker(Job job, noFigure& worker) override;

    /// Fordert Baumaterial an
    void orderConstructionMaterial();
    /// Wird aufgerufen, wenn der Bauarbeiter kündigt
    void abrogate();
    /// Eine bestellte Ware konnte doch nicht kommen
    void wareLost(Ware& ware) override;
    /// Gibt den Bau-Fortschritt zurück
    unsigned char getBuildProgress(bool percent = true) const;

    unsigned calcDistributionPoints(GoodType goodtype);

    /// Wird aufgerufen, wenn eine neue Ware zum dem Gebäude geliefert wird (nicht wenn sie bestellt wurde vom Gebäude!)
    void takeWare(Ware* ware) override;
    /// Gibt zurück, ob die Baustelle fertiggestellt ist
    bool isBuildingComplete();

    /// Aufgerufen, wenn Planierung beendet wurde
    void planingFinished();
    /// Gibt zurück, ob eine bestimmte Baustellen eine Baustelle ist, die vom Schiff aus errichtet wurde
    bool isHarborBuildingSiteFromSea() const;
};
