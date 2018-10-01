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
#ifndef NO_FLAG_H_INCLUDED
#define NO_FLAG_H_INCLUDED

#include "noRoadNode.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/MapTypes.h"
#include <boost/array.hpp>
class FOWObject;
class SerializedGameData;
class Ware;
class noFigure;

class noFlag : public noRoadNode
{
public:
    noFlag(const MapPoint pt, const unsigned char player, const unsigned char dis_dir = 0xFF);
    noFlag(SerializedGameData& sgd, const unsigned obj_id);
    ~noFlag() override;

    void Destroy() override { Destroy_noFlag(); }
    void Serialize(SerializedGameData& sgd) const override { Serialize_noFlag(sgd); }

    inline GO_Type GetGOT() const override { return GOT_FLAG; }
    inline FlagType GetFlagType() const { return flagtype; }
    /// Gibt Auskunft darüber, ob noch Platz für eine Ware an der Flagge ist.
    inline bool IsSpaceForWare() const { return GetNumWares() < wares.size(); }

    void Draw(DrawPoint drawPt) override;

    BlockingManner GetBM() const override { return BlockingManner::Flag; }

    /// Erzeugt von ihnen selbst ein FOW Objekt als visuelle "Erinnerung" für den Fog of War.
    FOWObject* CreateFOWObject() const override;
    /// Legt eine Ware an der Flagge ab.
    void AddWare(Ware*& ware) override;
    /// Gibt die Anzahl der Waren zurück, die an der Flagge liegen.
    unsigned GetNumWares() const;
    /// Wählt eine Ware von einer Flagge aus (anhand der Transportreihenfolge), entfernt sie von der Flagge und gibt sie zurück.
    Ware* SelectWare(const Direction dir, const bool swap_wares, const noFigure* const carrier);
    /// Prüft, ob es Waren gibt, die auf den Weg in Richtung dir getragen werden müssen.
    unsigned GetNumWaresForRoad(const Direction dir) const;
    /// Gibt Wegstrafpunkte für das Pathfinden für Waren, die in eine bestimmte Richtung noch transportiert werden müssen.
    unsigned GetPunishmentPoints(const Direction dir) const override;
    /// Zerstört evtl. vorhandenes Gebäude bzw. Baustelle vor der Flagge.
    void DestroyAttachedBuilding();
    /// Baut normale Flaggen zu "gloriösen" aus bei Eselstraßen.
    void Upgrade();
    /// Feind übernimmt die Flagge.
    void Capture(const unsigned char new_owner);
    /// Ist diese Flagge für eine bestimmte Lagerhausflüchtlingsgruppe (BWU) nicht zugänglich?
    bool IsImpossibleForBWU(const unsigned bwu_id) const;
    /// Hinzufügen, dass diese Flagge für eine bestimmte Lagerhausgruppe nicht zugänglich ist.
    void ImpossibleForBWU(const unsigned bwu_id);

protected:
    void Destroy_noFlag();
    void Serialize_noFlag(SerializedGameData& sgd) const;

private:
    unsigned short ani_offset;
    FlagType flagtype;

    /// Die Waren, die an dieser Flagge liegen
    boost::array<Ware*, 8> wares;

    /// Wieviele BWU-Teile es maximal geben soll, also wieviele abgebrannte Lagerhausgruppen
    /// gleichzeitig die Flagge als nicht begehbar deklarieren können.
    static const unsigned MAX_BWU = 4;

    /// Nicht erreichbar für Massenflüchtlinge
    struct BurnedWarehouseUnit
    {
        unsigned id;      /// ID der Gruppe
        unsigned last_gf; /// letzter TÜV, ob man auch nicht hinkommt, in GF
    };
    boost::array<BurnedWarehouseUnit, MAX_BWU> bwus;
};

#endif // !NO_FLAG_H_INCLUDED
