// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "noRoadNode.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/MapTypes.h"
#include <boost/container/static_vector.hpp>
#include <array>
#include <memory>

class FOWObject;
class SerializedGameData;
class Ware;
class noFigure;

class noFlag : public noRoadNode
{
public:
    noFlag(MapPoint pos, unsigned char player);
    noFlag(SerializedGameData& sgd, unsigned obj_id);
    ~noFlag() override;

    void destroy() override;
    void serialize(SerializedGameData& sgd) const override;

    GO_Type getGOT() const final { return GO_Type::Flag; }
    FlagType getFlagType() const { return flagtype; }
    /// Gibt Auskunft darüber, ob noch Platz für eine Ware an der Flagge ist.
    bool hasSpaceForWare() const { return wares.size() < wares.max_size(); }

    void draw(DrawPoint drawPt) override;

    BlockingManner getBM() const override { return BlockingManner::Flag; }

    /// Erzeugt von ihnen selbst ein FOW Objekt als visuelle "Erinnerung" für den Fog of War.
    std::unique_ptr<FOWObject> createFOWObject() const override;
    /// Legt eine Ware an der Flagge ab.
    void addWare(std::unique_ptr<Ware> ware) override;
    /// Gibt die Anzahl der Waren zurück, die an der Flagge liegen.
    unsigned getNumWares() const { return wares.size(); }
    /// Wählt eine Ware von einer Flagge aus (anhand der Transportreihenfolge), entfernt sie von der Flagge und gibt sie
    /// zurück.
    std::unique_ptr<Ware> selectWare(Direction roadDir, bool swap_wares, const noFigure* carrier);
    /// Prüft, ob es Waren gibt, die auf den Weg in Richtung dir getragen werden müssen.
    unsigned getNumWaresForRoad(Direction dir) const;
    /// Penalty for transporting a ware in a specific direction
    unsigned getPunishmentPoints(Direction dir) const override;
    /// Zerstört evtl. vorhandenes Gebäude bzw. Baustelle vor der Flagge.
    void destroyAttachedBuilding();
    /// Baut normale Flaggen zu "gloriösen" aus bei Eselstraßen.
    void upgrade();
    /// Feind übernimmt die Flagge.
    void capture(unsigned char new_owner);
    /// Ist diese Flagge für eine bestimmte Lagerhausflüchtlingsgruppe (BWU) nicht zugänglich?
    bool isImpossibleForBWU(unsigned bwu_id) const;
    /// Hinzufügen, dass diese Flagge für eine bestimmte Lagerhausgruppe nicht zugänglich ist.
    void impossibleForBWU(unsigned bwu_id);

private:
    unsigned short ani_offset;
    FlagType flagtype;

    /// Die Waren, die an dieser Flagge liegen
    boost::container::static_vector<std::unique_ptr<Ware>, 8> wares;

    /// Wieviele BWU-Teile es maximal geben soll, also wieviele abgebrannte Lagerhausgruppen
    /// gleichzeitig die Flagge als nicht begehbar deklarieren können.
    static const unsigned MAX_BWU = 4;

    /// Nicht erreichbar für Massenflüchtlinge
    struct BurnedWarehouseUnit
    {
        unsigned id;      /// ID der Gruppe
        unsigned last_gf; /// letzter TÜV, ob man auch nicht hinkommt, in GF
    };
    std::array<BurnedWarehouseUnit, MAX_BWU> bwus;
};
