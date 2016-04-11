// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef ROADSEGMENT_H_INCLUDED
#define ROADSEGMENT_H_INCLUDED

#pragma once

#include "GameObject.h"
#include <boost/array.hpp>
#include <vector>

class nofCarrier;
class noRoadNode;
class noFlag;
class noFigure;
class SerializedGameData;

class RoadSegment : public GameObject
{
    public:
        enum RoadType
        {
            RT_NORMAL,  /// Normale Straße bzw. Bergstraße
            RT_DONKEY,  /// Eselstraße
            RT_BOAT     /// Wasserstraße
        };

    public:
        RoadSegment(const RoadType rt, noRoadNode* const f1, noRoadNode* const f2, const std::vector<unsigned char>& route);
        RoadSegment(SerializedGameData& sgd, const unsigned obj_id);

        /// zerstört das Objekt.
        void Destroy() override { Destroy_RoadSegment(); }
        /// serialisiert das Objekt.
        void Serialize(SerializedGameData& sgd) const override { Serialize_RoadSegment(sgd); }
        /// liefert den GO-Type.
        inline GO_Type GetGOT() const override { return GOT_ROADSEGMENT; }
        /// Gibt die ID (0 oder 1) eines RoadNodes dieser Straße zurück (die Flagge muss zu dieser Straße gehören, sonst kommt Müll raus!!)
        inline bool GetNodeID(const noRoadNode* rn) { return (rn == f2); }
        /// Gibt Straßen-Typ zurück
        inline RoadType GetRoadType() const { return rt; }
        /// Gibt die Länge der Staße zurück
        inline unsigned GetLength() const { return route.size(); }
        /// gibt Flagge 1 zurück
        inline noRoadNode* GetF1() const { return f1; }
        /// setzt Flagge 1 auf o
        inline void SetF1(noRoadNode* o) { f1 = o; }
        /// gibt Flagge 2 zurück
        inline noRoadNode* GetF2() const { return f2; }
        /// setzt Flagge 2 auf o
        inline void SetF2(noRoadNode* o) { f2 = o; }
        /// gibt die Route nr zurück
        inline unsigned char GetRoute(unsigned short nr) const { return route.at(nr); }
        /// setzt die Route nr auf r
        inline void SetRoute(unsigned short nr, unsigned char r) { route[nr] = r; }
        /// gibt den Carrier nr zurück
        inline nofCarrier* getCarrier(unsigned char nr) const { return carriers_[nr]; }
        /// setzt den Carrier nr auf c
        inline void setCarrier(unsigned char nr, nofCarrier* c) { RTTR_Assert(!c || !hasCarrier(nr)); carriers_[nr] = c; }
        /// haben wir den Carrier "nr"?
        inline bool hasCarrier(unsigned char nr) const { return (carriers_[nr] != NULL); }
        /// Braucht die Straße einen Esel? Nur wenn sie auch einen Träger schon hat!
        inline bool NeedDonkey() const { return (rt == RT_DONKEY && carriers_[0] && !carriers_[1]); }
        /// Hat einen Esel als Arbeiter dazubekommen.
        inline void GotDonkey(nofCarrier* donkey) { RTTR_Assert(!carriers_[1]); carriers_[1] = donkey; }

        /// haben wir überhaupt Carrier?
        inline bool isOccupied() const
        {
            return((carriers_[0]) || (carriers_[1]));
        }

        inline unsigned char GetDir(const bool dir, const unsigned int id) const
        {
            if(dir)
                return (route[route.size() - id - 1] + 3) % 6;
            else
                return route[id];
        }

        /// zerteilt die Straße in 2 Teile.
        void SplitRoad(noFlag* splitflag);
        /// Überprüft ob es an den Flaggen noch Waren zu tragen gibt für den Träger.
        bool AreWareJobs(const bool flag, unsigned int carrier_type, const bool take_ware_immediately) const;
        /// Eine Ware sagt Bescheid, dass sie über dem Weg getragen werden will.
        void AddWareJob(const noRoadNode* rn);
        /// Eine Ware will nicht mehr befördert werden.
        void WareJobRemoved(const noFigure* const exception);
        /// Baut die Straße zu einer Eselstraße aus.
        void UpgradeDonkeyRoad();
        /// Soll versuchen einen Esel zu bekommen.
        void TryGetDonkey();
        /// Ein Träger muss kündigen, aus welchen Gründen auch immer.
        void CarrierAbrogated(nofCarrier* carrier);
        /// given a flag returns the other end location
        noFlag* GetOtherFlag(const noFlag* flag);
        /// given a flag returns last direction of the route towards the other flag
        unsigned char GetOtherFlagDir(const noFlag* flag);

    protected:
        /// zerstört das Objekt.
        void Destroy_RoadSegment();
        /// serialisiert das Objekt.
        void Serialize_RoadSegment(SerializedGameData& sgd) const;

    private:
        /// Straßentyp
        RoadType rt;
        /// die 2 Roadnodes, die den Weg eingrenzen
        noRoadNode* f1, *f2;
        /// Beschreibung des Weges, ist length groß und liegt als Beschreibung der einzelnen Richtungen vor (von f1 zu f2)
        std::vector<unsigned char> route;
        /// Träger (und ggf. Esel), der auf diesem Weg arbeitet
        boost::array<nofCarrier*, 2> carriers_;
};

#endif // !ROADSEGMENT_H_INCLUDED
