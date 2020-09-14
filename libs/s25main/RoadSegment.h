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

#include "GameObject.h"
#include "gameTypes/Direction.h"
#include <array>
#include <vector>

class nofCarrier;
class noRoadNode;
class noFlag;
class noFigure;
class SerializedGameData;
enum class CarrierType : uint8_t;

enum class RoadType : uint8_t
{
    Normal, /// Normal or mountain road
    Donkey, /// upgraded (with donkey) road
    Water   /// waterway
};
DEFINE_MAX_ENUM_VALUE(RoadType, RoadType::Water)

class RoadSegment : public GameObject
{
public:
    RoadSegment(RoadType rt, noRoadNode* f1, noRoadNode* f2, std::vector<Direction> route);
    RoadSegment(SerializedGameData& sgd, unsigned obj_id);

    /// zerstört das Objekt.
    void Destroy() override { Destroy_RoadSegment(); }
    /// serialisiert das Objekt.
    void Serialize(SerializedGameData& sgd) const override { Serialize_RoadSegment(sgd); }
    /// liefert den GO-Type.
    GO_Type GetGOT() const override { return GOT_ROADSEGMENT; }
    /// Gibt die ID (0 oder 1) eines RoadNodes dieser Straße zurück (die Flagge muss zu dieser Straße gehören, sonst kommt Müll raus!!)
    bool GetNodeID(const noRoadNode& rn) const;
    /// Gibt Straßen-Typ zurück
    RoadType GetRoadType() const { return rt; }
    /// Gibt die Länge der Staße zurück
    unsigned GetLength() const { return route.size(); }
    /// gibt Flagge 1 zurück
    noRoadNode* GetF1() const { return f1; }
    /// setzt Flagge 1 auf o
    void SetF1(noRoadNode* o) { f1 = o; }
    /// gibt Flagge 2 zurück
    noRoadNode* GetF2() const { return f2; }
    /// setzt Flagge 2 auf o
    void SetF2(noRoadNode* o) { f2 = o; }
    /// gibt die Route nr zurück
    Direction GetRoute(unsigned nr) const { return route.at(nr); }
    /// setzt die Route nr auf r
    void SetRoute(unsigned short nr, Direction r) { route[nr] = r; }
    /// gibt den Carrier nr zurück
    nofCarrier* getCarrier(unsigned char nr) const { return carriers_[nr]; }
    /// setzt den Carrier nr auf c
    void setCarrier(unsigned char nr, nofCarrier* c)
    {
        RTTR_Assert(!c || !hasCarrier(nr));
        carriers_[nr] = c;
    }
    /// haben wir den Carrier "nr"?
    bool hasCarrier(unsigned char nr) const { return (carriers_[nr] != nullptr); }
    /// Braucht die Straße einen Esel? Nur wenn sie auch einen Träger schon hat!
    bool NeedDonkey() const { return (rt == RoadType::Donkey && carriers_[0] && !carriers_[1]); }
    /// Hat einen Esel als Arbeiter dazubekommen.
    void GotDonkey(nofCarrier* donkey)
    {
        RTTR_Assert(!carriers_[1]);
        carriers_[1] = donkey;
    }

    /// haben wir überhaupt Carrier?
    bool isOccupied() const { return (carriers_[0] || carriers_[1]); }

    Direction GetDir(bool bwdDir, unsigned id) const
    {
        if(bwdDir)
            return route[route.size() - id - 1] + 3u;
        else
            return route[id];
    }

    /// zerteilt die Straße in 2 Teile.
    void SplitRoad(noFlag* splitflag);
    /// Überprüft ob es an den Flaggen noch Waren zu tragen gibt für den Träger.
    bool AreWareJobs(bool flag, CarrierType ct, bool take_ware_immediately) const;
    /// Eine Ware sagt Bescheid, dass sie über dem Weg getragen werden will.
    void AddWareJob(const noRoadNode* rn);
    /// Eine Ware will nicht mehr befördert werden.
    void WareJobRemoved(const noFigure* exception);
    /// Baut die Straße zu einer Eselstraße aus.
    void UpgradeDonkeyRoad();
    /// Soll versuchen einen Esel zu bekommen.
    void TryGetDonkey();
    /// Ein Träger muss kündigen, aus welchen Gründen auch immer.
    void CarrierAbrogated(nofCarrier* carrier);
    /// given a flag returns the flag at the other end
    const noFlag& GetOtherFlag(const noFlag& flag) const;
    /// given a flag returns last direction of the route towards the other flag
    Direction GetOtherFlagDir(const noFlag& flag) const;

protected:
    /// zerstört das Objekt.
    void Destroy_RoadSegment();
    /// serialisiert das Objekt.
    void Serialize_RoadSegment(SerializedGameData& sgd) const;

private:
    /// Straßentyp
    RoadType rt;
    /// die 2 Roadnodes, die den Weg eingrenzen
    noRoadNode *f1, *f2;
    /// Beschreibung des Weges, ist length groß und liegt als Beschreibung der einzelnen Richtungen vor (von f1 zu f2)
    std::vector<Direction> route;
    /// Träger (und ggf. Esel), der auf diesem Weg arbeitet
    std::array<nofCarrier*, 2> carriers_;
};
