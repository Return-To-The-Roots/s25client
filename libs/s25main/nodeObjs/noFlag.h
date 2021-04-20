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

    void Destroy() override;
    void Serialize(SerializedGameData& sgd) const override;

    inline GO_Type GetGOT() const final { return GO_Type::Flag; }
    inline FlagType GetFlagType() const { return flagtype; }
    /// Gibt Auskunft darüber, ob noch Platz für eine Ware an der Flagge ist.
    inline bool HasSpaceForWare() const { return wares.size() < wares.max_size(); }

    void Draw(DrawPoint drawPt) override;

    BlockingManner GetBM() const override { return BlockingManner::Flag; }

    /// Erzeugt von ihnen selbst ein FOW Objekt als visuelle "Erinnerung" für den Fog of War.
    std::unique_ptr<FOWObject> CreateFOWObject() const override;
    /// Legt eine Ware an der Flagge ab.
    void AddWare(std::unique_ptr<Ware> ware) override;
    /// Gibt die Anzahl der Waren zurück, die an der Flagge liegen.
    unsigned GetNumWares() const { return wares.size(); }
    /// Wählt eine Ware von einer Flagge aus (anhand der Transportreihenfolge), entfernt sie von der Flagge und gibt sie
    /// zurück.
    std::unique_ptr<Ware> SelectWare(Direction roadDir, bool swap_wares, const noFigure* carrier);
    /// Prüft, ob es Waren gibt, die auf den Weg in Richtung dir getragen werden müssen.
    unsigned GetNumWaresForRoad(Direction dir) const;
    /// Gibt Wegstrafpunkte für das Pathfinden für Waren, die in eine bestimmte Richtung noch transportiert werden
    /// müssen.
    unsigned GetPunishmentPoints(Direction dir) const override;
    /// Zerstört evtl. vorhandenes Gebäude bzw. Baustelle vor der Flagge.
    void DestroyAttachedBuilding();
    /// Baut normale Flaggen zu "gloriösen" aus bei Eselstraßen.
    void Upgrade();
    /// Feind übernimmt die Flagge.
    void Capture(unsigned char new_owner);
    /// Ist diese Flagge für eine bestimmte Lagerhausflüchtlingsgruppe (BWU) nicht zugänglich?
    bool IsImpossibleForBWU(unsigned bwu_id) const;
    /// Hinzufügen, dass diese Flagge für eine bestimmte Lagerhausgruppe nicht zugänglich ist.
    void ImpossibleForBWU(unsigned bwu_id);

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
