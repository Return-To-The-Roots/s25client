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
#ifndef NOCOORDBASE_H_INCLUDED
#define NOCOORDBASE_H_INCLUDED

#pragma once

#include "noBase.h"
#include "gameTypes/MapTypes.h"
class SerializedGameData;

class noCoordBase : public noBase
{

    public:

        noCoordBase(const NodalObjectType nop, const MapPoint pt) : noBase(nop), pos(pt) {}
        noCoordBase(SerializedGameData& sgd, const unsigned obj_id);

        /// Aufräummethoden
    protected:  void Destroy_noCoordBase() { Destroy_noBase(); }
    public:     void Destroy() override { Destroy_noCoordBase(); }

        /// Serialisierungsfunktionen
    protected:  void Serialize_noCoordBase(SerializedGameData& sgd) const;
    public:     void Serialize(SerializedGameData& sgd) const override { Serialize_noCoordBase(sgd); }

        /// liefert die X-Koordinate.
        MapCoord GetX() const { return pos.x; }
        /// liefert die Y-Koordinate.
        MapCoord GetY() const { return pos.y; }

        /// Returns position
        MapPoint GetPos() const { return pos; }

        /// Liefert GUI-ID zurück für die Fenster
        unsigned CreateGUIID() const;

    protected:
        MapPoint pos;
};

#endif // !NOCOORDBASE_H_INCLUDED
